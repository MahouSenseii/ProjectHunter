#include "Stats/Library/FunctionLibraries/EquipmentStatsApplier.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/HunterAttributeSet.h"
#include "Core/Logging/ProjectHunterLogMacros.h"
#include "GameplayEffect.h"
#include "Item/ItemInstance.h"
#include "Item/Library/Enums/AffixEnums.h"
#include "Item/Library/Structs/ItemStructs.h"
#include "Equipment/Components/EquipmentManager.h"
#include "Stats/Components/StatsManager.h"
#include "Stats/Library/FunctionLibraries/ItemLocalStatResolver.h"
#include "Stats/Library/FunctionLibraries/StatsAttributeResolver.h"
#include "Stats/Library/FunctionLibraries/StatsModifierMath.h"

namespace EquipmentStatsApplierPrivate
{
	// One slot per damage type, index order: Physical, Fire, Ice, Lightning, Light, Corruption.
	constexpr int32 NumWeaponDamageTypes = 6;

	struct FWeaponDamageSide
	{
		float Base = 0.f;
		float LocalFlat = 0.f;
		float LocalIncreasedPct = 0.f;
		float LocalMoreMultiplier = 1.f;

		bool HasContribution() const
		{
			return Base > 0.f
				|| !FMath::IsNearlyZero(LocalFlat)
				|| !FMath::IsNearlyZero(LocalIncreasedPct)
				|| !FMath::IsNearlyEqual(LocalMoreMultiplier, 1.f);
		}

		float Resolve() const
		{
			return FMath::Max(0.f,
				(Base + LocalFlat)
				* FStatsModifierMath::PercentToMultiplier(LocalIncreasedPct)
				* LocalMoreMultiplier);
		}
	};

	struct FWeaponDamageAccumulator
	{
		FWeaponDamageSide Min[NumWeaponDamageTypes];
		FWeaponDamageSide Max[NumWeaponDamageTypes];
	};

	void GetWeaponMinMaxAttributes(const int32 TypeIndex, FGameplayAttribute& OutMin, FGameplayAttribute& OutMax)
	{
		switch (TypeIndex)
		{
		case 0:
			OutMin = UHunterAttributeSet::GetMinPhysicalDamageAttribute();
			OutMax = UHunterAttributeSet::GetMaxPhysicalDamageAttribute();
			break;
		case 1:
			OutMin = UHunterAttributeSet::GetMinFireDamageAttribute();
			OutMax = UHunterAttributeSet::GetMaxFireDamageAttribute();
			break;
		case 2:
			OutMin = UHunterAttributeSet::GetMinIceDamageAttribute();
			OutMax = UHunterAttributeSet::GetMaxIceDamageAttribute();
			break;
		case 3:
			OutMin = UHunterAttributeSet::GetMinLightningDamageAttribute();
			OutMax = UHunterAttributeSet::GetMaxLightningDamageAttribute();
			break;
		case 4:
			OutMin = UHunterAttributeSet::GetMinLightDamageAttribute();
			OutMax = UHunterAttributeSet::GetMaxLightDamageAttribute();
			break;
		case 5:
			OutMin = UHunterAttributeSet::GetMinCorruptionDamageAttribute();
			OutMax = UHunterAttributeSet::GetMaxCorruptionDamageAttribute();
			break;
		default:
			break;
		}
	}

	void SeedWeaponBase(const FBaseWeaponStats& WeaponStats, FWeaponDamageAccumulator& Accum)
	{
		Accum.Min[0].Base = WeaponStats.MinPhysicalDamage;   Accum.Max[0].Base = WeaponStats.MaxPhysicalDamage;
		Accum.Min[1].Base = WeaponStats.MinFireDamage;       Accum.Max[1].Base = WeaponStats.MaxFireDamage;
		Accum.Min[2].Base = WeaponStats.MinIceDamage;        Accum.Max[2].Base = WeaponStats.MaxIceDamage;
		Accum.Min[3].Base = WeaponStats.MinLightningDamage;  Accum.Max[3].Base = WeaponStats.MaxLightningDamage;
		Accum.Min[4].Base = WeaponStats.MinLightDamage;      Accum.Max[4].Base = WeaponStats.MaxLightDamage;
		Accum.Min[5].Base = WeaponStats.MinCorruptionDamage; Accum.Max[5].Base = WeaponStats.MaxCorruptionDamage;
	}

