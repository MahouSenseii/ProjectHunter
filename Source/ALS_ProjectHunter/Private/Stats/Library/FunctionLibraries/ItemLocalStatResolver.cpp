#include "Stats/Library/FunctionLibraries/ItemLocalStatResolver.h"

#include "AbilitySystem/HunterAttributeSet.h"
#include "Item/ItemInstance.h"
#include "Item/Library/Structs/ItemStatsStructs.h"
#include "Item/Library/Structs/ItemStructs.h"
#include "Stats/Library/FunctionLibraries/StatsModifierMath.h"

namespace ItemLocalStatResolverPrivate
{
	struct FLocalScalarAccumulator
	{
		float Base = 0.f;
		float Flat = 0.f;
		float IncreasedPercent = 0.f;
		float Product = 1.f;
		float Override = 0.f;
		bool bHasOverride = false;

		void Accumulate(const FPHAttributeData& Stat, const float Value)
		{
			switch (Stat.ModifyType)
			{
			case EModifyType::MT_Add:
			case EModifyType::MT_AddRange:
				Flat += Value;
				break;
			case EModifyType::MT_Increased:
				IncreasedPercent += Value;
				break;
			case EModifyType::MT_Reduced:
				IncreasedPercent -= FMath::Abs(Value);
				break;
			case EModifyType::MT_Multiply:
			case EModifyType::MT_More:
			case EModifyType::MT_MultiplyRange:
				Product *= FStatsModifierMath::PercentToMultiplier(Value);
				break;
			case EModifyType::MT_Less:
				Product *= FStatsModifierMath::PercentToMultiplier(-FMath::Abs(Value));
				break;
			case EModifyType::MT_Override:
				Override = Value;
				bHasOverride = true;
				break;
			default:
				break;
			}
		}

		float Resolve() const
		{
			if (bHasOverride)
			{
				return FMath::Max(0.f, Override);
			}

			return FMath::Max(
				0.f,
				(Base + Flat)
					* FStatsModifierMath::PercentToMultiplier(IncreasedPercent)
					* Product);
		}
	};

	bool MatchesAttribute(
		const FPHAttributeData& Stat,
		const FGameplayAttribute& Attribute,
		const FName AttributeName)
	{
		return (Stat.ModifiedAttribute.IsValid() && Stat.ModifiedAttribute == Attribute)
			|| Stat.AttributeName == AttributeName;
	}

	bool IsLocal(const FPHAttributeData& Stat)
	{
		return Stat.IsLocal();
	}

	bool TryToCombatDamageType(
		const EDamageType DamageType,
		EHunterDamageType& OutDamageType)
	{
		switch (DamageType)
		{
		case EDamageType::DT_Physical:   OutDamageType = EHunterDamageType::Physical; return true;
		case EDamageType::DT_Fire:       OutDamageType = EHunterDamageType::Fire; return true;
		case EDamageType::DT_Ice:        OutDamageType = EHunterDamageType::Ice; return true;
		case EDamageType::DT_Lightning:  OutDamageType = EHunterDamageType::Lightning; return true;
		case EDamageType::DT_Light:      OutDamageType = EHunterDamageType::Light; return true;
		case EDamageType::DT_Corruption: OutDamageType = EHunterDamageType::Corruption; return true;
		default:                         return false;
		}
	}

	void ResolveScalar(
		float& Value,
		const FPHItemStats& ItemStats,
		const FGameplayAttribute& Attribute,
		const FName AttributeName)
	{
		FLocalScalarAccumulator Accumulator;
		Accumulator.Base = Value;

		ItemStats.ForEachStat([&](const FPHAttributeData& Stat)
		{
			if (IsLocal(Stat) && MatchesAttribute(Stat, Attribute, AttributeName))
			{
				Accumulator.Accumulate(Stat, Stat.RolledStatValue);
			}
		});

		Value = Accumulator.Resolve();
	}

