#include "Combat/Components/CombatManager.h"

#include "AbilitySystem/HunterAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameFramework/Actor.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "PHGameplayTags.h"
#include "Combat/Components/CombatStatusManager.h"
#include "Stats/StatsModifierMath.h"

DEFINE_LOG_CATEGORY(LogCombatManager);

namespace CombatManagerComponentPrivate
{
	constexpr float DefaultCritMultiplier = 1.5f;
	constexpr float MinResistancePercent = -100.f;
	constexpr float DefaultMaxResistancePercent = 90.f;
	constexpr float DefaultBlockAngleDegrees = 120.f;
	constexpr float ArmourHitSizeScale = 10.f;
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

	float GetDamageByType(const FCombatDamagePacket& Packet, const EHunterDamageType DamageType)
	{
		switch (DamageType)
		{
		case EHunterDamageType::Physical:
			return Packet.Physical;
		case EHunterDamageType::Fire:
			return Packet.Fire;
		case EHunterDamageType::Ice:
			return Packet.Ice;
		case EHunterDamageType::Lightning:
			return Packet.Lightning;
		case EHunterDamageType::Light:
			return Packet.Light;
		case EHunterDamageType::Corruption:
			return Packet.Corruption;
		default:
			return 0.f;
		}
	}

	float GetValueByType(const FCombatDamageTypeValues& Values, const EHunterDamageType DamageType)
	{
		switch (DamageType)
		{
		case EHunterDamageType::Physical:
			return Values.Physical;
		case EHunterDamageType::Fire:
			return Values.Fire;
		case EHunterDamageType::Ice:
			return Values.Ice;
		case EHunterDamageType::Lightning:
			return Values.Lightning;
		case EHunterDamageType::Light:
			return Values.Light;
		case EHunterDamageType::Corruption:
			return Values.Corruption;
		default:
			return 0.f;
		}
	}

	float GetMultiplierByType(const FCombatDamageTypeMultipliers& Multipliers, const EHunterDamageType DamageType)
	{
		switch (DamageType)
		{
		case EHunterDamageType::Physical:
			return Multipliers.Physical;
		case EHunterDamageType::Fire:
			return Multipliers.Fire;
		case EHunterDamageType::Ice:
			return Multipliers.Ice;
		case EHunterDamageType::Lightning:
			return Multipliers.Lightning;
		case EHunterDamageType::Light:
			return Multipliers.Light;
		case EHunterDamageType::Corruption:
			return Multipliers.Corruption;
		default:
			return 1.f;
		}
	}

	const FCombatDamageTypeValues& GetTransformValuesBySource(
		const FCombatDamageTransformRules& Rules,
		const EHunterDamageType SourceType)
	{
		switch (SourceType)
		{
		case EHunterDamageType::Physical:
			return Rules.FromPhysical;
		case EHunterDamageType::Fire:
			return Rules.FromFire;
		case EHunterDamageType::Ice:
			return Rules.FromIce;
		case EHunterDamageType::Lightning:
			return Rules.FromLightning;
		case EHunterDamageType::Light:
			return Rules.FromLight;
		case EHunterDamageType::Corruption:
			return Rules.FromCorruption;
		default:
			return Rules.FromPhysical;
		}
	}

	void AddTransformValues(FCombatDamageTypeValues& Out, const FCombatDamageTypeValues& A, const FCombatDamageTypeValues& B)
	{
		Out.Physical = A.Physical + B.Physical;
		Out.Fire = A.Fire + B.Fire;
		Out.Ice = A.Ice + B.Ice;
		Out.Lightning = A.Lightning + B.Lightning;
		Out.Light = A.Light + B.Light;
		Out.Corruption = A.Corruption + B.Corruption;
	}

	void SetDamageByType(FCombatDamagePacket& Packet, const EHunterDamageType DamageType, const float Value)
	{
		switch (DamageType)
		{
		case EHunterDamageType::Physical:
			Packet.Physical = Value;
			break;
		case EHunterDamageType::Fire:
			Packet.Fire = Value;
			break;
		case EHunterDamageType::Ice:
			Packet.Ice = Value;
			break;
		case EHunterDamageType::Lightning:
			Packet.Lightning = Value;
			break;
		case EHunterDamageType::Light:
			Packet.Light = Value;
			break;
		case EHunterDamageType::Corruption:
			Packet.Corruption = Value;
			break;
		default:
			break;
		}
	}

	void AddDamageByType(FCombatDamagePacket& Packet, const EHunterDamageType DamageType, const float Value)
	{
		SetDamageByType(Packet, DamageType, GetDamageByType(Packet, DamageType) + Value);
	}

	float GetTotalDamage(const FCombatDamagePacket& Packet)
	{
		return Packet.Physical + Packet.Fire + Packet.Ice + Packet.Lightning + Packet.Light + Packet.Corruption;
	}

	void UpdatePacketTotal(FCombatDamagePacket& Packet)
	{
		Packet.TotalPreMitigation = GetTotalDamage(Packet);
	}

	bool IsElementalDamageType(const EHunterDamageType DamageType)
	{
		return DamageType == EHunterDamageType::Fire
			|| DamageType == EHunterDamageType::Ice
			|| DamageType == EHunterDamageType::Lightning
			|| DamageType == EHunterDamageType::Light;
	}

	float GetResultTakenByType(const FCombatResolveResult& Result, const EHunterDamageType DamageType)
	{
		switch (DamageType)
		{
		case EHunterDamageType::Physical:
			return Result.PhysicalTaken;
		case EHunterDamageType::Fire:
			return Result.FireTaken;
		case EHunterDamageType::Ice:
			return Result.IceTaken;
		case EHunterDamageType::Lightning:
			return Result.LightningTaken;
		case EHunterDamageType::Light:
			return Result.LightTaken;
		case EHunterDamageType::Corruption:
			return Result.CorruptionTaken;
		default:
			return 0.f;
		}
	}

	void SetResultTakenByType(FCombatResolveResult& Result, const EHunterDamageType DamageType, const float Value)
	{
		switch (DamageType)
		{
		case EHunterDamageType::Physical:
			Result.PhysicalTaken = Value;
			break;
		case EHunterDamageType::Fire:
			Result.FireTaken = Value;
			break;
		case EHunterDamageType::Ice:
			Result.IceTaken = Value;
			break;
		case EHunterDamageType::Lightning:
			Result.LightningTaken = Value;
			break;
		case EHunterDamageType::Light:
			Result.LightTaken = Value;
			break;
		case EHunterDamageType::Corruption:
			Result.CorruptionTaken = Value;
			break;
		default:
			break;
		}
	}

	void SetResultBlockedByType(FCombatResolveResult& Result, const EHunterDamageType DamageType, const float Value)
	{
		switch (DamageType)
		{
		case EHunterDamageType::Physical:
			Result.PhysicalBlocked = Value;
			break;
		case EHunterDamageType::Fire:
			Result.FireBlocked = Value;
			break;
		case EHunterDamageType::Ice:
			Result.IceBlocked = Value;
			break;
		case EHunterDamageType::Lightning:
			Result.LightningBlocked = Value;
			break;
		case EHunterDamageType::Light:
			Result.LightBlocked = Value;
			break;
		case EHunterDamageType::Corruption:
			Result.CorruptionBlocked = Value;
			break;
		default:
			break;
		}
	}

	float GetNeutralMultiplier(const float Value)
	{
		return Value > 0.f ? Value : 1.f;
	}

	FString FormatPacket(const FCombatDamagePacket& Packet)
	{
		return FString::Printf(
			TEXT("Phys=%.2f Fire=%.2f Ice=%.2f Lightning=%.2f Light=%.2f Corruption=%.2f Total=%.2f Crit=%s x%.2f"),
			Packet.Physical,
			Packet.Fire,
			Packet.Ice,
			Packet.Lightning,
			Packet.Light,
			Packet.Corruption,
			Packet.TotalPreMitigation,
			Packet.bCrit ? TEXT("true") : TEXT("false"),
			Packet.CritMultiplierApplied);
	}

	FString FormatResult(const FCombatResolveResult& Result)
	{
		return FString::Printf(
			TEXT("Taken[Phys=%.2f Fire=%.2f Ice=%.2f Lightning=%.2f Light=%.2f Corruption=%.2f Total=%.2f] Block[Applied=%s GuardBroken=%s TotalBefore=%.2f TotalAfter=%.2f Blocked=%.2f Stamina=%.2f] Shield=%.2f Health=%.2f Killed=%s"),
			Result.PhysicalTaken,
			Result.FireTaken,
			Result.IceTaken,
			Result.LightningTaken,
			Result.LightTaken,
			Result.CorruptionTaken,
			Result.TotalDamageTaken,
			Result.bWasBlocked ? TEXT("true") : TEXT("false"),
			Result.bGuardBroken ? TEXT("true") : TEXT("false"),
			Result.TotalDamageBeforeBlock,
			Result.TotalDamageAfterBlock,
			Result.TotalBlockedAmount,
			Result.DamageToStamina,
			Result.DamageToArcaneShield,
			Result.DamageToHealth,
			Result.bKilledTarget ? TEXT("true") : TEXT("false"));
	}

	float ApplyPercentIncrease(const float BaseValue, const float IncreasedPct)
	{
		return BaseValue * (1.f + (IncreasedPct / 100.f));
	}

	float GetDurationOrDefault(const float ConfiguredDuration, const float DefaultDuration)
	{
		return ConfiguredDuration > 0.f ? ConfiguredDuration : DefaultDuration;
	}

	bool RollPercentChance(const float ChancePercent)
	{
		return ChancePercent > 0.f && FMath::FRandRange(0.f, 100.f) < ChancePercent;
	}
}

UCombatManager::UCombatManager()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCombatIncomingHitEditContext::RejectHit()
{
	bApplyHit = false;
}

FCombatHitPacket UCombatManager::MakeCombatHitPacket(
	const FCombatOffensePacket& Offense,
	const FCombatDefensePacket& Defense,
	const FCombatCostPacket& Cost,
	const EHitResponse HitResponse,
	const bool bCanApplyAilments)
{
	FCombatHitPacket Packet;
	Packet.Offense = Offense;
	Packet.Defense = Defense;
	Packet.Cost = Cost;
	Packet.HitResponse = HitResponse;
	Packet.bCanApplyAilments = bCanApplyAilments;
	CombatManagerComponentPrivate::UpdatePacketTotal(Packet.Offense.BaseDamage);
	return Packet;
}

bool UCombatManager::ApplyHit(AActor* AttackerActor, AActor* DefenderActor,
	const FCombatHitPacket& HitPacket, FCombatResolveResult& OutResult)
{
	OutResult = FCombatResolveResult{};

	if (!IsValid(AttackerActor) || !IsValid(DefenderActor))
	{
		UE_LOG(LogCombatManager, Warning, TEXT("ApplyHit failed because attacker or defender was invalid. Attacker=%s Defender=%s"),
			*GetNameSafe(AttackerActor), *GetNameSafe(DefenderActor));
		return false;
	}

	const UHunterAttributeSet* AttackerAttributes = GetHunterAttributeSetFromActor(AttackerActor);
	const UHunterAttributeSet* DefenderAttributes = GetHunterAttributeSetFromActor(DefenderActor);
	if (!DefenderAttributes)
	{
		return false;
	}

	FCombatHitPacket EffectiveHitPacket = HitPacket;
	CombatManagerComponentPrivate::UpdatePacketTotal(EffectiveHitPacket.Offense.BaseDamage);
	if (DefenderActor->HasAuthority() && OnEditIncomingHitPacket.IsBound())
	{
		UCombatIncomingHitEditContext* EditContext = NewObject<UCombatIncomingHitEditContext>(this);
		EditContext->AttackerActor = AttackerActor;
		EditContext->DefenderActor = DefenderActor;
		EditContext->HitPacket = EffectiveHitPacket;

		OnEditIncomingHitPacket.Broadcast(EditContext);

		if (!EditContext->bApplyHit)
		{
			UE_LOG(LogCombatManager, Verbose, TEXT("ApplyHit rejected by incoming packet edit. Attacker=%s Defender=%s"),
				*GetNameSafe(AttackerActor), *GetNameSafe(DefenderActor));
			return false;
		}

		EffectiveHitPacket = EditContext->HitPacket;
		CombatManagerComponentPrivate::UpdatePacketTotal(EffectiveHitPacket.Offense.BaseDamage);
	}

	if ((EffectiveHitPacket.Offense.bAddAttackerAttributeDamage ||
		EffectiveHitPacket.Offense.bApplyAttackerDamageConversion ||
		EffectiveHitPacket.Offense.bApplyAttackerAttributeModifiers ||
		EffectiveHitPacket.Cost.bPayOnApply) && !AttackerAttributes)
	{
		UE_LOG(LogCombatManager, Warning,
			TEXT("ApplyHit needs attacker attributes for this packet but %s has no UHunterAttributeSet."),
			*GetNameSafe(AttackerActor));
		return false;
	}

	UE_LOG(LogCombatManager, Verbose, TEXT("ApplyHit started. Attacker=%s Defender=%s"),
		*GetNameSafe(AttackerActor), *GetNameSafe(DefenderActor));

	FCombatDamagePacket OutgoingPacket = BuildOutgoingDamagePacketFromHitPacket(EffectiveHitPacket, AttackerAttributes, AttackerActor);
	UE_LOG(LogCombatManager, Verbose, TEXT("ApplyHit packet after offense calculation: %s"),
		*CombatManagerComponentPrivate::FormatPacket(OutgoingPacket));

	OutResult = MitigateDamagePacketAgainstAttributes(
		OutgoingPacket,
		AttackerActor,
		DefenderActor,
		AttackerAttributes,
		DefenderAttributes,
		&EffectiveHitPacket);
	OutResult.HitResponse = EffectiveHitPacket.HitResponse;
	OutResult.bShouldApplyAilments = EffectiveHitPacket.bCanApplyAilments;

	if (!DefenderActor->HasAuthority())
	{
		UE_LOG(LogCombatManager, Verbose, TEXT("ApplyHit ran without server authority. Returning preview result only for %s."),
			*GetNameSafe(DefenderActor));
		return true;
	}

	if (EffectiveHitPacket.Cost.bPayOnApply)
	{
		FSkillDamagePacket CostPacket;
		CostPacket.StaminaCost = EffectiveHitPacket.Cost.StaminaCost;
		CostPacket.ManaCost = EffectiveHitPacket.Cost.ManaCost;
		CostPacket.HealthCost = EffectiveHitPacket.Cost.HealthCost;
		ApplySkillCostToSource(AttackerActor, CostPacket);
	}

	EvaluateStagger(DefenderActor, DefenderAttributes, OutResult);
	ApplyHitResponse(EffectiveHitPacket, OutResult);

	UAbilitySystemComponent* AttackerASC = GetAbilitySystemComponentFromActor(AttackerActor);

	ApplyResolvedDamage(AttackerActor, DefenderActor, OutResult);
	OutResult.bKilledTarget = DefenderAttributes->GetHealth() <= 0.f;
	ApplyOnHitEffects(AttackerActor, DefenderActor, OutResult, AttackerASC, AttackerAttributes);

	UE_LOG(LogCombatManager, Verbose, TEXT("ApplyHit completed. Attacker=%s Defender=%s %s"),
		*GetNameSafe(AttackerActor),
		*GetNameSafe(DefenderActor),
		*CombatManagerComponentPrivate::FormatResult(OutResult));

	return true;
}

