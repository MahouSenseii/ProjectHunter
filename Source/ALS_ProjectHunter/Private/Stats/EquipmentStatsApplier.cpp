#include "Stats/EquipmentStatsApplier.h"

#include "AbilitySystemComponent.h"
#include "Core/Logging/ProjectHunterLogMacros.h"
#include "GameplayEffect.h"
#include "Item/ItemInstance.h"
#include "Item/Library/AffixEnums.h"
#include "Item/Library/ItemStructs.h"
#include "Equipment/Components/EquipmentManager.h"
#include "Stats/Components/StatsManager.h"
#include "Stats/StatsAttributeResolver.h"
#include "Stats/StatsModifierMath.h"

void FEquipmentStatsApplier::ApplyEquipmentStats(UStatsManager& Manager, UItemInstance* Item)
{
	if (!Item)
	{
		PH_LOG_WARNING(LogStatsManager, "ApplyEquipmentStats failed: Item was invalid.");
		return;
	}

	UAbilitySystemComponent* ASC = FStatsAttributeResolver::GetAbilitySystemComponent(Manager);
	if (!ASC)
	{
		PH_LOG_ERROR(LogStatsManager, "ApplyEquipmentStats failed: AbilitySystemComponent was unavailable.");
		return;
	}

	AActor* Owner = Manager.GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		PH_LOG_WARNING(LogStatsManager, "ApplyEquipmentStats failed: Must be called on the server.");
		return;
	}

	if (Manager.ActiveEquipmentEffects.Contains(Item->UniqueID))
	{
		PH_LOG_WARNING(LogStatsManager, "ApplyEquipmentStats skipped: Equipment stats were already active for Item=%s.", *Item->GetName());
		return;
	}

	TArray<FPHAttributeData> AllStats = Item->Stats.GetAllStats();
	if (AllStats.Num() == 0)
	{
		UE_LOG(LogStatsManager, Verbose, TEXT("StatsManager: Item %s has no stats to apply"), *Item->GetName());
		return;
	}

	FGameplayEffectSpecHandle EffectSpec = CreateEquipmentEffect(Manager, Item, AllStats);
	if (!EffectSpec.IsValid())
	{
		PH_LOG_ERROR(LogStatsManager, "ApplyEquipmentStats failed: Could not create an equipment effect for Item=%s.", *Item->GetName());
		return;
	}

	const FActiveGameplayEffectHandle EffectHandle = ASC->ApplyGameplayEffectSpecToSelf(*EffectSpec.Data.Get());
	if (EffectHandle.IsValid())
	{
		Manager.ActiveEquipmentEffects.Add(Item->UniqueID, EffectHandle);
		Manager.ActiveEquipmentItems.Add(Item->UniqueID, Item);

		UE_LOG(LogStatsManager, Log, TEXT("StatsManager: Applied %d stats from %s (GUID: %s)"),
			AllStats.Num(), *Item->GetName(), *Item->UniqueID.ToString());
	}
	else
	{
		PH_LOG_ERROR(LogStatsManager, "ApplyEquipmentStats failed: Could not apply the equipment effect for Item=%s.", *Item->GetName());
	}
}

void FEquipmentStatsApplier::RemoveEquipmentStats(UStatsManager& Manager, UItemInstance* Item)
{
	if (!Item)
	{
		PH_LOG_WARNING(LogStatsManager, "RemoveEquipmentStats failed: Item was invalid.");
		return;
	}

	UAbilitySystemComponent* ASC = FStatsAttributeResolver::GetAbilitySystemComponent(Manager);
	if (!ASC)
	{
		PH_LOG_ERROR(LogStatsManager, "RemoveEquipmentStats failed: AbilitySystemComponent was unavailable.");
		return;
	}

	AActor* Owner = Manager.GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		PH_LOG_WARNING(LogStatsManager, "RemoveEquipmentStats failed: Must be called on the server.");
		return;
	}

	FActiveGameplayEffectHandle* EffectHandle = Manager.ActiveEquipmentEffects.Find(Item->UniqueID);
	if (!EffectHandle || !EffectHandle->IsValid())
	{
		PH_LOG_WARNING(LogStatsManager, "RemoveEquipmentStats skipped: No active equipment effect was found for Item=%s.", *Item->GetName());
		return;
	}

	ASC->RemoveActiveGameplayEffect(*EffectHandle);
	Manager.ActiveEquipmentEffects.Remove(Item->UniqueID);
	Manager.ActiveEquipmentItems.Remove(Item->UniqueID);

	UE_LOG(LogStatsManager, Log, TEXT("StatsManager: Removed equipment stats for %s (GUID: %s)"),
		*Item->GetName(), *Item->UniqueID.ToString());
}