	bool ResolveWeaponDamageAttribute(const FGameplayAttribute& Attribute, int32& OutTypeIndex, bool& bOutIsMin)
	{
		for (int32 TypeIndex = 0; TypeIndex < NumWeaponDamageTypes; ++TypeIndex)
		{
			FGameplayAttribute MinAttr;
			FGameplayAttribute MaxAttr;
			GetWeaponMinMaxAttributes(TypeIndex, MinAttr, MaxAttr);

			if (Attribute == MinAttr)
			{
				OutTypeIndex = TypeIndex;
				bOutIsMin = true;
				return true;
			}
			if (Attribute == MaxAttr)
			{
				OutTypeIndex = TypeIndex;
				bOutIsMin = false;
				return true;
			}
		}
		return false;
	}

	bool IsLocalAffix(const FPHAttributeData& Stat)
	{
		return Stat.IsLocal();
	}

	bool AccumulateLocalDamageMod(const FPHAttributeData& Stat, FWeaponDamageSide& Side)
	{
		switch (Stat.ModifyType)
		{
		case EModifyType::MT_Add:
		case EModifyType::MT_AddRange:
			Side.LocalFlat += Stat.RolledStatValue;
			return true;

		case EModifyType::MT_Reduced:
			Side.LocalIncreasedPct -= FMath::Abs(Stat.RolledStatValue);
			return true;

		case EModifyType::MT_Increased:
			Side.LocalIncreasedPct += Stat.RolledStatValue;
			return true;

		case EModifyType::MT_Multiply:
		case EModifyType::MT_More:
		case EModifyType::MT_MultiplyRange:
			Side.LocalMoreMultiplier *= FStatsModifierMath::PercentToMultiplier(Stat.RolledStatValue);
			return true;

		case EModifyType::MT_Less:
			Side.LocalMoreMultiplier *= FStatsModifierMath::PercentToMultiplier(-FMath::Abs(Stat.RolledStatValue));
			return true;

		default:
			return false;
		}
	}

	/** One attribute/value pair an affix contributes. A damage range contributes two. */
	struct FStatContribution
	{
		FGameplayAttribute Attribute;
		float Value = 0.f;
	};

	/**
	 * Expand one affix into everything it should modify.
	 *
	 * A range affix rolls two endpoints, and globally it has to reach both the
	 * Min and Max attribute of its damage type. Only the local weapon resolver
	 * ever read the second endpoint, so "Adds 5-12 Fire Damage" on a ring - which
	 * has no local resolver and must go global - applied its lower roll and
	 * dropped the upper one, while the tooltip still promised the full range.
	 *
	 * @return False when a range affix could not be paired, so the caller can warn.
	 */
	bool GetStatContributions(
		const FPHAttributeData& Stat,
		const FGameplayAttribute& ResolvedAttribute,
		TArray<FStatContribution>& OutContributions)
	{
		OutContributions.Reset();
		OutContributions.Add({ ResolvedAttribute, Stat.RolledStatValue });

		if (!Stat.UsesValueRange())
		{
			return true;
		}

		// Range affixes are authored against the Min attribute of a damage type;
		// anything else has no second attribute to carry the upper endpoint.
		int32 TypeIndex = INDEX_NONE;
		bool bIsMin = false;
		if (!ResolveWeaponDamageAttribute(ResolvedAttribute, TypeIndex, bIsMin) || !bIsMin)
		{
			return false;
		}

		FGameplayAttribute MinAttribute;
		FGameplayAttribute MaxAttribute;
		GetWeaponMinMaxAttributes(TypeIndex, MinAttribute, MaxAttribute);
		OutContributions.Add({ MaxAttribute, Stat.RolledSecondaryStatValue });

		return true;
	}

