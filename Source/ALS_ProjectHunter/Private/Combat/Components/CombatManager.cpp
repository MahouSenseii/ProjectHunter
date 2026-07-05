#include "Combat/Components/CombatManager.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystem/HunterAttributeSet.h"
#include "Combat/Components/CombatStatusManager.h"
#include "Combat/Library/CombatFunctionLibrary.h"
#include "GameFramework/Actor.h"
#include "GameplayEffect.h"
#include "PHGameplayTags.h"

DEFINE_LOG_CATEGORY(LogCombatManager);

#if !UE_BUILD_SHIPPING
static TAutoConsoleVariable<int32> CVarDebugCombat(
	TEXT("Hunter.Debug.Combat"),
	0,
	TEXT("Log the per-stage combat damage breakdown for every ApplyHit\n")
	TEXT("0: Disabled (default)\n")
	TEXT("1: Log base roll, conversion, scaling, crit, mitigation, block, and routing"),
	ECVF_Cheat
);
#endif

namespace CombatManagerPrivate
{
	bool IsCombatDebugLoggingEnabled()
	{
#if !UE_BUILD_SHIPPING
		return CVarDebugCombat.GetValueOnGameThread() != 0;
#else
		return false;
#endif
	}

	constexpr float DefaultCritMultiplier = 1.5f;
	constexpr float MinResistancePercent = -100.f;
	constexpr float DefaultMaxResistancePercent = 90.f;
	constexpr float DefaultBlockAngleDegrees = 120.f;

	// PoE-style armour: mitigation = Armour / (Armour + Scale * HitSize).
	// Bigger hits punch through the same armour harder.
	constexpr float ArmourHitSizeScale = 10.f;
	constexpr float MaxArmourMitigation = 0.9f;

	constexpr float DefaultBleedDuration = 4.f;
	constexpr float DefaultIgniteDuration = 4.f;
	constexpr float DefaultFreezeDuration = 1.5f;
	constexpr float DefaultShockDuration = 4.f;
	constexpr float DefaultPetrifyDuration = 2.f;
	constexpr float DefaultCorruptionDuration = 4.f;
	constexpr float BleedDamagePerTickPercent = 0.2f;
	constexpr float IgniteDamagePerTickPercent = 0.25f;
	constexpr float CorruptionDamagePerTickPercent = 0.2f;
	constexpr float DefaultShockAmpFraction = 0.2f;
	constexpr float DefaultChillSlowFraction = 0.3f;
	constexpr float DefaultChillDuration = 2.f;

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

	float GetResultTakenByType(const FCombatResolveResult& Result, const EHunterDamageType DamageType)
	{
		switch (DamageType)
		{
		case EHunterDamageType::Physical:   return Result.PhysicalTaken;
		case EHunterDamageType::Fire:       return Result.FireTaken;
		case EHunterDamageType::Ice:        return Result.IceTaken;
		case EHunterDamageType::Lightning:  return Result.LightningTaken;
		case EHunterDamageType::Light:      return Result.LightTaken;
		case EHunterDamageType::Corruption: return Result.CorruptionTaken;
		default:                            return 0.f;
		}
	}

	void SetResultTakenByType(FCombatResolveResult& Result, const EHunterDamageType DamageType, const float Value)
	{
		switch (DamageType)
		{
		case EHunterDamageType::Physical:   Result.PhysicalTaken = Value; break;
		case EHunterDamageType::Fire:       Result.FireTaken = Value; break;
		case EHunterDamageType::Ice:        Result.IceTaken = Value; break;
		case EHunterDamageType::Lightning:  Result.LightningTaken = Value; break;
		case EHunterDamageType::Light:      Result.LightTaken = Value; break;
		case EHunterDamageType::Corruption: Result.CorruptionTaken = Value; break;
		default: break;
		}
	}

