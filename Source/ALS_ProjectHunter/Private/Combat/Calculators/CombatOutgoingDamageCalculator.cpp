#include "Combat/Calculators/CombatOutgoingDamageCalculator.h"

#include "AbilitySystem/HunterAttributeSet.h"

DEFINE_LOG_CATEGORY_STATIC(LogCombatOutgoingDamageCalculator, Log, All);

#if !UE_BUILD_SHIPPING
static TAutoConsoleVariable<int32> CVarDebugCombatDamage(
	TEXT("Hunter.Debug.Combat"),
	0,
	TEXT("Log the per-stage combat damage breakdown for every ApplyHit\n")
	TEXT("0: Disabled (default)\n")
	TEXT("1: Log base roll, conversion, scaling, crit, mitigation, block, and routing"),
	ECVF_Cheat
);
#endif

namespace CombatOutgoingDamageCalculatorPrivate
{
	bool IsCombatDebugLoggingEnabled()
	{
#if !UE_BUILD_SHIPPING
		return CVarDebugCombatDamage.GetValueOnGameThread() != 0;
#else
		return false;
#endif
	}

	constexpr float DefaultCritMultiplier = 1.5f;

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

	// Multiplier attributes default to 0 when untouched; 0 means "no modifier".
	float GetNeutralMultiplier(const float Value)
	{
		return Value > 0.f ? Value : 1.f;
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

	/**
	 * Attribute conversion percent from one damage type to another.
	 * Same-type and unknown pairs return 0.
	 */
	float GetConversionPercent(
		const UHunterAttributeSet* Attributes,
		const EHunterDamageType From,
		const EHunterDamageType To)
	{
		if (!Attributes || From == To)
		{
			return 0.f;
		}

		switch (From)
		{
		case EHunterDamageType::Physical:
			switch (To)
			{
			case EHunterDamageType::Fire:       return Attributes->GetPhysicalToFire();
			case EHunterDamageType::Ice:        return Attributes->GetPhysicalToIce();
			case EHunterDamageType::Lightning:  return Attributes->GetPhysicalToLightning();
			case EHunterDamageType::Light:      return Attributes->GetPhysicalToLight();
			case EHunterDamageType::Corruption: return Attributes->GetPhysicalToCorruption();
			default: return 0.f;
			}
		case EHunterDamageType::Fire:
			switch (To)
			{
			case EHunterDamageType::Physical:   return Attributes->GetFireToPhysical();
			case EHunterDamageType::Ice:        return Attributes->GetFireToIce();
			case EHunterDamageType::Lightning:  return Attributes->GetFireToLightning();
			case EHunterDamageType::Light:      return Attributes->GetFireToLight();
			case EHunterDamageType::Corruption: return Attributes->GetFireToCorruption();
			default: return 0.f;
			}
		case EHunterDamageType::Ice:
			switch (To)
			{
			case EHunterDamageType::Physical:   return Attributes->GetIceToPhysical();
			case EHunterDamageType::Fire:       return Attributes->GetIceToFire();
			case EHunterDamageType::Lightning:  return Attributes->GetIceToLightning();
			case EHunterDamageType::Light:      return Attributes->GetIceToLight();
			case EHunterDamageType::Corruption: return Attributes->GetIceToCorruption();
			default: return 0.f;
			}
		case EHunterDamageType::Lightning:
			switch (To)
			{
			case EHunterDamageType::Physical:   return Attributes->GetLightningToPhysical();
			case EHunterDamageType::Fire:       return Attributes->GetLightningToFire();
			case EHunterDamageType::Ice:        return Attributes->GetLightningToIce();
			case EHunterDamageType::Light:      return Attributes->GetLightningToLight();
			case EHunterDamageType::Corruption: return Attributes->GetLightningToCorruption();
			default: return 0.f;
			}
		case EHunterDamageType::Light:
			switch (To)
			{
			case EHunterDamageType::Physical:   return Attributes->GetLightToPhysical();
			case EHunterDamageType::Fire:       return Attributes->GetLightToFire();
			case EHunterDamageType::Ice:        return Attributes->GetLightToIce();
			case EHunterDamageType::Lightning:  return Attributes->GetLightToLightning();
			case EHunterDamageType::Corruption: return Attributes->GetLightToCorruption();
			default: return 0.f;
			}
		case EHunterDamageType::Corruption:
			switch (To)
			{
			case EHunterDamageType::Physical:   return Attributes->GetCorruptionToPhysical();
			case EHunterDamageType::Fire:       return Attributes->GetCorruptionToFire();
			case EHunterDamageType::Ice:        return Attributes->GetCorruptionToIce();
			case EHunterDamageType::Lightning:  return Attributes->GetCorruptionToLightning();
			case EHunterDamageType::Light:      return Attributes->GetCorruptionToLight();
			default: return 0.f;
			}
		default:
			return 0.f;
		}
	}
}

float FCombatOutgoingDamageCalculator::RollDamageRange(const float MinDamage, const float MaxDamage)
{
	const float Low = FMath::Max(0.f, FMath::Min(MinDamage, MaxDamage));
	const float High = FMath::Max(0.f, FMath::Max(MinDamage, MaxDamage));
	return High > Low ? FMath::FRandRange(Low, High) : High;
}

FCombatDamagePacket FCombatOutgoingDamageCalculator::BuildOutgoingDamagePacket(
	const UHunterAttributeSet* AttackerAttributes,
	const FAnimationDamageInfo& DamageInfo)
{
	FCombatDamagePacket Packet;
	if (!AttackerAttributes)
	{
		return Packet;
	}

	const bool bDebugLog = CombatOutgoingDamageCalculatorPrivate::IsCombatDebugLoggingEnabled();

	for (const EHunterDamageType DamageType : CombatOutgoingDamageCalculatorPrivate::AllDamageTypes)
	{
		CombatOutgoingDamageCalculatorPrivate::SetPacketDamage(
			Packet, DamageType, CalculateBaseDamageForType(DamageType, AttackerAttributes));
	}

	if (bDebugLog)
	{
		UE_LOG(LogCombatOutgoingDamageCalculator, Log,
			TEXT("[CombatDebug] Weapon attrs: Phys %.1f-%.1f (+%.1f flat) Fire %.1f-%.1f (+%.1f) Ice %.1f-%.1f (+%.1f) Lightning %.1f-%.1f (+%.1f) Light %.1f-%.1f (+%.1f) Corruption %.1f-%.1f (+%.1f)"),
			AttackerAttributes->GetMinPhysicalDamage(), AttackerAttributes->GetMaxPhysicalDamage(), AttackerAttributes->GetPhysicalFlatDamage(),
			AttackerAttributes->GetMinFireDamage(), AttackerAttributes->GetMaxFireDamage(), AttackerAttributes->GetFireFlatDamage(),
			AttackerAttributes->GetMinIceDamage(), AttackerAttributes->GetMaxIceDamage(), AttackerAttributes->GetIceFlatDamage(),
			AttackerAttributes->GetMinLightningDamage(), AttackerAttributes->GetMaxLightningDamage(), AttackerAttributes->GetLightningFlatDamage(),
			AttackerAttributes->GetMinLightDamage(), AttackerAttributes->GetMaxLightDamage(), AttackerAttributes->GetLightFlatDamage(),
			AttackerAttributes->GetMinCorruptionDamage(), AttackerAttributes->GetMaxCorruptionDamage(), AttackerAttributes->GetCorruptionFlatDamage());
		UE_LOG(LogCombatOutgoingDamageCalculator, Log, TEXT("[CombatDebug] Stage 1 base roll:      %s"),
			*FormatPacket(Packet));
	}

	// Conversion first: converted damage scales only with modifiers of its
	// final type, never both.
	Packet = ApplyDamageConversion(Packet, AttackerAttributes);

	if (bDebugLog)
	{
		UE_LOG(LogCombatOutgoingDamageCalculator, Log, TEXT("[CombatDebug] Stage 2 post-conversion: %s"),
			*FormatPacket(Packet));
	}

	for (const EHunterDamageType DamageType : CombatOutgoingDamageCalculatorPrivate::AllDamageTypes)
	{
		const float BaseDamage = CombatOutgoingDamageCalculatorPrivate::GetPacketDamage(Packet, DamageType);
		if (BaseDamage <= 0.f)
		{
			continue;
		}

		const float IncreasedPercent = GetIncreasedDamagePercent(DamageType, AttackerAttributes, DamageInfo);
		const float AfterIncreased = FMath::Max(
			0.f, CombatOutgoingDamageCalculatorPrivate::ApplyPercentIncrease(BaseDamage, IncreasedPercent));
		const float MoreMultiplier = GetMoreDamageMultiplier(DamageType, AttackerAttributes);

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

	ResolveCriticalStrike(Packet, AttackerAttributes, DamageInfo);

	if (bDebugLog)
	{
		UE_LOG(LogCombatOutgoingDamageCalculator, Log, TEXT("[CombatDebug] Stage 4 post-crit:       %s"),
			*FormatPacket(Packet));
	}

	return Packet;
}

float FCombatOutgoingDamageCalculator::CalculateBaseDamageForType(
	const EHunterDamageType DamageType,
	const UHunterAttributeSet* AttackerAttributes)
{
	if (!AttackerAttributes)
	{
		return 0.f;
	}

	float WeaponMin = 0.f;
	float WeaponMax = 0.f;
	float FlatDamage = 0.f;

	switch (DamageType)
	{
	case EHunterDamageType::Physical:
		WeaponMin = AttackerAttributes->GetMinPhysicalDamage();
		WeaponMax = AttackerAttributes->GetMaxPhysicalDamage();
		FlatDamage = AttackerAttributes->GetPhysicalFlatDamage();
		break;
	case EHunterDamageType::Fire:
		WeaponMin = AttackerAttributes->GetMinFireDamage();
		WeaponMax = AttackerAttributes->GetMaxFireDamage();
		FlatDamage = AttackerAttributes->GetFireFlatDamage();
		break;
	case EHunterDamageType::Ice:
		WeaponMin = AttackerAttributes->GetMinIceDamage();
		WeaponMax = AttackerAttributes->GetMaxIceDamage();
		FlatDamage = AttackerAttributes->GetIceFlatDamage();
		break;
	case EHunterDamageType::Lightning:
		WeaponMin = AttackerAttributes->GetMinLightningDamage();
		WeaponMax = AttackerAttributes->GetMaxLightningDamage();
		FlatDamage = AttackerAttributes->GetLightningFlatDamage();
		break;
	case EHunterDamageType::Light:
		WeaponMin = AttackerAttributes->GetMinLightDamage();
		WeaponMax = AttackerAttributes->GetMaxLightDamage();
		FlatDamage = AttackerAttributes->GetLightFlatDamage();
		break;
	case EHunterDamageType::Corruption:
		WeaponMin = AttackerAttributes->GetMinCorruptionDamage();
		WeaponMax = AttackerAttributes->GetMaxCorruptionDamage();
		FlatDamage = AttackerAttributes->GetCorruptionFlatDamage();
		break;
	default:
		return 0.f;
	}

	return FMath::Max(0.f, RollDamageRange(WeaponMin, WeaponMax) + FlatDamage);
}

FCombatDamagePacket FCombatOutgoingDamageCalculator::ApplyDamageConversion(
	const FCombatDamagePacket& InPacket,
	const UHunterAttributeSet* AttackerAttributes)
{
	if (!AttackerAttributes)
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
		for (const EHunterDamageType ToType : CombatOutgoingDamageCalculatorPrivate::AllDamageTypes)
		{
			TotalConversionPercent += FMath::Max(
				0.f, CombatOutgoingDamageCalculatorPrivate::GetConversionPercent(AttackerAttributes, FromType, ToType));
		}

		// Over-allocated conversion (total > 100%) scales down proportionally
		// so the hit never gains free damage from stacking conversion sources.
		const float ConversionScale = TotalConversionPercent > 100.f
			? 100.f / TotalConversionPercent
			: 1.f;

		float ConvertedAway = 0.f;
		for (const EHunterDamageType ToType : CombatOutgoingDamageCalculatorPrivate::AllDamageTypes)
		{
			const float Percent = FMath::Max(
				0.f, CombatOutgoingDamageCalculatorPrivate::GetConversionPercent(AttackerAttributes, FromType, ToType))
				* ConversionScale;
			if (Percent <= 0.f)
			{
				continue;
			}

			const float ConvertedAmount = SourceDamage * (Percent / 100.f);
			ConvertedAway += ConvertedAmount;
			CombatOutgoingDamageCalculatorPrivate::SetPacketDamage(
				OutPacket, ToType,
				CombatOutgoingDamageCalculatorPrivate::GetPacketDamage(OutPacket, ToType) + ConvertedAmount);
		}

		const float Remainder = FMath::Max(0.f, SourceDamage - ConvertedAway);
		CombatOutgoingDamageCalculatorPrivate::SetPacketDamage(
			OutPacket, FromType,
			CombatOutgoingDamageCalculatorPrivate::GetPacketDamage(OutPacket, FromType) + Remainder);
	}

	CombatOutgoingDamageCalculatorPrivate::UpdatePacketTotal(OutPacket);
	return OutPacket;
}

float FCombatOutgoingDamageCalculator::GetIncreasedDamagePercent(
	const EHunterDamageType DamageType,
	const UHunterAttributeSet* AttackerAttributes,
	const FAnimationDamageInfo& DamageInfo)
{
	if (!AttackerAttributes)
	{
		return 0.f;
	}

	float TotalIncreasedPercent = AttackerAttributes->GetGlobalDamages();

	switch (DamageType)
	{
	case EHunterDamageType::Physical:
		TotalIncreasedPercent += AttackerAttributes->GetPhysicalPercentDamage();
		TotalIncreasedPercent += DamageInfo.BaseMulti.Physical;
		break;
	case EHunterDamageType::Fire:
		TotalIncreasedPercent += AttackerAttributes->GetFirePercentDamage();
		TotalIncreasedPercent += DamageInfo.BaseMulti.Fire;
		break;
	case EHunterDamageType::Ice:
		TotalIncreasedPercent += AttackerAttributes->GetIcePercentDamage();
		TotalIncreasedPercent += DamageInfo.BaseMulti.Ice;
		break;
	case EHunterDamageType::Lightning:
		TotalIncreasedPercent += AttackerAttributes->GetLightningPercentDamage();
		TotalIncreasedPercent += DamageInfo.BaseMulti.Lightning;
		break;
	case EHunterDamageType::Light:
		TotalIncreasedPercent += AttackerAttributes->GetLightPercentDamage();
		TotalIncreasedPercent += DamageInfo.BaseMulti.Light;
		break;
	case EHunterDamageType::Corruption:
		TotalIncreasedPercent += AttackerAttributes->GetCorruptionPercentDamage();
		TotalIncreasedPercent += DamageInfo.BaseMulti.Corruption;
		break;
	default:
		return 0.f;
	}

	if (CombatOutgoingDamageCalculatorPrivate::IsElementalDamageType(DamageType))
	{
		TotalIncreasedPercent += AttackerAttributes->GetElementalDamage();
	}

	// Tag-conditional increased buckets. Each true flag opts this hit into the
	// matching attribute the same way skill tags gate support scaling.
	if (DamageInfo.Tags.bIsMelee)
	{
		TotalIncreasedPercent += AttackerAttributes->GetMeleeDamage();
	}
	if (DamageInfo.Tags.bIsRanged)
	{
		TotalIncreasedPercent += AttackerAttributes->GetRangedDamage();
	}
	if (DamageInfo.Tags.bIsSpell)
	{
		TotalIncreasedPercent += AttackerAttributes->GetSpellDamage();
	}
	if (DamageInfo.Tags.bIsArea)
	{
		TotalIncreasedPercent += AttackerAttributes->GetAreaDamage();
	}
	if (DamageInfo.Tags.bIsDamageOverTime)
	{
		TotalIncreasedPercent += AttackerAttributes->GetDamageOverTime();
	}
	if (DamageInfo.Tags.bIsChainHit)
	{
		TotalIncreasedPercent += AttackerAttributes->GetChainDamage();
	}

	const float MaxEffectiveHealth = FMath::Max(
		AttackerAttributes->GetMaxEffectiveHealth(), AttackerAttributes->GetMaxHealth());
	if (MaxEffectiveHealth > 0.f)
	{
		const float HealthPercent = AttackerAttributes->GetHealth() / MaxEffectiveHealth;
		if (HealthPercent >= 0.999f)
		{
			TotalIncreasedPercent += AttackerAttributes->GetDamageBonusWhileAtFullHP();
		}
		else if (HealthPercent <= 0.35f)
		{
			TotalIncreasedPercent += AttackerAttributes->GetDamageBonusWhileAtLowHP();
		}
	}

	return TotalIncreasedPercent;
}

float FCombatOutgoingDamageCalculator::GetMoreDamageMultiplier(
	const EHunterDamageType DamageType,
	const UHunterAttributeSet* AttackerAttributes)
{
	if (!AttackerAttributes)
	{
		return 1.f;
	}

	float Multiplier = CombatOutgoingDamageCalculatorPrivate::GetNeutralMultiplier(AttackerAttributes->GetGlobalMoreDamage());

	if (CombatOutgoingDamageCalculatorPrivate::IsElementalDamageType(DamageType))
	{
		Multiplier *= CombatOutgoingDamageCalculatorPrivate::GetNeutralMultiplier(AttackerAttributes->GetElementalMoreDamage());
	}

	switch (DamageType)
	{
	case EHunterDamageType::Physical:
		Multiplier *= CombatOutgoingDamageCalculatorPrivate::GetNeutralMultiplier(AttackerAttributes->GetPhysicalMoreDamage());
		break;
	case EHunterDamageType::Fire:
		Multiplier *= CombatOutgoingDamageCalculatorPrivate::GetNeutralMultiplier(AttackerAttributes->GetFireMoreDamage());
		break;
	case EHunterDamageType::Ice:
		Multiplier *= CombatOutgoingDamageCalculatorPrivate::GetNeutralMultiplier(AttackerAttributes->GetIceMoreDamage());
		break;
	case EHunterDamageType::Lightning:
		Multiplier *= CombatOutgoingDamageCalculatorPrivate::GetNeutralMultiplier(AttackerAttributes->GetLightningMoreDamage());
		break;
	case EHunterDamageType::Light:
		Multiplier *= CombatOutgoingDamageCalculatorPrivate::GetNeutralMultiplier(AttackerAttributes->GetLightMoreDamage());
		break;
	case EHunterDamageType::Corruption:
		Multiplier *= CombatOutgoingDamageCalculatorPrivate::GetNeutralMultiplier(AttackerAttributes->GetCorruptionMoreDamage());
		break;
	default:
		break;
	}

	return FMath::Max(0.f, Multiplier);
}

void FCombatOutgoingDamageCalculator::ResolveCriticalStrike(
	FCombatDamagePacket& Packet,
	const UHunterAttributeSet* AttackerAttributes,
	const FAnimationDamageInfo& DamageInfo)
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

