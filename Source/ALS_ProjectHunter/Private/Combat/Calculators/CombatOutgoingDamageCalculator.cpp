#include "Combat/Calculators/CombatOutgoingDamageCalculator.h"

#include "AbilitySystem/HunterAttributeSet.h"
#include "Combat/Library/CombatDebug.h"
#include "Stats/Library/Structs/ResolvedItemStats.h"
#include "Stats/Library/Structs/StatsStructs.h"

DEFINE_LOG_CATEGORY_STATIC(LogCombatOutgoingDamageCalculator, Log, All);

namespace CombatOutgoingDamageCalculatorPrivate
{
	bool IsCombatDebugLoggingEnabled()
	{
		return PHCombatDebug::IsCombatDebugLoggingEnabled();
	}

	constexpr EHunterDamageType AllDamageTypes[] =
	{
		EHunterDamageType::Physical,
		EHunterDamageType::Fire,
		EHunterDamageType::Ice,
		EHunterDamageType::Lightning,
		EHunterDamageType::Light,
		EHunterDamageType::Corruption
	};

	bool IsElementalDamageType(const EHunterDamageType DamageType)
	{
		return DamageType == EHunterDamageType::Fire
			|| DamageType == EHunterDamageType::Ice
			|| DamageType == EHunterDamageType::Lightning
			|| DamageType == EHunterDamageType::Light;
	}

	float ApplyPercentIncrease(const float BaseValue, const float IncreasedPercent)
	{
		return BaseValue * (1.f + (IncreasedPercent / 100.f));
	}

	// Multiplier attributes use a literal ratio: 1.0 neutral, 0.5 half, 0.0 none,
	// 2.0 double. Every one of them is seeded to 1.0 in UHunterAttributeSet's
	// constructor, so an incoming 0 is a real "deal no damage" modifier and must
	// survive. Only negative values are nonsense, and those clamp to 0.
	float SanitizeMultiplier(const float Value)
	{
		return FMath::Max(0.f, Value);
	}

	float GetPacketDamage(const FCombatDamagePacket& Packet, const EHunterDamageType DamageType)
	{
		switch (DamageType)
		{
		case EHunterDamageType::Physical:   return Packet.Physical;
		case EHunterDamageType::Fire:       return Packet.Fire;
		case EHunterDamageType::Ice:        return Packet.Ice;
		case EHunterDamageType::Lightning:  return Packet.Lightning;
		case EHunterDamageType::Light:      return Packet.Light;
		case EHunterDamageType::Corruption: return Packet.Corruption;
		default:                            return 0.f;
		}
	}

	void SetPacketDamage(FCombatDamagePacket& Packet, const EHunterDamageType DamageType, const float Value)
	{
		switch (DamageType)
		{
		case EHunterDamageType::Physical:   Packet.Physical = Value; break;
		case EHunterDamageType::Fire:       Packet.Fire = Value; break;
		case EHunterDamageType::Ice:        Packet.Ice = Value; break;
		case EHunterDamageType::Lightning:  Packet.Lightning = Value; break;
		case EHunterDamageType::Light:      Packet.Light = Value; break;
		case EHunterDamageType::Corruption: Packet.Corruption = Value; break;
		default: break;
		}
	}

	void UpdatePacketTotal(FCombatDamagePacket& Packet)
	{
		Packet.TotalPreMitigation =
			Packet.Physical + Packet.Fire + Packet.Ice +
			Packet.Lightning + Packet.Light + Packet.Corruption;
	}

	float GetSkillBaseDamage(const FAnimationSkillBaseDamage& BaseDamage, const EHunterDamageType DamageType)
	{
		switch (DamageType)
		{
		case EHunterDamageType::Physical:   return BaseDamage.Physical;
		case EHunterDamageType::Fire:       return BaseDamage.Fire;
		case EHunterDamageType::Ice:        return BaseDamage.Ice;
		case EHunterDamageType::Lightning:  return BaseDamage.Lightning;
		case EHunterDamageType::Light:      return BaseDamage.Light;
		case EHunterDamageType::Corruption: return BaseDamage.Corruption;
		default:                            return 0.f;
		}
	}

	float ResolveContextual(
		const FContextualStatModifierSnapshot* Snapshot,
		const FGameplayAttribute& Attribute,
		const float BaseValue)
	{
		return Snapshot ? Snapshot->Resolve(Attribute, BaseValue) : BaseValue;
	}

	void AddPacket(FCombatDamagePacket& Target, const FCombatDamagePacket& Source)
	{
		Target.Physical += Source.Physical;
		Target.Fire += Source.Fire;
		Target.Ice += Source.Ice;
		Target.Lightning += Source.Lightning;
		Target.Light += Source.Light;
		Target.Corruption += Source.Corruption;
		UpdatePacketTotal(Target);
	}