	void SetResultBlockedByType(FCombatResolveResult& Result, const EHunterDamageType DamageType, const float Value)
	{
		switch (DamageType)
		{
		case EHunterDamageType::Physical:   Result.PhysicalBlocked = Value; break;
		case EHunterDamageType::Fire:       Result.FireBlocked = Value; break;
		case EHunterDamageType::Ice:        Result.IceBlocked = Value; break;
		case EHunterDamageType::Lightning:  Result.LightningBlocked = Value; break;
		case EHunterDamageType::Light:      Result.LightBlocked = Value; break;
		case EHunterDamageType::Corruption: Result.CorruptionBlocked = Value; break;
		default: break;
		}
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

	FVector ResolveDamagePopupWorldLocation(const AActor* TargetActor)
	{
		if (!IsValid(TargetActor))
		{
			return FVector::ZeroVector;
		}

		return TargetActor->GetActorLocation() +
			FVector(0.f, 0.f, TargetActor->GetSimpleCollisionHalfHeight());
	}

	FString FormatPacket(const FCombatDamagePacket& Packet)
	{
		return FString::Printf(
			TEXT("Phys=%.1f Fire=%.1f Ice=%.1f Lightning=%.1f Light=%.1f Corruption=%.1f Crit=%s x%.2f Total=%.1f"),
			Packet.Physical, Packet.Fire, Packet.Ice, Packet.Lightning, Packet.Light, Packet.Corruption,
			Packet.bCrit ? TEXT("yes") : TEXT("no"), Packet.CritMultiplierApplied, Packet.TotalPreMitigation);
	}

	FString FormatResult(const FCombatResolveResult& Result)
	{
		return FString::Printf(
			TEXT("Taken=%.1f (Health=%.1f Shield=%.1f Stamina=%.1f) Blocked=%.1f Crit=%s Killed=%s Stagger=%s"),
			Result.TotalDamageTaken, Result.DamageToHealth, Result.DamageToArcaneShield, Result.DamageToStamina,
			Result.TotalBlockedAmount,
			Result.bWasCrit ? TEXT("yes") : TEXT("no"),
			Result.bKilledTarget ? TEXT("yes") : TEXT("no"),
			Result.bShouldStagger ? TEXT("yes") : TEXT("no"));
	}
}

void UCombatIncomingHitEditContext::RejectHit()
{
	bApplyHit = false;
}

UCombatManager::UCombatManager()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UCombatManager::ApplyHit(
	AActor* AttackerActor,
	AActor* DefenderActor,
	const FAnimationDamageInfo& DamageInfo,
	FCombatResolveResult& OutResult,
	const EHitResponse HitResponse,
	const bool bCanApplyAilments)
{
	OutResult = FCombatResolveResult{};

	if (!IsValid(AttackerActor) || !IsValid(DefenderActor))
	{
		UE_LOG(LogCombatManager, Warning,
			TEXT("ApplyHit failed because attacker or defender was invalid. Attacker=%s Defender=%s"),
			*GetNameSafe(AttackerActor), *GetNameSafe(DefenderActor));
		return false;
	}

	const UHunterAttributeSet* AttackerAttributes = GetHunterAttributeSetFromActor(AttackerActor);
	const UHunterAttributeSet* DefenderAttributes = GetHunterAttributeSetFromActor(DefenderActor);
	if (!AttackerAttributes || !DefenderAttributes)
	{
		UE_LOG(LogCombatManager, Warning,
			TEXT("ApplyHit requires a UHunterAttributeSet on both actors. Attacker=%s(%s) Defender=%s(%s)"),
			*GetNameSafe(AttackerActor), AttackerAttributes ? TEXT("ok") : TEXT("missing"),
			*GetNameSafe(DefenderActor), DefenderAttributes ? TEXT("ok") : TEXT("missing"));
		return false;
	}

	FAnimationDamageInfo EffectiveInfo = DamageInfo;
	EHitResponse EffectiveHitResponse = HitResponse;
	bool bEffectiveCanApplyAilments = bCanApplyAilments;

	const bool bHasAuthority = DefenderActor->HasAuthority();
	if (bHasAuthority && OnEditIncomingHit.IsBound())
	{
		UCombatIncomingHitEditContext* EditContext = NewObject<UCombatIncomingHitEditContext>(this);
		EditContext->AttackerActor = AttackerActor;
		EditContext->DefenderActor = DefenderActor;
		EditContext->DamageInfo = EffectiveInfo;
		EditContext->HitResponse = EffectiveHitResponse;
		EditContext->bCanApplyAilments = bEffectiveCanApplyAilments;

		OnEditIncomingHit.Broadcast(EditContext);

		if (!EditContext->bApplyHit)
		{
			UE_LOG(LogCombatManager, Verbose,
				TEXT("ApplyHit rejected by incoming hit edit. Attacker=%s Defender=%s"),
				*GetNameSafe(AttackerActor), *GetNameSafe(DefenderActor));
			return false;
		}

		EffectiveInfo = EditContext->DamageInfo;
		EffectiveHitResponse = EditContext->HitResponse;
		bEffectiveCanApplyAilments = EditContext->bCanApplyAilments;
	}

	// ── Outgoing: base -> conversion -> increased/more -> crit ───────────────
	FCombatDamagePacket OutgoingPacket = BuildOutgoingDamagePacket(AttackerAttributes, EffectiveInfo);
	UE_LOG(LogCombatManager, Verbose, TEXT("ApplyHit outgoing packet: %s"),
		*CombatManagerPrivate::FormatPacket(OutgoingPacket));

	// ── Incoming: armour/resists -> block -> taken multipliers -> routing ────
	OutResult = MitigateDamagePacket(
		OutgoingPacket, AttackerActor, DefenderActor,
		AttackerAttributes, DefenderAttributes, EffectiveInfo);

	// Stagger and hit-response gates run before application so both the
	// authoritative path and non-authority previews agree on the outcome.
	EvaluateStagger(DefenderActor, DefenderAttributes, OutResult);
	ApplyHitResponse(EffectiveHitResponse, bEffectiveCanApplyAilments, OutResult);

	if (!bHasAuthority)
	{
		UE_LOG(LogCombatManager, Verbose,
			TEXT("ApplyHit ran without server authority. Returning preview result only for %s."),
			*GetNameSafe(DefenderActor));
		return true;
	}

	UAbilitySystemComponent* AttackerASC = GetAbilitySystemComponentFromActor(AttackerActor);

	ApplyResolvedDamage(AttackerActor, DefenderActor, OutResult);
	OutResult.bKilledTarget = OutResult.HitResponse != EHitResponse::Invincible
		&& DefenderAttributes->GetHealth() <= 0.f;
	OutResult.HealthAfterHit = DefenderAttributes->GetHealth();

	ApplyOnHitRecovery(AttackerActor, OutResult, AttackerASC, AttackerAttributes);
	ApplyAilments(AttackerActor, DefenderActor, OutResult);
	ApplyReflect(AttackerActor, DefenderActor, OutResult);

	BroadcastDamagePopup(AttackerActor, DefenderActor, OutResult);

	UE_LOG(LogCombatManager, Verbose, TEXT("ApplyHit completed. Attacker=%s Defender=%s %s"),
		*GetNameSafe(AttackerActor), *GetNameSafe(DefenderActor),
		*CombatManagerPrivate::FormatResult(OutResult));

	return true;
}

UAbilitySystemComponent* UCombatManager::GetAbilitySystemComponentFromActor(const AActor* Actor)
{
	if (!IsValid(Actor))
	{
		return nullptr;
	}

	if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Actor))
	{
		return ASC;
	}

	return Actor->FindComponentByClass<UAbilitySystemComponent>();
}

const UHunterAttributeSet* UCombatManager::GetHunterAttributeSetFromActor(const AActor* Actor)
{
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActor(Actor);
	return ASC ? ASC->GetSet<UHunterAttributeSet>() : nullptr;
}

float UCombatManager::RollDamageRange(const float MinDamage, const float MaxDamage)
{
	const float Low = FMath::Max(0.f, FMath::Min(MinDamage, MaxDamage));
	const float High = FMath::Max(0.f, FMath::Max(MinDamage, MaxDamage));
	return High > Low ? FMath::FRandRange(Low, High) : High;
}

// ─── Outgoing stages ─────────────────────────────────────────────────────────

FCombatDamagePacket UCombatManager::BuildOutgoingDamagePacket(
	const UHunterAttributeSet* AttackerAttributes,
	const FAnimationDamageInfo& DamageInfo) const
{
	FCombatDamagePacket Packet;
	if (!AttackerAttributes)
	{
		return Packet;
	}

	const bool bDebugLog = CombatManagerPrivate::IsCombatDebugLoggingEnabled();

	for (const EHunterDamageType DamageType : CombatManagerPrivate::AllDamageTypes)
	{
		CombatManagerPrivate::SetPacketDamage(
			Packet, DamageType, CalculateBaseDamageForType(DamageType, AttackerAttributes));
	}

	if (bDebugLog)
	{
		UE_LOG(LogCombatManager, Log,
			TEXT("[CombatDebug] Weapon attrs: Phys %.1f-%.1f (+%.1f flat) Fire %.1f-%.1f (+%.1f) Ice %.1f-%.1f (+%.1f) Lightning %.1f-%.1f (+%.1f) Light %.1f-%.1f (+%.1f) Corruption %.1f-%.1f (+%.1f)"),
			AttackerAttributes->GetMinPhysicalDamage(), AttackerAttributes->GetMaxPhysicalDamage(), AttackerAttributes->GetPhysicalFlatDamage(),
			AttackerAttributes->GetMinFireDamage(), AttackerAttributes->GetMaxFireDamage(), AttackerAttributes->GetFireFlatDamage(),
			AttackerAttributes->GetMinIceDamage(), AttackerAttributes->GetMaxIceDamage(), AttackerAttributes->GetIceFlatDamage(),
			AttackerAttributes->GetMinLightningDamage(), AttackerAttributes->GetMaxLightningDamage(), AttackerAttributes->GetLightningFlatDamage(),
			AttackerAttributes->GetMinLightDamage(), AttackerAttributes->GetMaxLightDamage(), AttackerAttributes->GetLightFlatDamage(),
			AttackerAttributes->GetMinCorruptionDamage(), AttackerAttributes->GetMaxCorruptionDamage(), AttackerAttributes->GetCorruptionFlatDamage());
		UE_LOG(LogCombatManager, Log, TEXT("[CombatDebug] Stage 1 base roll:      %s"),
			*CombatManagerPrivate::FormatPacket(Packet));
	}

	// Conversion first: converted damage scales only with modifiers of its
	// final type, never both (PoE2 rule — no conversion double-dipping).
	Packet = ApplyDamageConversion(Packet, AttackerAttributes);

	if (bDebugLog)
	{
		UE_LOG(LogCombatManager, Log, TEXT("[CombatDebug] Stage 2 post-conversion: %s"),
			*CombatManagerPrivate::FormatPacket(Packet));
	}

	for (const EHunterDamageType DamageType : CombatManagerPrivate::AllDamageTypes)
	{
		const float BaseDamage = CombatManagerPrivate::GetPacketDamage(Packet, DamageType);
		if (BaseDamage <= 0.f)
		{
			continue;
		}

		const float IncreasedPercent = GetIncreasedDamagePercent(DamageType, AttackerAttributes, DamageInfo);
		const float AfterIncreased = FMath::Max(
			0.f, CombatManagerPrivate::ApplyPercentIncrease(BaseDamage, IncreasedPercent));
		const float MoreMultiplier = GetMoreDamageMultiplier(DamageType, AttackerAttributes);

		if (bDebugLog)
		{
			UE_LOG(LogCombatManager, Log,
				TEXT("[CombatDebug] Stage 3 scaling %-10s base=%.2f increased=%+.1f%% more=x%.3f -> %.2f"),
				*UEnum::GetDisplayValueAsText(DamageType).ToString(),
				BaseDamage, IncreasedPercent, MoreMultiplier,
				FMath::Max(0.f, AfterIncreased * MoreMultiplier));
		}

		CombatManagerPrivate::SetPacketDamage(
			Packet, DamageType, FMath::Max(0.f, AfterIncreased * MoreMultiplier));
	}

	ResolveCriticalStrike(Packet, AttackerAttributes, DamageInfo);

	if (bDebugLog)
	{
		UE_LOG(LogCombatManager, Log, TEXT("[CombatDebug] Stage 4 post-crit:       %s"),
			*CombatManagerPrivate::FormatPacket(Packet));
	}

	return Packet;
}