void UCombatManager::ApplyResolvedDamage(AActor* SourceActor, AActor* TargetActor, const FCombatResolveResult& Result) const
{
	if (!IsValid(TargetActor))
	{
		UE_LOG(LogCombatManager, Warning, TEXT("ApplyResolvedDamage failed because TargetActor was invalid. Source=%s"), *GetNameSafe(SourceActor));
		return;
	}

	if (!TargetActor->HasAuthority())
	{
		UE_LOG(LogCombatManager, Verbose, TEXT("ApplyResolvedDamage skipped on non-authority target %s."), *GetNameSafe(TargetActor));
		return;
	}

	if (Result.TotalDamageTaken <= 0.f && Result.DamageToStamina <= 0.f)
	{
		UE_LOG(LogCombatManager, Verbose, TEXT("ApplyResolvedDamage skipped because both health/shield damage and stamina loss were zero. Source=%s Target=%s"),
			*GetNameSafe(SourceActor), *GetNameSafe(TargetActor));
		return;
	}

	UAbilitySystemComponent* TargetASC = GetAbilitySystemComponentFromActor(TargetActor);
	const UHunterAttributeSet* TargetAttributes = GetHunterAttributeSetFromActor(TargetActor);
	if (!TargetASC || !TargetAttributes)
	{
		return;
	}

	// B-1 FIX: Route damage through GAS (PreAttributeChange/PostAttributeChange) so the
	// AttributeSet's clamping logic fires. If DamageApplicationGE is configured use the
	// proper GE path; otherwise fall back to direct mutation with a loud warning.
	if (DamageApplicationGE)
	{
		FGameplayEffectContextHandle Context = TargetASC->MakeEffectContext();
		Context.AddSourceObject(SourceActor ? SourceActor : GetOwner());

		FGameplayEffectSpecHandle Spec = TargetASC->MakeOutgoingSpec(DamageApplicationGE, 1.f, Context);
		if (Spec.IsValid())
		{
			const FPHGameplayTags& Tags = FPHGameplayTags::Get();

			// Negate: the GE modifier is Additive, so a negative magnitude subtracts.
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

			TargetASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());

			UE_LOG(LogCombatManager, Verbose,
				TEXT("Applied resolved damage via GE. Source=%s Target=%s Health-=%.2f ArcaneShield-=%.2f Stamina-=%.2f"),
				*GetNameSafe(SourceActor), *GetNameSafe(TargetActor),
				Result.DamageToHealth, Result.DamageToArcaneShield, Result.DamageToStamina);
		}
		else
		{
			UE_LOG(LogCombatManager, Error,
				TEXT("ApplyResolvedDamage: MakeOutgoingSpec failed for DamageApplicationGE on %s."),
				*GetNameSafe(this));
		}
	}
	else
	{
		// B-1 FALLBACK (configure DamageApplicationGE to fix this):
		// SetNumericAttributeBase bypasses PreAttributeChange clamping in the AttributeSet.
		UE_LOG(LogCombatManager, Warning,
			TEXT("ApplyResolvedDamage: DamageApplicationGE is not set on %s. Falling back to "
				 "SetNumericAttributeBase which bypasses GAS clamping. "
				 "Please configure DamageApplicationGE in Blueprint defaults."),
			*GetNameSafe(this));

		const float CurrentStamina     = FMath::Max(TargetAttributes->GetStamina(), 0.f);
		const float CurrentArcaneShield = FMath::Max(TargetAttributes->GetArcaneShield(), 0.f);
		const float CurrentHealth       = FMath::Max(TargetAttributes->GetHealth(), 0.f);

		const float NewStamina      = FMath::Max(0.f, CurrentStamina      - FMath::Max(Result.DamageToStamina, 0.f));
		const float NewArcaneShield = FMath::Max(0.f, CurrentArcaneShield - FMath::Max(Result.DamageToArcaneShield, 0.f));
		const float NewHealth       = FMath::Max(0.f, CurrentHealth       - FMath::Max(Result.DamageToHealth, 0.f));

		if (!FMath::IsNearlyEqual(CurrentStamina, NewStamina))
		{
			TargetASC->SetNumericAttributeBase(UHunterAttributeSet::GetStaminaAttribute(), NewStamina);
		}
		if (!FMath::IsNearlyEqual(CurrentArcaneShield, NewArcaneShield))
		{
			TargetASC->SetNumericAttributeBase(UHunterAttributeSet::GetArcaneShieldAttribute(), NewArcaneShield);
		}
		if (!FMath::IsNearlyEqual(CurrentHealth, NewHealth))
		{
			TargetASC->SetNumericAttributeBase(UHunterAttributeSet::GetHealthAttribute(), NewHealth);
		}

		UE_LOG(LogCombatManager, Verbose,
			TEXT("Applied resolved damage (direct). Source=%s Target=%s Stamina %.2f->%.2f ArcaneShield %.2f->%.2f Health %.2f->%.2f"),
			*GetNameSafe(SourceActor), *GetNameSafe(TargetActor),
			CurrentStamina, NewStamina,
			CurrentArcaneShield, NewArcaneShield,
			CurrentHealth, NewHealth);
	}
}

UAbilitySystemComponent* UCombatManager::GetAbilitySystemComponentFromActor(const AActor* Actor)
{
	if (!IsValid(Actor))
	{
		UE_LOG(LogCombatManager, Warning, TEXT("GetAbilitySystemComponentFromActor called with an invalid actor."));
		return nullptr;
	}

	if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Actor))
	{
		return ASC;
	}

	if (UAbilitySystemComponent* ASC = Actor->FindComponentByClass<UAbilitySystemComponent>())
	{
		return ASC;
	}

	UE_LOG(LogCombatManager, Warning, TEXT("No AbilitySystemComponent found on actor %s."), *GetNameSafe(Actor));
	return nullptr;
}

const UHunterAttributeSet* UCombatManager::GetHunterAttributeSetFromActor(const AActor* Actor)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActor(Actor);
	if (!ASC)
	{
		return nullptr;
	}

	const UHunterAttributeSet* AttributeSet = ASC->GetSet<UHunterAttributeSet>();
	if (!AttributeSet)
	{
		UE_LOG(LogCombatManager, Warning, TEXT("Actor %s has an ASC but no registered UHunterAttributeSet."), *GetNameSafe(Actor));
	}

	return AttributeSet;
}

FCombatDamagePacket UCombatManager::BuildOutgoingDamagePacketFromAttributes(
	const UHunterAttributeSet* SourceAttributes,
	AActor* SourceActor,
	AActor* TargetActor,
	const FSkillDamagePacket* SkillPacket) const
{
	FCombatDamagePacket BasePacket;

	if (!SourceAttributes)
	{
		return BasePacket;
	}

	BasePacket.Physical   = CalculateBaseDamageForType(EHunterDamageType::Physical,   SourceAttributes, SkillPacket);
	BasePacket.Fire       = CalculateBaseDamageForType(EHunterDamageType::Fire,        SourceAttributes, SkillPacket);
	BasePacket.Ice        = CalculateBaseDamageForType(EHunterDamageType::Ice,         SourceAttributes, SkillPacket);
	BasePacket.Lightning  = CalculateBaseDamageForType(EHunterDamageType::Lightning,   SourceAttributes, SkillPacket);
	BasePacket.Light      = CalculateBaseDamageForType(EHunterDamageType::Light,       SourceAttributes, SkillPacket);
	BasePacket.Corruption = CalculateBaseDamageForType(EHunterDamageType::Corruption,  SourceAttributes, SkillPacket);
	CombatManagerComponentPrivate::UpdatePacketTotal(BasePacket);

	FCombatDamagePacket ConvertedBasePacket = ApplyDamageConversionFromAttributes(BasePacket, SourceAttributes, SourceActor);

	FCombatDamagePacket Packet;
	Packet.Physical = CalculateScaledDamageForType(
		EHunterDamageType::Physical,
		CombatManagerComponentPrivate::GetDamageByType(ConvertedBasePacket, EHunterDamageType::Physical),
		SourceAttributes,
		SkillPacket);
	Packet.Fire = CalculateScaledDamageForType(
		EHunterDamageType::Fire,
		CombatManagerComponentPrivate::GetDamageByType(ConvertedBasePacket, EHunterDamageType::Fire),
		SourceAttributes,
		SkillPacket);
	Packet.Ice = CalculateScaledDamageForType(
		EHunterDamageType::Ice,
		CombatManagerComponentPrivate::GetDamageByType(ConvertedBasePacket, EHunterDamageType::Ice),
		SourceAttributes,
		SkillPacket);
	Packet.Lightning = CalculateScaledDamageForType(
		EHunterDamageType::Lightning,
		CombatManagerComponentPrivate::GetDamageByType(ConvertedBasePacket, EHunterDamageType::Lightning),
		SourceAttributes,
		SkillPacket);
	Packet.Light = CalculateScaledDamageForType(
		EHunterDamageType::Light,
		CombatManagerComponentPrivate::GetDamageByType(ConvertedBasePacket, EHunterDamageType::Light),
		SourceAttributes,
		SkillPacket);
	Packet.Corruption = CalculateScaledDamageForType(
		EHunterDamageType::Corruption,
		CombatManagerComponentPrivate::GetDamageByType(ConvertedBasePacket, EHunterDamageType::Corruption),
		SourceAttributes,
		SkillPacket);
	CombatManagerComponentPrivate::UpdatePacketTotal(Packet);

	// Respect the skill's crit flag. If no skill packet, crit is always eligible.
	const bool bCanCrit = (SkillPacket == nullptr) || SkillPacket->bCanCrit;
	ResolveCriticalStrike(Packet, SourceAttributes, bCanCrit, SkillPacket);

	UE_LOG(LogCombatManager, Verbose, TEXT("Built outgoing packet. Source=%s Target=%s %s"),
		*GetNameSafe(SourceActor),
		*GetNameSafe(TargetActor),
		*CombatManagerComponentPrivate::FormatPacket(Packet));

	return Packet;
}