	float GetAttributeWeaponMin(
		const UHunterAttributeSet* Attributes,
		const EHunterDamageType DamageType,
		const FContextualStatModifierSnapshot* Snapshot)
	{
		switch (DamageType)
		{
		case EHunterDamageType::Physical:   return ResolveContextual(Snapshot, UHunterAttributeSet::GetMinPhysicalDamageAttribute(), Attributes->GetMinPhysicalDamage());
		case EHunterDamageType::Fire:       return ResolveContextual(Snapshot, UHunterAttributeSet::GetMinFireDamageAttribute(), Attributes->GetMinFireDamage());
		case EHunterDamageType::Ice:        return ResolveContextual(Snapshot, UHunterAttributeSet::GetMinIceDamageAttribute(), Attributes->GetMinIceDamage());
		case EHunterDamageType::Lightning:  return ResolveContextual(Snapshot, UHunterAttributeSet::GetMinLightningDamageAttribute(), Attributes->GetMinLightningDamage());
		case EHunterDamageType::Light:      return ResolveContextual(Snapshot, UHunterAttributeSet::GetMinLightDamageAttribute(), Attributes->GetMinLightDamage());
		case EHunterDamageType::Corruption: return ResolveContextual(Snapshot, UHunterAttributeSet::GetMinCorruptionDamageAttribute(), Attributes->GetMinCorruptionDamage());
		default:                            return 0.f;
		}
	}

	float GetAttributeWeaponMax(
		const UHunterAttributeSet* Attributes,
		const EHunterDamageType DamageType,
		const FContextualStatModifierSnapshot* Snapshot)
	{
		switch (DamageType)
		{
		case EHunterDamageType::Physical:   return ResolveContextual(Snapshot, UHunterAttributeSet::GetMaxPhysicalDamageAttribute(), Attributes->GetMaxPhysicalDamage());
		case EHunterDamageType::Fire:       return ResolveContextual(Snapshot, UHunterAttributeSet::GetMaxFireDamageAttribute(), Attributes->GetMaxFireDamage());
		case EHunterDamageType::Ice:        return ResolveContextual(Snapshot, UHunterAttributeSet::GetMaxIceDamageAttribute(), Attributes->GetMaxIceDamage());
		case EHunterDamageType::Lightning:  return ResolveContextual(Snapshot, UHunterAttributeSet::GetMaxLightningDamageAttribute(), Attributes->GetMaxLightningDamage());
		case EHunterDamageType::Light:      return ResolveContextual(Snapshot, UHunterAttributeSet::GetMaxLightDamageAttribute(), Attributes->GetMaxLightDamage());
		case EHunterDamageType::Corruption: return ResolveContextual(Snapshot, UHunterAttributeSet::GetMaxCorruptionDamageAttribute(), Attributes->GetMaxCorruptionDamage());
		default:                            return 0.f;
		}
	}

	float GetFlatDamage(
		const UHunterAttributeSet* Attributes,
		const EHunterDamageType DamageType,
		const FContextualStatModifierSnapshot* Snapshot)
	{
		switch (DamageType)
		{
		case EHunterDamageType::Physical:   return ResolveContextual(Snapshot, UHunterAttributeSet::GetPhysicalFlatDamageAttribute(), Attributes->GetPhysicalFlatDamage());
		case EHunterDamageType::Fire:       return ResolveContextual(Snapshot, UHunterAttributeSet::GetFireFlatDamageAttribute(), Attributes->GetFireFlatDamage());
		case EHunterDamageType::Ice:        return ResolveContextual(Snapshot, UHunterAttributeSet::GetIceFlatDamageAttribute(), Attributes->GetIceFlatDamage());
		case EHunterDamageType::Lightning:  return ResolveContextual(Snapshot, UHunterAttributeSet::GetLightningFlatDamageAttribute(), Attributes->GetLightningFlatDamage());
		case EHunterDamageType::Light:      return ResolveContextual(Snapshot, UHunterAttributeSet::GetLightFlatDamageAttribute(), Attributes->GetLightFlatDamage());
		case EHunterDamageType::Corruption: return ResolveContextual(Snapshot, UHunterAttributeSet::GetCorruptionFlatDamageAttribute(), Attributes->GetCorruptionFlatDamage());
		default:                            return 0.f;
		}
	}