float UCombatManager::CalculateBaseDamageForType(
	const EHunterDamageType DamageType,
	const UHunterAttributeSet* AttackerAttributes) const
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

FCombatDamagePacket UCombatManager::ApplyDamageConversion(
	const FCombatDamagePacket& InPacket,
	const UHunterAttributeSet* AttackerAttributes) const
{
	if (!AttackerAttributes)
	{
		return InPacket;
	}

	FCombatDamagePacket OutPacket = InPacket;
	for (const EHunterDamageType DamageType : CombatManagerPrivate::AllDamageTypes)
	{
		CombatManagerPrivate::SetPacketDamage(OutPacket, DamageType, 0.f);
	}

	for (const EHunterDamageType FromType : CombatManagerPrivate::AllDamageTypes)
	{
		const float SourceDamage = FMath::Max(
			0.f, CombatManagerPrivate::GetPacketDamage(InPacket, FromType));
		if (SourceDamage <= 0.f)
		{
			continue;
		}

		float TotalConversionPercent = 0.f;
		for (const EHunterDamageType ToType : CombatManagerPrivate::AllDamageTypes)
		{
			TotalConversionPercent += FMath::Max(
				0.f, CombatManagerPrivate::GetConversionPercent(AttackerAttributes, FromType, ToType));
		}

		// Over-allocated conversion (total > 100%) scales down proportionally
		// so the hit never gains free damage from stacking conversion sources.
		const float ConversionScale = TotalConversionPercent > 100.f
			? 100.f / TotalConversionPercent
			: 1.f;

		float ConvertedAway = 0.f;
		for (const EHunterDamageType ToType : CombatManagerPrivate::AllDamageTypes)
		{
			const float Percent = FMath::Max(
				0.f, CombatManagerPrivate::GetConversionPercent(AttackerAttributes, FromType, ToType))
				* ConversionScale;
			if (Percent <= 0.f)
			{
				continue;
			}

			const float ConvertedAmount = SourceDamage * (Percent / 100.f);
			ConvertedAway += ConvertedAmount;
			CombatManagerPrivate::SetPacketDamage(
				OutPacket, ToType,
				CombatManagerPrivate::GetPacketDamage(OutPacket, ToType) + ConvertedAmount);
		}

		const float Remainder = FMath::Max(0.f, SourceDamage - ConvertedAway);
		CombatManagerPrivate::SetPacketDamage(
			OutPacket, FromType,
			CombatManagerPrivate::GetPacketDamage(OutPacket, FromType) + Remainder);
	}

	CombatManagerPrivate::UpdatePacketTotal(OutPacket);
	return OutPacket;
}

float UCombatManager::GetIncreasedDamagePercent(
	const EHunterDamageType DamageType,
	const UHunterAttributeSet* AttackerAttributes,
	const FAnimationDamageInfo& DamageInfo) const
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

	if (CombatManagerPrivate::IsElementalDamageType(DamageType))
	{
		TotalIncreasedPercent += AttackerAttributes->GetElementalDamage();
	}

	// Tag-conditional increased buckets. Each true flag opts this hit into the
	// matching attribute the same way PoE2 skill tags gate support scaling.
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

float UCombatManager::GetMoreDamageMultiplier(
	const EHunterDamageType DamageType,
	const UHunterAttributeSet* AttackerAttributes) const
{
	if (!AttackerAttributes)
	{
		return 1.f;
	}

	float Multiplier = CombatManagerPrivate::GetNeutralMultiplier(AttackerAttributes->GetGlobalMoreDamage());

	if (CombatManagerPrivate::IsElementalDamageType(DamageType))
	{
		Multiplier *= CombatManagerPrivate::GetNeutralMultiplier(AttackerAttributes->GetElementalMoreDamage());
	}

	switch (DamageType)
	{
	case EHunterDamageType::Physical:
		Multiplier *= CombatManagerPrivate::GetNeutralMultiplier(AttackerAttributes->GetPhysicalMoreDamage());
		break;
	case EHunterDamageType::Fire:
		Multiplier *= CombatManagerPrivate::GetNeutralMultiplier(AttackerAttributes->GetFireMoreDamage());
		break;
	case EHunterDamageType::Ice:
		Multiplier *= CombatManagerPrivate::GetNeutralMultiplier(AttackerAttributes->GetIceMoreDamage());
		break;
	case EHunterDamageType::Lightning:
		Multiplier *= CombatManagerPrivate::GetNeutralMultiplier(AttackerAttributes->GetLightningMoreDamage());
		break;
	case EHunterDamageType::Light:
		Multiplier *= CombatManagerPrivate::GetNeutralMultiplier(AttackerAttributes->GetLightMoreDamage());
		break;
	case EHunterDamageType::Corruption:
		Multiplier *= CombatManagerPrivate::GetNeutralMultiplier(AttackerAttributes->GetCorruptionMoreDamage());
		break;
	default:
		break;
	}

	return FMath::Max(0.f, Multiplier);
}

void UCombatManager::ResolveCriticalStrike(
	FCombatDamagePacket& Packet,
	const UHunterAttributeSet* AttackerAttributes,
	const FAnimationDamageInfo& DamageInfo) const
{
	Packet.bCrit = false;
	Packet.CritMultiplierApplied = 1.f;

	// Damage-over-time hits never crit (PoE rule), and the animation gate
	// controls everything else including forced crits.
	if (!DamageInfo.Crit.bCanCrit || DamageInfo.Tags.bIsDamageOverTime || !AttackerAttributes)
	{
		CombatManagerPrivate::UpdatePacketTotal(Packet);
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
		CombatManagerPrivate::UpdatePacketTotal(Packet);
		return;
	}

	float CritMultiplier = AttackerAttributes->GetCritMultiplier() > 0.f
		? AttackerAttributes->GetCritMultiplier()
		: CombatManagerPrivate::DefaultCritMultiplier;
	if (DamageInfo.Tags.bIsSpell)
	{
		const float SpellCritMultiplier =
			CombatManagerPrivate::GetNeutralMultiplier(AttackerAttributes->GetSpellsCritMultiplier());
		CritMultiplier += FMath::Max(0.f, SpellCritMultiplier - 1.f);
	}
	CritMultiplier += FMath::Max(0.f, DamageInfo.Crit.CritMultiplier);
	CritMultiplier = FMath::Max(CritMultiplier, 0.f);

	for (const EHunterDamageType DamageType : CombatManagerPrivate::AllDamageTypes)
	{
		CombatManagerPrivate::SetPacketDamage(
			Packet, DamageType,
			CombatManagerPrivate::GetPacketDamage(Packet, DamageType) * CritMultiplier);
	}

	Packet.bCrit = true;
	Packet.CritMultiplierApplied = CritMultiplier;
	CombatManagerPrivate::UpdatePacketTotal(Packet);
}