FCombatDamagePacket UCombatManager::BuildOutgoingDamagePacketFromHitPacket(
	const FCombatHitPacket& HitPacket,
	const UHunterAttributeSet* AttackerAttributes,
	AActor* AttackerActor) const
{
	FCombatDamagePacket Packet = HitPacket.Offense.BaseDamage;
	CombatManagerComponentPrivate::UpdatePacketTotal(Packet);

	if (HitPacket.Offense.bAddAttackerAttributeDamage && AttackerAttributes)
	{
		FSkillDamagePacket AttributeDamagePacket;
		AttributeDamagePacket.WeaponDamageEffectiveness = FMath::Max(HitPacket.Offense.WeaponDamageEffectiveness, 0.f);
		AttributeDamagePacket.bLayerCharacterStats = true;

		CombatManagerComponentPrivate::AddDamageByType(Packet, EHunterDamageType::Physical,
			CalculateBaseDamageForType(EHunterDamageType::Physical, AttackerAttributes, &AttributeDamagePacket));
		CombatManagerComponentPrivate::AddDamageByType(Packet, EHunterDamageType::Fire,
			CalculateBaseDamageForType(EHunterDamageType::Fire, AttackerAttributes, &AttributeDamagePacket));
		CombatManagerComponentPrivate::AddDamageByType(Packet, EHunterDamageType::Ice,
			CalculateBaseDamageForType(EHunterDamageType::Ice, AttackerAttributes, &AttributeDamagePacket));
		CombatManagerComponentPrivate::AddDamageByType(Packet, EHunterDamageType::Lightning,
			CalculateBaseDamageForType(EHunterDamageType::Lightning, AttackerAttributes, &AttributeDamagePacket));
		CombatManagerComponentPrivate::AddDamageByType(Packet, EHunterDamageType::Light,
			CalculateBaseDamageForType(EHunterDamageType::Light, AttackerAttributes, &AttributeDamagePacket));
		CombatManagerComponentPrivate::AddDamageByType(Packet, EHunterDamageType::Corruption,
			CalculateBaseDamageForType(EHunterDamageType::Corruption, AttackerAttributes, &AttributeDamagePacket));
		CombatManagerComponentPrivate::UpdatePacketTotal(Packet);
	}

	FCombatDamageTransformRules ConversionRules;
	if (HitPacket.Offense.bApplyAttackerDamageConversion && AttackerAttributes)
	{
		ConversionRules = BuildDamageConversionRulesFromAttributes(AttackerAttributes);
	}
	if (HitPacket.Offense.bApplyPacketDamageConversion)
	{
		ConversionRules = CombineDamageTransformRules(ConversionRules, HitPacket.Offense.DamageConversionPercent);
	}

	const FCombatDamageTransformRules GainAsExtraRules = HitPacket.Offense.bApplyPacketGainAsExtra
		? HitPacket.Offense.GainAsExtraPercent
		: FCombatDamageTransformRules{};
	Packet = ApplyDamageTransformRules(Packet, ConversionRules, GainAsExtraRules);

	FCombatDamagePacket ScaledPacket;
	ScaledPacket.Physical = CalculateScaledDamageForHitPacketType(
		EHunterDamageType::Physical,
		CombatManagerComponentPrivate::GetDamageByType(Packet, EHunterDamageType::Physical),
		AttackerAttributes,
		HitPacket);
	ScaledPacket.Fire = CalculateScaledDamageForHitPacketType(
		EHunterDamageType::Fire,
		CombatManagerComponentPrivate::GetDamageByType(Packet, EHunterDamageType::Fire),
		AttackerAttributes,
		HitPacket);
	ScaledPacket.Ice = CalculateScaledDamageForHitPacketType(
		EHunterDamageType::Ice,
		CombatManagerComponentPrivate::GetDamageByType(Packet, EHunterDamageType::Ice),
		AttackerAttributes,
		HitPacket);
	ScaledPacket.Lightning = CalculateScaledDamageForHitPacketType(
		EHunterDamageType::Lightning,
		CombatManagerComponentPrivate::GetDamageByType(Packet, EHunterDamageType::Lightning),
		AttackerAttributes,
		HitPacket);
	ScaledPacket.Light = CalculateScaledDamageForHitPacketType(
		EHunterDamageType::Light,
		CombatManagerComponentPrivate::GetDamageByType(Packet, EHunterDamageType::Light),
		AttackerAttributes,
		HitPacket);
	ScaledPacket.Corruption = CalculateScaledDamageForHitPacketType(
		EHunterDamageType::Corruption,
		CombatManagerComponentPrivate::GetDamageByType(Packet, EHunterDamageType::Corruption),
		AttackerAttributes,
		HitPacket);
	CombatManagerComponentPrivate::UpdatePacketTotal(ScaledPacket);

	ResolveHitPacketCriticalStrike(ScaledPacket, AttackerAttributes, HitPacket);
	return ScaledPacket;
}

FCombatDamagePacket UCombatManager::ApplyDamageConversionFromAttributes(const FCombatDamagePacket& InPacket, const UHunterAttributeSet* SourceAttributes, AActor* SourceActor) const
{
	if (!SourceAttributes)
	{
		return InPacket;
	}

	const FCombatDamagePacket Packet = ApplyDamageTransformRules(
		InPacket,
		BuildDamageConversionRulesFromAttributes(SourceAttributes),
		FCombatDamageTransformRules{});

	UE_LOG(LogCombatManager, Verbose, TEXT("Applied conversion for %s. %s"), *GetNameSafe(SourceActor), *CombatManagerComponentPrivate::FormatPacket(Packet));

	return Packet;
}

FCombatDamageTransformRules UCombatManager::BuildDamageConversionRulesFromAttributes(const UHunterAttributeSet* SourceAttributes) const
{
	FCombatDamageTransformRules Rules;
	if (!SourceAttributes)
	{
		return Rules;
	}

	Rules.FromPhysical.Fire = SourceAttributes->GetPhysicalToFire();
	Rules.FromPhysical.Ice = SourceAttributes->GetPhysicalToIce();
	Rules.FromPhysical.Lightning = SourceAttributes->GetPhysicalToLightning();
	Rules.FromPhysical.Light = SourceAttributes->GetPhysicalToLight();
	Rules.FromPhysical.Corruption = SourceAttributes->GetPhysicalToCorruption();

	Rules.FromFire.Physical = SourceAttributes->GetFireToPhysical();
	Rules.FromFire.Ice = SourceAttributes->GetFireToIce();
	Rules.FromFire.Lightning = SourceAttributes->GetFireToLightning();
	Rules.FromFire.Light = SourceAttributes->GetFireToLight();
	Rules.FromFire.Corruption = SourceAttributes->GetFireToCorruption();

	Rules.FromIce.Physical = SourceAttributes->GetIceToPhysical();
	Rules.FromIce.Fire = SourceAttributes->GetIceToFire();
	Rules.FromIce.Lightning = SourceAttributes->GetIceToLightning();
	Rules.FromIce.Light = SourceAttributes->GetIceToLight();
	Rules.FromIce.Corruption = SourceAttributes->GetIceToCorruption();

	Rules.FromLightning.Physical = SourceAttributes->GetLightningToPhysical();
	Rules.FromLightning.Fire = SourceAttributes->GetLightningToFire();
	Rules.FromLightning.Ice = SourceAttributes->GetLightningToIce();
	Rules.FromLightning.Light = SourceAttributes->GetLightningToLight();
	Rules.FromLightning.Corruption = SourceAttributes->GetLightningToCorruption();

	Rules.FromLight.Physical = SourceAttributes->GetLightToPhysical();
	Rules.FromLight.Fire = SourceAttributes->GetLightToFire();
	Rules.FromLight.Ice = SourceAttributes->GetLightToIce();
	Rules.FromLight.Lightning = SourceAttributes->GetLightToLightning();
	Rules.FromLight.Corruption = SourceAttributes->GetLightToCorruption();

	Rules.FromCorruption.Physical = SourceAttributes->GetCorruptionToPhysical();
	Rules.FromCorruption.Fire = SourceAttributes->GetCorruptionToFire();
	Rules.FromCorruption.Ice = SourceAttributes->GetCorruptionToIce();
	Rules.FromCorruption.Lightning = SourceAttributes->GetCorruptionToLightning();
	Rules.FromCorruption.Light = SourceAttributes->GetCorruptionToLight();

	return Rules;
}

FCombatDamageTransformRules UCombatManager::CombineDamageTransformRules(
	const FCombatDamageTransformRules& A,
	const FCombatDamageTransformRules& B) const
{
	FCombatDamageTransformRules Result;
	CombatManagerComponentPrivate::AddTransformValues(Result.FromPhysical, A.FromPhysical, B.FromPhysical);
	CombatManagerComponentPrivate::AddTransformValues(Result.FromFire, A.FromFire, B.FromFire);
	CombatManagerComponentPrivate::AddTransformValues(Result.FromIce, A.FromIce, B.FromIce);
	CombatManagerComponentPrivate::AddTransformValues(Result.FromLightning, A.FromLightning, B.FromLightning);
	CombatManagerComponentPrivate::AddTransformValues(Result.FromLight, A.FromLight, B.FromLight);
	CombatManagerComponentPrivate::AddTransformValues(Result.FromCorruption, A.FromCorruption, B.FromCorruption);
	return Result;
}

FCombatDamagePacket UCombatManager::ApplyDamageTransformRules(
	const FCombatDamagePacket& InPacket,
	const FCombatDamageTransformRules& ConversionRules,
	const FCombatDamageTransformRules& GainAsExtraRules) const
{
	FCombatDamagePacket OutPacket;

	const auto ApplySource = [&](const EHunterDamageType SourceType)
	{
		const float SourceDamage = FMath::Max(CombatManagerComponentPrivate::GetDamageByType(InPacket, SourceType), 0.f);
		if (SourceDamage <= 0.f)
		{
			return;
		}

		const FCombatDamageTypeValues& ConversionValues =
			CombatManagerComponentPrivate::GetTransformValuesBySource(ConversionRules, SourceType);
		const FCombatDamageTypeValues& GainValues =
			CombatManagerComponentPrivate::GetTransformValuesBySource(GainAsExtraRules, SourceType);

		float RequestedConversionPct = 0.f;
		for (const EHunterDamageType DestinationType : {
			EHunterDamageType::Physical,
			EHunterDamageType::Fire,
			EHunterDamageType::Ice,
			EHunterDamageType::Lightning,
			EHunterDamageType::Light,
			EHunterDamageType::Corruption })
		{
			if (DestinationType != SourceType)
			{
				RequestedConversionPct += FMath::Max(
					CombatManagerComponentPrivate::GetValueByType(ConversionValues, DestinationType),
					0.f);
			}
		}

		const float ConversionScale = RequestedConversionPct > 100.f ? (100.f / RequestedConversionPct) : 1.f;
		float ConvertedAmount = 0.f;

		for (const EHunterDamageType DestinationType : {
			EHunterDamageType::Physical,
			EHunterDamageType::Fire,
			EHunterDamageType::Ice,
			EHunterDamageType::Lightning,
			EHunterDamageType::Light,
			EHunterDamageType::Corruption })
		{
			const float GainPct = FMath::Max(
				CombatManagerComponentPrivate::GetValueByType(GainValues, DestinationType),
				0.f);
			if (GainPct > 0.f)
			{
				CombatManagerComponentPrivate::AddDamageByType(OutPacket, DestinationType, SourceDamage * (GainPct / 100.f));
			}

			if (DestinationType == SourceType)
			{
				continue;
			}

			const float ConversionPct = FMath::Max(
				CombatManagerComponentPrivate::GetValueByType(ConversionValues, DestinationType),
				0.f) * ConversionScale;
			if (ConversionPct <= 0.f)
			{
				continue;
			}

			const float DestinationAmount = SourceDamage * (ConversionPct / 100.f);
			CombatManagerComponentPrivate::AddDamageByType(OutPacket, DestinationType, DestinationAmount);
			ConvertedAmount += DestinationAmount;
		}

		CombatManagerComponentPrivate::AddDamageByType(
			OutPacket,
			SourceType,
			FMath::Max(0.f, SourceDamage - ConvertedAmount));
	};

	ApplySource(EHunterDamageType::Physical);
	ApplySource(EHunterDamageType::Fire);
	ApplySource(EHunterDamageType::Ice);
	ApplySource(EHunterDamageType::Lightning);
	ApplySource(EHunterDamageType::Light);
	ApplySource(EHunterDamageType::Corruption);

	CombatManagerComponentPrivate::UpdatePacketTotal(OutPacket);
	return OutPacket;
}