	float CritChance = AttackerAttributes->GetCritChance() + DamageInfo.Crit.CritChance;
	if (DamageInfo.Tags.bIsSpell)
	{
		CritChance += AttackerAttributes->GetSpellsCritChance();
	}
	CritChance = FMath::Clamp(CritChance, 0.f, 100.f);

	const bool bCritSucceeded = DamageInfo.Crit.bForceCrit
		|| (CritChance > 0.f && FMath::FRandRange(0.f, 100.f) < CritChance);
	if (!bCritSucceeded)
	{
		CombatOutgoingDamageCalculatorPrivate::UpdatePacketTotal(Packet);
		return;
	}

	float CritMultiplier = AttackerAttributes->GetCritMultiplier() > 0.f
		? AttackerAttributes->GetCritMultiplier()
		: CombatOutgoingDamageCalculatorPrivate::DefaultCritMultiplier;
	if (DamageInfo.Tags.bIsSpell)
	{
		const float SpellCritMultiplier =
			CombatOutgoingDamageCalculatorPrivate::GetNeutralMultiplier(AttackerAttributes->GetSpellsCritMultiplier());
		CritMultiplier += FMath::Max(0.f, SpellCritMultiplier - 1.f);
	}
	CritMultiplier += FMath::Max(0.f, DamageInfo.Crit.CritMultiplier);
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