// ─── Incoming stages ─────────────────────────────────────────────────────────

FCombatResolveResult UCombatManager::MitigateDamagePacket(
	const FCombatDamagePacket& InPacket,
	AActor* AttackerActor,
	AActor* DefenderActor,
	const UHunterAttributeSet* AttackerAttributes,
	const UHunterAttributeSet* DefenderAttributes,
	const FAnimationDamageInfo& DamageInfo) const
{
	FCombatResolveResult Result;
	Result.PreMitigationPacket = InPacket;
	Result.bWasCrit = InPacket.bCrit;

	if (!DefenderAttributes)
	{
		return Result;
	}

	// Physical mitigates through armour, reduced by armour piercing.
	const float EffectiveArmour = FMath::Max(
		(DefenderAttributes->GetArmour() + DefenderAttributes->GetArmourFlatBonus()) *
		(1.f + (DefenderAttributes->GetArmourPercentBonus() / 100.f)),
		0.f);
	const float IncomingPhysical = FMath::Max(InPacket.Physical, 0.f);
	const float ArmourPiercingPercent = FMath::Clamp(
		(AttackerAttributes ? AttackerAttributes->GetArmourPiercing() : 0.f) +
		DamageInfo.Piercing.ArmourPiercing,
		0.f, 100.f);
	const float ArmourAfterPierce = EffectiveArmour * (1.f - (ArmourPiercingPercent / 100.f));
	const float PhysicalMitigation = IncomingPhysical > 0.f
		? FMath::Clamp(
			ArmourAfterPierce /
			(ArmourAfterPierce + (IncomingPhysical * CombatManagerPrivate::ArmourHitSizeScale)),
			0.f, CombatManagerPrivate::MaxArmourMitigation)
		: 0.f;

	// A landed physical hit always deals at least 1 damage through armour.
	const float PhysicalAfterArmour = IncomingPhysical * (1.f - PhysicalMitigation);
	Result.PhysicalTaken = IncomingPhysical > 0.f
		? FMath::Min(IncomingPhysical, FMath::Max(1.f, PhysicalAfterArmour))
		: 0.f;

	const bool bDebugLog = CombatManagerPrivate::IsCombatDebugLoggingEnabled();
	if (bDebugLog && IncomingPhysical > 0.f)
	{
		UE_LOG(LogCombatManager, Log,
			TEXT("[CombatDebug] Stage 5 armour: Armour=%.1f pierce=%.1f%% mitigation=%.1f%% physical %.2f -> %.2f"),
			EffectiveArmour, ArmourPiercingPercent, PhysicalMitigation * 100.f,
			IncomingPhysical, Result.PhysicalTaken);
	}

	// Every non-physical type mitigates through its resistance, reduced by
	// piercing and clamped to [-100, per-type max cap].
	const auto MitigateTypedDamage = [&](const EHunterDamageType DamageType) -> float
	{
		const float IncomingDamage = FMath::Max(
			CombatManagerPrivate::GetPacketDamage(InPacket, DamageType), 0.f);
		if (IncomingDamage <= 0.f)
		{
			return 0.f;
		}

		const float Resistance = GetResistanceValue(DamageType, DefenderAttributes)
			- GetResistancePierceValue(DamageType, AttackerAttributes, DamageInfo);
		const float ClampedResistance = FMath::Clamp(
			Resistance,
			CombatManagerPrivate::MinResistancePercent,
			GetResistanceCap(DamageType, DefenderAttributes));
		const float MitigatedDamage = FMath::Max(0.f, IncomingDamage * (1.f - (ClampedResistance / 100.f)));

		if (bDebugLog)
		{
			UE_LOG(LogCombatManager, Log,
				TEXT("[CombatDebug] Stage 5 resist %-10s res=%.1f%% (clamped) %.2f -> %.2f"),
				*UEnum::GetDisplayValueAsText(DamageType).ToString(),
				ClampedResistance, IncomingDamage, MitigatedDamage);
		}

		return MitigatedDamage;
	};

	Result.FireTaken = MitigateTypedDamage(EHunterDamageType::Fire);
	Result.IceTaken = MitigateTypedDamage(EHunterDamageType::Ice);
	Result.LightningTaken = MitigateTypedDamage(EHunterDamageType::Lightning);
	Result.LightTaken = MitigateTypedDamage(EHunterDamageType::Light);
	Result.CorruptionTaken = MitigateTypedDamage(EHunterDamageType::Corruption);

	Result.TotalDamageBeforeBlock =
		Result.PhysicalTaken + Result.FireTaken + Result.IceTaken +
		Result.LightningTaken + Result.LightTaken + Result.CorruptionTaken;
	Result.TotalDamageAfterBlock = Result.TotalDamageBeforeBlock;

	ApplyBlockingToMitigatedResult(AttackerActor, DefenderActor, DefenderAttributes, Result);
	ApplyStaminaBlockCost(DefenderAttributes, Result);

	for (const EHunterDamageType DamageType : CombatManagerPrivate::AllDamageTypes)
	{
		const float DamageAfterBlock = CombatManagerPrivate::GetResultTakenByType(Result, DamageType);
		const float TakenMultiplier = GetDamageTakenMultiplier(DamageType, DefenderAttributes);

		if (bDebugLog && DamageAfterBlock > 0.f && !FMath::IsNearlyEqual(TakenMultiplier, 1.f))
		{
			UE_LOG(LogCombatManager, Log,
				TEXT("[CombatDebug] Stage 7 taken-mult %-10s x%.3f %.2f -> %.2f"),
				*UEnum::GetDisplayValueAsText(DamageType).ToString(),
				TakenMultiplier, DamageAfterBlock, DamageAfterBlock * TakenMultiplier);
		}

		CombatManagerPrivate::SetResultTakenByType(
			Result, DamageType, FMath::Max(0.f, DamageAfterBlock * TakenMultiplier));
	}

	Result.TotalDamageTaken =
		Result.PhysicalTaken + Result.FireTaken + Result.IceTaken +
		Result.LightningTaken + Result.LightTaken + Result.CorruptionTaken;

	// ArcaneShield absorbs before Health.
	const float CurrentArcaneShield = FMath::Max(DefenderAttributes->GetArcaneShield(), 0.f);
	Result.DamageToArcaneShield = FMath::Min(CurrentArcaneShield, Result.TotalDamageTaken);
	Result.DamageToHealth = FMath::Max(0.f, Result.TotalDamageTaken - Result.DamageToArcaneShield);

	if (bDebugLog)
	{
		if (Result.bWasBlocked)
		{
			UE_LOG(LogCombatManager, Log,
				TEXT("[CombatDebug] Stage 6 block: blocked=%.2f afterBlock=%.2f staminaCost=%.2f guardBroken=%s"),
				Result.TotalBlockedAmount, Result.TotalDamageAfterBlock, Result.DamageToStamina,
				Result.bGuardBroken ? TEXT("yes") : TEXT("no"));
		}
		UE_LOG(LogCombatManager, Log,
			TEXT("[CombatDebug] Stage 8 routing: total=%.2f -> shield=%.2f health=%.2f"),
			Result.TotalDamageTaken, Result.DamageToArcaneShield, Result.DamageToHealth);
	}

	return Result;
}