	/**
	 * Attribute conversion percent from one damage type to another.
	 * Same-type and unknown pairs return 0.
	 */
	float GetConversionPercent(
		const UHunterAttributeSet* Attributes,
		const EHunterDamageType From,
		const EHunterDamageType To,
		const FContextualStatModifierSnapshot* Snapshot)
	{
		if (!Attributes || From == To)
		{
			return 0.f;
		}

		#define PH_RETURN_CONVERSION(Name) \
			return ResolveContextual(Snapshot, UHunterAttributeSet::Get##Name##Attribute(), Attributes->Get##Name())

		switch (From)
		{
		case EHunterDamageType::Physical:
			switch (To)
			{
			case EHunterDamageType::Fire:       PH_RETURN_CONVERSION(PhysicalToFire);
			case EHunterDamageType::Ice:        PH_RETURN_CONVERSION(PhysicalToIce);
			case EHunterDamageType::Lightning:  PH_RETURN_CONVERSION(PhysicalToLightning);
			case EHunterDamageType::Light:      PH_RETURN_CONVERSION(PhysicalToLight);
			case EHunterDamageType::Corruption: PH_RETURN_CONVERSION(PhysicalToCorruption);
			default: return 0.f;
			}
		case EHunterDamageType::Fire:
			switch (To)
			{
			case EHunterDamageType::Physical:   PH_RETURN_CONVERSION(FireToPhysical);
			case EHunterDamageType::Ice:        PH_RETURN_CONVERSION(FireToIce);
			case EHunterDamageType::Lightning:  PH_RETURN_CONVERSION(FireToLightning);
			case EHunterDamageType::Light:      PH_RETURN_CONVERSION(FireToLight);
			case EHunterDamageType::Corruption: PH_RETURN_CONVERSION(FireToCorruption);
			default: return 0.f;
			}
		case EHunterDamageType::Ice:
			switch (To)
			{
			case EHunterDamageType::Physical:   PH_RETURN_CONVERSION(IceToPhysical);
			case EHunterDamageType::Fire:       PH_RETURN_CONVERSION(IceToFire);
			case EHunterDamageType::Lightning:  PH_RETURN_CONVERSION(IceToLightning);
			case EHunterDamageType::Light:      PH_RETURN_CONVERSION(IceToLight);
			case EHunterDamageType::Corruption: PH_RETURN_CONVERSION(IceToCorruption);
			default: return 0.f;
			}
		case EHunterDamageType::Lightning:
			switch (To)
			{
			case EHunterDamageType::Physical:   PH_RETURN_CONVERSION(LightningToPhysical);
			case EHunterDamageType::Fire:       PH_RETURN_CONVERSION(LightningToFire);
			case EHunterDamageType::Ice:        PH_RETURN_CONVERSION(LightningToIce);
			case EHunterDamageType::Light:      PH_RETURN_CONVERSION(LightningToLight);
			case EHunterDamageType::Corruption: PH_RETURN_CONVERSION(LightningToCorruption);
			default: return 0.f;
			}
		case EHunterDamageType::Light:
			switch (To)
			{
			case EHunterDamageType::Physical:   PH_RETURN_CONVERSION(LightToPhysical);
			case EHunterDamageType::Fire:       PH_RETURN_CONVERSION(LightToFire);
			case EHunterDamageType::Ice:        PH_RETURN_CONVERSION(LightToIce);
			case EHunterDamageType::Lightning:  PH_RETURN_CONVERSION(LightToLightning);
			case EHunterDamageType::Corruption: PH_RETURN_CONVERSION(LightToCorruption);
			default: return 0.f;
			}
		case EHunterDamageType::Corruption:
			switch (To)
			{
			case EHunterDamageType::Physical:   PH_RETURN_CONVERSION(CorruptionToPhysical);
			case EHunterDamageType::Fire:       PH_RETURN_CONVERSION(CorruptionToFire);
			case EHunterDamageType::Ice:        PH_RETURN_CONVERSION(CorruptionToIce);
			case EHunterDamageType::Lightning:  PH_RETURN_CONVERSION(CorruptionToLightning);
			case EHunterDamageType::Light:      PH_RETURN_CONVERSION(CorruptionToLight);
			default: return 0.f;
			}
		default:
			#undef PH_RETURN_CONVERSION
			return 0.f;
		}
	}
}

float FCombatOutgoingDamageCalculator::RollDamageRange(
	const float MinDamage,
	const float MaxDamage,
	FRandomStream& RandomStream)
{
	const float Low = FMath::Max(0.f, FMath::Min(MinDamage, MaxDamage));
	const float High = FMath::Max(0.f, FMath::Max(MinDamage, MaxDamage));
	return High > Low ? RandomStream.FRandRange(Low, High) : High;
}

FCombatDamagePacket FCombatOutgoingDamageCalculator::BuildOutgoingDamagePacket(
	const UHunterAttributeSet* AttackerAttributes,
	const FAnimationDamageInfo& DamageInfo,
	FRandomStream& RandomStream,
	const FResolvedWeaponStats* WeaponStats,
	const FContextualStatModifierSnapshot* ContextualModifiers)
{
	FCombatDamagePacket Packet;
	if (!AttackerAttributes)
	{
		return Packet;
	}

	const bool bDebugLog = CombatOutgoingDamageCalculatorPrivate::IsCombatDebugLoggingEnabled();

	FCombatDamagePacket WeaponPacket;
	FCombatDamagePacket AddedAndSkillPacket;
	const float WeaponEffectiveness = FMath::Max(0.f, DamageInfo.WeaponDamageEffectivenessPercent) / 100.f;
	const float AddedEffectiveness = FMath::Max(0.f, DamageInfo.AddedDamageEffectivenessPercent) / 100.f;

	for (const EHunterDamageType DamageType : CombatOutgoingDamageCalculatorPrivate::AllDamageTypes)
	{
		const float WeaponMin = WeaponStats && WeaponStats->bIsValid
			? WeaponStats->GetMinDamage(DamageType)
			: CombatOutgoingDamageCalculatorPrivate::GetAttributeWeaponMin(AttackerAttributes, DamageType, ContextualModifiers);
		const float WeaponMax = WeaponStats && WeaponStats->bIsValid
			? WeaponStats->GetMaxDamage(DamageType)
			: CombatOutgoingDamageCalculatorPrivate::GetAttributeWeaponMax(AttackerAttributes, DamageType, ContextualModifiers);

		CombatOutgoingDamageCalculatorPrivate::SetPacketDamage(
			WeaponPacket,
			DamageType,
			RollDamageRange(WeaponMin, WeaponMax, RandomStream) * WeaponEffectiveness);

		const float AddedDamage = CombatOutgoingDamageCalculatorPrivate::GetFlatDamage(
			AttackerAttributes, DamageType, ContextualModifiers) * AddedEffectiveness;
		const float SkillBaseDamage = CombatOutgoingDamageCalculatorPrivate::GetSkillBaseDamage(
			DamageInfo.SkillBaseDamage, DamageType);
		CombatOutgoingDamageCalculatorPrivate::SetPacketDamage(
			AddedAndSkillPacket,
			DamageType,
			FMath::Max(0.f, AddedDamage + SkillBaseDamage));
	}
	CombatOutgoingDamageCalculatorPrivate::UpdatePacketTotal(WeaponPacket);
	CombatOutgoingDamageCalculatorPrivate::UpdatePacketTotal(AddedAndSkillPacket);

	// Damage over time is authored in its final type. Hit conversion never
	// rewrites it. For hits, local weapon conversion happens before the skill's
	// own conversion, then character/equipment conversion runs last.
	if (!DamageInfo.Tags.bIsDamageOverTime
		&& WeaponStats
		&& WeaponStats->bIsValid
		&& !WeaponStats->LocalDamageConversions.IsEmpty())
	{
		WeaponPacket = ApplyDamageConversionRules(
			WeaponPacket, WeaponStats->LocalDamageConversions);
	}

	Packet = WeaponPacket;
	CombatOutgoingDamageCalculatorPrivate::AddPacket(Packet, AddedAndSkillPacket);

	if (!DamageInfo.Tags.bIsDamageOverTime)
	{
		if (!DamageInfo.SkillDamageConversions.IsEmpty())
		{
			Packet = ApplyDamageConversionRules(Packet, DamageInfo.SkillDamageConversions);
		}
		Packet = ApplyDamageConversion(Packet, AttackerAttributes, ContextualModifiers);
	}

	if (bDebugLog)
	{
		UE_LOG(LogCombatOutgoingDamageCalculator, Log,
			TEXT("[CombatDebug] Weapon source: %s"),
			WeaponStats && WeaponStats->bIsValid ? TEXT("resolved item") : TEXT("character attributes"));
		UE_LOG(LogCombatOutgoingDamageCalculator, Log, TEXT("[CombatDebug] Base and conversion:    %s"),
			*FormatPacket(Packet));
	}

	for (const EHunterDamageType DamageType : CombatOutgoingDamageCalculatorPrivate::AllDamageTypes)
	{
		const float BaseDamage = CombatOutgoingDamageCalculatorPrivate::GetPacketDamage(Packet, DamageType);
		if (BaseDamage <= 0.f)
		{
			continue;
		}

		const float IncreasedPercent = GetIncreasedDamagePercent(
			DamageType, AttackerAttributes, DamageInfo, ContextualModifiers);
		const float AfterIncreased = FMath::Max(
			0.f, CombatOutgoingDamageCalculatorPrivate::ApplyPercentIncrease(BaseDamage, IncreasedPercent));
		const float MoreMultiplier = GetMoreDamageMultiplier(
			DamageType, AttackerAttributes, ContextualModifiers);

		if (bDebugLog)
		{
			UE_LOG(LogCombatOutgoingDamageCalculator, Log,
				TEXT("[CombatDebug] Stage 3 scaling %-10s base=%.2f increased=%+.1f%% more=x%.3f -> %.2f"),
				*UEnum::GetDisplayValueAsText(DamageType).ToString(),
				BaseDamage, IncreasedPercent, MoreMultiplier,
				FMath::Max(0.f, AfterIncreased * MoreMultiplier));
		}

		CombatOutgoingDamageCalculatorPrivate::SetPacketDamage(
			Packet, DamageType, FMath::Max(0.f, AfterIncreased * MoreMultiplier));
	}

	ResolveCriticalStrike(
		Packet, AttackerAttributes, DamageInfo, RandomStream, WeaponStats, ContextualModifiers);

	if (bDebugLog)
	{
		UE_LOG(LogCombatOutgoingDamageCalculator, Log, TEXT("[CombatDebug] Stage 4 post-crit:       %s"),
			*FormatPacket(Packet));
	}

	return Packet;
}

float FCombatOutgoingDamageCalculator::CalculateBaseDamageForType(
	const EHunterDamageType DamageType,
	const UHunterAttributeSet* AttackerAttributes,
	const FAnimationDamageInfo& DamageInfo,
	FRandomStream& RandomStream,
	const FResolvedWeaponStats* WeaponStats,
	const FContextualStatModifierSnapshot* ContextualModifiers)
{
	if (!AttackerAttributes)
	{
		return 0.f;
	}

	const float WeaponMin = WeaponStats && WeaponStats->bIsValid
		? WeaponStats->GetMinDamage(DamageType)
		: CombatOutgoingDamageCalculatorPrivate::GetAttributeWeaponMin(AttackerAttributes, DamageType, ContextualModifiers);
	const float WeaponMax = WeaponStats && WeaponStats->bIsValid
		? WeaponStats->GetMaxDamage(DamageType)
		: CombatOutgoingDamageCalculatorPrivate::GetAttributeWeaponMax(AttackerAttributes, DamageType, ContextualModifiers);
	const float FlatDamage = CombatOutgoingDamageCalculatorPrivate::GetFlatDamage(
		AttackerAttributes, DamageType, ContextualModifiers);

	const float WeaponEffectiveness = FMath::Max(0.f, DamageInfo.WeaponDamageEffectivenessPercent) / 100.f;
	const float AddedEffectiveness = FMath::Max(0.f, DamageInfo.AddedDamageEffectivenessPercent) / 100.f;
	const float WeaponDamage = RollDamageRange(WeaponMin, WeaponMax, RandomStream) * WeaponEffectiveness;
	const float AddedDamage = FlatDamage * AddedEffectiveness;
	const float SkillBaseDamage = CombatOutgoingDamageCalculatorPrivate::GetSkillBaseDamage(
		DamageInfo.SkillBaseDamage, DamageType);

	return FMath::Max(0.f, WeaponDamage + AddedDamage + SkillBaseDamage);
}

FCombatDamagePacket FCombatOutgoingDamageCalculator::ApplyDamageConversionRules(
	const FCombatDamagePacket& InPacket,
	const TArray<FCombatDamageConversionRule>& Rules)
{
	if (Rules.IsEmpty())
	{
		return InPacket;
	}

	FCombatDamagePacket OutPacket = InPacket;
	for (const EHunterDamageType DamageType : CombatOutgoingDamageCalculatorPrivate::AllDamageTypes)
	{
		CombatOutgoingDamageCalculatorPrivate::SetPacketDamage(OutPacket, DamageType, 0.f);
	}

	for (const EHunterDamageType FromType : CombatOutgoingDamageCalculatorPrivate::AllDamageTypes)
	{
		const float SourceDamage = FMath::Max(
			0.f, CombatOutgoingDamageCalculatorPrivate::GetPacketDamage(InPacket, FromType));
		if (SourceDamage <= 0.f)
		{
			continue;
		}

		float TotalConversionPercent = 0.f;
		for (const FCombatDamageConversionRule& Rule : Rules)
		{
			if (!Rule.bGainAsExtra && Rule.From == FromType && Rule.To != FromType)
			{
				TotalConversionPercent += FMath::Max(0.f, Rule.Percent);
			}
		}

		// Over-allocated conversion (total > 100%) scales down proportionally
		// so the hit never gains free damage from stacking conversion sources.
		const float ConversionScale = TotalConversionPercent > 100.f
			? 100.f / TotalConversionPercent
			: 1.f;

		float ConvertedAway = 0.f;
		for (const FCombatDamageConversionRule& Rule : Rules)
		{
			if (Rule.From != FromType || Rule.To == FromType)
			{
				continue;
			}

			const float Percent = FMath::Max(0.f, Rule.Percent)
				* (Rule.bGainAsExtra ? 1.f : ConversionScale);
			if (Percent <= 0.f)
			{
				continue;
			}

			const float ConvertedAmount = SourceDamage * (Percent / 100.f);
			if (!Rule.bGainAsExtra)
			{
				ConvertedAway += ConvertedAmount;
			}
			CombatOutgoingDamageCalculatorPrivate::SetPacketDamage(
				OutPacket, Rule.To,
				CombatOutgoingDamageCalculatorPrivate::GetPacketDamage(OutPacket, Rule.To) + ConvertedAmount);
		}

		const float Remainder = FMath::Max(0.f, SourceDamage - ConvertedAway);
		CombatOutgoingDamageCalculatorPrivate::SetPacketDamage(
			OutPacket, FromType,
			CombatOutgoingDamageCalculatorPrivate::GetPacketDamage(OutPacket, FromType) + Remainder);
	}

	CombatOutgoingDamageCalculatorPrivate::UpdatePacketTotal(OutPacket);
	return OutPacket;
}

FCombatDamagePacket FCombatOutgoingDamageCalculator::ApplyDamageConversion(
	const FCombatDamagePacket& InPacket,
	const UHunterAttributeSet* AttackerAttributes,
	const FContextualStatModifierSnapshot* ContextualModifiers)
{
	if (!AttackerAttributes)
	{
		return InPacket;
	}

	TArray<FCombatDamageConversionRule> Rules;
	Rules.Reserve(30 + (ContextualModifiers
		? ContextualModifiers->DamageConversionRules.Num()
		: 0));
	for (const EHunterDamageType FromType : CombatOutgoingDamageCalculatorPrivate::AllDamageTypes)
	{
		for (const EHunterDamageType ToType : CombatOutgoingDamageCalculatorPrivate::AllDamageTypes)
		{
			const float Percent = CombatOutgoingDamageCalculatorPrivate::GetConversionPercent(
				AttackerAttributes, FromType, ToType, ContextualModifiers);
			if (Percent <= 0.f)
			{
				continue;
			}

			FCombatDamageConversionRule& Rule = Rules.AddDefaulted_GetRef();
			Rule.From = FromType;
			Rule.To = ToType;
			Rule.Percent = Percent;
		}
	}
	if (ContextualModifiers)
	{
		Rules.Append(ContextualModifiers->DamageConversionRules);
	}

	return ApplyDamageConversionRules(InPacket, Rules);
}

float FCombatOutgoingDamageCalculator::GetIncreasedDamagePercent(
	const EHunterDamageType DamageType,
	const UHunterAttributeSet* AttackerAttributes,
	const FAnimationDamageInfo& DamageInfo,
	const FContextualStatModifierSnapshot* ContextualModifiers)
{
	if (!AttackerAttributes)
	{
		return 0.f;
	}

	auto Resolve = [ContextualModifiers](const FGameplayAttribute& Attribute, const float BaseValue)
	{
		return CombatOutgoingDamageCalculatorPrivate::ResolveContextual(
			ContextualModifiers, Attribute, BaseValue);
	};

	float TotalIncreasedPercent = Resolve(
		UHunterAttributeSet::GetGlobalDamagesAttribute(), AttackerAttributes->GetGlobalDamages());

	switch (DamageType)
	{
	case EHunterDamageType::Physical:
		TotalIncreasedPercent += Resolve(UHunterAttributeSet::GetPhysicalPercentDamageAttribute(), AttackerAttributes->GetPhysicalPercentDamage());
		TotalIncreasedPercent += DamageInfo.BaseMulti.Physical;
		break;
	case EHunterDamageType::Fire:
		TotalIncreasedPercent += Resolve(UHunterAttributeSet::GetFirePercentDamageAttribute(), AttackerAttributes->GetFirePercentDamage());
		TotalIncreasedPercent += DamageInfo.BaseMulti.Fire;
		break;
	case EHunterDamageType::Ice:
		TotalIncreasedPercent += Resolve(UHunterAttributeSet::GetIcePercentDamageAttribute(), AttackerAttributes->GetIcePercentDamage());
		TotalIncreasedPercent += DamageInfo.BaseMulti.Ice;
		break;
	case EHunterDamageType::Lightning:
		TotalIncreasedPercent += Resolve(UHunterAttributeSet::GetLightningPercentDamageAttribute(), AttackerAttributes->GetLightningPercentDamage());
		TotalIncreasedPercent += DamageInfo.BaseMulti.Lightning;
		break;
	case EHunterDamageType::Light:
		TotalIncreasedPercent += Resolve(UHunterAttributeSet::GetLightPercentDamageAttribute(), AttackerAttributes->GetLightPercentDamage());
		TotalIncreasedPercent += DamageInfo.BaseMulti.Light;
		break;
	case EHunterDamageType::Corruption:
		TotalIncreasedPercent += Resolve(UHunterAttributeSet::GetCorruptionPercentDamageAttribute(), AttackerAttributes->GetCorruptionPercentDamage());
		TotalIncreasedPercent += DamageInfo.BaseMulti.Corruption;
		break;
	default:
		return 0.f;
	}

	if (CombatOutgoingDamageCalculatorPrivate::IsElementalDamageType(DamageType))
	{
		TotalIncreasedPercent += Resolve(UHunterAttributeSet::GetElementalDamageAttribute(), AttackerAttributes->GetElementalDamage());
	}

	// Tag-conditional increased buckets. Each true flag opts this hit into the
	// matching attribute the same way skill tags gate support scaling.
	if (DamageInfo.Tags.bIsMelee)
	{
		TotalIncreasedPercent += Resolve(UHunterAttributeSet::GetMeleeDamageAttribute(), AttackerAttributes->GetMeleeDamage());
	}
	if (DamageInfo.Tags.bIsRanged)
	{
		TotalIncreasedPercent += Resolve(UHunterAttributeSet::GetRangedDamageAttribute(), AttackerAttributes->GetRangedDamage());
	}
	if (DamageInfo.Tags.bIsSpell)
	{
		TotalIncreasedPercent += Resolve(UHunterAttributeSet::GetSpellDamageAttribute(), AttackerAttributes->GetSpellDamage());
	}
	if (DamageInfo.Tags.bIsArea)
	{
		TotalIncreasedPercent += Resolve(UHunterAttributeSet::GetAreaDamageAttribute(), AttackerAttributes->GetAreaDamage());
	}
	if (DamageInfo.Tags.bIsDamageOverTime)
	{
		TotalIncreasedPercent += Resolve(UHunterAttributeSet::GetDamageOverTimeAttribute(), AttackerAttributes->GetDamageOverTime());
	}
	if (DamageInfo.Tags.bIsChainHit)
	{
		TotalIncreasedPercent += Resolve(UHunterAttributeSet::GetChainDamageAttribute(), AttackerAttributes->GetChainDamage());
	}

	const float MaxEffectiveHealth = FMath::Max(
		AttackerAttributes->GetMaxEffectiveHealth(), AttackerAttributes->GetMaxHealth());
	if (MaxEffectiveHealth > 0.f)
	{
		const float HealthPercent = AttackerAttributes->GetHealth() / MaxEffectiveHealth;
		if (HealthPercent >= 0.999f)
		{
			TotalIncreasedPercent += Resolve(UHunterAttributeSet::GetDamageBonusWhileAtFullHPAttribute(), AttackerAttributes->GetDamageBonusWhileAtFullHP());
		}
		else if (HealthPercent <= 0.35f)
		{
			TotalIncreasedPercent += Resolve(UHunterAttributeSet::GetDamageBonusWhileAtLowHPAttribute(), AttackerAttributes->GetDamageBonusWhileAtLowHP());
		}
	}

	return TotalIncreasedPercent;
}

float FCombatOutgoingDamageCalculator::GetMoreDamageMultiplier(
	const EHunterDamageType DamageType,
	const UHunterAttributeSet* AttackerAttributes,
	const FContextualStatModifierSnapshot* ContextualModifiers)
{
	if (!AttackerAttributes)
	{
		return 1.f;
	}

	auto Resolve = [ContextualModifiers](const FGameplayAttribute& Attribute, const float BaseValue)
	{
		return CombatOutgoingDamageCalculatorPrivate::ResolveContextual(
			ContextualModifiers, Attribute, BaseValue);
	};

	float Multiplier = CombatOutgoingDamageCalculatorPrivate::SanitizeMultiplier(
		Resolve(UHunterAttributeSet::GetGlobalMoreDamageAttribute(), AttackerAttributes->GetGlobalMoreDamage()));

	if (CombatOutgoingDamageCalculatorPrivate::IsElementalDamageType(DamageType))
	{
		Multiplier *= CombatOutgoingDamageCalculatorPrivate::SanitizeMultiplier(
			Resolve(UHunterAttributeSet::GetElementalMoreDamageAttribute(), AttackerAttributes->GetElementalMoreDamage()));
	}

	switch (DamageType)
	{
	case EHunterDamageType::Physical:
		Multiplier *= CombatOutgoingDamageCalculatorPrivate::SanitizeMultiplier(Resolve(UHunterAttributeSet::GetPhysicalMoreDamageAttribute(), AttackerAttributes->GetPhysicalMoreDamage()));
		break;
	case EHunterDamageType::Fire:
		Multiplier *= CombatOutgoingDamageCalculatorPrivate::SanitizeMultiplier(Resolve(UHunterAttributeSet::GetFireMoreDamageAttribute(), AttackerAttributes->GetFireMoreDamage()));
		break;
	case EHunterDamageType::Ice:
		Multiplier *= CombatOutgoingDamageCalculatorPrivate::SanitizeMultiplier(Resolve(UHunterAttributeSet::GetIceMoreDamageAttribute(), AttackerAttributes->GetIceMoreDamage()));
		break;
	case EHunterDamageType::Lightning:
		Multiplier *= CombatOutgoingDamageCalculatorPrivate::SanitizeMultiplier(Resolve(UHunterAttributeSet::GetLightningMoreDamageAttribute(), AttackerAttributes->GetLightningMoreDamage()));
		break;
	case EHunterDamageType::Light:
		Multiplier *= CombatOutgoingDamageCalculatorPrivate::SanitizeMultiplier(Resolve(UHunterAttributeSet::GetLightMoreDamageAttribute(), AttackerAttributes->GetLightMoreDamage()));
		break;
	case EHunterDamageType::Corruption:
		Multiplier *= CombatOutgoingDamageCalculatorPrivate::SanitizeMultiplier(Resolve(UHunterAttributeSet::GetCorruptionMoreDamageAttribute(), AttackerAttributes->GetCorruptionMoreDamage()));
		break;
	default:
		break;
	}

	return FMath::Max(0.f, Multiplier);
}

void FCombatOutgoingDamageCalculator::ResolveCriticalStrike(
	FCombatDamagePacket& Packet,
	const UHunterAttributeSet* AttackerAttributes,
	const FAnimationDamageInfo& DamageInfo,
	FRandomStream& RandomStream,
	const FResolvedWeaponStats* WeaponStats,
	const FContextualStatModifierSnapshot* ContextualModifiers)
{
	Packet.bCrit = false;
	Packet.CritMultiplierApplied = 1.f;

	// Damage-over-time hits never crit, and the animation gate
	// controls everything else including forced crits.
	if (!DamageInfo.Crit.bCanCrit || DamageInfo.Tags.bIsDamageOverTime || !AttackerAttributes)
	{
		CombatOutgoingDamageCalculatorPrivate::UpdatePacketTotal(Packet);
		return;
	}

	float CritChance = CombatOutgoingDamageCalculatorPrivate::ResolveContextual(
		ContextualModifiers,
		UHunterAttributeSet::GetCritChanceAttribute(),
		AttackerAttributes->GetCritChance()) + DamageInfo.Crit.CritChance;
	if (!DamageInfo.Tags.bIsSpell && WeaponStats && WeaponStats->bIsValid)
	{
		CritChance += WeaponStats->Values.CriticalStrikeChance;
	}
	if (DamageInfo.Tags.bIsSpell)
	{
		CritChance += CombatOutgoingDamageCalculatorPrivate::ResolveContextual(
			ContextualModifiers,
			UHunterAttributeSet::GetSpellsCritChanceAttribute(),
			AttackerAttributes->GetSpellsCritChance());
	}
	CritChance = FMath::Clamp(CritChance, 0.f, 100.f);

	const bool bCritSucceeded = DamageInfo.Crit.bForceCrit
		|| (CritChance > 0.f && RandomStream.FRandRange(0.f, 100.f) < CritChance);
	if (!bCritSucceeded)
	{
		CombatOutgoingDamageCalculatorPrivate::UpdatePacketTotal(Packet);
		return;
	}

	// Crit multiplier units, stated once so the three inputs cannot drift apart:
	//   CritMultiplier attribute       - absolute ratio. 1.5 = 150% damage on crit.
	//   SpellsCritMultiplier attribute - absolute ratio, same scale.
	//   DamageInfo.Crit.CritMultiplier - additive ratio DELTA. 0.5 = +50%.
	// CritMultiplier owns the base critical ratio. SpellsCritMultiplier is a
	// spell-only ratio whose neutral value is 1.0, so only its delta stacks here.
	const float BaseCritRatio =
		CombatOutgoingDamageCalculatorPrivate::SanitizeMultiplier(
			CombatOutgoingDamageCalculatorPrivate::ResolveContextual(
				ContextualModifiers,
				UHunterAttributeSet::GetCritMultiplierAttribute(),
				AttackerAttributes->GetCritMultiplier()));

	float CritMultiplier = BaseCritRatio;
	if (DamageInfo.Tags.bIsSpell)
	{
		const float SpellCritRatio =
			CombatOutgoingDamageCalculatorPrivate::SanitizeMultiplier(
				CombatOutgoingDamageCalculatorPrivate::ResolveContextual(
					ContextualModifiers,
					UHunterAttributeSet::GetSpellsCritMultiplierAttribute(),
					AttackerAttributes->GetSpellsCritMultiplier()));
		CritMultiplier += SpellCritRatio - 1.f;
	}
	CritMultiplier += DamageInfo.Crit.CritMultiplier;
	CritMultiplier = FMath::Max(CritMultiplier, 0.f);

	for (const EHunterDamageType DamageType : CombatOutgoingDamageCalculatorPrivate::AllDamageTypes)
	{
		CombatOutgoingDamageCalculatorPrivate::SetPacketDamage(
			Packet, DamageType,
			CombatOutgoingDamageCalculatorPrivate::GetPacketDamage(Packet, DamageType) * CritMultiplier);
	}

	Packet.bCrit = true;
	Packet.CritMultiplierApplied = CritMultiplier;
	CombatOutgoingDamageCalculatorPrivate::UpdatePacketTotal(Packet);
}

FString FCombatOutgoingDamageCalculator::FormatPacket(const FCombatDamagePacket& Packet)
{
	return FString::Printf(
		TEXT("Phys=%.1f Fire=%.1f Ice=%.1f Lightning=%.1f Light=%.1f Corruption=%.1f Crit=%s x%.2f Total=%.1f"),
		Packet.Physical, Packet.Fire, Packet.Ice, Packet.Lightning, Packet.Light, Packet.Corruption,
		Packet.bCrit ? TEXT("yes") : TEXT("no"), Packet.CritMultiplierApplied, Packet.TotalPreMitigation);
}