FCombatResolveResult UCombatManager::MitigateDamagePacketAgainstAttributes(
	const FCombatDamagePacket& InPacket,
	AActor* SourceActor,
	AActor* TargetActor,
	const UHunterAttributeSet* SourceAttributes,
	const UHunterAttributeSet* TargetAttributes,
	const FCombatHitPacket* HitPacket) const
{
	FCombatResolveResult Result;
	Result.PreMitigationPacket = InPacket;
	Result.bWasCrit = InPacket.bCrit;

	if (!TargetAttributes)
	{
		return Result;
	}

	float EffectiveArmour = FMath::Max(
		(TargetAttributes->GetArmour() + TargetAttributes->GetArmourFlatBonus()) * (1.f + (TargetAttributes->GetArmourPercentBonus() / 100.f)),
		0.f);
	if (HitPacket)
	{
		EffectiveArmour = HitPacket->Defense.bOverrideArmour
			? FMath::Max(HitPacket->Defense.ArmourOverride, 0.f)
			: EffectiveArmour;
		EffectiveArmour = FMath::Max(
			0.f,
			(EffectiveArmour + HitPacket->Defense.ArmourFlatBonus) *
			(1.f + (HitPacket->Defense.ArmourPercentBonus / 100.f)));
	}
	const float IncomingPhysical = FMath::Max(InPacket.Physical, 0.f);
	float ArmourPiercingPct = SourceAttributes
		? FMath::Clamp(SourceAttributes->GetArmourPiercing(), 0.f, 100.f)
		: 0.f;
	if (HitPacket)
	{
		ArmourPiercingPct += HitPacket->Offense.ArmourPiercingPercent;
	}
	ArmourPiercingPct = FMath::Clamp(ArmourPiercingPct, 0.f, 100.f);
	const float EffectiveArmourAfterPierce = EffectiveArmour * (1.f - (ArmourPiercingPct / 100.f));
	const float PhysicalMitigationPct = IncomingPhysical > 0.f
		? FMath::Clamp(
			EffectiveArmourAfterPierce / (EffectiveArmourAfterPierce + (IncomingPhysical * CombatManagerComponentPrivate::ArmourHitSizeScale)),
			0.f,
			0.9f)
		: 0.f;

	const float PhysicalAfterArmour = IncomingPhysical * (1.f - PhysicalMitigationPct);
	Result.PhysicalTaken = IncomingPhysical > 0.f
		? FMath::Min(IncomingPhysical, FMath::Max(1.f, PhysicalAfterArmour))
		: 0.f;

	const auto CalculateMitigatedTypedDamage = [&](const EHunterDamageType DamageType) -> float
	{
		const float IncomingDamage = FMath::Max(CombatManagerComponentPrivate::GetDamageByType(InPacket, DamageType), 0.f);
		if (IncomingDamage <= 0.f)
		{
			return 0.f;
		}

		float DefenderResistance = GetResistanceValue(DamageType, TargetAttributes);
		float PacketPierce = 0.f;
		if (HitPacket)
		{
			DefenderResistance = HitPacket->Defense.bOverrideResistances
				? CombatManagerComponentPrivate::GetValueByType(HitPacket->Defense.ResistanceOverride, DamageType)
				: DefenderResistance;
			DefenderResistance += CombatManagerComponentPrivate::GetValueByType(HitPacket->Defense.ResistanceBonus, DamageType);
			PacketPierce = CombatManagerComponentPrivate::GetValueByType(HitPacket->Offense.ResistancePiercingPercent, DamageType);
		}
		const float EffectiveResistance = DefenderResistance - GetPierceValue(DamageType, SourceAttributes) - PacketPierce;
		const float ResistanceCap = GetResistanceCap(DamageType, TargetAttributes);
		const float ClampedResistance = FMath::Clamp(EffectiveResistance, CombatManagerComponentPrivate::MinResistancePercent, ResistanceCap);
		return FMath::Max(0.f, IncomingDamage * (1.f - (ClampedResistance / 100.f)));
	};

	Result.FireTaken = CalculateMitigatedTypedDamage(EHunterDamageType::Fire);
	Result.IceTaken = CalculateMitigatedTypedDamage(EHunterDamageType::Ice);
	Result.LightningTaken = CalculateMitigatedTypedDamage(EHunterDamageType::Lightning);
	Result.LightTaken = CalculateMitigatedTypedDamage(EHunterDamageType::Light);
	Result.CorruptionTaken = CalculateMitigatedTypedDamage(EHunterDamageType::Corruption);

	Result.TotalDamageBeforeBlock =
		Result.PhysicalTaken +
		Result.FireTaken +
		Result.IceTaken +
		Result.LightningTaken +
		Result.LightTaken +
		Result.CorruptionTaken;
	Result.TotalDamageAfterBlock = Result.TotalDamageBeforeBlock;

	if (!HitPacket || !HitPacket->Defense.bIgnoreBlock)
	{
		ApplyBlockingToMitigatedResult(SourceActor, TargetActor, TargetAttributes, Result);
		ApplyStaminaBlockCost(TargetAttributes, Result);
	}

	for (const EHunterDamageType DamageType : {
		EHunterDamageType::Physical,
		EHunterDamageType::Fire,
		EHunterDamageType::Ice,
		EHunterDamageType::Lightning,
		EHunterDamageType::Light,
		EHunterDamageType::Corruption })
	{
		const float DamageAfterBlock = CombatManagerComponentPrivate::GetResultTakenByType(Result, DamageType);
		float DamageTakenMultiplier = GetDamageTakenMultiplier(DamageType, TargetAttributes);
		if (HitPacket)
		{
			DamageTakenMultiplier *= CombatManagerComponentPrivate::GetNeutralMultiplier(
				CombatManagerComponentPrivate::GetMultiplierByType(HitPacket->Defense.DamageTakenMultiplier, DamageType));
		}
		CombatManagerComponentPrivate::SetResultTakenByType(
			Result,
			DamageType,
			FMath::Max(0.f, DamageAfterBlock * DamageTakenMultiplier));
	}

	Result.TotalDamageTaken =
		Result.PhysicalTaken +
		Result.FireTaken +
		Result.IceTaken +
		Result.LightningTaken +
		Result.LightTaken +
		Result.CorruptionTaken;

	const float CurrentArcaneShield = FMath::Max(TargetAttributes->GetArcaneShield(), 0.f);
	const float CurrentHealth = FMath::Max(TargetAttributes->GetHealth(), 0.f);
	Result.DamageToArcaneShield = FMath::Min(CurrentArcaneShield, Result.TotalDamageTaken);
	Result.DamageToHealth = FMath::Max(0.f, Result.TotalDamageTaken - Result.DamageToArcaneShield);
	Result.bKilledTarget = (CurrentHealth - Result.DamageToHealth) <= 0.f;

	return Result;
}

float UCombatManager::RollDamageRange(const float MinDamage, const float MaxDamage)
{
	if (MinDamage <= 0.f && MaxDamage <= 0.f)
	{
		return 0.f;
	}

	const float SafeMin = FMath::Max(MinDamage, 0.f);
	const float SafeMax = FMath::Max(MaxDamage, SafeMin);
	return FMath::FRandRange(SafeMin, SafeMax);
}

float UCombatManager::CalculateBaseDamageForType(
	const EHunterDamageType DamageType,
	const UHunterAttributeSet* SourceAttributes,
	const FSkillDamagePacket* SkillPacket) const
{
	if (!SourceAttributes)
	{
		return 0.f;
	}

	// ── Character attribute weapon ranges ─────────────────────────────────────
	float WeaponMin = 0.f;
	float WeaponMax = 0.f;
	float FlatDamage = 0.f;

	switch (DamageType)
	{
	case EHunterDamageType::Physical:
		WeaponMin = SourceAttributes->GetMinPhysicalDamage();
		WeaponMax = SourceAttributes->GetMaxPhysicalDamage();
		FlatDamage = SourceAttributes->GetPhysicalFlatDamage();
		break;
	case EHunterDamageType::Fire:
		WeaponMin = SourceAttributes->GetMinFireDamage();
		WeaponMax = SourceAttributes->GetMaxFireDamage();
		FlatDamage = SourceAttributes->GetFireFlatDamage();
		break;
	case EHunterDamageType::Ice:
		WeaponMin = SourceAttributes->GetMinIceDamage();
		WeaponMax = SourceAttributes->GetMaxIceDamage();
		FlatDamage = SourceAttributes->GetIceFlatDamage();
		break;
	case EHunterDamageType::Lightning:
		WeaponMin = SourceAttributes->GetMinLightningDamage();
		WeaponMax = SourceAttributes->GetMaxLightningDamage();
		FlatDamage = SourceAttributes->GetLightningFlatDamage();
		break;
	case EHunterDamageType::Light:
		WeaponMin = SourceAttributes->GetMinLightDamage();
		WeaponMax = SourceAttributes->GetMaxLightDamage();
		FlatDamage = SourceAttributes->GetLightFlatDamage();
		break;
	case EHunterDamageType::Corruption:
		WeaponMin = SourceAttributes->GetMinCorruptionDamage();
		WeaponMax = SourceAttributes->GetMaxCorruptionDamage();
		FlatDamage = SourceAttributes->GetCorruptionFlatDamage();
		break;
	default:
		return 0.f;
	}

	// ── Base damage roll ──────────────────────────────────────────────────────
	// PoE2 model:
	//   WeaponRoll  = attribute weapon range × WeaponDamageEffectiveness
	//   SkillRoll   = skill's own min/max range (always 0 if no SkillPacket)
	//   FlatBonus   = character flat damage modifiers (always applied)
	//   Base        = WeaponRoll + SkillRoll + FlatBonus

	float SkillMin = 0.f;
	float SkillMax = 0.f;
	float WeaponEffectiveness = 1.f;

	if (SkillPacket)
	{
		// bLayerCharacterStats = false → pure skill damage, ignore weapon stats entirely.
		WeaponEffectiveness = SkillPacket->bLayerCharacterStats ? SkillPacket->WeaponDamageEffectiveness : 0.f;

		// Per-type skill base ranges and PoE2 multiplier fields
		switch (DamageType)
		{
		case EHunterDamageType::Physical:
			SkillMin = SkillPacket->MinPhysical;
			SkillMax = SkillPacket->MaxPhysical;
			break;
		case EHunterDamageType::Fire:
			SkillMin = SkillPacket->MinFire;
			SkillMax = SkillPacket->MaxFire;
			break;
		case EHunterDamageType::Ice:
			SkillMin = SkillPacket->MinIce;
			SkillMax = SkillPacket->MaxIce;
			break;
		case EHunterDamageType::Lightning:
			SkillMin = SkillPacket->MinLightning;
			SkillMax = SkillPacket->MaxLightning;
			break;
		case EHunterDamageType::Light:
			SkillMin = SkillPacket->MinLight;
			SkillMax = SkillPacket->MaxLight;
			break;
		case EHunterDamageType::Corruption:
			SkillMin = SkillPacket->MinCorruption;
			SkillMax = SkillPacket->MaxCorruption;
			break;
		default:
			break;
		}
	}

	const float WeaponRoll = RollDamageRange(WeaponMin, WeaponMax) * WeaponEffectiveness;
	const float SkillRoll  = RollDamageRange(SkillMin,  SkillMax);
	const float BaseDamage = FMath::Max(0.f, WeaponRoll + SkillRoll + FlatDamage);

	return BaseDamage;
}

float UCombatManager::CalculateScaledDamageForType(
	const EHunterDamageType DamageType,
	const float BaseDamage,
	const UHunterAttributeSet* SourceAttributes,
	const FSkillDamagePacket* SkillPacket) const
{
	if (!SourceAttributes || BaseDamage <= 0.f)
	{
		return 0.f;
	}

	float TypeIncreasePct = 0.f;
	float SkillIncreasedPct = 0.f;
	float SkillMore = 1.f;

	switch (DamageType)
	{
	case EHunterDamageType::Physical:
		TypeIncreasePct = SourceAttributes->GetPhysicalPercentDamage();
		if (SkillPacket)
		{
			SkillIncreasedPct = SkillPacket->PhysicalIncreasedPercent;
			SkillMore = SkillPacket->PhysicalMore;
		}
		break;
	case EHunterDamageType::Fire:
		TypeIncreasePct = SourceAttributes->GetFirePercentDamage();
		if (SkillPacket)
		{
			SkillIncreasedPct = SkillPacket->FireIncreasedPercent;
			SkillMore = SkillPacket->FireMore;
		}
		break;
	case EHunterDamageType::Ice:
		TypeIncreasePct = SourceAttributes->GetIcePercentDamage();
		if (SkillPacket)
		{
			SkillIncreasedPct = SkillPacket->IceIncreasedPercent;
			SkillMore = SkillPacket->IceMore;
		}
		break;
	case EHunterDamageType::Lightning:
		TypeIncreasePct = SourceAttributes->GetLightningPercentDamage();
		if (SkillPacket)
		{
			SkillIncreasedPct = SkillPacket->LightningIncreasedPercent;
			SkillMore = SkillPacket->LightningMore;
		}
		break;
	case EHunterDamageType::Light:
		TypeIncreasePct = SourceAttributes->GetLightPercentDamage();
		if (SkillPacket)
		{
			SkillIncreasedPct = SkillPacket->LightIncreasedPercent;
			SkillMore = SkillPacket->LightMore;
		}
		break;
	case EHunterDamageType::Corruption:
		TypeIncreasePct = SourceAttributes->GetCorruptionPercentDamage();
		if (SkillPacket)
		{
			SkillIncreasedPct = SkillPacket->CorruptionIncreasedPercent;
			SkillMore = SkillPacket->CorruptionMore;
		}
		break;
	default:
		return 0.f;
	}

	float TotalIncreasedPct = SourceAttributes->GetGlobalDamages() + TypeIncreasePct + SkillIncreasedPct;
	if (CombatManagerComponentPrivate::IsElementalDamageType(DamageType))
	{
		TotalIncreasedPct += SourceAttributes->GetElementalDamage();
	}

	if (SkillPacket)
	{
		if (SkillPacket->bIsMelee)
		{
			TotalIncreasedPct += SourceAttributes->GetMeleeDamage();
		}
		if (SkillPacket->bIsRanged)
		{
			TotalIncreasedPct += SourceAttributes->GetRangedDamage();
		}
		if (SkillPacket->bIsSpell)
		{
			TotalIncreasedPct += SourceAttributes->GetSpellDamage();
		}
		if (SkillPacket->bIsArea)
		{
			TotalIncreasedPct += SourceAttributes->GetAreaDamage();
		}
		if (SkillPacket->bIsDamageOverTime)
		{
			TotalIncreasedPct += SourceAttributes->GetDamageOverTime();
		}
		if (SkillPacket->bIsChainHit)
		{
			TotalIncreasedPct += SourceAttributes->GetChainDamage();
		}
	}

	const float MaxEffectiveHealth = FMath::Max(SourceAttributes->GetMaxEffectiveHealth(), SourceAttributes->GetMaxHealth());
	if (MaxEffectiveHealth > 0.f)
	{
		const float HealthPercent = SourceAttributes->GetHealth() / MaxEffectiveHealth;
		if (HealthPercent >= 0.999f)
		{
			TotalIncreasedPct += SourceAttributes->GetDamageBonusWhileAtFullHP();
		}
		else if (HealthPercent <= 0.35f)
		{
			TotalIncreasedPct += SourceAttributes->GetDamageBonusWhileAtLowHP();
		}
	}

	const float DamageAfterIncreased = FMath::Max(
		0.f,
		CombatManagerComponentPrivate::ApplyPercentIncrease(BaseDamage, TotalIncreasedPct));
	const float CharMore = GetMoreDamageMultiplier(DamageType, SourceAttributes);
	const float TotalMore = FMath::Max(0.f, CharMore * SkillMore);

	return FMath::Max(0.f, DamageAfterIncreased * TotalMore);
}