float UCombatManager::GetResistanceValue(
	const EHunterDamageType DamageType,
	const UHunterAttributeSet* DefenderAttributes)
{
	if (!DefenderAttributes)
	{
		return 0.f;
	}

	const float GlobalDefenses = DefenderAttributes->GetGlobalDefenses();

	switch (DamageType)
	{
	case EHunterDamageType::Fire:
		return GlobalDefenses + DefenderAttributes->GetFireResistanceFlatBonus()
			+ DefenderAttributes->GetFireResistancePercentBonus();
	case EHunterDamageType::Ice:
		return GlobalDefenses + DefenderAttributes->GetIceResistanceFlatBonus()
			+ DefenderAttributes->GetIceResistancePercentBonus();
	case EHunterDamageType::Lightning:
		return GlobalDefenses + DefenderAttributes->GetLightningResistanceFlatBonus()
			+ DefenderAttributes->GetLightningResistancePercentBonus();
	case EHunterDamageType::Light:
		return GlobalDefenses + DefenderAttributes->GetLightResistanceFlatBonus()
			+ DefenderAttributes->GetLightResistancePercentBonus();
	case EHunterDamageType::Corruption:
		return GlobalDefenses + DefenderAttributes->GetCorruptionResistanceFlatBonus()
			+ DefenderAttributes->GetCorruptionResistancePercentBonus();
	default:
		return 0.f;
	}
}

float UCombatManager::GetResistanceCap(
	const EHunterDamageType DamageType,
	const UHunterAttributeSet* DefenderAttributes) const
{
	if (!DefenderAttributes)
	{
		return CombatManagerPrivate::DefaultMaxResistancePercent;
	}

	float Cap = 0.f;
	switch (DamageType)
	{
	case EHunterDamageType::Fire:       Cap = DefenderAttributes->GetMaxFireResistance(); break;
	case EHunterDamageType::Ice:        Cap = DefenderAttributes->GetMaxIceResistance(); break;
	case EHunterDamageType::Lightning:  Cap = DefenderAttributes->GetMaxLightningResistance(); break;
	case EHunterDamageType::Light:      Cap = DefenderAttributes->GetMaxLightResistance(); break;
	case EHunterDamageType::Corruption: Cap = DefenderAttributes->GetMaxCorruptionResistance(); break;
	default: break;
	}

	return Cap > 0.f ? Cap : CombatManagerPrivate::DefaultMaxResistancePercent;
}

float UCombatManager::GetResistancePierceValue(
	const EHunterDamageType DamageType,
	const UHunterAttributeSet* AttackerAttributes,
	const FAnimationDamageInfo& DamageInfo) const
{
	float AttributePierce = 0.f;
	float AnimationPierce = 0.f;

	switch (DamageType)
	{
	case EHunterDamageType::Fire:
		AttributePierce = AttackerAttributes ? AttackerAttributes->GetFirePiercing() : 0.f;
		AnimationPierce = DamageInfo.Piercing.Fire;
		break;
	case EHunterDamageType::Ice:
		AttributePierce = AttackerAttributes ? AttackerAttributes->GetIcePiercing() : 0.f;
		AnimationPierce = DamageInfo.Piercing.Ice;
		break;
	case EHunterDamageType::Lightning:
		AttributePierce = AttackerAttributes ? AttackerAttributes->GetLightningPiercing() : 0.f;
		AnimationPierce = DamageInfo.Piercing.Lightning;
		break;
	case EHunterDamageType::Light:
		AttributePierce = AttackerAttributes ? AttackerAttributes->GetLightPiercing() : 0.f;
		AnimationPierce = DamageInfo.Piercing.Light;
		break;
	case EHunterDamageType::Corruption:
		AttributePierce = AttackerAttributes ? AttackerAttributes->GetCorruptionPiercing() : 0.f;
		AnimationPierce = DamageInfo.Piercing.Corruption;
		break;
	default:
		return 0.f;
	}

	return FMath::Clamp(AttributePierce + AnimationPierce, 0.f, 100.f);
}

float UCombatManager::GetDamageTakenMultiplier(
	const EHunterDamageType DamageType,
	const UHunterAttributeSet* DefenderAttributes) const
{
	if (!DefenderAttributes)
	{
		return 1.f;
	}

	float Multiplier = CombatManagerPrivate::GetNeutralMultiplier(
		DefenderAttributes->GetGlobalDamageTakenMultiplier());

	if (CombatManagerPrivate::IsElementalDamageType(DamageType))
	{
		Multiplier *= CombatManagerPrivate::GetNeutralMultiplier(
			DefenderAttributes->GetElementalDamageTakenMultiplier());
	}

	switch (DamageType)
	{
	case EHunterDamageType::Physical:
		Multiplier *= CombatManagerPrivate::GetNeutralMultiplier(DefenderAttributes->GetPhysicalDamageTakenMultiplier());
		break;
	case EHunterDamageType::Fire:
		Multiplier *= CombatManagerPrivate::GetNeutralMultiplier(DefenderAttributes->GetFireDamageTakenMultiplier());
		break;
	case EHunterDamageType::Ice:
		Multiplier *= CombatManagerPrivate::GetNeutralMultiplier(DefenderAttributes->GetIceDamageTakenMultiplier());
		break;
	case EHunterDamageType::Lightning:
		Multiplier *= CombatManagerPrivate::GetNeutralMultiplier(DefenderAttributes->GetLightningDamageTakenMultiplier());
		break;
	case EHunterDamageType::Light:
		Multiplier *= CombatManagerPrivate::GetNeutralMultiplier(DefenderAttributes->GetLightDamageTakenMultiplier());
		break;
	case EHunterDamageType::Corruption:
		Multiplier *= CombatManagerPrivate::GetNeutralMultiplier(DefenderAttributes->GetCorruptionDamageTakenMultiplier());
		break;
	default:
		break;
	}

	return FMath::Max(0.f, Multiplier);
}

bool UCombatManager::IsActorBlocking(AActor* Actor) const
{
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActor(Actor);
	if (!ASC)
	{
		return false;
	}

	const FPHGameplayTags& Tags = FPHGameplayTags::Get();
	return ASC->HasMatchingGameplayTag(Tags.Condition_Self_IsBlocking)
		|| ASC->HasMatchingGameplayTag(Tags.Condition_Target_IsBlocking);
}

bool UCombatManager::CanBlockHit(
	AActor* AttackerActor,
	AActor* DefenderActor,
	const UHunterAttributeSet* DefenderAttributes) const
{
	if (!IsValid(DefenderActor) || !DefenderAttributes || !IsActorBlocking(DefenderActor))
	{
		return false;
	}

	if (!IsValid(AttackerActor))
	{
		return true;
	}

	const FVector DirectionToAttacker =
		(AttackerActor->GetActorLocation() - DefenderActor->GetActorLocation()).GetSafeNormal();
	if (DirectionToAttacker.IsNearlyZero())
	{
		return true;
	}

	const float BlockAngleDegrees = DefenderAttributes->GetBlockAngle() > 0.f
		? DefenderAttributes->GetBlockAngle()
		: CombatManagerPrivate::DefaultBlockAngleDegrees;
	const float HalfAngleRadians =
		FMath::DegreesToRadians(FMath::Clamp(BlockAngleDegrees, 0.f, 180.f) * 0.5f);
	const float ForwardDot = FVector::DotProduct(
		DefenderActor->GetActorForwardVector().GetSafeNormal(), DirectionToAttacker);

	return ForwardDot >= FMath::Cos(HalfAngleRadians);
}