	void ResolveDefensiveScalar(
		float& Value,
		const FPHItemStats& ItemStats,
		const FGameplayAttribute& BaseAttribute,
		const FGameplayAttribute& FlatAttribute,
		const FGameplayAttribute& PercentAttribute,
		const FName BaseName,
		const FName FlatName,
		const FName PercentName)
	{
		FLocalScalarAccumulator Accumulator;
		Accumulator.Base = Value;

		ItemStats.ForEachStat([&](const FPHAttributeData& Stat)
		{
			if (!IsLocal(Stat))
			{
				return;
			}

			if (MatchesAttribute(Stat, PercentAttribute, PercentName))
			{
				Accumulator.IncreasedPercent += Stat.ModifyType == EModifyType::MT_Reduced
					? -FMath::Abs(Stat.RolledStatValue)
					: Stat.RolledStatValue;
				return;
			}

			if (MatchesAttribute(Stat, BaseAttribute, BaseName)
				|| MatchesAttribute(Stat, FlatAttribute, FlatName))
			{
				Accumulator.Accumulate(Stat, Stat.RolledStatValue);
			}
		});

		Value = Accumulator.Resolve();
	}

	void ResolveDamageRange(
		float& MinValue,
		float& MaxValue,
		const FPHItemStats& ItemStats,
		const FGameplayAttribute& MinAttribute,
		const FGameplayAttribute& MaxAttribute,
		const FGameplayAttribute& PercentAttribute,
		const FName MinName,
		const FName MaxName,
		const FName PercentName)
	{
		FLocalScalarAccumulator MinAccum;
		FLocalScalarAccumulator MaxAccum;
		MinAccum.Base = MinValue;
		MaxAccum.Base = MaxValue;

		ItemStats.ForEachStat([&](const FPHAttributeData& Stat)
		{
			if (!IsLocal(Stat))
			{
				return;
			}

			if (MatchesAttribute(Stat, PercentAttribute, PercentName))
			{
				MinAccum.Accumulate(Stat, Stat.RolledStatValue);
				MaxAccum.Accumulate(Stat, Stat.RolledStatValue);
				return;
			}

			if (MatchesAttribute(Stat, MinAttribute, MinName))
			{
				MinAccum.Accumulate(Stat, Stat.RolledStatValue);
				if (Stat.UsesValueRange())
				{
					MaxAccum.Accumulate(Stat, Stat.RolledSecondaryStatValue);
				}
				return;
			}

			if (MatchesAttribute(Stat, MaxAttribute, MaxName))
			{
				MaxAccum.Accumulate(Stat, Stat.RolledStatValue);
			}
		});

		MinValue = MinAccum.Resolve();
		MaxValue = FMath::Max(MinValue, MaxAccum.Resolve());
	}
}

float FResolvedWeaponStats::GetMinDamage(const EHunterDamageType DamageType) const
{
	switch (DamageType)
	{
	case EHunterDamageType::Physical:   return Values.MinPhysicalDamage;
	case EHunterDamageType::Fire:       return Values.MinFireDamage;
	case EHunterDamageType::Ice:        return Values.MinIceDamage;
	case EHunterDamageType::Lightning:  return Values.MinLightningDamage;
	case EHunterDamageType::Light:      return Values.MinLightDamage;
	case EHunterDamageType::Corruption: return Values.MinCorruptionDamage;
	default:                            return 0.f;
	}
}

float FResolvedWeaponStats::GetMaxDamage(const EHunterDamageType DamageType) const
{
	switch (DamageType)
	{
	case EHunterDamageType::Physical:   return Values.MaxPhysicalDamage;
	case EHunterDamageType::Fire:       return Values.MaxFireDamage;
	case EHunterDamageType::Ice:        return Values.MaxIceDamage;
	case EHunterDamageType::Lightning:  return Values.MaxLightningDamage;
	case EHunterDamageType::Light:      return Values.MaxLightDamage;
	case EHunterDamageType::Corruption: return Values.MaxCorruptionDamage;
	default:                            return 0.f;
	}
}