float UCombatManager::CalculateScaledDamageForHitPacketType(
	const EHunterDamageType DamageType,
	const float BaseDamage,
	const UHunterAttributeSet* SourceAttributes,
	const FCombatHitPacket& HitPacket) const
{
	if (BaseDamage <= 0.f)
	{
		return 0.f;
	}

	float TotalIncreasedPct =
		HitPacket.Offense.GlobalIncreasedPercent +
		CombatManagerComponentPrivate::GetValueByType(HitPacket.Offense.TypeIncreasedPercent, DamageType);

	if (CombatManagerComponentPrivate::IsElementalDamageType(DamageType))
	{
		TotalIncreasedPct += HitPacket.Offense.ElementalIncreasedPercent;
	}

	if (HitPacket.Offense.bApplyAttackerAttributeModifiers && SourceAttributes)
	{
		TotalIncreasedPct += SourceAttributes->GetGlobalDamages();

		switch (DamageType)
		{
		case EHunterDamageType::Physical:
			TotalIncreasedPct += SourceAttributes->GetPhysicalPercentDamage();
			break;
		case EHunterDamageType::Fire:
			TotalIncreasedPct += SourceAttributes->GetFirePercentDamage();
			break;
		case EHunterDamageType::Ice:
			TotalIncreasedPct += SourceAttributes->GetIcePercentDamage();
			break;
		case EHunterDamageType::Lightning:
			TotalIncreasedPct += SourceAttributes->GetLightningPercentDamage();
			break;
		case EHunterDamageType::Light:
			TotalIncreasedPct += SourceAttributes->GetLightPercentDamage();
			break;
		case EHunterDamageType::Corruption:
			TotalIncreasedPct += SourceAttributes->GetCorruptionPercentDamage();
			break;
		default:
			break;
		}

		if (CombatManagerComponentPrivate::IsElementalDamageType(DamageType))
		{
			TotalIncreasedPct += SourceAttributes->GetElementalDamage();
		}
		if (HitPacket.Offense.bIsMelee)
		{
			TotalIncreasedPct += SourceAttributes->GetMeleeDamage();
		}
		if (HitPacket.Offense.bIsRanged)
		{
			TotalIncreasedPct += SourceAttributes->GetRangedDamage();
		}
		if (HitPacket.Offense.bIsSpell)
		{
			TotalIncreasedPct += SourceAttributes->GetSpellDamage();
		}
		if (HitPacket.Offense.bIsArea)
		{
			TotalIncreasedPct += SourceAttributes->GetAreaDamage();
		}
		if (HitPacket.Offense.bIsDamageOverTime)
		{
			TotalIncreasedPct += SourceAttributes->GetDamageOverTime();
		}
		if (HitPacket.Offense.bIsChainHit)
		{
			TotalIncreasedPct += SourceAttributes->GetChainDamage();
		}

		const float MaxEffectiveHealth = FMath::Max(SourceAttributes->GetMaxEffectiveHealth(), SourceAttributes->GetMaxHealth());
		if (MaxEffectiveHealth > 0.f)
		{
			const float HealthPercent = SourceAttributes->GetHealth() / MaxEffectiveHealth;
			if (HealthPercent >= 0.999f)
			{
				TotalIncreasedPct += SourceAttributes->GetDamageBonusWhileAtFullHP();
			}
			else if (HealthPercent <= 0.35f)
			{
				TotalIncreasedPct += SourceAttributes->GetDamageBonusWhileAtLowHP();
			}
		}
	}

	const float DamageAfterIncreased = FMath::Max(
		0.f,
		CombatManagerComponentPrivate::ApplyPercentIncrease(BaseDamage, TotalIncreasedPct));

	float TotalMore = CombatManagerComponentPrivate::GetNeutralMultiplier(HitPacket.Offense.GlobalMoreMultiplier);
	TotalMore *= CombatManagerComponentPrivate::GetNeutralMultiplier(
		CombatManagerComponentPrivate::GetMultiplierByType(HitPacket.Offense.TypeMoreMultiplier, DamageType));
	if (CombatManagerComponentPrivate::IsElementalDamageType(DamageType))
	{
		TotalMore *= CombatManagerComponentPrivate::GetNeutralMultiplier(HitPacket.Offense.ElementalMoreMultiplier);
	}
	if (HitPacket.Offense.bApplyAttackerAttributeModifiers && SourceAttributes)
	{
		TotalMore *= GetMoreDamageMultiplier(DamageType, SourceAttributes);
	}

	return FMath::Max(0.f, DamageAfterIncreased * FMath::Max(0.f, TotalMore));
}

float UCombatManager::GetResistanceValue(const EHunterDamageType DamageType, const UHunterAttributeSet* TargetAttributes)
{
	if (!TargetAttributes)
	{
		return 0.f;
	}

	const float GlobalDefenses = TargetAttributes->GetGlobalDefenses();

	switch (DamageType)
	{
	case EHunterDamageType::Fire:
		return GlobalDefenses + TargetAttributes->GetFireResistanceFlatBonus() + TargetAttributes->GetFireResistancePercentBonus();
	case EHunterDamageType::Ice:
		return GlobalDefenses + TargetAttributes->GetIceResistanceFlatBonus() + TargetAttributes->GetIceResistancePercentBonus();
	case EHunterDamageType::Lightning:
		return GlobalDefenses + TargetAttributes->GetLightningResistanceFlatBonus() + TargetAttributes->GetLightningResistancePercentBonus();
	case EHunterDamageType::Light:
		return GlobalDefenses + TargetAttributes->GetLightResistanceFlatBonus() + TargetAttributes->GetLightResistancePercentBonus();
	case EHunterDamageType::Corruption:
		return GlobalDefenses + TargetAttributes->GetCorruptionResistanceFlatBonus() + TargetAttributes->GetCorruptionResistancePercentBonus();
	default:
		return 0.f;
	}
}

float UCombatManager::GetResistanceCap(const EHunterDamageType DamageType, const UHunterAttributeSet* TargetAttributes) const
{
	if (!TargetAttributes)
	{
		return CombatManagerComponentPrivate::DefaultMaxResistancePercent;
	}

	float MaxResistance = CombatManagerComponentPrivate::DefaultMaxResistancePercent;

	switch (DamageType)
	{
	case EHunterDamageType::Fire:
		MaxResistance = TargetAttributes->GetMaxFireResistance();
		break;
	case EHunterDamageType::Ice:
		MaxResistance = TargetAttributes->GetMaxIceResistance();
		break;
	case EHunterDamageType::Lightning:
		MaxResistance = TargetAttributes->GetMaxLightningResistance();
		break;
	case EHunterDamageType::Light:
		MaxResistance = TargetAttributes->GetMaxLightResistance();
		break;
	case EHunterDamageType::Corruption:
		MaxResistance = TargetAttributes->GetMaxCorruptionResistance();
		break;
	default:
		break;
	}

	return MaxResistance > 0.f ? MaxResistance : CombatManagerComponentPrivate::DefaultMaxResistancePercent;
}

float UCombatManager::GetPierceValue(const EHunterDamageType DamageType, const UHunterAttributeSet* SourceAttributes)
{
	if (!SourceAttributes)
	{
		return 0.f;
	}

	switch (DamageType)
	{
	case EHunterDamageType::Fire:
		return FMath::Max(SourceAttributes->GetFirePiercing(), 0.f);
	case EHunterDamageType::Ice:
		return FMath::Max(SourceAttributes->GetIcePiercing(), 0.f);
	case EHunterDamageType::Lightning:
		return FMath::Max(SourceAttributes->GetLightningPiercing(), 0.f);
	case EHunterDamageType::Light:
		return FMath::Max(SourceAttributes->GetLightPiercing(), 0.f);
	case EHunterDamageType::Corruption:
		return FMath::Max(SourceAttributes->GetCorruptionPiercing(), 0.f);
	default:
		return 0.f;
	}
}

bool UCombatManager::IsActorBlocking(AActor* Actor) const
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActor(Actor);
	if (!ASC)
	{
		return false;
	}

	const FPHGameplayTags& GameplayTags = FPHGameplayTags::Get();
	return ASC->HasMatchingGameplayTag(GameplayTags.Condition_Self_IsBlocking)
		|| ASC->HasMatchingGameplayTag(GameplayTags.Condition_Target_IsBlocking);
}

bool UCombatManager::CanBlockHit(AActor* SourceActor, AActor* TargetActor, const UHunterAttributeSet* TargetAttributes) const
{
	if (!IsValid(TargetActor) || !TargetAttributes || !IsActorBlocking(TargetActor))
	{
		return false;
	}

	if (!IsValid(SourceActor))
	{
		return true;
	}
	const FVector DirectionToSource = (SourceActor->GetActorLocation() - TargetActor->GetActorLocation()).GetSafeNormal();
	if (DirectionToSource.IsNearlyZero())
	{
		return true;
	}

	const float BlockAngleDegrees = TargetAttributes->GetBlockAngle() > 0.f
		? TargetAttributes->GetBlockAngle()
		: CombatManagerComponentPrivate::DefaultBlockAngleDegrees;
	const float HalfAngleRadians = FMath::DegreesToRadians(FMath::Clamp(BlockAngleDegrees, 0.f, 180.f) * 0.5f);
	const float RequiredDot = FMath::Cos(HalfAngleRadians);
	const float ForwardDot = FVector::DotProduct(TargetActor->GetActorForwardVector().GetSafeNormal(), DirectionToSource);

	return ForwardDot >= RequiredDot;
}

float UCombatManager::GetBlockTypeMultiplier(const EHunterDamageType DamageType, const UHunterAttributeSet* TargetAttributes) const
{
	if (!TargetAttributes)
	{
		return 1.f;
	}

	switch (DamageType)
	{
	case EHunterDamageType::Physical:
		return CombatManagerComponentPrivate::GetNeutralMultiplier(TargetAttributes->GetBlockPhysicalMultiplier());
	case EHunterDamageType::Fire:
	case EHunterDamageType::Ice:
	case EHunterDamageType::Lightning:
	case EHunterDamageType::Light:
		return CombatManagerComponentPrivate::GetNeutralMultiplier(TargetAttributes->GetBlockElementalMultiplier());
	case EHunterDamageType::Corruption:
		return CombatManagerComponentPrivate::GetNeutralMultiplier(TargetAttributes->GetBlockCorruptionMultiplier());
	default:
		return 1.f;
	}
}

