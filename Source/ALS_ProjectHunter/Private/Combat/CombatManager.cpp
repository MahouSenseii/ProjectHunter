#include "Combat/CombatManager.h"

#include "AbilitySystem/HunterAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameFramework/Actor.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "PHGameplayTags.h"

DEFINE_LOG_CATEGORY(LogCombatManager);

namespace CombatManagerComponentPrivate
{
	constexpr float DefaultCritMultiplier = 1.5f;
	constexpr float MinResistancePercent = -100.f;
	constexpr float DefaultMaxResistancePercent = 90.f;
	constexpr float DefaultBlockAngleDegrees = 120.f;

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
}

UCombatManager::UCombatManager()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UCombatManager::ResolveHit(AActor* SourceActor, AActor* TargetActor, FCombatResolveResult& OutResult)
{
	OutResult = FCombatResolveResult{};

	if (!IsValid(SourceActor) || !IsValid(TargetActor))
	{
		UE_LOG(LogCombatManager, Warning, TEXT("ResolveHit failed because SourceActor or TargetActor was invalid. Source=%s Target=%s"),
			*GetNameSafe(SourceActor), *GetNameSafe(TargetActor));
		return false;
	}

	const UHunterAttributeSet* SourceAttributes = GetHunterAttributeSetFromActor(SourceActor);
	const UHunterAttributeSet* TargetAttributes = GetHunterAttributeSetFromActor(TargetActor);
	if (!SourceAttributes || !TargetAttributes)
	{
		return false;
	}

	UE_LOG(LogCombatManager, Verbose, TEXT("ResolveHit started. Source=%s Target=%s"), *GetNameSafe(SourceActor), *GetNameSafe(TargetActor));

	FCombatDamagePacket OutgoingPacket = BuildOutgoingDamagePacketFromAttributes(SourceAttributes, SourceActor, TargetActor);
	UE_LOG(LogCombatManager, Verbose, TEXT("Outgoing packet before conversion: %s"), *CombatManagerComponentPrivate::FormatPacket(OutgoingPacket));

	FCombatDamagePacket ConvertedPacket = ApplyDamageConversionFromAttributes(OutgoingPacket, SourceAttributes, SourceActor);
	UE_LOG(LogCombatManager, Verbose, TEXT("Outgoing packet after conversion: %s"), *CombatManagerComponentPrivate::FormatPacket(ConvertedPacket));

	OutResult = MitigateDamagePacketAgainstAttributes(ConvertedPacket, SourceActor, TargetActor, SourceAttributes, TargetAttributes);
	UE_LOG(LogCombatManager, Verbose, TEXT("Mitigated result before application: %s"), *CombatManagerComponentPrivate::FormatResult(OutResult));

	if (!TargetActor->HasAuthority())
	{
		UE_LOG(LogCombatManager, Verbose, TEXT("ResolveHit ran without server authority. Returning preview result only for %s."), *GetNameSafe(TargetActor));
		return true;
	}

	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActor(SourceActor);

	ApplyResolvedDamage(SourceActor, TargetActor, OutResult);

	// OPT: Reuse the already-cached TargetAttributes pointer — the object is the same,
	// only its field values change after ApplyResolvedDamage. Avoids a redundant lookup.
	OutResult.bKilledTarget = TargetAttributes->GetHealth() <= 0.f;

	// OPT: Pass pre-fetched SourceASC and SourceAttributes to avoid 2 more lookups inside.
	ApplyOnHitEffects(SourceActor, TargetActor, OutResult, SourceASC, SourceAttributes);

	UE_LOG(LogCombatManager, Verbose, TEXT("ResolveHit completed. Source=%s Target=%s %s"),
		*GetNameSafe(SourceActor),
		*GetNameSafe(TargetActor),
		*CombatManagerComponentPrivate::FormatResult(OutResult));

	return true;
}

FCombatDamagePacket UCombatManager::BuildOutgoingDamagePacket(AActor* SourceActor, AActor* TargetActor) const
{
	const UHunterAttributeSet* SourceAttributes = GetHunterAttributeSetFromActor(SourceActor);
	return SourceAttributes ? BuildOutgoingDamagePacketFromAttributes(SourceAttributes, SourceActor, TargetActor) : FCombatDamagePacket{};
}

FCombatDamagePacket UCombatManager::ApplyDamageConversion(const FCombatDamagePacket& InPacket, AActor* SourceActor) const
{
	const UHunterAttributeSet* SourceAttributes = GetHunterAttributeSetFromActor(SourceActor);
	return SourceAttributes ? ApplyDamageConversionFromAttributes(InPacket, SourceAttributes, SourceActor) : InPacket;
}