void FEquipmentStatsApplier::RefreshEquipmentStats(UStatsManager& Manager)
{
	UAbilitySystemComponent* ASC = FStatsAttributeResolver::GetAbilitySystemComponent(Manager);
	AActor* Owner = Manager.GetOwner();
	if (!ASC || !Owner || !Owner->HasAuthority())
	{
		return;
	}

	TArray<TObjectPtr<UItemInstance>> ItemsToReapply;
	if (const UEquipmentManager* EquipmentManager = Owner->FindComponentByClass<UEquipmentManager>())
	{
		for (UItemInstance* Item : EquipmentManager->GetAllEquippedItems())
		{
			if (IsValid(Item))
			{
				ItemsToReapply.Add(Item);
			}
		}
	}
	else
	{
		Manager.ActiveEquipmentItems.GenerateValueArray(ItemsToReapply);
	}

	const int32 NumEffects = Manager.ActiveEquipmentEffects.Num();
	for (const TPair<FGuid, FActiveGameplayEffectHandle>& Pair : Manager.ActiveEquipmentEffects)
	{
		if (Pair.Value.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(Pair.Value);
		}
	}

	Manager.ActiveEquipmentEffects.Empty();
	Manager.ActiveEquipmentItems.Empty();

	int32 Reapplied = 0;
	for (UItemInstance* Item : ItemsToReapply)
	{
		if (IsValid(Item))
		{
			ApplyEquipmentStats(Manager, Item);
			++Reapplied;
		}
	}

	UE_LOG(LogStatsManager, Log, TEXT("StatsManager: Refreshed equipment stats (removed %d, reapplied %d)"), NumEffects, Reapplied);
}

void FEquipmentStatsApplier::HandleEquipmentChanged(UStatsManager& Manager, UItemInstance* NewItem, UItemInstance* OldItem)
{
	if (!Manager.GetOwner() || !Manager.GetOwner()->HasAuthority())
	{
		return;
	}

	if (OldItem && OldItem != NewItem)
	{
		RemoveEquipmentStats(Manager, OldItem);
	}

	if (NewItem && NewItem != OldItem)
	{
		ApplyEquipmentStats(Manager, NewItem);
	}
}

FGameplayEffectSpecHandle FEquipmentStatsApplier::CreateEquipmentEffect(UStatsManager& Manager, UItemInstance* Item, const TArray<FPHAttributeData>& Stats)
{
	UAbilitySystemComponent* ASC = FStatsAttributeResolver::GetAbilitySystemComponent(Manager);
	if (!ASC || !Item)
	{
		return FGameplayEffectSpecHandle();
	}

	UGameplayEffect* Effect = NewObject<UGameplayEffect>(Manager.GetOwner());
	Effect->DurationPolicy = EGameplayEffectDurationType::Infinite;

	int32 ModifiersAdded = 0;
	for (const FPHAttributeData& Stat : Stats)
	{
		if (!Stat.bIsIdentified)
		{
			continue;
		}

		FGameplayAttribute Attribute = Stat.ModifiedAttribute;
		if (!Attribute.IsValid() && Stat.AttributeName != NAME_None)
		{
			FStatsAttributeResolver::ResolveAttributeByName(Manager, Stat.AttributeName, Attribute);
		}

		if (!Attribute.IsValid())
		{
			PH_LOG_WARNING(LogStatsManager, "CreateEquipmentEffect skipped Stat=%s because it could not resolve to a valid attribute.", *Stat.AttributeName.ToString());
			continue;
		}

		if (ApplyStatModifier(Effect, Stat, Attribute))
		{
			++ModifiersAdded;
		}
	}

	if (ModifiersAdded == 0)
	{
		PH_LOG_WARNING(LogStatsManager, "CreateEquipmentEffect failed: No valid modifiers were found for Item=%s.", *Item->GetName());
		return FGameplayEffectSpecHandle();
	}

	FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
	EffectContext.AddSourceObject(Item);

	FGameplayEffectSpecHandle SpecHandle;
	SpecHandle.Data = MakeShared<FGameplayEffectSpec>(Effect, EffectContext, 1.0f);

	UE_LOG(LogStatsManager, Verbose, TEXT("StatsManager: Created equipment effect for item '%s' with %d modifiers"),
		*Item->GetName(), ModifiersAdded);

	return SpecHandle;
}

bool FEquipmentStatsApplier::ApplyStatModifier(UGameplayEffect* Effect, const FPHAttributeData& Stat, const FGameplayAttribute& Attribute)
{
	if (!Effect || !Attribute.IsValid())
	{
		return false;
	}

	FResolvedStatModifier ResolvedModifier;
	if (!FStatsModifierMath::ResolveGameplayModifier(Stat.ModifyType, Stat.RolledStatValue, ResolvedModifier)
		|| !ResolvedModifier.bCreatesGameplayModifier)
	{
		PH_LOG_WARNING(LogStatsManager, "ApplyStatModifier skipped: Unsupported ModifyType=%d for Attribute=%s.", static_cast<int32>(Stat.ModifyType), *Attribute.GetName());
		return false;
	}

	FGameplayModifierInfo Modifier;
	Modifier.Attribute = Attribute;
	Modifier.ModifierOp = ResolvedModifier.ModOp;
	Modifier.ModifierMagnitude = FScalableFloat(ResolvedModifier.Magnitude);
	Effect->Modifiers.Add(Modifier);

	UE_LOG(LogStatsManager, VeryVerbose, TEXT("StatsManager: Added modifier: %s (%s) = %.2f [Op: %d]"),
		*Attribute.GetName(), *Stat.AttributeName.ToString(), ResolvedModifier.Magnitude, static_cast<int32>(ResolvedModifier.ModOp));

	return true;
}