float UCombatManager::GetMoreDamageMultiplier(const EHunterDamageType DamageType, const UHunterAttributeSet* SourceAttributes) const
{
	if (!SourceAttributes)
	{
		return 1.f;
	}

	float Multiplier = CombatManagerComponentPrivate::GetNeutralMultiplier(SourceAttributes->GetGlobalMoreDamage());

	switch (DamageType)
	{
	case EHunterDamageType::Physical:
		Multiplier *= CombatManagerComponentPrivate::GetNeutralMultiplier(SourceAttributes->GetPhysicalMoreDamage());
		break;
	case EHunterDamageType::Fire:
		Multiplier *= CombatManagerComponentPrivate::GetNeutralMultiplier(SourceAttributes->GetElementalMoreDamage());
		Multiplier *= CombatManagerComponentPrivate::GetNeutralMultiplier(SourceAttributes->GetFireMoreDamage());
		break;
	case EHunterDamageType::Ice:
		Multiplier *= CombatManagerComponentPrivate::GetNeutralMultiplier(SourceAttributes->GetElementalMoreDamage());
		Multiplier *= CombatManagerComponentPrivate::GetNeutralMultiplier(SourceAttributes->GetIceMoreDamage());
		break;
	case EHunterDamageType::Lightning:
		Multiplier *= CombatManagerComponentPrivate::GetNeutralMultiplier(SourceAttributes->GetElementalMoreDamage());
		Multiplier *= CombatManagerComponentPrivate::GetNeutralMultiplier(SourceAttributes->GetLightningMoreDamage());
		break;
	case EHunterDamageType::Light:
		Multiplier *= CombatManagerComponentPrivate::GetNeutralMultiplier(SourceAttributes->GetElementalMoreDamage());
		Multiplier *= CombatManagerComponentPrivate::GetNeutralMultiplier(SourceAttributes->GetLightMoreDamage());
		break;
	case EHunterDamageType::Corruption:
		Multiplier *= CombatManagerComponentPrivate::GetNeutralMultiplier(SourceAttributes->GetCorruptionMoreDamage());
		break;
	default:
		break;
	}

	return FMath::Max(0.f, Multiplier);
}

float UCombatManager::GetDamageTakenMultiplier(const EHunterDamageType DamageType, const UHunterAttributeSet* TargetAttributes) const
{
	if (!TargetAttributes)
	{
		return 1.f;
	}

	float Multiplier = CombatManagerComponentPrivate::GetNeutralMultiplier(TargetAttributes->GetGlobalDamageTakenMultiplier());

	switch (DamageType)
	{
	case EHunterDamageType::Physical:
		Multiplier *= CombatManagerComponentPrivate::GetNeutralMultiplier(TargetAttributes->GetPhysicalDamageTakenMultiplier());
		break;
	case EHunterDamageType::Fire:
		Multiplier *= CombatManagerComponentPrivate::GetNeutralMultiplier(TargetAttributes->GetElementalDamageTakenMultiplier());
		Multiplier *= CombatManagerComponentPrivate::GetNeutralMultiplier(TargetAttributes->GetFireDamageTakenMultiplier());
		break;
	case EHunterDamageType::Ice:
		Multiplier *= CombatManagerComponentPrivate::GetNeutralMultiplier(TargetAttributes->GetElementalDamageTakenMultiplier());
		Multiplier *= CombatManagerComponentPrivate::GetNeutralMultiplier(TargetAttributes->GetIceDamageTakenMultiplier());
		break;
	case EHunterDamageType::Lightning:
		Multiplier *= CombatManagerComponentPrivate::GetNeutralMultiplier(TargetAttributes->GetElementalDamageTakenMultiplier());
		Multiplier *= CombatManagerComponentPrivate::GetNeutralMultiplier(TargetAttributes->GetLightningDamageTakenMultiplier());
		break;
	case EHunterDamageType::Light:
		Multiplier *= CombatManagerComponentPrivate::GetNeutralMultiplier(TargetAttributes->GetElementalDamageTakenMultiplier());
		Multiplier *= CombatManagerComponentPrivate::GetNeutralMultiplier(TargetAttributes->GetLightDamageTakenMultiplier());
		break;
	case EHunterDamageType::Corruption:
		Multiplier *= CombatManagerComponentPrivate::GetNeutralMultiplier(TargetAttributes->GetCorruptionDamageTakenMultiplier());
		break;
	default:
		break;
	}

	return FMath::Max(0.f, Multiplier);
}

void UCombatManager::ApplyBlockingToMitigatedResult(AActor* SourceActor, AActor* TargetActor, const UHunterAttributeSet* TargetAttributes, FCombatResolveResult& InOutResult) const
{
	InOutResult.TotalDamageAfterBlock = InOutResult.TotalDamageBeforeBlock;

	if (!CanBlockHit(SourceActor, TargetActor, TargetAttributes))
	{
		return;
	}

	const float BlockStrengthPct = FMath::Clamp(TargetAttributes ? (TargetAttributes->GetBlockStrength() / 100.f) : 0.f, 0.f, 0.95f);
	const float FlatBlockAmount = TargetAttributes ? FMath::Max(TargetAttributes->GetFlatBlockAmount(), 0.f) : 0.f;
	const float ChipDamagePct = FMath::Clamp(TargetAttributes ? (TargetAttributes->GetChipDamageWhileBlocking() / 100.f) : 0.f, 0.f, 1.f);

	InOutResult.bWasBlocked = true;

	float TotalAfterBlock = 0.f;
	float TotalBlockedAmount = 0.f;

	for (const EHunterDamageType DamageType : {
		EHunterDamageType::Physical,
		EHunterDamageType::Fire,
		EHunterDamageType::Ice,
		EHunterDamageType::Lightning,
		EHunterDamageType::Light,
		EHunterDamageType::Corruption })
	{
		const float DamageBeforeBlock = FMath::Max(0.f, CombatManagerComponentPrivate::GetResultTakenByType(InOutResult, DamageType));
		const float BlockedInput = DamageBeforeBlock * GetBlockTypeMultiplier(DamageType, TargetAttributes);
		const float PercentBlocked = BlockedInput * BlockStrengthPct;
		const float BlockedTotal = PercentBlocked + FlatBlockAmount;
		const float DamageAfterBlock = FMath::Max(0.f, BlockedInput - BlockedTotal);
		const float ChipFloor = BlockedInput * ChipDamagePct;
		const float FinalBlockedTypeDamage = FMath::Max(DamageAfterBlock, ChipFloor);
		const float BlockedAmount = FMath::Max(0.f, DamageBeforeBlock - FinalBlockedTypeDamage);

		CombatManagerComponentPrivate::SetResultTakenByType(InOutResult, DamageType, FinalBlockedTypeDamage);
		CombatManagerComponentPrivate::SetResultBlockedByType(InOutResult, DamageType, BlockedAmount);

		TotalAfterBlock += FinalBlockedTypeDamage;
		TotalBlockedAmount += BlockedAmount;
	}

	InOutResult.TotalDamageAfterBlock = TotalAfterBlock;
	InOutResult.TotalBlockedAmount = TotalBlockedAmount;

	UE_LOG(LogCombatManager, Verbose, TEXT("Blocking applied. Source=%s Target=%s BlockedAmount=%.2f PostBlockDamage=%.2f"),
		*GetNameSafe(SourceActor),
		*GetNameSafe(TargetActor),
		InOutResult.TotalBlockedAmount,
		InOutResult.TotalDamageAfterBlock);
}

void UCombatManager::ApplyStaminaBlockCost(const UHunterAttributeSet* TargetAttributes, FCombatResolveResult& InOutResult) const
{
	if (!TargetAttributes || !InOutResult.bWasBlocked || InOutResult.TotalBlockedAmount <= 0.f)
	{
		return;
	}

	const float BlockStaminaCostMultiplier = CombatManagerComponentPrivate::GetNeutralMultiplier(TargetAttributes->GetBlockStaminaCostMultiplier());
	const float RequestedStaminaCost = FMath::Max(0.f, InOutResult.TotalBlockedAmount * BlockStaminaCostMultiplier);
	const float CurrentStamina = FMath::Max(TargetAttributes->GetStamina(), 0.f);
	const float ActualStaminaCost = FMath::Min(CurrentStamina, RequestedStaminaCost);
	const float RemainingStamina = FMath::Max(0.f, CurrentStamina - ActualStaminaCost);
	const float GuardBreakThreshold = FMath::Max(TargetAttributes->GetGuardBreakThreshold(), 0.f);

	InOutResult.DamageToStamina = ActualStaminaCost;
	InOutResult.bGuardBroken = (GuardBreakThreshold > 0.f && InOutResult.TotalBlockedAmount >= GuardBreakThreshold)
		|| (CurrentStamina > 0.f && ActualStaminaCost > 0.f && RemainingStamina <= 0.f);

	UE_LOG(LogCombatManager, Verbose, TEXT("Block stamina cost resolved. Requested=%.2f Applied=%.2f Remaining=%.2f GuardBroken=%s"),
		RequestedStaminaCost,
		ActualStaminaCost,
		RemainingStamina,
		InOutResult.bGuardBroken ? TEXT("true") : TEXT("false"));
}

void UCombatManager::ResolveCriticalStrike(FCombatDamagePacket& Packet,
	const UHunterAttributeSet* SourceAttributes, const bool bCanCrit, const FSkillDamagePacket* SkillPacket) const
{
	Packet.bCrit = false;
	Packet.CritMultiplierApplied = 1.f;

	if (!bCanCrit)
	{
		UE_LOG(LogCombatManager, Verbose, TEXT("ResolveCriticalStrike: crit disabled by skill packet."));
		CombatManagerComponentPrivate::UpdatePacketTotal(Packet);
		return;
	}

	if (!SourceAttributes)
	{
		CombatManagerComponentPrivate::UpdatePacketTotal(Packet);
		return;
	}

	float CritChance = SourceAttributes->GetCritChance();
	if (SkillPacket && SkillPacket->bIsSpell)
	{
		CritChance += SourceAttributes->GetSpellsCritChance();
	}
	CritChance = FMath::Clamp(CritChance, 0.f, 100.f);
	if (CritChance <= 0.f)
	{
		CombatManagerComponentPrivate::UpdatePacketTotal(Packet);
		return;
	}

	const float CritRoll = FMath::FRandRange(0.f, 100.f);
	if (CritRoll >= CritChance)
	{
		UE_LOG(LogCombatManager, Verbose, TEXT("Critical strike failed. Roll=%.2f Chance=%.2f"), CritRoll, CritChance);
		CombatManagerComponentPrivate::UpdatePacketTotal(Packet);
		return;
	}

	float CritMultiplier = SourceAttributes->GetCritMultiplier() > 0.f
		? SourceAttributes->GetCritMultiplier()
		: CombatManagerComponentPrivate::DefaultCritMultiplier;
	if (SkillPacket && SkillPacket->bIsSpell)
	{
		const float SpellCritMultiplier = CombatManagerComponentPrivate::GetNeutralMultiplier(SourceAttributes->GetSpellsCritMultiplier());
		CritMultiplier += FMath::Max(0.f, SpellCritMultiplier - 1.f);
	}

	Packet.Physical *= CritMultiplier;
	Packet.Fire *= CritMultiplier;
	Packet.Ice *= CritMultiplier;
	Packet.Lightning *= CritMultiplier;
	Packet.Light *= CritMultiplier;
	Packet.Corruption *= CritMultiplier;
	Packet.bCrit = true;
	Packet.CritMultiplierApplied = CritMultiplier;

	CombatManagerComponentPrivate::UpdatePacketTotal(Packet);
	UE_LOG(LogCombatManager, Verbose, TEXT("Critical strike succeeded. Roll=%.2f Chance=%.2f Multiplier=%.2f"), CritRoll, CritChance, CritMultiplier);
}

void UCombatManager::ResolveHitPacketCriticalStrike(FCombatDamagePacket& Packet,
	const UHunterAttributeSet* AttackerAttributes,
	const FCombatHitPacket& HitPacket) const
{
	Packet.bCrit = false;
	Packet.CritMultiplierApplied = 1.f;

	if (!HitPacket.Offense.bCanCrit)
	{
		CombatManagerComponentPrivate::UpdatePacketTotal(Packet);
		return;
	}

	float CritChance = HitPacket.Offense.CritChanceBonus;
	if (HitPacket.Offense.bApplyAttackerAttributeModifiers && AttackerAttributes)
	{
		CritChance += AttackerAttributes->GetCritChance();
		if (HitPacket.Offense.bIsSpell)
		{
			CritChance += AttackerAttributes->GetSpellsCritChance();
		}
	}
	CritChance = FMath::Clamp(CritChance, 0.f, 100.f);

	if (!HitPacket.Offense.bForceCrit && (CritChance <= 0.f || FMath::FRandRange(0.f, 100.f) >= CritChance))
	{
		CombatManagerComponentPrivate::UpdatePacketTotal(Packet);
		return;
	}

	float CritMultiplier = HitPacket.Offense.CritMultiplierOverride > 0.f
		? HitPacket.Offense.CritMultiplierOverride
		: CombatManagerComponentPrivate::DefaultCritMultiplier;
	if (HitPacket.Offense.bApplyAttackerAttributeModifiers && AttackerAttributes)
	{
		CritMultiplier = HitPacket.Offense.CritMultiplierOverride > 0.f
			? HitPacket.Offense.CritMultiplierOverride
			: (AttackerAttributes->GetCritMultiplier() > 0.f
				? AttackerAttributes->GetCritMultiplier()
				: CombatManagerComponentPrivate::DefaultCritMultiplier);
		if (HitPacket.Offense.bIsSpell)
		{
			const float SpellCritMultiplier = CombatManagerComponentPrivate::GetNeutralMultiplier(AttackerAttributes->GetSpellsCritMultiplier());
			CritMultiplier += FMath::Max(0.f, SpellCritMultiplier - 1.f);
		}
	}
	CritMultiplier += FMath::Max(0.f, HitPacket.Offense.CritMultiplierBonus);
	CritMultiplier = FMath::Max(CritMultiplier, 0.f);

	Packet.Physical *= CritMultiplier;
	Packet.Fire *= CritMultiplier;
	Packet.Ice *= CritMultiplier;
	Packet.Lightning *= CritMultiplier;
	Packet.Light *= CritMultiplier;
	Packet.Corruption *= CritMultiplier;
	Packet.bCrit = true;
	Packet.CritMultiplierApplied = CritMultiplier;

	CombatManagerComponentPrivate::UpdatePacketTotal(Packet);
}