float UCombatManager::GetBlockTypeMultiplier(
	const EHunterDamageType DamageType,
	const UHunterAttributeSet* DefenderAttributes) const
{
	if (!DefenderAttributes)
	{
		return 1.f;
	}

	switch (DamageType)
	{
	case EHunterDamageType::Physical:
		return CombatManagerPrivate::GetNeutralMultiplier(DefenderAttributes->GetBlockPhysicalMultiplier());
	case EHunterDamageType::Fire:
	case EHunterDamageType::Ice:
	case EHunterDamageType::Lightning:
	case EHunterDamageType::Light:
		return CombatManagerPrivate::GetNeutralMultiplier(DefenderAttributes->GetBlockElementalMultiplier());
	case EHunterDamageType::Corruption:
		return CombatManagerPrivate::GetNeutralMultiplier(DefenderAttributes->GetBlockCorruptionMultiplier());
	default:
		return 1.f;
	}
}

void UCombatManager::ApplyBlockingToMitigatedResult(
	AActor* AttackerActor,
	AActor* DefenderActor,
	const UHunterAttributeSet* DefenderAttributes,
	FCombatResolveResult& InOutResult) const
{
	InOutResult.TotalDamageAfterBlock = InOutResult.TotalDamageBeforeBlock;

	if (!CanBlockHit(AttackerActor, DefenderActor, DefenderAttributes))
	{
		return;
	}

	const float BlockStrengthFraction = FMath::Clamp(
		DefenderAttributes ? (DefenderAttributes->GetBlockStrength() / 100.f) : 0.f, 0.f, 0.95f);
	const float FlatBlockAmount = DefenderAttributes
		? FMath::Max(DefenderAttributes->GetFlatBlockAmount(), 0.f)
		: 0.f;
	const float ChipDamageFraction = FMath::Clamp(
		DefenderAttributes ? (DefenderAttributes->GetChipDamageWhileBlocking() / 100.f) : 0.f, 0.f, 1.f);

	InOutResult.bWasBlocked = true;

	float TotalIncomingForBlock = 0.f;
	for (const EHunterDamageType DamageType : CombatManagerPrivate::AllDamageTypes)
	{
		TotalIncomingForBlock += FMath::Max(0.f,
			CombatManagerPrivate::GetResultTakenByType(InOutResult, DamageType)
			* GetBlockTypeMultiplier(DamageType, DefenderAttributes));
	}

	float TotalAfterBlock = 0.f;
	float TotalBlockedAmount = 0.f;

	for (const EHunterDamageType DamageType : CombatManagerPrivate::AllDamageTypes)
	{
		const float DamageBeforeBlock = FMath::Max(
			0.f, CombatManagerPrivate::GetResultTakenByType(InOutResult, DamageType));
		const float BlockedInput = DamageBeforeBlock * GetBlockTypeMultiplier(DamageType, DefenderAttributes);

		// Flat block splits across types proportionally to their contribution.
		const float TypeFlatShare = TotalIncomingForBlock > 0.f
			? FlatBlockAmount * (BlockedInput / TotalIncomingForBlock)
			: 0.f;

		const float BlockedTotal = (BlockedInput * BlockStrengthFraction) + TypeFlatShare;
		const float DamageAfterBlock = FMath::Max(0.f, BlockedInput - BlockedTotal);

		// Chip damage guarantees a floor of unblockable bleed-through.
		const float ChipFloor = BlockedInput * ChipDamageFraction;
		const float FinalTypeDamage = FMath::Max(DamageAfterBlock, ChipFloor);
		const float BlockedAmount = FMath::Max(0.f, DamageBeforeBlock - FinalTypeDamage);

		CombatManagerPrivate::SetResultTakenByType(InOutResult, DamageType, FinalTypeDamage);
		CombatManagerPrivate::SetResultBlockedByType(InOutResult, DamageType, BlockedAmount);

		TotalAfterBlock += FinalTypeDamage;
		TotalBlockedAmount += BlockedAmount;
	}

	InOutResult.TotalDamageAfterBlock = TotalAfterBlock;
	InOutResult.TotalBlockedAmount = TotalBlockedAmount;
}

void UCombatManager::ApplyStaminaBlockCost(
	const UHunterAttributeSet* DefenderAttributes,
	FCombatResolveResult& InOutResult) const
{
	if (!DefenderAttributes || !InOutResult.bWasBlocked || InOutResult.TotalBlockedAmount <= 0.f)
	{
		return;
	}

	const float CostMultiplier = CombatManagerPrivate::GetNeutralMultiplier(
		DefenderAttributes->GetBlockStaminaCostMultiplier());
	const float RequestedCost = FMath::Max(0.f, InOutResult.TotalBlockedAmount * CostMultiplier);
	const float CurrentStamina = FMath::Max(DefenderAttributes->GetStamina(), 0.f);
	const float ActualCost = FMath::Min(CurrentStamina, RequestedCost);
	const float RemainingStamina = FMath::Max(0.f, CurrentStamina - ActualCost);
	const float GuardBreakThreshold = FMath::Max(DefenderAttributes->GetGuardBreakThreshold(), 0.f);

	InOutResult.DamageToStamina = ActualCost;
	InOutResult.bGuardBroken =
		(GuardBreakThreshold > 0.f && InOutResult.TotalBlockedAmount >= GuardBreakThreshold)
		|| (CurrentStamina > 0.f && ActualCost > 0.f && RemainingStamina <= 0.f);
}

void UCombatManager::EvaluateStagger(
	AActor* DefenderActor,
	const UHunterAttributeSet* DefenderAttributes,
	FCombatResolveResult& InOutResult) const
{
	if (!DefenderAttributes || InOutResult.DamageToStamina <= 0.f)
	{
		return;
	}

	const float StaminaAfterHit = FMath::Max(
		0.f, DefenderAttributes->GetStamina() - InOutResult.DamageToStamina);
	if (StaminaAfterHit > 0.f)
	{
		return;
	}

	const UAbilitySystemComponent* DefenderASC = GetAbilitySystemComponentFromActor(DefenderActor);
	if (!DefenderASC)
	{
		return;
	}

	// Skill execution grants hyper-armor against stagger.
	if (DefenderASC->HasMatchingGameplayTag(FPHGameplayTags::Get().State_Self_ExecutingSkill))
	{
		return;
	}

	InOutResult.bShouldStagger = true;
}

