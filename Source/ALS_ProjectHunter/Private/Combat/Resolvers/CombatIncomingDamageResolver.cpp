#include "Combat/Resolvers/CombatIncomingDamageResolver.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystem/HunterAttributeSet.h"
#include "Combat/Library/CombatDebug.h"
#include "GameFramework/Actor.h"
#include "Tags/PHGameplayTags.h"

DEFINE_LOG_CATEGORY_STATIC(LogCombatIncomingDamageResolver, Log, All);

namespace CombatIncomingDamageResolverPrivate
{
	bool IsCombatDebugLoggingEnabled()
	{
		return PHCombatDebug::IsCombatDebugLoggingEnabled();
	}

	constexpr float MinResistancePercent = -100.f;
	constexpr float DefaultMaxResistancePercent = 90.f;
	constexpr float DefaultBlockAngleDegrees = 120.f;

	// Hit-size armour mitigation: Armour / (Armour + Scale * HitSize).
	// Bigger hits punch through the same armour harder.
	constexpr float ArmourHitSizeScale = 10.f;
	constexpr float MaxArmourMitigation = 0.9f;

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

	// Multiplier attributes use a literal ratio: 1.0 neutral, 0.5 half, 0.0 none,
	// 2.0 double. Every one of them is seeded to 1.0 in UHunterAttributeSet's
	// constructor, so an incoming 0 is a real "take no damage" modifier and must
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
}

UAbilitySystemComponent* FCombatIncomingDamageResolver::GetAbilitySystemComponentFromActor(const AActor* Actor)
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