void UCombatManager::EvaluateStagger(AActor* TargetActor, const UHunterAttributeSet* TargetAttributes,
	FCombatResolveResult& InOutResult) const
{
	// Stagger fires when a hit drains stamina to zero and the target is NOT mid-skill.
	// Skills are protected by the State_Self_ExecutingSkill tag — if it's present, no stagger.
	if (!TargetAttributes || InOutResult.DamageToStamina <= 0.f)
	{
		return;
	}

	const float StaminaAfterHit = FMath::Max(0.f, TargetAttributes->GetStamina() - InOutResult.DamageToStamina);
	if (StaminaAfterHit > 0.f)
	{
		return; // Stamina not depleted — no stagger
	}

	UAbilitySystemComponent* TargetASC = GetAbilitySystemComponentFromActor(TargetActor);
	if (!TargetASC)
	{
		return;
	}

	const FPHGameplayTags& Tags = FPHGameplayTags::Get();
	if (TargetASC->HasMatchingGameplayTag(Tags.State_Self_ExecutingSkill))
	{
		UE_LOG(LogCombatManager, Verbose,
			TEXT("EvaluateStagger: stamina depleted but %s is executing a skill — stagger suppressed."),
			*GetNameSafe(TargetActor));
		return;
	}

	InOutResult.bShouldStagger = true;
	UE_LOG(LogCombatManager, Verbose,
		TEXT("EvaluateStagger: stagger triggered on %s (stamina %.2f → 0)."),
		*GetNameSafe(TargetActor), TargetAttributes->GetStamina());
}

void UCombatManager::ApplyHitResponse(const FCombatHitPacket& HitPacket, FCombatResolveResult& InOutResult) const
{
	InOutResult.HitResponse = HitPacket.HitResponse;
	InOutResult.bShouldApplyAilments = HitPacket.bCanApplyAilments;

	switch (InOutResult.HitResponse)
	{
	case EHitResponse::Parry:
		InOutResult.DamageToHealth      = 0.f;
		InOutResult.DamageToArcaneShield = 0.f;
		InOutResult.DamageToStamina     = 0.f;
		InOutResult.TotalDamageTaken    = 0.f;
		InOutResult.bShouldApplyAilments = true;
		InOutResult.bShouldStagger       = false;
		InOutResult.bKilledTarget        = false;
		UE_LOG(LogCombatManager, Verbose, TEXT("ApplyHitResponse: Parry zeroed damage and kept flat ailment rolls enabled."));
		break;

	case EHitResponse::Invincible:
		InOutResult.DamageToHealth       = 0.f;
		InOutResult.DamageToArcaneShield  = 0.f;
		InOutResult.DamageToStamina      = 0.f;
		InOutResult.TotalDamageTaken     = 0.f;
		InOutResult.bShouldApplyAilments  = false;
		InOutResult.bShouldStagger        = false;
		InOutResult.bKilledTarget         = false;
		UE_LOG(LogCombatManager, Verbose, TEXT("ApplyHitResponse: Invincible zeroed damage and ailments."));
		break;

	case EHitResponse::Absorbed:
		UE_LOG(LogCombatManager, Verbose, TEXT("ApplyHitResponse: Absorbed response left packet damage as authored."));
		break;

	case EHitResponse::Normal:
	default:
		break;
	}

	return;
}

void UCombatManager::ApplySkillCostToSource(AActor* SourceActor, const FSkillDamagePacket& SkillPacket) const
{
	const UHunterAttributeSet* SourceAttributes = GetHunterAttributeSetFromActor(SourceActor);
	const float StaminaCost = SourceAttributes
		? FStatsModifierMath::ApplyPercentChange(SkillPacket.StaminaCost, SourceAttributes->GetStaminaCostChanges())
		: FMath::Max(SkillPacket.StaminaCost, 0.f);
	const float ManaCost = SourceAttributes
		? FStatsModifierMath::ApplyPercentChange(SkillPacket.ManaCost, SourceAttributes->GetManaCostChanges())
		: FMath::Max(SkillPacket.ManaCost, 0.f);
	const float HealthCost = SourceAttributes
		? FStatsModifierMath::ApplyPercentChange(SkillPacket.HealthCost, SourceAttributes->GetHealthCostChanges())
		: FMath::Max(SkillPacket.HealthCost, 0.f);

	if (StaminaCost <= 0.f && ManaCost <= 0.f && HealthCost <= 0.f)
	{
		return;
	}

	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActor(SourceActor);
	if (!SourceASC)
	{
		return;
	}

	if (CostApplicationGE)
	{
		FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
		Context.AddSourceObject(SourceActor);
		FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(CostApplicationGE, 1.f, Context);
		if (Spec.IsValid())
		{
			const FPHGameplayTags& Tags = FPHGameplayTags::Get();
			if (HealthCost > 0.f)
			{
				Spec.Data->SetSetByCallerMagnitude(Tags.Data_Cost_Health, -HealthCost);
			}
			if (StaminaCost > 0.f)
			{
				Spec.Data->SetSetByCallerMagnitude(Tags.Data_Cost_Stamina, -StaminaCost);
			}
			if (ManaCost > 0.f)
			{
				Spec.Data->SetSetByCallerMagnitude(Tags.Data_Cost_Mana, -ManaCost);
			}

			SourceASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());

			UE_LOG(LogCombatManager, Verbose,
				TEXT("ApplySkillCostToSource: paid costs from %s via CostApplicationGE. Health-=%.2f Mana-=%.2f Stamina-=%.2f"),
				*GetNameSafe(SourceActor), HealthCost, ManaCost, StaminaCost);
		}
		else
		{
			UE_LOG(LogCombatManager, Error,
				TEXT("ApplySkillCostToSource: MakeOutgoingSpec failed for CostApplicationGE on %s."),
				*GetNameSafe(this));
		}

		return;
	}

	if (!SourceAttributes)
	{
		return;
	}

	UE_LOG(LogCombatManager, Warning,
		TEXT("ApplySkillCostToSource: CostApplicationGE is not set on %s. Falling back to direct cost mutation."),
		*GetNameSafe(this));

	const float CurrentHealth = FMath::Max(SourceAttributes->GetHealth(), 0.f);
	const float CurrentMana = FMath::Max(SourceAttributes->GetMana(), 0.f);
	const float CurrentStamina = FMath::Max(SourceAttributes->GetStamina(), 0.f);

	const float NewHealth = FMath::Max(0.f, CurrentHealth - HealthCost);
	const float NewMana = FMath::Max(0.f, CurrentMana - ManaCost);
	const float NewStamina = FMath::Max(0.f, CurrentStamina - StaminaCost);

	if (!FMath::IsNearlyEqual(CurrentHealth, NewHealth))
	{
		SourceASC->SetNumericAttributeBase(UHunterAttributeSet::GetHealthAttribute(), NewHealth);
	}
	if (!FMath::IsNearlyEqual(CurrentMana, NewMana))
	{
		SourceASC->SetNumericAttributeBase(UHunterAttributeSet::GetManaAttribute(), NewMana);
	}
	if (!FMath::IsNearlyEqual(CurrentStamina, NewStamina))
	{
		SourceASC->SetNumericAttributeBase(UHunterAttributeSet::GetStaminaAttribute(), NewStamina);
	}
}

void UCombatManager::ApplyOnHitEffects(
	AActor* SourceActor,
	AActor* TargetActor,
	const FCombatResolveResult& Result,
	UAbilitySystemComponent* CachedSourceASC,
	const UHunterAttributeSet* CachedSourceAttributes) const
{
	if (!IsValid(SourceActor) || !IsValid(TargetActor) || !TargetActor->HasAuthority())
	{
		return;
	}

	const bool bDealtDamage = Result.TotalDamageTaken > 0.f;

	// OPT: Use pre-cached pointers from ResolveHit when available; fall back to lookup
	// only when called standalone (e.g. from Blueprint).
	UAbilitySystemComponent* SourceASC =
		CachedSourceASC ? CachedSourceASC : GetAbilitySystemComponentFromActor(SourceActor);
	const UHunterAttributeSet* SourceAttributes =
		CachedSourceAttributes ? CachedSourceAttributes : GetHunterAttributeSetFromActor(SourceActor);

	if (bDealtDamage && SourceASC && SourceAttributes)
	{
		const float LifeOnHit    = FMath::Max(SourceAttributes->GetLifeOnHit(),    0.f);
		const float ManaOnHit    = FMath::Max(SourceAttributes->GetManaOnHit(),    0.f);
		const float StaminaOnHit = FMath::Max(SourceAttributes->GetStaminaOnHit(), 0.f);

		if (LifeOnHit > 0.f || ManaOnHit > 0.f || StaminaOnHit > 0.f)
		{
			if (RecoveryApplicationGE)
			{
				// I-01 FIX: Route on-hit recovery through the GAS GE pipeline so
				// PreAttributeChange clamping fires correctly. Previously called
				// SetNumericAttributeBase directly, bypassing all GAS attribute hooks.
				FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
				Context.AddSourceObject(SourceActor);

				FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(RecoveryApplicationGE, 1.f, Context);
				if (Spec.IsValid())
				{
					// Tags must match the SetByCaller tags configured on RecoveryApplicationGE.
					// Use RequestGameplayTag to keep the code decoupled from FPHGameplayTags
					// so designers can choose tag names freely in the GE asset.
					static const FGameplayTag RecoveryHealthTag   = FGameplayTag::RequestGameplayTag(FName("Data.Recovery.Health"));
					static const FGameplayTag RecoveryManaTag     = FGameplayTag::RequestGameplayTag(FName("Data.Recovery.Mana"));
					static const FGameplayTag RecoveryStaminaTag  = FGameplayTag::RequestGameplayTag(FName("Data.Recovery.Stamina"));

					if (LifeOnHit > 0.f)
					{
						Spec.Data->SetSetByCallerMagnitude(RecoveryHealthTag, LifeOnHit);
					}
					if (ManaOnHit > 0.f)
					{
						Spec.Data->SetSetByCallerMagnitude(RecoveryManaTag, ManaOnHit);
					}
					if (StaminaOnHit > 0.f)
					{
						Spec.Data->SetSetByCallerMagnitude(RecoveryStaminaTag, StaminaOnHit);
					}

					SourceASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());

					UE_LOG(LogCombatManager, Verbose,
						TEXT("Applied on-hit recovery via GE. Source=%s Life+%.2f Mana+%.2f Stamina+%.2f"),
						*GetNameSafe(SourceActor), LifeOnHit, ManaOnHit, StaminaOnHit);
				}
				else
				{
					UE_LOG(LogCombatManager, Error,
						TEXT("ApplyOnHitEffects: MakeOutgoingSpec failed for RecoveryApplicationGE on %s."),
						*GetNameSafe(this));
				}
			}
			else
			{
				// I-01 FALLBACK: Direct attribute mutation bypasses GAS clamping hooks.
				// Assign RecoveryApplicationGE in Blueprint defaults to fix this.
				UE_LOG(LogCombatManager, Warning,
					TEXT("ApplyOnHitEffects: RecoveryApplicationGE is not set on %s. "
					     "On-hit recovery (Life+%.2f Mana+%.2f Stamina+%.2f) skipped. "
					     "Please configure RecoveryApplicationGE in Blueprint defaults."),
					*GetNameSafe(this), LifeOnHit, ManaOnHit, StaminaOnHit);
			}
		}
	}

	ApplyAilments(SourceActor, TargetActor, Result);
	if (bDealtDamage)
	{
		ApplyReflect(SourceActor, TargetActor, Result);
	}
}