FCombatResolveResult UCombatManager::MitigateDamagePacket(const FCombatDamagePacket& InPacket, AActor* SourceActor, AActor* TargetActor) const
{
	const UHunterAttributeSet* SourceAttributes = GetHunterAttributeSetFromActor(SourceActor);
	const UHunterAttributeSet* TargetAttributes = GetHunterAttributeSetFromActor(TargetActor);
	return (SourceAttributes && TargetAttributes)
		? MitigateDamagePacketAgainstAttributes(InPacket, SourceActor, TargetActor, SourceAttributes, TargetAttributes)
		: FCombatResolveResult{};
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

FCombatDamagePacket UCombatManager::BuildOutgoingDamagePacketFromAttributes(const UHunterAttributeSet* SourceAttributes, AActor* SourceActor, AActor* TargetActor) const
{
	FCombatDamagePacket Packet;

	if (!SourceAttributes)
	{
		return Packet;
	}

	Packet.Physical = CalculateOutgoingDamageForType(EHunterDamageType::Physical, SourceAttributes);
	Packet.Fire = CalculateOutgoingDamageForType(EHunterDamageType::Fire, SourceAttributes);
	Packet.Ice = CalculateOutgoingDamageForType(EHunterDamageType::Ice, SourceAttributes);
	Packet.Lightning = CalculateOutgoingDamageForType(EHunterDamageType::Lightning, SourceAttributes);
	Packet.Light = CalculateOutgoingDamageForType(EHunterDamageType::Light, SourceAttributes);
	Packet.Corruption = CalculateOutgoingDamageForType(EHunterDamageType::Corruption, SourceAttributes);

	CombatManagerComponentPrivate::UpdatePacketTotal(Packet);

	ResolveCriticalStrike(Packet, SourceAttributes);

	UE_LOG(LogCombatManager, Verbose, TEXT("Built outgoing packet. Source=%s Target=%s %s"),
		*GetNameSafe(SourceActor),
		*GetNameSafe(TargetActor),
		*CombatManagerComponentPrivate::FormatPacket(Packet));

	return Packet;
}

FCombatDamagePacket UCombatManager::ApplyDamageConversionFromAttributes(const FCombatDamagePacket& InPacket, const UHunterAttributeSet* SourceAttributes, AActor* SourceActor) const
{
	if (!SourceAttributes)
	{
		return InPacket;
	}

	FCombatDamagePacket Packet = InPacket;
	const float PhysicalDamageBeforeConversion = FMath::Max(Packet.Physical, 0.f);
	if (PhysicalDamageBeforeConversion <= 0.f)
	{
		return Packet;
	}

	const float RequestedToFire = FMath::Max(SourceAttributes->GetPhysicalToFire(), 0.f);
	const float RequestedToIce = FMath::Max(SourceAttributes->GetPhysicalToIce(), 0.f);
	const float RequestedToLightning = FMath::Max(SourceAttributes->GetPhysicalToLightning(), 0.f);
	const float RequestedToLight = FMath::Max(SourceAttributes->GetPhysicalToLight(), 0.f);
	const float RequestedToCorruption = FMath::Max(SourceAttributes->GetPhysicalToCorruption(), 0.f);

	const float TotalRequestedConversion = RequestedToFire + RequestedToIce + RequestedToLightning + RequestedToLight + RequestedToCorruption;
	const float ConversionScale = TotalRequestedConversion > 100.f ? (100.f / TotalRequestedConversion) : 1.f;

	const auto ConvertFromPhysical = [&](const float RequestedPct, const EHunterDamageType DestinationType) -> float
	{
		const float AppliedPct = FMath::Clamp(RequestedPct * ConversionScale, 0.f, 100.f);
		const float ConvertedAmount = PhysicalDamageBeforeConversion * (AppliedPct / 100.f);
		CombatManagerComponentPrivate::AddDamageByType(Packet, DestinationType, ConvertedAmount);
		return ConvertedAmount;
	};

	float TotalConvertedAmount = 0.f;
	TotalConvertedAmount += ConvertFromPhysical(RequestedToFire, EHunterDamageType::Fire);
	TotalConvertedAmount += ConvertFromPhysical(RequestedToIce, EHunterDamageType::Ice);
	TotalConvertedAmount += ConvertFromPhysical(RequestedToLightning, EHunterDamageType::Lightning);
	TotalConvertedAmount += ConvertFromPhysical(RequestedToLight, EHunterDamageType::Light);
	TotalConvertedAmount += ConvertFromPhysical(RequestedToCorruption, EHunterDamageType::Corruption);

	Packet.Physical = FMath::Max(0.f, PhysicalDamageBeforeConversion - TotalConvertedAmount);
	CombatManagerComponentPrivate::UpdatePacketTotal(Packet);

	UE_LOG(LogCombatManager, Verbose, TEXT("Applied conversion for %s. %s"), *GetNameSafe(SourceActor), *CombatManagerComponentPrivate::FormatPacket(Packet));

	return Packet;
}

FCombatResolveResult UCombatManager::MitigateDamagePacketAgainstAttributes(const FCombatDamagePacket& InPacket, AActor* SourceActor, AActor* TargetActor, const UHunterAttributeSet* SourceAttributes, const UHunterAttributeSet* TargetAttributes) const
{
	FCombatResolveResult Result;
	Result.PreMitigationPacket = InPacket;
	Result.bWasCrit = InPacket.bCrit;

	if (!SourceAttributes || !TargetAttributes)
	{
		return Result;
	}

	const float EffectiveArmour = FMath::Max(
		(TargetAttributes->GetArmour() + TargetAttributes->GetArmourFlatBonus()) * (1.f + (TargetAttributes->GetArmourPercentBonus() / 100.f)),
		0.f);
	const float PhysicalMitigationPct = FMath::Clamp(
		EffectiveArmour / (EffectiveArmour + 100.f),
		0.f,
		0.9f);

	Result.PhysicalTaken = FMath::Max(0.f, InPacket.Physical * (1.f - PhysicalMitigationPct));

	const auto CalculateMitigatedTypedDamage = [&](const EHunterDamageType DamageType) -> float
	{
		const float IncomingDamage = FMath::Max(CombatManagerComponentPrivate::GetDamageByType(InPacket, DamageType), 0.f);
		if (IncomingDamage <= 0.f)
		{
			return 0.f;
		}

		const float EffectiveResistance = GetResistanceValue(DamageType, TargetAttributes) - GetPierceValue(DamageType, SourceAttributes);
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

	ApplyBlockingToMitigatedResult(SourceActor, TargetActor, TargetAttributes, Result);
	ApplyStaminaBlockCost(TargetAttributes, Result);

	for (const EHunterDamageType DamageType : {
		EHunterDamageType::Physical,
		EHunterDamageType::Fire,
		EHunterDamageType::Ice,
		EHunterDamageType::Lightning,
		EHunterDamageType::Light,
		EHunterDamageType::Corruption })
	{
		const float DamageAfterBlock = CombatManagerComponentPrivate::GetResultTakenByType(Result, DamageType);
		const float DamageTakenMultiplier = GetDamageTakenMultiplier(DamageType, TargetAttributes);
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

float UCombatManager::CalculateOutgoingDamageForType(const EHunterDamageType DamageType, const UHunterAttributeSet* SourceAttributes) const
{
	if (!SourceAttributes)
	{
		return 0.f;
	}

	float MinDamage = 0.f;
	float MaxDamage = 0.f;
	float FlatDamage = 0.f;
	float TypeIncreasePct = 0.f;

	switch (DamageType)
	{
	case EHunterDamageType::Physical:
		MinDamage = SourceAttributes->GetMinPhysicalDamage();
		MaxDamage = SourceAttributes->GetMaxPhysicalDamage();
		FlatDamage = SourceAttributes->GetPhysicalFlatDamage();
		TypeIncreasePct = SourceAttributes->GetPhysicalPercentDamage();
		break;
	case EHunterDamageType::Fire:
		MinDamage = SourceAttributes->GetMinFireDamage();
		MaxDamage = SourceAttributes->GetMaxFireDamage();
		FlatDamage = SourceAttributes->GetFireFlatDamage();
		TypeIncreasePct = SourceAttributes->GetFirePercentDamage();
		break;
	case EHunterDamageType::Ice:
		MinDamage = SourceAttributes->GetMinIceDamage();
		MaxDamage = SourceAttributes->GetMaxIceDamage();
		FlatDamage = SourceAttributes->GetIceFlatDamage();
		TypeIncreasePct = SourceAttributes->GetIcePercentDamage();
		break;
	case EHunterDamageType::Lightning:
		MinDamage = SourceAttributes->GetMinLightningDamage();
		MaxDamage = SourceAttributes->GetMaxLightningDamage();
		FlatDamage = SourceAttributes->GetLightningFlatDamage();
		TypeIncreasePct = SourceAttributes->GetLightningPercentDamage();
		break;
	case EHunterDamageType::Light:
		MinDamage = SourceAttributes->GetMinLightDamage();
		MaxDamage = SourceAttributes->GetMaxLightDamage();
		FlatDamage = SourceAttributes->GetLightFlatDamage();
		TypeIncreasePct = SourceAttributes->GetLightPercentDamage();
		break;
	case EHunterDamageType::Corruption:
		MinDamage = SourceAttributes->GetMinCorruptionDamage();
		MaxDamage = SourceAttributes->GetMaxCorruptionDamage();
		FlatDamage = SourceAttributes->GetCorruptionFlatDamage();
		TypeIncreasePct = SourceAttributes->GetCorruptionPercentDamage();
		break;
	default:
		return 0.f;
	}

	const float BaseRolledDamage = RollDamageRange(MinDamage, MaxDamage);
	const float BaseDamageWithFlatBonus = FMath::Max(0.f, BaseRolledDamage + FlatDamage);

	float TotalIncreasedPct = SourceAttributes->GetGlobalDamages() + TypeIncreasePct;
	if (CombatManagerComponentPrivate::IsElementalDamageType(DamageType))
	{
		TotalIncreasedPct += SourceAttributes->GetElementalDamage();
	}

	const float DamageAfterIncreased = FMath::Max(0.f, CombatManagerComponentPrivate::ApplyPercentIncrease(BaseDamageWithFlatBonus, TotalIncreasedPct));
	const float DamageAfterMore = FMath::Max(0.f, DamageAfterIncreased * GetMoreDamageMultiplier(DamageType, SourceAttributes));

	return DamageAfterMore;
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

void UCombatManager::ResolveCriticalStrike(FCombatDamagePacket& Packet, const UHunterAttributeSet* SourceAttributes) const
{
	Packet.bCrit = false;
	Packet.CritMultiplierApplied = 1.f;

	if (!SourceAttributes)
	{
		CombatManagerComponentPrivate::UpdatePacketTotal(Packet);
		return;
	}

	const float CritChance = FMath::Clamp(SourceAttributes->GetCritChance(), 0.f, 100.f);
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

	const float CritMultiplier = SourceAttributes->GetCritMultiplier() > 0.f
		? SourceAttributes->GetCritMultiplier()
		: CombatManagerComponentPrivate::DefaultCritMultiplier;

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

void UCombatManager::ApplyOnHitEffects(
	AActor* SourceActor,
	AActor* TargetActor,
	const FCombatResolveResult& Result,
	UAbilitySystemComponent* CachedSourceASC,
	const UHunterAttributeSet* CachedSourceAttributes) const
{
	if (Result.TotalDamageTaken <= 0.f || !IsValid(SourceActor) || !IsValid(TargetActor) || !TargetActor->HasAuthority())
	{
		return;
	}

	// OPT: Use pre-cached pointers from ResolveHit when available; fall back to lookup
	// only when called standalone (e.g. from Blueprint).
	UAbilitySystemComponent* SourceASC =
		CachedSourceASC ? CachedSourceASC : GetAbilitySystemComponentFromActor(SourceActor);
	const UHunterAttributeSet* SourceAttributes =
		CachedSourceAttributes ? CachedSourceAttributes : GetHunterAttributeSetFromActor(SourceActor);

	if (SourceASC && SourceAttributes)
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
	ApplyReflect(SourceActor, TargetActor, Result);
}

void UCombatManager::ApplyAilments(AActor* SourceActor, AActor* TargetActor, const FCombatResolveResult& Result) const
{
	// I-03: Structural skeleton for ailment application. Probability calculations are
	// implemented here so hit-by-hit logic is deterministic. Actual GE application is
	// deferred until ailment GE assets are created in Blueprint (per user instruction).
	//
	// OPT-AILMENT: Early-out until ailment GE assets exist. Remove this guard once
	// at least one ailment GE is wired up — the per-ailment TODO blocks below will
	// handle the rest individually.
	// ────────────────────────────────────────────────────────────────────────
	// To re-enable: delete (or #if 0) the early-return below and assign
	// AilmentGE class references on CombatManager defaults in Blueprint.
	// ────────────────────────────────────────────────────────────────────────
	UE_LOG(LogCombatManager, VeryVerbose,
		TEXT("ApplyAilments: Skipped — ailment GE assets not yet created. "
		     "Remove early-return in CombatManager.cpp once GEs are ready."));
	return; // OPT-AILMENT: no-op until GE hookup — avoids dead probability math every hit

	if (!IsValid(SourceActor) || !IsValid(TargetActor))
	{
		return;
	}

	const UHunterAttributeSet* SourceAttributes = GetHunterAttributeSetFromActor(SourceActor);
	if (!SourceAttributes)
	{
		return;
	}

	// ── Bleed (Physical) ──────────────────────────────────────────────────
	if (Result.PhysicalTaken > 0.f)
	{
		const float BleedChance = FMath::Clamp(SourceAttributes->GetChanceToBleed(), 0.f, 100.f);
		if (BleedChance > 0.f && FMath::FRandRange(0.f, 100.f) < BleedChance)
		{
			UE_LOG(LogCombatManager, Verbose, TEXT("ApplyAilments: Bleed triggered on %s (chance=%.1f%%). GE hookup pending."), *GetNameSafe(TargetActor), BleedChance);
			// TODO: Apply Bleed GE via TargetASC once GE asset is created in Blueprint.
		}
	}

	// ── Ignite (Fire) ─────────────────────────────────────────────────────
	if (Result.FireTaken > 0.f)
	{
		const float IgniteChance = FMath::Clamp(SourceAttributes->GetChanceToIgnite(), 0.f, 100.f);
		if (IgniteChance > 0.f && FMath::FRandRange(0.f, 100.f) < IgniteChance)
		{
			UE_LOG(LogCombatManager, Verbose, TEXT("ApplyAilments: Ignite triggered on %s (chance=%.1f%%). GE hookup pending."), *GetNameSafe(TargetActor), IgniteChance);
			// TODO: Apply Ignite GE via TargetASC once GE asset is created in Blueprint.
		}
	}

	// ── Freeze (Ice) ──────────────────────────────────────────────────────
	if (Result.IceTaken > 0.f)
	{
		const float FreezeChance = FMath::Clamp(SourceAttributes->GetChanceToFreeze(), 0.f, 100.f);
		if (FreezeChance > 0.f && FMath::FRandRange(0.f, 100.f) < FreezeChance)
		{
			UE_LOG(LogCombatManager, Verbose, TEXT("ApplyAilments: Freeze triggered on %s (chance=%.1f%%). GE hookup pending."), *GetNameSafe(TargetActor), FreezeChance);
			// TODO: Apply Freeze GE via TargetASC once GE asset is created in Blueprint.
		}
	}

	// ── Shock (Lightning) ─────────────────────────────────────────────────
	if (Result.LightningTaken > 0.f)
	{
		const float ShockChance = FMath::Clamp(SourceAttributes->GetChanceToShock(), 0.f, 100.f);
		if (ShockChance > 0.f && FMath::FRandRange(0.f, 100.f) < ShockChance)
		{
			UE_LOG(LogCombatManager, Verbose, TEXT("ApplyAilments: Shock triggered on %s (chance=%.1f%%). GE hookup pending."), *GetNameSafe(TargetActor), ShockChance);
			// TODO: Apply Shock GE via TargetASC once GE asset is created in Blueprint.
		}
	}

	// ── Petrify (Light) ───────────────────────────────────────────────────
	if (Result.LightTaken > 0.f)
	{
		const float PetrifyChance = FMath::Clamp(SourceAttributes->GetChanceToPetrify(), 0.f, 100.f);
		if (PetrifyChance > 0.f && FMath::FRandRange(0.f, 100.f) < PetrifyChance)
		{
			UE_LOG(LogCombatManager, Verbose, TEXT("ApplyAilments: Petrify triggered on %s (chance=%.1f%%). GE hookup pending."), *GetNameSafe(TargetActor), PetrifyChance);
			// TODO: Apply Petrify GE via TargetASC once GE asset is created in Blueprint.
		}
	}

	// ── Corruption Ailment (Corruption) ───────────────────────────────────
	if (Result.CorruptionTaken > 0.f)
	{
		const float CorruptChance = FMath::Clamp(SourceAttributes->GetChanceToCorrupt(), 0.f, 100.f);
		if (CorruptChance > 0.f && FMath::FRandRange(0.f, 100.f) < CorruptChance)
		{
			UE_LOG(LogCombatManager, Verbose, TEXT("ApplyAilments: Corruption ailment triggered on %s (chance=%.1f%%). GE hookup pending."), *GetNameSafe(TargetActor), CorruptChance);
			// TODO: Apply Corruption Ailment GE via TargetASC once GE asset is created in Blueprint.
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
		TEXT("ApplyReflect: Reflecting %.2f damage back to %s from %s. GE hookup pending."),
		TotalReflect, *GetNameSafe(SourceActor), *GetNameSafe(TargetActor));

	// TODO: Apply TotalReflect to SourceActor via a direct GAS GE (non-recursive path).
	// Do NOT call ResolveHit(TargetActor, SourceActor, ...) here — that would recurse
	// infinitely if the source also has reflect. Use a lightweight instant GE that
	// bypasses ailment and reflect processing.
}