FResolvedWeaponStats FItemLocalStatResolver::ResolveWeapon(
	const FBaseWeaponStats& BaseStats,
	const FPHItemStats& ItemStats)
{
	using namespace ItemLocalStatResolverPrivate;

	FResolvedWeaponStats Result;
	Result.bIsValid = true;
	Result.Values = BaseStats;

	ResolveDamageRange(Result.Values.MinPhysicalDamage, Result.Values.MaxPhysicalDamage, ItemStats,
		UHunterAttributeSet::GetMinPhysicalDamageAttribute(), UHunterAttributeSet::GetMaxPhysicalDamageAttribute(),
		UHunterAttributeSet::GetPhysicalPercentDamageAttribute(), TEXT("MinPhysicalDamage"), TEXT("MaxPhysicalDamage"), TEXT("PhysicalPercentDamage"));
	ResolveDamageRange(Result.Values.MinFireDamage, Result.Values.MaxFireDamage, ItemStats,
		UHunterAttributeSet::GetMinFireDamageAttribute(), UHunterAttributeSet::GetMaxFireDamageAttribute(),
		UHunterAttributeSet::GetFirePercentDamageAttribute(), TEXT("MinFireDamage"), TEXT("MaxFireDamage"), TEXT("FirePercentDamage"));
	ResolveDamageRange(Result.Values.MinIceDamage, Result.Values.MaxIceDamage, ItemStats,
		UHunterAttributeSet::GetMinIceDamageAttribute(), UHunterAttributeSet::GetMaxIceDamageAttribute(),
		UHunterAttributeSet::GetIcePercentDamageAttribute(), TEXT("MinIceDamage"), TEXT("MaxIceDamage"), TEXT("IcePercentDamage"));
	ResolveDamageRange(Result.Values.MinLightningDamage, Result.Values.MaxLightningDamage, ItemStats,
		UHunterAttributeSet::GetMinLightningDamageAttribute(), UHunterAttributeSet::GetMaxLightningDamageAttribute(),
		UHunterAttributeSet::GetLightningPercentDamageAttribute(), TEXT("MinLightningDamage"), TEXT("MaxLightningDamage"), TEXT("LightningPercentDamage"));
	ResolveDamageRange(Result.Values.MinLightDamage, Result.Values.MaxLightDamage, ItemStats,
		UHunterAttributeSet::GetMinLightDamageAttribute(), UHunterAttributeSet::GetMaxLightDamageAttribute(),
		UHunterAttributeSet::GetLightPercentDamageAttribute(), TEXT("MinLightDamage"), TEXT("MaxLightDamage"), TEXT("LightPercentDamage"));
	ResolveDamageRange(Result.Values.MinCorruptionDamage, Result.Values.MaxCorruptionDamage, ItemStats,
		UHunterAttributeSet::GetMinCorruptionDamageAttribute(), UHunterAttributeSet::GetMaxCorruptionDamageAttribute(),
		UHunterAttributeSet::GetCorruptionPercentDamageAttribute(), TEXT("MinCorruptionDamage"), TEXT("MaxCorruptionDamage"), TEXT("CorruptionPercentDamage"));

	ResolveScalar(Result.Values.AttackSpeed, ItemStats,
		UHunterAttributeSet::GetAttackSpeedAttribute(), TEXT("AttackSpeed"));
	ResolveScalar(Result.Values.CriticalStrikeChance, ItemStats,
		UHunterAttributeSet::GetCritChanceAttribute(), TEXT("CritChance"));
	ResolveScalar(Result.Values.Range, ItemStats,
		UHunterAttributeSet::GetAttackRangeAttribute(), TEXT("AttackRange"));

	ItemStats.ForEachStat([&Result](const FPHAttributeData& Stat)
	{
		if (!Stat.IsLocal() || Stat.ModifyType != EModifyType::MT_ConvertTo
			|| Stat.FromDamageType == Stat.ToDamageType)
		{
			return;
		}

		EHunterDamageType FromType;
		EHunterDamageType ToType;
		if (!ItemLocalStatResolverPrivate::TryToCombatDamageType(
				Stat.FromDamageType, FromType)
			|| !ItemLocalStatResolverPrivate::TryToCombatDamageType(
				Stat.ToDamageType, ToType))
		{
			return;
		}

		FCombatDamageConversionRule& Rule = Result.LocalDamageConversions.AddDefaulted_GetRef();
		Rule.From = FromType;
		Rule.To = ToType;
		Rule.Percent = FMath::Max(0.f, Stat.RolledStatValue);
		Rule.bGainAsExtra = Stat.bGainAsExtra;
	});

	return Result;
}