void UCombatManager::ApplyHitResponse(
	const EHitResponse HitResponse,
	const bool bCanApplyAilments,
	FCombatResolveResult& InOutResult) const
{
	InOutResult.HitResponse = HitResponse;
	InOutResult.bShouldApplyAilments = bCanApplyAilments;

	switch (HitResponse)
	{
	case EHitResponse::Parry:
		// Damage cancelled, but contact was made: per-type taken values stay
		// intact so ailments still roll with real magnitudes.
		InOutResult.DamageToHealth = 0.f;
		InOutResult.DamageToArcaneShield = 0.f;
		InOutResult.DamageToStamina = 0.f;
		InOutResult.TotalDamageTaken = 0.f;
		InOutResult.bShouldApplyAilments = bCanApplyAilments;
		InOutResult.bShouldStagger = false;
		InOutResult.bKilledTarget = false;
		break;

	case EHitResponse::Invincible:
		InOutResult.PhysicalTaken = 0.f;
		InOutResult.FireTaken = 0.f;
		InOutResult.IceTaken = 0.f;
		InOutResult.LightningTaken = 0.f;
		InOutResult.LightTaken = 0.f;
		InOutResult.CorruptionTaken = 0.f;
		InOutResult.PhysicalBlocked = 0.f;
		InOutResult.FireBlocked = 0.f;
		InOutResult.IceBlocked = 0.f;
		InOutResult.LightningBlocked = 0.f;
		InOutResult.LightBlocked = 0.f;
		InOutResult.CorruptionBlocked = 0.f;
		InOutResult.TotalDamageBeforeBlock = 0.f;
		InOutResult.TotalDamageAfterBlock = 0.f;
		InOutResult.TotalBlockedAmount = 0.f;
		InOutResult.DamageToStamina = 0.f;
		InOutResult.DamageToArcaneShield = 0.f;
		InOutResult.DamageToHealth = 0.f;
		InOutResult.TotalDamageTaken = 0.f;
		InOutResult.bShouldApplyAilments = false;
		InOutResult.bShouldStagger = false;
		InOutResult.bWasBlocked = false;
		InOutResult.bGuardBroken = false;
		InOutResult.bWasCrit = false;
		InOutResult.bKilledTarget = false;
		break;

	case EHitResponse::Blocked:
	case EHitResponse::Normal:
	default:
		break;
	}
}

// ─── Application ─────────────────────────────────────────────────────────────

void UCombatManager::ApplyResolvedDamage(
	AActor* AttackerActor,
	AActor* DefenderActor,
	const FCombatResolveResult& Result) const
{
	if (!IsValid(DefenderActor) || !DefenderActor->HasAuthority())
	{
		return;
	}

	if (Result.TotalDamageTaken <= 0.f && Result.DamageToStamina <= 0.f)
	{
		return;
	}

	UAbilitySystemComponent* DefenderASC = GetAbilitySystemComponentFromActor(DefenderActor);
	const UHunterAttributeSet* DefenderAttributes = GetHunterAttributeSetFromActor(DefenderActor);
	if (!DefenderASC || !DefenderAttributes)
	{
		return;
	}

	if (DamageApplicationGE)
	{
		FGameplayEffectContextHandle Context = DefenderASC->MakeEffectContext();
		Context.AddSourceObject(AttackerActor ? AttackerActor : GetOwner());

		const FGameplayEffectSpecHandle Spec = DefenderASC->MakeOutgoingSpec(DamageApplicationGE, 1.f, Context);
		if (!Spec.IsValid())
		{
			UE_LOG(LogCombatManager, Error,
				TEXT("ApplyResolvedDamage: MakeOutgoingSpec failed for DamageApplicationGE on %s."),
				*GetNameSafe(this));
			return;
		}

		const FPHGameplayTags& Tags = FPHGameplayTags::Get();
		if (Result.DamageToHealth > 0.f)
		{
			Spec.Data->SetSetByCallerMagnitude(Tags.Data_Damage_Health, -Result.DamageToHealth);
		}
		if (Result.DamageToArcaneShield > 0.f)
		{
			Spec.Data->SetSetByCallerMagnitude(Tags.Data_Damage_ArcaneShield, -Result.DamageToArcaneShield);
		}
		if (Result.DamageToStamina > 0.f)
		{
			Spec.Data->SetSetByCallerMagnitude(Tags.Data_Damage_Stamina, -Result.DamageToStamina);
		}

		DefenderASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		return;
	}

	// Fallback path bypasses GAS clamping — configure DamageApplicationGE.
	UE_LOG(LogCombatManager, Warning,
		TEXT("ApplyResolvedDamage: DamageApplicationGE is not set on %s. Falling back to SetNumericAttributeBase."),
		*GetNameSafe(this));

	const float NewStamina = FMath::Max(
		0.f, DefenderAttributes->GetStamina() - FMath::Max(Result.DamageToStamina, 0.f));
	const float NewArcaneShield = FMath::Max(
		0.f, DefenderAttributes->GetArcaneShield() - FMath::Max(Result.DamageToArcaneShield, 0.f));
	const float NewHealth = FMath::Max(
		0.f, DefenderAttributes->GetHealth() - FMath::Max(Result.DamageToHealth, 0.f));

	DefenderASC->SetNumericAttributeBase(UHunterAttributeSet::GetStaminaAttribute(), NewStamina);
	DefenderASC->SetNumericAttributeBase(UHunterAttributeSet::GetArcaneShieldAttribute(), NewArcaneShield);
	DefenderASC->SetNumericAttributeBase(UHunterAttributeSet::GetHealthAttribute(), NewHealth);
}

void UCombatManager::ApplyOnHitRecovery(
	AActor* AttackerActor,
	const FCombatResolveResult& Result,
	UAbilitySystemComponent* AttackerASC,
	const UHunterAttributeSet* AttackerAttributes) const
{
	if (!AttackerASC || !AttackerAttributes || Result.TotalDamageTaken <= 0.f)
	{
		return;
	}

	const float HealthRecovery = FMath::Max(0.f, AttackerAttributes->GetLifeOnHit())
		+ FMath::Max(0.f, Result.TotalDamageTaken * (AttackerAttributes->GetLifeLeech() / 100.f));
	const float ManaRecovery = FMath::Max(0.f, AttackerAttributes->GetManaOnHit())
		+ FMath::Max(0.f, Result.TotalDamageTaken * (AttackerAttributes->GetManaLeech() / 100.f));
	const float StaminaRecovery = FMath::Max(0.f, AttackerAttributes->GetStaminaOnHit())
		+ FMath::Max(0.f, Result.TotalDamageTaken * (AttackerAttributes->GetStaminaLeechPercent() / 100.f));

	if (HealthRecovery <= 0.f && ManaRecovery <= 0.f && StaminaRecovery <= 0.f)
	{
		return;
	}

	if (!RecoveryApplicationGE)
	{
		UE_LOG(LogCombatManager, Warning,
			TEXT("ApplyOnHitRecovery: RecoveryApplicationGE is not set on %s — on-hit recovery skipped."),
			*GetNameSafe(this));
		return;
	}

	FGameplayEffectContextHandle Context = AttackerASC->MakeEffectContext();
	Context.AddSourceObject(AttackerActor);

	const FGameplayEffectSpecHandle Spec = AttackerASC->MakeOutgoingSpec(RecoveryApplicationGE, 1.f, Context);
	if (!Spec.IsValid())
	{
		return;
	}

	const FPHGameplayTags& Tags = FPHGameplayTags::Get();
	if (HealthRecovery > 0.f)
	{
		Spec.Data->SetSetByCallerMagnitude(Tags.Data_Recovery_Health, HealthRecovery);
	}
	if (ManaRecovery > 0.f)
	{
		Spec.Data->SetSetByCallerMagnitude(Tags.Data_Recovery_Mana, ManaRecovery);
	}
	if (StaminaRecovery > 0.f)
	{
		Spec.Data->SetSetByCallerMagnitude(Tags.Data_Recovery_Stamina, StaminaRecovery);
	}

	AttackerASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
}

UCombatStatusManager* UCombatManager::ResolveStatusManager(AActor* PreferredActor) const
{
	if (const AActor* Owner = GetOwner())
	{
		if (UCombatStatusManager* OwnerManager = Owner->FindComponentByClass<UCombatStatusManager>())
		{
			return OwnerManager;
		}
	}

	return IsValid(PreferredActor)
		? PreferredActor->FindComponentByClass<UCombatStatusManager>()
		: nullptr;
}

