#include "Stats/EquipmentStatsApplier.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/HunterAttributeSet.h"
#include "Core/Logging/ProjectHunterLogMacros.h"
#include "GameplayEffect.h"
#include "Item/ItemInstance.h"
#include "Item/Library/AffixEnums.h"
#include "Item/Library/ItemStructs.h"
#include "Equipment/Components/EquipmentManager.h"
#include "Stats/Components/StatsManager.h"
#include "Stats/StatsAttributeResolver.h"
#include "Stats/StatsModifierMath.h"

namespace EquipmentStatsApplierPrivate
{
	// One slot per damage type, index order: Physical, Fire, Ice, Lightning, Light, Corruption.
	constexpr int32 NumWeaponDamageTypes = 6;

	/**
	 * One side (Min or Max) of one damage type on a weapon.
	 * Local affix math mirrors POE: (Base + LocalFlat) * (1 + LocalIncreasedPct/100).
	 * Local "More/Less" is folded into LocalIncreasedPct — acceptable simplification
	 * while local mods are rare; revisit if local More-stacking becomes a build axis.
	 */
	struct FWeaponDamageSide
	{
		float Base = 0.f;
		float LocalFlat = 0.f;
		float LocalIncreasedPct = 0.f;

		bool HasContribution() const
		{
			return Base > 0.f
				|| !FMath::IsNearlyZero(LocalFlat)
				|| !FMath::IsNearlyZero(LocalIncreasedPct);
		}

		float Resolve() const
		{
			return FMath::Max(0.f, (Base + LocalFlat) * FStatsModifierMath::PercentToMultiplier(LocalIncreasedPct));
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

	/** True (+ type/side out-params) when Attribute is one of the 12 weapon damage range attributes. */
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
		return Stat.ModifiedLocation == EAffixScope::AS_Local
			|| Stat.bIsLocalToWeapon                  // legacy flag
			|| Stat.bAffectsBaseWeaponStatsDirectly;  // legacy flag
	}

	/** Fold one local damage affix into the accumulator. False if the ModifyType isn't foldable. */
	bool AccumulateLocalDamageMod(const FPHAttributeData& Stat, FWeaponDamageSide& Side)
	{
		switch (Stat.ModifyType)
		{
		case EModifyType::MT_Add:
		case EModifyType::MT_AddRange:
			Side.LocalFlat += Stat.RolledStatValue;
			return true;

		case EModifyType::MT_Reduced:
			Side.LocalFlat -= FMath::Abs(Stat.RolledStatValue);
			return true;

		case EModifyType::MT_Increased:
		case EModifyType::MT_Multiply:
		case EModifyType::MT_More:
			Side.LocalIncreasedPct += Stat.RolledStatValue;
			return true;

		case EModifyType::MT_Less:
			Side.LocalIncreasedPct -= FMath::Abs(Stat.RolledStatValue);
			return true;

		default:
			return false;
		}
	}

	void AddFlatModifier(UGameplayEffect* Effect, const FGameplayAttribute& Attribute, const float Magnitude)
	{
		FGameplayModifierInfo Modifier;
		Modifier.Attribute = Attribute;
		Modifier.ModifierOp = EGameplayModOp::Additive;
		Modifier.ModifierMagnitude = FScalableFloat(Magnitude);
		Effect->Modifiers.Add(Modifier);
	}