FCombatResolveResult FCombatIncomingDamageResolver::MitigateDamagePacket(
	const FCombatDamagePacket& InPacket,
	AActor* AttackerActor,
	AActor* DefenderActor,
	const UHunterAttributeSet* AttackerAttributes,
	const UHunterAttributeSet* DefenderAttributes,
	const FAnimationDamageInfo& DamageInfo)
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
			(ArmourAfterPierce + (IncomingPhysical * CombatIncomingDamageResolverPrivate::ArmourHitSizeScale)),
			0.f, CombatIncomingDamageResolverPrivate::MaxArmourMitigation)
		: 0.f;

	// A landed physical hit always deals at least 1 damage through armour.
	const float PhysicalAfterArmour = IncomingPhysical * (1.f - PhysicalMitigation);
	Result.PhysicalTaken = IncomingPhysical > 0.f
		? FMath::Min(IncomingPhysical, FMath::Max(1.f, PhysicalAfterArmour))
		: 0.f;

	const bool bDebugLog = CombatIncomingDamageResolverPrivate::IsCombatDebugLoggingEnabled();
	if (bDebugLog && IncomingPhysical > 0.f)
	{
		UE_LOG(LogCombatIncomingDamageResolver, Log,
			TEXT("[CombatDebug] Stage 5 armour: Armour=%.1f pierce=%.1f%% mitigation=%.1f%% physical %.2f -> %.2f"),
			EffectiveArmour, ArmourPiercingPercent, PhysicalMitigation * 100.f,
			IncomingPhysical, Result.PhysicalTaken);
	}

	// Every non-physical type mitigates through its resistance, reduced by
	// piercing and clamped to [-100, per-type max cap].
	const auto MitigateTypedDamage = [&](const EHunterDamageType DamageType) -> float
	{
		const float IncomingDamage = FMath::Max(
			CombatIncomingDamageResolverPrivate::GetPacketDamage(InPacket, DamageType), 0.f);
		if (IncomingDamage <= 0.f)
		{
			return 0.f;
		}

		const float Resistance = GetResistanceValue(DamageType, DefenderAttributes)
			- GetResistancePierceValue(DamageType, AttackerAttributes, DamageInfo);
		const float ClampedResistance = FMath::Clamp(
			Resistance,
			CombatIncomingDamageResolverPrivate::MinResistancePercent,
			GetResistanceCap(DamageType, DefenderAttributes));
		const float MitigatedDamage = FMath::Max(0.f, IncomingDamage * (1.f - (ClampedResistance / 100.f)));

		if (bDebugLog)
		{
			UE_LOG(LogCombatIncomingDamageResolver, Log,
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

	for (const EHunterDamageType DamageType : CombatIncomingDamageResolverPrivate::AllDamageTypes)
	{
		const float DamageAfterBlock = CombatIncomingDamageResolverPrivate::GetResultTakenByType(Result, DamageType);
		const float TakenMultiplier = GetDamageTakenMultiplier(DamageType, DefenderAttributes);

		if (bDebugLog && DamageAfterBlock > 0.f && !FMath::IsNearlyEqual(TakenMultiplier, 1.f))
		{
			UE_LOG(LogCombatIncomingDamageResolver, Log,
				TEXT("[CombatDebug] Stage 7 taken-mult %-10s x%.3f %.2f -> %.2f"),
				*UEnum::GetDisplayValueAsText(DamageType).ToString(),
				TakenMultiplier, DamageAfterBlock, DamageAfterBlock * TakenMultiplier);
		}

		CombatIncomingDamageResolverPrivate::SetResultTakenByType(
			Result, DamageType, FMath::Max(0.f, DamageAfterBlock * TakenMultiplier));
	}

	Result.TotalDamageTaken =
		Result.PhysicalTaken + Result.FireTaken + Result.IceTaken +
		Result.LightningTaken + Result.LightTaken + Result.CorruptionTaken;

	// ArcaneShield absorbs before Health.
	const float CurrentArcaneShield = FMath::Max(DefenderAttributes->GetArcaneShield(), 0.f);
	const float CurrentHealth = FMath::Max(DefenderAttributes->GetHealth(), 0.f);
	Result.DamageToArcaneShield = FMath::Min(CurrentArcaneShield, Result.TotalDamageTaken);
	Result.DamageToHealth = FMath::Min(
		CurrentHealth,
		FMath::Max(0.f, Result.TotalDamageTaken - Result.DamageToArcaneShield));
	Result.TotalDamageApplied = Result.DamageToArcaneShield + Result.DamageToHealth;

	if (bDebugLog)
	{
		if (Result.bWasBlocked)
		{
			UE_LOG(LogCombatIncomingDamageResolver, Log,
				TEXT("[CombatDebug] Stage 6 block: blocked=%.2f afterBlock=%.2f staminaCost=%.2f guardBroken=%s"),
				Result.TotalBlockedAmount, Result.TotalDamageAfterBlock, Result.DamageToStamina,
				Result.bGuardBroken ? TEXT("yes") : TEXT("no"));
		}
		UE_LOG(LogCombatIncomingDamageResolver, Log,
			TEXT("[CombatDebug] Stage 8 routing: total=%.2f -> shield=%.2f health=%.2f"),
			Result.TotalDamageTaken, Result.DamageToArcaneShield, Result.DamageToHealth);
	}

	return Result;
}

float FCombatIncomingDamageResolver::GetResistanceValue(
	const EHunterDamageType DamageType,
	const UHunterAttributeSet* DefenderAttributes)
{
	if (!DefenderAttributes)
	{
		return 0.f;
	}

	const float GlobalDefenses = DefenderAttributes->GetGlobalDefenses();
	const auto ResolveResistance = [GlobalDefenses](const float FlatPoints, const float IncreasedPercent)
	{
		const float BaseResistancePoints = GlobalDefenses + FlatPoints;
		return BaseResistancePoints * (1.f + (IncreasedPercent / 100.f));
	};

	switch (DamageType)
	{
	case EHunterDamageType::Fire:
		return ResolveResistance(
			DefenderAttributes->GetFireResistanceFlatBonus(),
			DefenderAttributes->GetFireResistancePercentBonus());
	case EHunterDamageType::Ice:
		return ResolveResistance(
			DefenderAttributes->GetIceResistanceFlatBonus(),
			DefenderAttributes->GetIceResistancePercentBonus());
	case EHunterDamageType::Lightning:
		return ResolveResistance(
			DefenderAttributes->GetLightningResistanceFlatBonus(),
			DefenderAttributes->GetLightningResistancePercentBonus());
	case EHunterDamageType::Light:
		return ResolveResistance(
			DefenderAttributes->GetLightResistanceFlatBonus(),
			DefenderAttributes->GetLightResistancePercentBonus());
	case EHunterDamageType::Corruption:
		return ResolveResistance(
			DefenderAttributes->GetCorruptionResistanceFlatBonus(),
			DefenderAttributes->GetCorruptionResistancePercentBonus());
	default:
		return 0.f;
	}
}

float FCombatIncomingDamageResolver::GetResistanceCap(
	const EHunterDamageType DamageType,
	const UHunterAttributeSet* DefenderAttributes)
{
	if (!DefenderAttributes)
	{
		return CombatIncomingDamageResolverPrivate::DefaultMaxResistancePercent;
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

	return Cap > 0.f ? Cap : CombatIncomingDamageResolverPrivate::DefaultMaxResistancePercent;
}

float FCombatIncomingDamageResolver::GetResistancePierceValue(
	const EHunterDamageType DamageType,
	const UHunterAttributeSet* AttackerAttributes,
	const FAnimationDamageInfo& DamageInfo)
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

float FCombatIncomingDamageResolver::GetDamageTakenMultiplier(
	const EHunterDamageType DamageType,
	const UHunterAttributeSet* DefenderAttributes)
{
	if (!DefenderAttributes)
	{
		return 1.f;
	}

	float Multiplier = CombatIncomingDamageResolverPrivate::SanitizeMultiplier(
		DefenderAttributes->GetGlobalDamageTakenMultiplier());

	if (CombatIncomingDamageResolverPrivate::IsElementalDamageType(DamageType))
	{
		Multiplier *= CombatIncomingDamageResolverPrivate::SanitizeMultiplier(
			DefenderAttributes->GetElementalDamageTakenMultiplier());
	}

	switch (DamageType)
	{
	case EHunterDamageType::Physical:
		Multiplier *= CombatIncomingDamageResolverPrivate::SanitizeMultiplier(DefenderAttributes->GetPhysicalDamageTakenMultiplier());
		break;
	case EHunterDamageType::Fire:
		Multiplier *= CombatIncomingDamageResolverPrivate::SanitizeMultiplier(DefenderAttributes->GetFireDamageTakenMultiplier());
		break;
	case EHunterDamageType::Ice:
		Multiplier *= CombatIncomingDamageResolverPrivate::SanitizeMultiplier(DefenderAttributes->GetIceDamageTakenMultiplier());
		break;
	case EHunterDamageType::Lightning:
		Multiplier *= CombatIncomingDamageResolverPrivate::SanitizeMultiplier(DefenderAttributes->GetLightningDamageTakenMultiplier());
		break;
	case EHunterDamageType::Light:
		Multiplier *= CombatIncomingDamageResolverPrivate::SanitizeMultiplier(DefenderAttributes->GetLightDamageTakenMultiplier());
		break;
	case EHunterDamageType::Corruption:
		Multiplier *= CombatIncomingDamageResolverPrivate::SanitizeMultiplier(DefenderAttributes->GetCorruptionDamageTakenMultiplier());
		break;
	default:
		break;
	}

	return FMath::Max(0.f, Multiplier);
}

bool FCombatIncomingDamageResolver::IsActorBlocking(AActor* Actor)
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

bool FCombatIncomingDamageResolver::CanBlockHit(
	AActor* AttackerActor,
	AActor* DefenderActor,
	const UHunterAttributeSet* DefenderAttributes)
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
		: CombatIncomingDamageResolverPrivate::DefaultBlockAngleDegrees;
	const float HalfAngleRadians =
		FMath::DegreesToRadians(FMath::Clamp(BlockAngleDegrees, 0.f, 180.f) * 0.5f);
	const float ForwardDot = FVector::DotProduct(
		DefenderActor->GetActorForwardVector().GetSafeNormal(), DirectionToAttacker);

	return ForwardDot >= FMath::Cos(HalfAngleRadians);
}

float FCombatIncomingDamageResolver::GetBlockTypeMultiplier(
	const EHunterDamageType DamageType,
	const UHunterAttributeSet* DefenderAttributes)
{
	if (!DefenderAttributes)
	{
		return 1.f;
	}

	switch (DamageType)
	{
	case EHunterDamageType::Physical:
		return CombatIncomingDamageResolverPrivate::SanitizeMultiplier(DefenderAttributes->GetBlockPhysicalMultiplier());
	case EHunterDamageType::Fire:
	case EHunterDamageType::Ice:
	case EHunterDamageType::Lightning:
	case EHunterDamageType::Light:
		return CombatIncomingDamageResolverPrivate::SanitizeMultiplier(DefenderAttributes->GetBlockElementalMultiplier());
	case EHunterDamageType::Corruption:
		return CombatIncomingDamageResolverPrivate::SanitizeMultiplier(DefenderAttributes->GetBlockCorruptionMultiplier());
	default:
		return 1.f;
	}
}

void FCombatIncomingDamageResolver::ApplyBlockingToMitigatedResult(
	AActor* AttackerActor,
	AActor* DefenderActor,
	const UHunterAttributeSet* DefenderAttributes,
	FCombatResolveResult& InOutResult)
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
	for (const EHunterDamageType DamageType : CombatIncomingDamageResolverPrivate::AllDamageTypes)
	{
		TotalIncomingForBlock += FMath::Max(
			0.f, CombatIncomingDamageResolverPrivate::GetResultTakenByType(InOutResult, DamageType));
	}

	float TotalAfterBlock = 0.f;
	float TotalBlockedAmount = 0.f;

	for (const EHunterDamageType DamageType : CombatIncomingDamageResolverPrivate::AllDamageTypes)
	{
		const float DamageBeforeBlock = FMath::Max(
			0.f, CombatIncomingDamageResolverPrivate::GetResultTakenByType(InOutResult, DamageType));

		// Flat block splits across types proportionally to their contribution.
		const float TypeFlatShare = TotalIncomingForBlock > 0.f
			? FlatBlockAmount * (DamageBeforeBlock / TotalIncomingForBlock)
			: 0.f;

		const float BlockedTotal = (DamageBeforeBlock * BlockStrengthFraction) + TypeFlatShare;
		const float DamageAfterAbsorption = FMath::Max(0.f, DamageBeforeBlock - BlockedTotal);
		// Typed block multipliers are explicitly damage-taken multipliers while
		// guarding: 1 is neutral, below 1 is stronger, above 1 is a weakness.
		const float DamageAfterBlock = DamageAfterAbsorption
			* GetBlockTypeMultiplier(DamageType, DefenderAttributes);

		// Chip damage guarantees a floor of unblockable bleed-through.
		const float ChipFloor = DamageBeforeBlock * ChipDamageFraction;
		const float FinalTypeDamage = FMath::Max(DamageAfterBlock, ChipFloor);
		const float BlockedAmount = FMath::Max(0.f, DamageBeforeBlock - FinalTypeDamage);

		CombatIncomingDamageResolverPrivate::SetResultTakenByType(InOutResult, DamageType, FinalTypeDamage);
		CombatIncomingDamageResolverPrivate::SetResultBlockedByType(InOutResult, DamageType, BlockedAmount);

		TotalAfterBlock += FinalTypeDamage;
		TotalBlockedAmount += BlockedAmount;
	}

	InOutResult.TotalDamageAfterBlock = TotalAfterBlock;
	InOutResult.TotalBlockedAmount = TotalBlockedAmount;
}

void FCombatIncomingDamageResolver::ApplyStaminaBlockCost(
	const UHunterAttributeSet* DefenderAttributes,
	FCombatResolveResult& InOutResult)
{
	if (!DefenderAttributes || !InOutResult.bWasBlocked || InOutResult.TotalBlockedAmount <= 0.f)
	{
		return;
	}

	const float CostMultiplier = CombatIncomingDamageResolverPrivate::SanitizeMultiplier(
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

void FCombatIncomingDamageResolver::EvaluateStagger(
	AActor* DefenderActor,
	const UHunterAttributeSet* DefenderAttributes,
	const FAnimationDamageInfo& DamageInfo,
	FCombatResolveResult& InOutResult)
{
	if (!DefenderAttributes)
	{
		return;
	}

	if (InOutResult.bGuardBroken)
	{
		InOutResult.bShouldStagger = true;
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

	const float RawPoiseDamage = DamageInfo.PoiseDamage > 0.f
		? DamageInfo.PoiseDamage
		: InOutResult.TotalDamageTaken;
	const float PoiseResistancePercent = FMath::Clamp(
		DefenderAttributes->GetPoiseResistance(), 0.f, 95.f);
	InOutResult.EffectivePoiseDamage = RawPoiseDamage * (1.f - (PoiseResistancePercent / 100.f));

	const float PoiseThreshold = FMath::Max(DefenderAttributes->GetPoise(), 0.f);
	InOutResult.bShouldStagger = PoiseThreshold > 0.f
		&& InOutResult.EffectivePoiseDamage >= PoiseThreshold;
}

EHitResponse FCombatIncomingDamageResolver::ResolveDefenderHitResponse(
	AActor* DefenderActor,
	const EHitResponse RequestedResponse,
	bool& bOutOverrodeCaller)
{
	bOutOverrodeCaller = false;

	const UAbilitySystemComponent* DefenderASC = GetAbilitySystemComponentFromActor(DefenderActor);
	if (!DefenderASC)
	{
		// No defender ASC means no defender-owned defensive state exists to trust.
		// Refuse a caller-asserted negation rather than granting it for free.
		if (RequestedResponse == EHitResponse::Parry || RequestedResponse == EHitResponse::Invincible)
		{
			bOutOverrodeCaller = true;
			return EHitResponse::Normal;
		}
		return RequestedResponse;
	}

	const FPHGameplayTags& Tags = FPHGameplayTags::Get();

	// Invincibility outranks everything: i-frames negate a hit that would
	// otherwise have been parried or blocked.
	if (DefenderASC->HasMatchingGameplayTag(Tags.Condition_Self_IsInvincible))
	{
		bOutOverrodeCaller = RequestedResponse != EHitResponse::Invincible;
		return EHitResponse::Invincible;
	}

	if (DefenderASC->HasMatchingGameplayTag(Tags.Condition_Self_IsParrying))
	{
		bOutOverrodeCaller = RequestedResponse != EHitResponse::Parry;
		return EHitResponse::Parry;
	}

	// The defender is neither invincible nor parrying, so the attacking side
	// does not get to claim either outcome.
	if (RequestedResponse == EHitResponse::Parry || RequestedResponse == EHitResponse::Invincible)
	{
		bOutOverrodeCaller = true;
		return EHitResponse::Normal;
	}

	return RequestedResponse;
}

void FCombatIncomingDamageResolver::ApplyHitResponse(
	const EHitResponse HitResponse,
	const bool bCanApplyAilments,
	FCombatResolveResult& InOutResult)
{
	InOutResult.HitResponse = HitResponse;
	InOutResult.bShouldApplyAilments = bCanApplyAilments;

	switch (HitResponse)
	{
	case EHitResponse::Parry:
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
		InOutResult.DamageToHealth = 0.f;
		InOutResult.DamageToArcaneShield = 0.f;
		InOutResult.DamageToStamina = 0.f;
		InOutResult.TotalDamageTaken = 0.f;
		InOutResult.TotalDamageApplied = 0.f;
		InOutResult.EffectivePoiseDamage = 0.f;
		InOutResult.bShouldApplyAilments = false;
		InOutResult.bShouldStagger = false;
		InOutResult.bWasBlocked = false;
		InOutResult.bGuardBroken = false;
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
		InOutResult.TotalDamageApplied = 0.f;
		InOutResult.EffectivePoiseDamage = 0.f;
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

FString FCombatIncomingDamageResolver::FormatResult(const FCombatResolveResult& Result)
{
	return FString::Printf(
		TEXT("Taken=%.1f (Health=%.1f Shield=%.1f Stamina=%.1f) Blocked=%.1f Crit=%s Killed=%s Stagger=%s"),
		Result.TotalDamageTaken, Result.DamageToHealth, Result.DamageToArcaneShield, Result.DamageToStamina,
		Result.TotalBlockedAmount,
		Result.bWasCrit ? TEXT("yes") : TEXT("no"),
		Result.bKilledTarget ? TEXT("yes") : TEXT("no"),
		Result.bShouldStagger ? TEXT("yes") : TEXT("no"));
}