FResolvedArmorStats FItemLocalStatResolver::ResolveArmor(
	const FBaseArmorStats& BaseStats,
	const FPHItemStats& ItemStats)
{
	using namespace ItemLocalStatResolverPrivate;

	FResolvedArmorStats Result;
	Result.bIsValid = true;
	Result.Values = BaseStats;

	ResolveDefensiveScalar(Result.Values.Armor, ItemStats,
		UHunterAttributeSet::GetArmourAttribute(), UHunterAttributeSet::GetArmourFlatBonusAttribute(),
		UHunterAttributeSet::GetArmourPercentBonusAttribute(), TEXT("Armour"), TEXT("ArmourFlatBonus"), TEXT("ArmourPercentBonus"));
	ResolveDefensiveScalar(Result.Values.FireResistance, ItemStats,
		UHunterAttributeSet::GetFireResistanceFlatBonusAttribute(), UHunterAttributeSet::GetFireResistanceFlatBonusAttribute(),
		UHunterAttributeSet::GetFireResistancePercentBonusAttribute(), TEXT("FireResistance"), TEXT("FireResistanceFlatBonus"), TEXT("FireResistancePercentBonus"));
	ResolveDefensiveScalar(Result.Values.IceResistance, ItemStats,
		UHunterAttributeSet::GetIceResistanceFlatBonusAttribute(), UHunterAttributeSet::GetIceResistanceFlatBonusAttribute(),
		UHunterAttributeSet::GetIceResistancePercentBonusAttribute(), TEXT("IceResistance"), TEXT("IceResistanceFlatBonus"), TEXT("IceResistancePercentBonus"));
	ResolveDefensiveScalar(Result.Values.LightningResistance, ItemStats,
		UHunterAttributeSet::GetLightningResistanceFlatBonusAttribute(), UHunterAttributeSet::GetLightningResistanceFlatBonusAttribute(),
		UHunterAttributeSet::GetLightningResistancePercentBonusAttribute(), TEXT("LightningResistance"), TEXT("LightningResistanceFlatBonus"), TEXT("LightningResistancePercentBonus"));
	ResolveDefensiveScalar(Result.Values.LightResistance, ItemStats,
		UHunterAttributeSet::GetLightResistanceFlatBonusAttribute(), UHunterAttributeSet::GetLightResistanceFlatBonusAttribute(),
		UHunterAttributeSet::GetLightResistancePercentBonusAttribute(), TEXT("LightResistance"), TEXT("LightResistanceFlatBonus"), TEXT("LightResistancePercentBonus"));
	ResolveDefensiveScalar(Result.Values.CorruptionResistance, ItemStats,
		UHunterAttributeSet::GetCorruptionResistanceFlatBonusAttribute(), UHunterAttributeSet::GetCorruptionResistanceFlatBonusAttribute(),
		UHunterAttributeSet::GetCorruptionResistancePercentBonusAttribute(), TEXT("CorruptionResistance"), TEXT("CorruptionResistanceFlatBonus"), TEXT("CorruptionResistancePercentBonus"));

	return Result;
}

bool FItemLocalStatResolver::ResolveWeapon(const UItemInstance* Item, FResolvedWeaponStats& OutStats)
{
	OutStats = FResolvedWeaponStats{};
	const FItemBase* BaseData = IsValid(Item) ? Item->GetBaseData() : nullptr;
	if (!BaseData || !BaseData->IsWeapon())
	{
		return false;
	}

	OutStats = ResolveWeapon(BaseData->WeaponStats, Item->Stats);
	return true;
}

bool FItemLocalStatResolver::ResolveArmor(const UItemInstance* Item, FResolvedArmorStats& OutStats)
{
	OutStats = FResolvedArmorStats{};
	const FItemBase* BaseData = IsValid(Item) ? Item->GetBaseData() : nullptr;
	if (!BaseData || !BaseData->IsArmor())
	{
		return false;
	}

	OutStats = ResolveArmor(BaseData->ArmorStats, Item->Stats);
	return true;
}

bool UItemLocalStatFunctionLibrary::ResolveWeaponStats(
	const UItemInstance* Item,
	FResolvedWeaponStats& OutStats)
{
	return FItemLocalStatResolver::ResolveWeapon(Item, OutStats);
}

bool UItemLocalStatFunctionLibrary::ResolveArmorStats(
	const UItemInstance* Item,
	FResolvedArmorStats& OutStats)
{
	return FItemLocalStatResolver::ResolveArmor(Item, OutStats);
}