	void AddFlatModifier(UGameplayEffect* Effect, const FGameplayAttribute& Attribute, const float Magnitude)
	{
		FGameplayModifierInfo Modifier;
		Modifier.Attribute = Attribute;
		Modifier.ModifierOp = EGameplayModOp::Additive;
		Modifier.ModifierMagnitude = FScalableFloat(Magnitude);
		Effect->Modifiers.Add(Modifier);
	}

	bool IsProductModifier(const FPHAttributeData& Stat)
	{
		if (Stat.GameplayEffect
			|| Stat.Condition != EAffixCondition::AC_None
			|| Stat.ModifiedLocation == EAffixScope::AS_Conditional
			|| Stat.ModifiedLocation == EAffixScope::AS_Skill
			|| !Stat.RequiredSourceTags.IsEmpty()
			|| !Stat.BlockedSourceTags.IsEmpty()
			|| !Stat.RequiredTargetTags.IsEmpty()
			|| !Stat.BlockedTargetTags.IsEmpty()
			|| IsLocalAffix(Stat))
		{
			return false;
		}

		return Stat.ModifyType == EModifyType::MT_Multiply
			|| Stat.ModifyType == EModifyType::MT_More
			|| Stat.ModifyType == EModifyType::MT_MultiplyRange
			|| Stat.ModifyType == EModifyType::MT_Less;
	}

	float GetProductFactor(const FPHAttributeData& Stat, const float Value)
	{
		return Stat.ModifyType == EModifyType::MT_Less
			? FStatsModifierMath::PercentToMultiplier(-FMath::Abs(Value))
			: FStatsModifierMath::PercentToMultiplier(Value);
	}