	/** True if any FBaseWeaponStats damage range or FBaseArmorStats value would emit a modifier. */
	bool HasBaseEquipmentContribution(const FItemBase& Base)
	{
		if (Base.IsWeapon())
		{
			const FBaseWeaponStats& W = Base.WeaponStats;
			return W.MinPhysicalDamage > 0.f || W.MaxPhysicalDamage > 0.f
				|| W.MinFireDamage > 0.f || W.MaxFireDamage > 0.f
				|| W.MinIceDamage > 0.f || W.MaxIceDamage > 0.f
				|| W.MinLightningDamage > 0.f || W.MaxLightningDamage > 0.f
				|| W.MinLightDamage > 0.f || W.MaxLightDamage > 0.f
				|| W.MinCorruptionDamage > 0.f || W.MaxCorruptionDamage > 0.f;
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

	// A plain weapon/armor with zero affixes still contributes its BASE stats
	// (FBaseWeaponStats damage ranges / FBaseArmorStats), so only skip when the
	// item has neither affixes nor base contributions.
	const FItemBase* BaseData = Item->GetBaseData();
	const bool bHasBaseContribution = BaseData
		&& EquipmentStatsApplierPrivate::HasBaseEquipmentContribution(*BaseData);

	if (AllStats.Num() == 0 && !bHasBaseContribution)
	{
		UE_LOG(LogStatsManager, Verbose, TEXT("StatsManager: Item %s has no stats to apply"), *Item->GetName());
		return;
	}

	FGameplayEffectSpecHandle EffectSpec = CreateEquipmentEffect(Manager, Item, AllStats);
	if (!EffectSpec.IsValid())
	{
		PH_LOG_WARNING(LogStatsManager, "ApplyEquipmentStats: No applicable modifiers (or spec creation failed) for Item=%s.", *Item->GetName());
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

	using namespace EquipmentStatsApplierPrivate;

	// ── Base item stats (previously authored in the DataTable but never applied) ──
	// Weapons: damage ranges seed the accumulator; LOCAL affixes fold into it below
	// so "(Base + LocalFlat) * (1 + LocalPct)" lands as one flat modifier per
	// attribute. Armor: Armour + resistances emit directly.
	//
	// NOTE (dual wield): with two weapons equipped, both contribute to the same
	// Min/Max attributes — the combat roll treats them as one combined range.
	// Revisit when main/off-hand alternation becomes a design requirement.
	const FItemBase* BaseData = Item->GetBaseData();
	const bool bIsWeapon = BaseData && BaseData->IsWeapon();

	FWeaponDamageAccumulator WeaponAccum;
	if (bIsWeapon)
	{
		SeedWeaponBase(BaseData->WeaponStats, WeaponAccum);
	}

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

		// LOCAL affixes modify this item, not the character sheet. On weapons,
		// fold damage-range locals into the weapon contribution; anything else
		// falls through to global application with a warning so data problems
		// are visible instead of silently global (the old behavior).
		if (IsLocalAffix(Stat))
		{
			int32 TypeIndex = INDEX_NONE;
			bool bIsMin = false;

			if (bIsWeapon && ResolveWeaponDamageAttribute(Attribute, TypeIndex, bIsMin))
			{
				FWeaponDamageSide& Side = bIsMin
					? WeaponAccum.Min[TypeIndex]
					: WeaponAccum.Max[TypeIndex];

				if (AccumulateLocalDamageMod(Stat, Side))
				{
					continue; // emitted later with the weapon base
				}
			}

			PH_LOG_WARNING(LogStatsManager,
				"CreateEquipmentEffect: LOCAL affix '%s' on Item=%s targets %s, which has no local fold path. Applying globally.",
				*Stat.AttributeName.ToString(), *Item->GetName(), *Attribute.GetName());
		}

		if (ApplyStatModifier(Effect, Stat, Attribute))
		{
			++ModifiersAdded;
		}
	}

	// ── Emit weapon damage ranges (base + folded locals) ──
	if (bIsWeapon)
	{
		for (int32 TypeIndex = 0; TypeIndex < NumWeaponDamageTypes; ++TypeIndex)
		{
			FGameplayAttribute MinAttr;
			FGameplayAttribute MaxAttr;
			GetWeaponMinMaxAttributes(TypeIndex, MinAttr, MaxAttr);

			if (WeaponAccum.Min[TypeIndex].HasContribution())
			{
				AddFlatModifier(Effect, MinAttr, WeaponAccum.Min[TypeIndex].Resolve());
				++ModifiersAdded;
			}
			if (WeaponAccum.Max[TypeIndex].HasContribution())
			{
				AddFlatModifier(Effect, MaxAttr, WeaponAccum.Max[TypeIndex].Resolve());
				++ModifiersAdded;
			}
		}
	}

	// ── Emit armor base stats ──
	if (BaseData && BaseData->IsArmor())
	{
		const FBaseArmorStats& Armor = BaseData->ArmorStats;

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
