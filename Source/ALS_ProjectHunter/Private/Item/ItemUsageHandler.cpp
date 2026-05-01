#include "Item/ItemUsageHandler.h"

#include "AbilitySystemComponent.h"
#include "Core/Logging/ProjectHunterLogMacros.h"
#include "GameplayEffect.h"
#include "Item/ItemInstance.h"
#include "Item/ItemStackingHandler.h"
#include "Item/Library/ItemLog.h"

void FItemUsageHandler::ApplyAffixesToCharacter(UItemInstance& Item, UAbilitySystemComponent* ASC)
{
	if (!ASC || !Item.IsEquipment())
	{
		return;
	}

	RemoveAffixesFromCharacter(Item, ASC);

	const TArray<FPHAttributeData> AllAffixes = Item.Stats.GetAllStats();
	for (const FPHAttributeData& Affix : AllAffixes)
	{
		if (!Affix.bIsIdentified)
		{
			continue;
		}

		if (Affix.bIsLocalToWeapon || Affix.bAffectsBaseWeaponStatsDirectly)
		{
			continue;
		}

		if (!Affix.ModifiedAttribute.IsValid())
		{
			continue;
		}

		UE_LOG(LogItemInstance, Log, TEXT("Applied affix: %s = %f (Corrupted: %s)"),
			*Affix.AttributeName.ToString(),
			Affix.RolledStatValue,
			Affix.IsCorruptedAffix() ? TEXT("YES") : TEXT("NO"));
	}

	Item.bEffectsActive = true;
}

void FItemUsageHandler::RemoveAffixesFromCharacter(UItemInstance& Item, UAbilitySystemComponent* ASC)
{
	if (!ASC)
	{
		return;
	}

	for (const FActiveGameplayEffectHandle& Handle : Item.AppliedEffectHandles)
	{
		ASC->RemoveActiveGameplayEffect(Handle);
	}

	Item.AppliedEffectHandles.Empty();
	Item.bEffectsActive = false;
}

bool FItemUsageHandler::UseConsumable(UItemInstance& Item, AActor* Target)
{
	if (!CanUseConsumable(Item) || !Target)
	{
		return false;
	}

	FItemBase* Base = Item.GetBaseData();
	if (!Base)
	{
		return false;
	}

	if (!ApplyConsumableEffects(Item, Target))
	{
		return false;
	}

	Item.LastUseTime = Item.GetWorld() ? Item.GetWorld()->GetTimeSeconds() : 0.0f;

	if (Base->ConsumableData.MaxUses > 1)
	{
		ReduceUses(Item, 1);
	}
	else
	{
		FItemStackingHandler::RemoveFromStack(Item, 1);
	}

	return true;
}

bool FItemUsageHandler::CanUseConsumable(const UItemInstance& Item)
{
	if (!Item.IsConsumable())
	{
		return false;
	}

	FItemBase* Base = Item.GetBaseData();
	if (!Base)
	{
		return false;
	}

	if (Base->ConsumableData.Cooldown > 0.0f && Item.LastUseTime > 0.0f)
	{
		const UWorld* World = Item.GetWorld();
		if (World)
		{
			const float Elapsed = World->GetTimeSeconds() - Item.LastUseTime;
			if (Elapsed < Base->ConsumableData.Cooldown)
			{
				return false;
			}
		}
	}

	if (Base->ConsumableData.MaxUses > 1 && Item.RemainingUses <= 0)
	{
		return false;
	}

	return Item.Quantity > 0;
}

float FItemUsageHandler::GetCooldownProgress(const UItemInstance& Item)
{
	FItemBase* Base = Item.GetBaseData();
	if (!Base || Base->ConsumableData.Cooldown <= 0.0f)
	{
		return 1.0f;
	}

	if (Item.LastUseTime <= 0.0f)
	{
		return 1.0f;
	}

	const UWorld* World = Item.GetWorld();
	if (!World)
	{
		return 1.0f;
	}

	const float Elapsed = World->GetTimeSeconds() - Item.LastUseTime;
	return FMath::Clamp(Elapsed / Base->ConsumableData.Cooldown, 0.0f, 1.0f);
}

bool FItemUsageHandler::ReduceUses(UItemInstance& Item, int32 Amount)
{
	Item.RemainingUses = FMath::Max(0, Item.RemainingUses - Amount);
	return Item.RemainingUses <= 0;
}

bool FItemUsageHandler::ApplyConsumableEffects(UItemInstance& Item, AActor* Target)
{
	FItemBase* Base = Item.GetBaseData();
	if (!Base)
	{
		return false;
	}

	if (Base->ConsumableData.EffectsToApply.IsEmpty())
	{
		PH_LOG_WARNING(LogItemInstance, "ApplyConsumableEffects: '%s' has no EffectsToApply configured.",
			*Item.GetBaseItemName().ToString());
		return true;
	}

	UAbilitySystemComponent* TargetASC = Target->FindComponentByClass<UAbilitySystemComponent>();
	if (!TargetASC)
	{
		PH_LOG_WARNING(LogItemInstance, "ApplyConsumableEffects: Target '%s' has no AbilitySystemComponent.",
			*Target->GetName());
		return false;
	}

	bool bAllApplied = true;
	for (const TSubclassOf<UGameplayEffect>& GEClass : Base->ConsumableData.EffectsToApply)
	{
		if (!GEClass)
		{
			PH_LOG_WARNING(LogItemInstance, "ApplyConsumableEffects: Null GE class in '%s' EffectsToApply was skipped.",
				*Item.GetBaseItemName().ToString());
			continue;
		}

		FGameplayEffectContextHandle Context = TargetASC->MakeEffectContext();
		FGameplayEffectSpecHandle Spec = TargetASC->MakeOutgoingSpec(GEClass, Item.ItemLevel, Context);
		if (Spec.IsValid())
		{
			const FActiveGameplayEffectHandle Handle = TargetASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
			if (!Handle.IsValid())
			{
				PH_LOG_WARNING(LogItemInstance, "ApplyConsumableEffects: Failed to apply '%s' to '%s'.",
					*GEClass->GetName(), *Target->GetName());
				bAllApplied = false;
			}
		}
		else
		{
			bAllApplied = false;
		}
	}

	return bAllApplied;
}