void UCombatManager::ApplyAilments(AActor* SourceActor, AActor* TargetActor, const FCombatResolveResult& Result) const
{
	if (!Result.bShouldApplyAilments)
	{
		UE_LOG(LogCombatManager, VeryVerbose,
			TEXT("ApplyAilments: skipped because bShouldApplyAilments is false. Target=%s"),
			*GetNameSafe(TargetActor));
		return;
	}

	if (!IsValid(SourceActor) || !IsValid(TargetActor))
	{
		return;
	}

	const UHunterAttributeSet* SourceAttributes = GetHunterAttributeSetFromActor(SourceActor);
	if (!SourceAttributes)
	{
		return;
	}

	UCombatStatusManager* SourceCombatStatusManager = SourceActor->FindComponentByClass<UCombatStatusManager>();
	if (!SourceCombatStatusManager)
	{
		UE_LOG(LogCombatManager, Warning,
			TEXT("ApplyAilments: %s has no CombatStatusManager, so ailments cannot be applied to %s."),
			*GetNameSafe(SourceActor), *GetNameSafe(TargetActor));
		return;
	}

	// bParryPath = true means ailments apply by flat chance only (no buildup contribution).
	// On the Normal path, ailments can also accumulate buildup toward threshold triggers.
	// TODO: Implement buildup tracking per ailment type when CombatStatusManager GEs are ready.
	const bool bParryPath = (Result.HitResponse == EHitResponse::Parry);

	// ── Bleed (Physical) ──────────────────────────────────────────────────────
	// On parry path: uses same flat chance roll — Elden Ring style, can proc on block.
	// PreMitigationPacket.Physical is used so the roll is based on raw incoming, not
	// post-armour damage (matching PoE2 and ER behavior).
	if (Result.PreMitigationPacket.Physical > 0.f || (bParryPath && Result.PreMitigationPacket.Physical > 0.f))
	{
		const float BleedChance = FMath::Clamp(SourceAttributes->GetChanceToBleed(), 0.f, 100.f);
		if (CombatManagerComponentPrivate::RollPercentChance(BleedChance))
		{
			const float DamagePerTick = Result.PreMitigationPacket.Physical * CombatManagerComponentPrivate::BleedDamagePerTickPercent;
			const float Duration = CombatManagerComponentPrivate::GetDurationOrDefault(SourceAttributes->GetBleedDuration(), CombatManagerComponentPrivate::DefaultBleedDuration);
			const FCombatStatusApplyResult ApplyResult = SourceCombatStatusManager->ApplyBleed(TargetActor, DamagePerTick, Duration, SourceActor);
			UE_LOG(LogCombatManager, Verbose,
				TEXT("ApplyAilments: Bleed %s on %s (chance=%.1f%% damage/tick=%.2f duration=%.2f %s)."),
				ApplyResult.bApplied ? TEXT("applied") : TEXT("failed"),
				*GetNameSafe(TargetActor), BleedChance, DamagePerTick, Duration, bParryPath ? TEXT("parry-flat") : TEXT("normal"));
		}
	}

	// ── Ignite (Fire) ─────────────────────────────────────────────────────────
	if (Result.PreMitigationPacket.Fire > 0.f)
	{
		const float IgniteChance = FMath::Clamp(SourceAttributes->GetChanceToIgnite(), 0.f, 100.f);
		if (CombatManagerComponentPrivate::RollPercentChance(IgniteChance))
		{
			const float DamagePerTick = Result.PreMitigationPacket.Fire * CombatManagerComponentPrivate::IgniteDamagePerTickPercent;
			const float Duration = CombatManagerComponentPrivate::GetDurationOrDefault(SourceAttributes->GetBurnDuration(), CombatManagerComponentPrivate::DefaultIgniteDuration);
			const FCombatStatusApplyResult ApplyResult = SourceCombatStatusManager->ApplyIgnite(TargetActor, DamagePerTick, Duration, SourceActor);
			UE_LOG(LogCombatManager, Verbose,
				TEXT("ApplyAilments: Ignite %s on %s (chance=%.1f%% damage/tick=%.2f duration=%.2f %s)."),
				ApplyResult.bApplied ? TEXT("applied") : TEXT("failed"),
				*GetNameSafe(TargetActor), IgniteChance, DamagePerTick, Duration, bParryPath ? TEXT("parry-flat") : TEXT("normal"));
		}
	}

	// ── Freeze (Ice) ──────────────────────────────────────────────────────────
	if (Result.PreMitigationPacket.Ice > 0.f)
	{
		const float FreezeChance = FMath::Clamp(SourceAttributes->GetChanceToFreeze(), 0.f, 100.f);
		if (CombatManagerComponentPrivate::RollPercentChance(FreezeChance))
		{
			const float Duration = CombatManagerComponentPrivate::GetDurationOrDefault(SourceAttributes->GetFreezeDuration(), CombatManagerComponentPrivate::DefaultFreezeDuration);
			const FCombatStatusApplyResult ApplyResult = SourceCombatStatusManager->ApplyFreeze(TargetActor, Duration, SourceActor);
			UE_LOG(LogCombatManager, Verbose,
				TEXT("ApplyAilments: Freeze %s on %s (chance=%.1f%% duration=%.2f %s)."),
				ApplyResult.bApplied ? TEXT("applied") : TEXT("failed"),
				*GetNameSafe(TargetActor), FreezeChance, Duration, bParryPath ? TEXT("parry-flat") : TEXT("normal"));
		}
	}

	// ── Shock (Lightning) ─────────────────────────────────────────────────────
	if (Result.PreMitigationPacket.Lightning > 0.f)
	{
		const float ShockChance = FMath::Clamp(SourceAttributes->GetChanceToShock(), 0.f, 100.f);
		if (CombatManagerComponentPrivate::RollPercentChance(ShockChance))
		{
			const float Duration = CombatManagerComponentPrivate::GetDurationOrDefault(SourceAttributes->GetShockDuration(), CombatManagerComponentPrivate::DefaultShockDuration);
			const FCombatStatusApplyResult ApplyResult = SourceCombatStatusManager->ApplyShock(TargetActor, CombatManagerComponentPrivate::DefaultShockAmpFraction, Duration, SourceActor);
			UE_LOG(LogCombatManager, Verbose,
				TEXT("ApplyAilments: Shock %s on %s (chance=%.1f%% amp=%.2f duration=%.2f %s)."),
				ApplyResult.bApplied ? TEXT("applied") : TEXT("failed"),
				*GetNameSafe(TargetActor), ShockChance, CombatManagerComponentPrivate::DefaultShockAmpFraction, Duration, bParryPath ? TEXT("parry-flat") : TEXT("normal"));
		}
	}

	// ── Petrify (Light) ───────────────────────────────────────────────────────
	if (Result.PreMitigationPacket.Light > 0.f)
	{
		const float PetrifyChance = FMath::Clamp(SourceAttributes->GetChanceToPetrify(), 0.f, 100.f);
		if (CombatManagerComponentPrivate::RollPercentChance(PetrifyChance))
		{
			const float Duration = CombatManagerComponentPrivate::GetDurationOrDefault(SourceAttributes->GetPetrifyBuildUpDuration(), CombatManagerComponentPrivate::DefaultPetrifyDuration);
			const FCombatStatusApplyResult ApplyResult = SourceCombatStatusManager->ApplyPetrify(TargetActor, Duration, SourceActor);
			UE_LOG(LogCombatManager, Verbose,
				TEXT("ApplyAilments: Petrify %s on %s (chance=%.1f%% duration=%.2f %s)."),
				ApplyResult.bApplied ? TEXT("applied") : TEXT("failed"),
				*GetNameSafe(TargetActor), PetrifyChance, Duration, bParryPath ? TEXT("parry-flat") : TEXT("normal"));
		}
	}

	// ── Corruption Ailment ────────────────────────────────────────────────────
	if (Result.PreMitigationPacket.Corruption > 0.f)
	{
		const float CorruptChance = FMath::Clamp(SourceAttributes->GetChanceToCorrupt(), 0.f, 100.f);
		if (CombatManagerComponentPrivate::RollPercentChance(CorruptChance))
		{
			const float DamagePerTick = Result.PreMitigationPacket.Corruption * CombatManagerComponentPrivate::CorruptionDamagePerTickPercent;
			const float Duration = CombatManagerComponentPrivate::GetDurationOrDefault(SourceAttributes->GetCorruptionDuration(), CombatManagerComponentPrivate::DefaultCorruptionDuration);
			const FCombatStatusApplyResult ApplyResult = SourceCombatStatusManager->ApplyCorruption(TargetActor, DamagePerTick, Duration, SourceActor);
			UE_LOG(LogCombatManager, Verbose,
				TEXT("ApplyAilments: Corruption %s on %s (chance=%.1f%% damage/tick=%.2f duration=%.2f %s)."),
				ApplyResult.bApplied ? TEXT("applied") : TEXT("failed"),
				*GetNameSafe(TargetActor), CorruptChance, DamagePerTick, Duration, bParryPath ? TEXT("parry-flat") : TEXT("normal"));
		}
	}
}

void UCombatManager::ApplyReflect(AActor* SourceActor, AActor* TargetActor, const FCombatResolveResult& Result) const
{
	// I-03: Structural skeleton for damage reflection. Probability and amount calculations
	// are implemented here. Actual damage delivery uses a dedicated non-recursive path
	// (direct attribute mutation rather than ResolveHit) to prevent infinite loops.

	if (!IsValid(SourceActor) || !IsValid(TargetActor))
	{
		return;
	}

	const UHunterAttributeSet* TargetAttributes = GetHunterAttributeSetFromActor(TargetActor);
	if (!TargetAttributes)
	{
		return;
	}

	// ── Physical Reflect ──────────────────────────────────────────────────
	// ReflectChancePhysical gates whether reflect fires; ReflectPhysical is the % amount.
	const float PhysReflectChance = FMath::Clamp(TargetAttributes->GetReflectChancePhysical(), 0.f, 100.f);
	const float PhysicalReflectPct = (PhysReflectChance > 0.f && FMath::FRandRange(0.f, 100.f) < PhysReflectChance)
		? FMath::Clamp(TargetAttributes->GetReflectPhysical(), 0.f, 100.f)
		: 0.f;
	const float PhysicalReflectAmount = Result.PhysicalTaken * (PhysicalReflectPct / 100.f);

	// ── Elemental Reflect ─────────────────────────────────────────────────
	const float ElemReflectChance = FMath::Clamp(TargetAttributes->GetReflectChanceElemental(), 0.f, 100.f);
	const float ElementalReflectPct = (ElemReflectChance > 0.f && FMath::FRandRange(0.f, 100.f) < ElemReflectChance)
		? FMath::Clamp(TargetAttributes->GetReflectElemental(), 0.f, 100.f)
		: 0.f;
	const float FireReflectAmount       = Result.FireTaken       * (ElementalReflectPct / 100.f);
	const float IceReflectAmount        = Result.IceTaken        * (ElementalReflectPct / 100.f);
	const float LightningReflectAmount  = Result.LightningTaken  * (ElementalReflectPct / 100.f);
	const float LightReflectAmount      = Result.LightTaken      * (ElementalReflectPct / 100.f);
	const float CorruptionReflectAmount = Result.CorruptionTaken * (ElementalReflectPct / 100.f);

	const float TotalReflect = PhysicalReflectAmount + FireReflectAmount + IceReflectAmount +
	                           LightningReflectAmount + LightReflectAmount + CorruptionReflectAmount;

	if (TotalReflect <= 0.f)
	{
		return;
	}

	UE_LOG(LogCombatManager, Verbose,
		TEXT("ApplyReflect: Reflecting %.2f damage back to %s from %s."),
		TotalReflect, *GetNameSafe(SourceActor), *GetNameSafe(TargetActor));

	// ── Apply reflect damage via a dedicated non-recursive GE ─────────────────
	// ReflectApplicationGE is a separate Instant GE that only touches Health.
	// It MUST NOT go through ResolveHit — doing so would recurse infinitely if the
	// source also has reflect on their character.
	if (!ReflectApplicationGE)
	{
		UE_LOG(LogCombatManager, Warning,
			TEXT("ApplyReflect: ReflectApplicationGE is not set on %s. "
			     "Reflect damage (%.2f) logged but not delivered. "
			     "Assign ReflectApplicationGE in Blueprint defaults."),
			*GetNameSafe(this), TotalReflect);
		return;
	}

	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActor(SourceActor);
	if (!SourceASC)
	{
		return;
	}

	FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
	Context.AddSourceObject(TargetActor); // Reflected from target back to source

	FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(ReflectApplicationGE, 1.f, Context);
	if (Spec.IsValid())
	{
		const FPHGameplayTags& Tags = FPHGameplayTags::Get();
		// Reflect always hits health directly — it bypasses shields and resistances.
		Spec.Data->SetSetByCallerMagnitude(Tags.Data_Damage_Health, -TotalReflect);
		SourceASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());

		UE_LOG(LogCombatManager, Verbose,
			TEXT("ApplyReflect: Applied %.2f reflected health damage to %s via ReflectApplicationGE."),
			TotalReflect, *GetNameSafe(SourceActor));
	}
	else
	{
		UE_LOG(LogCombatManager, Error,
			TEXT("ApplyReflect: MakeOutgoingSpec failed for ReflectApplicationGE on %s."),
			*GetNameSafe(this));
	}
}