void UCombatManager::ApplyAilments(
	AActor* AttackerActor,
	AActor* DefenderActor,
	const FCombatResolveResult& Result) const
{
	if (!Result.bShouldApplyAilments || Result.HitResponse == EHitResponse::Invincible)
	{
		return;
	}

	const UHunterAttributeSet* AttackerAttributes = GetHunterAttributeSetFromActor(AttackerActor);
	if (!AttackerAttributes || !IsValid(DefenderActor))
	{
		return;
	}

	UCombatStatusManager* StatusManager = ResolveStatusManager(AttackerActor);
	if (!StatusManager)
	{
		UE_LOG(LogCombatManager, Verbose,
			TEXT("ApplyAilments skipped: no UCombatStatusManager found for %s."),
			*GetNameSafe(AttackerActor));
		return;
	}

	const auto RollChance = [](const float ChancePercent) -> bool
	{
		return ChancePercent > 0.f && FMath::FRandRange(0.f, 100.f) < FMath::Min(ChancePercent, 100.f);
	};
	const auto ResolveDuration = [](const float AttributeDuration, const float DefaultDuration) -> float
	{
		return AttributeDuration > 0.f ? AttributeDuration : DefaultDuration;
	};

	// Each typed ailment requires that damage type to have actually landed.
	// Parry keeps per-type taken values alive precisely so these still work.
	if (Result.PhysicalTaken > 0.f && RollChance(AttackerAttributes->GetChanceToBleed()))
	{
		StatusManager->ApplyBleed(
			DefenderActor,
			Result.PhysicalTaken * CombatManagerPrivate::BleedDamagePerTickPercent,
			ResolveDuration(AttackerAttributes->GetBleedDuration(), CombatManagerPrivate::DefaultBleedDuration),
			AttackerActor);
	}

	if (Result.FireTaken > 0.f && RollChance(AttackerAttributes->GetChanceToIgnite()))
	{
		StatusManager->ApplyIgnite(
			DefenderActor,
			Result.FireTaken * CombatManagerPrivate::IgniteDamagePerTickPercent,
			ResolveDuration(AttackerAttributes->GetBurnDuration(), CombatManagerPrivate::DefaultIgniteDuration),
			AttackerActor);
	}

	if (Result.CorruptionTaken > 0.f && RollChance(AttackerAttributes->GetChanceToCorrupt()))
	{
		StatusManager->ApplyCorruption(
			DefenderActor,
			Result.CorruptionTaken * CombatManagerPrivate::CorruptionDamagePerTickPercent,
			ResolveDuration(AttackerAttributes->GetCorruptionDuration(), CombatManagerPrivate::DefaultCorruptionDuration),
			AttackerActor);
	}

	if (Result.IceTaken > 0.f)
	{
		// Cold damage always chills; freeze is the chance roll on top.
		StatusManager->ApplyChill(
			DefenderActor,
			CombatManagerPrivate::DefaultChillSlowFraction,
			CombatManagerPrivate::DefaultChillDuration,
			AttackerActor);

		if (RollChance(AttackerAttributes->GetChanceToFreeze()))
		{
			StatusManager->ApplyFreeze(
				DefenderActor,
				ResolveDuration(AttackerAttributes->GetFreezeDuration(), CombatManagerPrivate::DefaultFreezeDuration),
				AttackerActor);
		}
	}

	if (Result.LightningTaken > 0.f && RollChance(AttackerAttributes->GetChanceToShock()))
	{
		StatusManager->ApplyShock(
			DefenderActor,
			CombatManagerPrivate::DefaultShockAmpFraction,
			ResolveDuration(AttackerAttributes->GetShockDuration(), CombatManagerPrivate::DefaultShockDuration),
			AttackerActor);
	}

	if (Result.LightTaken > 0.f && RollChance(AttackerAttributes->GetChanceToPetrify()))
	{
		StatusManager->ApplyPetrify(
			DefenderActor,
			ResolveDuration(AttackerAttributes->GetPetrifyBuildUpDuration(), CombatManagerPrivate::DefaultPetrifyDuration),
			AttackerActor);
	}
}

void UCombatManager::ApplyReflect(
	AActor* AttackerActor,
	AActor* DefenderActor,
	const FCombatResolveResult& Result) const
{
	if (Result.TotalDamageTaken <= 0.f || !IsValid(AttackerActor) || !IsValid(DefenderActor))
	{
		return;
	}

	const UHunterAttributeSet* DefenderAttributes = GetHunterAttributeSetFromActor(DefenderActor);
	if (!DefenderAttributes)
	{
		return;
	}

	const auto RollChance = [](const float ChancePercent) -> bool
	{
		return ChancePercent > 0.f && FMath::FRandRange(0.f, 100.f) < FMath::Min(ChancePercent, 100.f);
	};

	float ReflectedDamage = 0.f;
	if (Result.PhysicalTaken > 0.f && RollChance(DefenderAttributes->GetReflectChancePhysical()))
	{
		ReflectedDamage += Result.PhysicalTaken * (FMath::Max(0.f, DefenderAttributes->GetReflectPhysical()) / 100.f);
	}

	const float ElementalTaken =
		Result.FireTaken + Result.IceTaken + Result.LightningTaken + Result.LightTaken;
	if (ElementalTaken > 0.f && RollChance(DefenderAttributes->GetReflectChanceElemental()))
	{
		ReflectedDamage += ElementalTaken * (FMath::Max(0.f, DefenderAttributes->GetReflectElemental()) / 100.f);
	}

	if (ReflectedDamage <= 0.f)
	{
		return;
	}

	UAbilitySystemComponent* AttackerASC = GetAbilitySystemComponentFromActor(AttackerActor);
	if (!AttackerASC)
	{
		return;
	}

	if (!ReflectApplicationGE)
	{
		UE_LOG(LogCombatManager, Warning,
			TEXT("ApplyReflect: %.2f reflect damage rolled but ReflectApplicationGE is not set on %s."),
			ReflectedDamage, *GetNameSafe(this));
		return;
	}

	FGameplayEffectContextHandle Context = AttackerASC->MakeEffectContext();
	Context.AddSourceObject(DefenderActor);

	const FGameplayEffectSpecHandle Spec = AttackerASC->MakeOutgoingSpec(ReflectApplicationGE, 1.f, Context);
	if (!Spec.IsValid())
	{
		return;
	}

	Spec.Data->SetSetByCallerMagnitude(FPHGameplayTags::Get().Data_Damage_Health, -ReflectedDamage);
	AttackerASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
}

void UCombatManager::BroadcastDamagePopup(
	AActor* AttackerActor,
	AActor* DefenderActor,
	const FCombatResolveResult& Result)
{
	if (Result.TotalDamageTaken <= KINDA_SMALL_NUMBER || !OnDamagePopupRequested.IsBound())
	{
		return;
	}

	FCombatDamagePopupData PopupData;
	PopupData.SourceActor = AttackerActor;
	PopupData.TargetActor = DefenderActor;
	PopupData.ResolveResult = Result;
	PopupData.TotalDamage = Result.TotalDamageTaken;
	PopupData.DominantDamageType = UCombatFunctionLibrary::GetDominantDamageTypeFromResolveResult(Result);
	PopupData.DisplayColor = UCombatFunctionLibrary::GetDefaultDamageTypeColor(PopupData.DominantDamageType);
	PopupData.WorldLocation = CombatManagerPrivate::ResolveDamagePopupWorldLocation(DefenderActor);
	PopupData.bWasCrit = Result.bWasCrit;
	PopupData.bWasBlocked = Result.bWasBlocked;
	PopupData.bKilledTarget = Result.bKilledTarget;

	OnDamagePopupRequested.Broadcast(PopupData);
}