	bool HasBaseEquipmentContribution(const FItemBase& Base)
	{
		if (Base.IsWeapon())
		{
			// Weapon-local values are selected per attack and never flattened into
			// shared character min/max attributes.
			return false;
		}

		if (Base.IsArmor())
		{
			const FBaseArmorStats& A = Base.ArmorStats;
			return !FMath::IsNearlyZero(A.Armor)
				|| !FMath::IsNearlyZero(A.FireResistance)
				|| !FMath::IsNearlyZero(A.IceResistance)
				|| !FMath::IsNearlyZero(A.LightningResistance)
				|| !FMath::IsNearlyZero(A.LightResistance)
				|| !FMath::IsNearlyZero(A.CorruptionResistance);
		}

		return false;
	}
}

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

	const FItemBase* BaseData = Item->GetBaseData();
	const bool bHasBaseContribution = BaseData
		&& EquipmentStatsApplierPrivate::HasBaseEquipmentContribution(*BaseData);

	if (AllStats.Num() == 0 && !bHasBaseContribution)
	{
		UE_LOG(LogStatsManager, Verbose, TEXT("StatsManager: Item %s has no stats to apply"), *Item->GetName());
		return;
	}

	TArray<FActiveGameplayEffectHandle> AppliedHandles;
	FGameplayEffectSpecHandle EffectSpec = CreateEquipmentEffect(Manager, Item, AllStats);
	if (EffectSpec.IsValid())
	{
		const FActiveGameplayEffectHandle EffectHandle = ASC->ApplyGameplayEffectSpecToSelf(*EffectSpec.Data.Get());
		if (EffectHandle.IsValid())
		{
			AppliedHandles.Add(EffectHandle);
		}
	}

	for (const FPHAttributeData& Stat : AllStats)
	{
		if (!Stat.GameplayEffect)
		{
			continue;
		}

		const UGameplayEffect* EffectCDO = Stat.GameplayEffect->GetDefaultObject<UGameplayEffect>();
		if (!EffectCDO || EffectCDO->DurationPolicy == EGameplayEffectDurationType::Instant)
		{
			PH_LOG_WARNING(LogStatsManager,
				"ApplyEquipmentStats skipped instant GameplayEffect '%s' on Item=%s because it could not be removed on unequip.",
				*GetNameSafe(Stat.GameplayEffect), *Item->GetName());
			continue;
		}

		FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
		Context.AddSourceObject(Item);
		const FGameplayEffectSpecHandle CustomSpec = ASC->MakeOutgoingSpec(Stat.GameplayEffect, 1.f, Context);
		if (!CustomSpec.IsValid())
		{
			continue;
		}

		const FActiveGameplayEffectHandle CustomHandle = ASC->ApplyGameplayEffectSpecToSelf(*CustomSpec.Data.Get());
		if (CustomHandle.IsValid())
		{
			AppliedHandles.Add(CustomHandle);
		}
	}

	const bool bHasProductModifier = AllStats.ContainsByPredicate(
		[](const FPHAttributeData& Stat)
		{
			return EquipmentStatsApplierPrivate::IsProductModifier(Stat);
		});

	if (!AppliedHandles.IsEmpty() || bHasProductModifier || AllStats.Num() > 0)
	{
		Manager.ActiveEquipmentEffects.Add(Item->UniqueID, MoveTemp(AppliedHandles));
		Manager.ActiveEquipmentItems.Add(Item->UniqueID, Item);
		RebuildEquipmentProductEffect(Manager);

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

	TArray<FActiveGameplayEffectHandle>* EffectHandles = Manager.ActiveEquipmentEffects.Find(Item->UniqueID);
	if (!EffectHandles)
	{
		PH_LOG_WARNING(LogStatsManager, "RemoveEquipmentStats skipped: No active equipment effect was found for Item=%s.", *Item->GetName());
		return;
	}

	for (const FActiveGameplayEffectHandle& EffectHandle : *EffectHandles)
	{
		if (EffectHandle.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(EffectHandle);
		}
	}
	Manager.ActiveEquipmentEffects.Remove(Item->UniqueID);
	Manager.ActiveEquipmentItems.Remove(Item->UniqueID);
	RebuildEquipmentProductEffect(Manager);

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
	if (Manager.ActiveEquipmentProductEffect.IsValid())
	{
		ASC->RemoveActiveGameplayEffect(Manager.ActiveEquipmentProductEffect);
		Manager.ActiveEquipmentProductEffect.Invalidate();
	}
	for (const TPair<FGuid, TArray<FActiveGameplayEffectHandle>>& Pair : Manager.ActiveEquipmentEffects)
	{
		for (const FActiveGameplayEffectHandle& EffectHandle : Pair.Value)
		{
			if (EffectHandle.IsValid())
			{
				ASC->RemoveActiveGameplayEffect(EffectHandle);
			}
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

	using namespace EquipmentStatsApplierPrivate;

	const FItemBase* BaseData = Item->GetBaseData();

	int32 ModifiersAdded = 0;
	for (const FPHAttributeData& Stat : Stats)
	{
		// Local modifiers are already folded into the owning item's immutable
		// weapon/armour snapshot. Applying one here would leak it globally.
		if (IsLocalAffix(Stat))
		{
			continue;
		}
		if (Stat.ModifyType == EModifyType::MT_ConvertTo)
		{
			// Conversion and gain-as-extra share one normalized per-hit stage;
			// representing either as a persistent scalar would duplicate it.
			continue;
		}

		if (Stat.GameplayEffect
			|| Stat.Condition != EAffixCondition::AC_None
			|| Stat.ModifiedLocation == EAffixScope::AS_Conditional
			|| Stat.ModifiedLocation == EAffixScope::AS_Skill
			|| !Stat.RequiredSourceTags.IsEmpty()
			|| !Stat.BlockedSourceTags.IsEmpty()
			|| !Stat.RequiredTargetTags.IsEmpty()
			|| !Stat.BlockedTargetTags.IsEmpty())
		{
			// Authored effects own persistent/non-combat behavior. Numeric combat
			// conditions are evaluated from source/skill/target context per hit.
			continue;
		}
		if (Stat.ModifyType == EModifyType::MT_GrantSkill
			|| Stat.ModifyType == EModifyType::MT_SetRank)
		{
			PH_LOG_WARNING(LogStatsManager,
				"CreateEquipmentEffect skipped affix '%s': Grant Skill and Set Rank require typed ability ownership or an authored GameplayEffect.",
				*Stat.AffixName.ToString());
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

		// GAS combines separate multiplicative modifiers additively around 1.0.
		// Fold equipment products into one exact multiplier effect instead.
		if (IsProductModifier(Stat))
		{
			continue;
		}

		if (ApplyStatModifier(Effect, Stat, Attribute))
		{
			++ModifiersAdded;
		}
	}

	if (BaseData && BaseData->IsArmor())
	{
		FResolvedArmorStats ResolvedArmor;
		FItemLocalStatResolver::ResolveArmor(Item, ResolvedArmor);
		const FBaseArmorStats& Armor = ResolvedArmor.Values;

		if (!FMath::IsNearlyZero(Armor.Armor))
		{
			AddFlatModifier(Effect, UHunterAttributeSet::GetArmourAttribute(), Armor.Armor);
			++ModifiersAdded;
		}
		if (!FMath::IsNearlyZero(Armor.FireResistance))
		{
			AddFlatModifier(Effect, UHunterAttributeSet::GetFireResistanceFlatBonusAttribute(), Armor.FireResistance);
			++ModifiersAdded;
		}
		if (!FMath::IsNearlyZero(Armor.IceResistance))
		{
			AddFlatModifier(Effect, UHunterAttributeSet::GetIceResistanceFlatBonusAttribute(), Armor.IceResistance);
			++ModifiersAdded;
		}
		if (!FMath::IsNearlyZero(Armor.LightningResistance))
		{
			AddFlatModifier(Effect, UHunterAttributeSet::GetLightningResistanceFlatBonusAttribute(), Armor.LightningResistance);
			++ModifiersAdded;
		}
		if (!FMath::IsNearlyZero(Armor.LightResistance))
		{
			AddFlatModifier(Effect, UHunterAttributeSet::GetLightResistanceFlatBonusAttribute(), Armor.LightResistance);
			++ModifiersAdded;
		}
		if (!FMath::IsNearlyZero(Armor.CorruptionResistance))
		{
			AddFlatModifier(Effect, UHunterAttributeSet::GetCorruptionResistanceFlatBonusAttribute(), Armor.CorruptionResistance);
			++ModifiersAdded;
		}
	}

	if (ModifiersAdded == 0)
	{
		UE_LOG(LogStatsManager, Verbose, TEXT("CreateEquipmentEffect: No valid modifiers were found for Item=%s."), *Item->GetName());
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
	using namespace EquipmentStatsApplierPrivate;

	if (!Effect || !Attribute.IsValid())
	{
		return false;
	}

	TArray<FStatContribution> Contributions;
	if (!GetStatContributions(Stat, Attribute, Contributions))
	{
		PH_LOG_WARNING(LogStatsManager,
			"ApplyStatModifier: Range affix '%s' targets Attribute=%s, which is not the Min attribute of a damage type; its upper endpoint cannot be applied.",
			*Stat.AffixName.ToString(), *Attribute.GetName());
	}

	bool bAddedAny = false;
	for (const FStatContribution& Contribution : Contributions)
	{
		if (!Contribution.Attribute.IsValid())
		{
			continue;
		}

		FResolvedStatModifier ResolvedModifier;
		if (!FStatsModifierMath::ResolveGameplayModifier(Stat.ModifyType, Contribution.Value, ResolvedModifier)
			|| !ResolvedModifier.bCreatesGameplayModifier)
		{
			PH_LOG_WARNING(LogStatsManager, "ApplyStatModifier skipped: Unsupported ModifyType=%d for Attribute=%s.",
				static_cast<int32>(Stat.ModifyType), *Contribution.Attribute.GetName());
			continue;
		}

		FGameplayModifierInfo Modifier;
		Modifier.Attribute = Contribution.Attribute;
		Modifier.ModifierOp = ResolvedModifier.ModOp;
		Modifier.ModifierMagnitude = FScalableFloat(ResolvedModifier.Magnitude);
		Effect->Modifiers.Add(Modifier);
		bAddedAny = true;

		UE_LOG(LogStatsManager, VeryVerbose, TEXT("StatsManager: Added modifier: %s (%s) = %.2f [Op: %d]"),
			*Contribution.Attribute.GetName(), *Stat.AttributeName.ToString(),
			ResolvedModifier.Magnitude, static_cast<int32>(ResolvedModifier.ModOp));
	}

	return bAddedAny;
}

void FEquipmentStatsApplier::RebuildEquipmentProductEffect(UStatsManager& Manager)
{
	using namespace EquipmentStatsApplierPrivate;

	UAbilitySystemComponent* ASC = FStatsAttributeResolver::GetAbilitySystemComponent(Manager);
	if (!ASC)
	{
		return;
	}

	if (Manager.ActiveEquipmentProductEffect.IsValid())
	{
		ASC->RemoveActiveGameplayEffect(Manager.ActiveEquipmentProductEffect);
		Manager.ActiveEquipmentProductEffect.Invalidate();
	}

	TMap<FGameplayAttribute, float> ProductsByAttribute;
	for (const TPair<FGuid, TObjectPtr<UItemInstance>>& Pair : Manager.ActiveEquipmentItems)
	{
		const UItemInstance* Item = Pair.Value;
		if (!IsValid(Item))
		{
			continue;
		}

		for (const FPHAttributeData& Stat : Item->Stats.GetAllStats())
		{
			if (!IsProductModifier(Stat))
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
				continue;
			}

			// MultiplyRange is a range type too, so it needs the same Min/Max
			// pairing the additive path does.
			TArray<FStatContribution> Contributions;
			GetStatContributions(Stat, Attribute, Contributions);

			for (const FStatContribution& Contribution : Contributions)
			{
				if (!Contribution.Attribute.IsValid())
				{
					continue;
				}

				float& Product = ProductsByAttribute.FindOrAdd(Contribution.Attribute, 1.f);
				Product *= GetProductFactor(Stat, Contribution.Value);
			}
		}
	}

	if (ProductsByAttribute.IsEmpty())
	{
		return;
	}

	UGameplayEffect* ProductEffect = NewObject<UGameplayEffect>(Manager.GetOwner());
	ProductEffect->DurationPolicy = EGameplayEffectDurationType::Infinite;
	for (const TPair<FGameplayAttribute, float>& Pair : ProductsByAttribute)
	{
		FGameplayModifierInfo Modifier;
		Modifier.Attribute = Pair.Key;
		Modifier.ModifierOp = EGameplayModOp::Multiplicitive;
		Modifier.ModifierMagnitude = FScalableFloat(FMath::Max(0.f, Pair.Value));
		ProductEffect->Modifiers.Add(Modifier);
	}

	FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
	EffectContext.AddSourceObject(&Manager);
	const FGameplayEffectSpec ProductSpec(ProductEffect, EffectContext, 1.f);
	Manager.ActiveEquipmentProductEffect = ASC->ApplyGameplayEffectSpecToSelf(ProductSpec);
}
