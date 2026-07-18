#include "Combat/Components/CombatManager.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystem/HunterAttributeSet.h"
#include "Combat/Components/UCombatStatusEffectApplier.h"
#include "Combat/Calculators/CombatOutgoingDamageCalculator.h"
#include "Combat/Resolvers/CombatIncomingDamageResolver.h"
#include "Combat/Library/FunctionLibraries/CombatFunctionLibrary.h"
#include "GameFramework/Actor.h"
#include "GameplayEffect.h"
#include "PHGameplayTags.h"

DEFINE_LOG_CATEGORY(LogCombatManager);

namespace CombatManagerPrivate
{
	// Ailment tuning. Hit-resolution math itself lives in FCombatOutgoingDamageCalculator
	// and FCombatIncomingDamageResolver; these constants are only used by
	// UCombatManager::ApplyAilments below.
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

	FVector ResolveDamagePopupWorldLocation(const AActor* TargetActor)
	{
		if (!IsValid(TargetActor))
		{
			return FVector::ZeroVector;
		}

		return TargetActor->GetActorLocation() +
			FVector(0.f, 0.f, TargetActor->GetSimpleCollisionHalfHeight());
	}
}

void UCombatIncomingHitEditContext::RejectHit()
{
	bApplyHit = false;
}

UCombatManager::UCombatManager()
{
	PrimaryComponentTick.bCanEverTick = false;

	// Owned sub-object, not a sibling actor component: CombatManager stays the
	// only actor component in this domain, and status effects still have one
	// clear owner via this instance rather than a FindComponentByClass search.
	CombatStatus = CreateDefaultSubobject<UCombatStatusEffectApplier>(TEXT("CombatStatus"));
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

	// Outgoing: base -> conversion -> increased/more -> crit.
	FCombatDamagePacket OutgoingPacket = FCombatOutgoingDamageCalculator::BuildOutgoingDamagePacket(AttackerAttributes, EffectiveInfo);
	UE_LOG(LogCombatManager, Verbose, TEXT("ApplyHit outgoing packet: %s"),
		*FCombatOutgoingDamageCalculator::FormatPacket(OutgoingPacket));

	// Incoming: armour/resists -> block -> taken multipliers -> routing.
	OutResult = FCombatIncomingDamageResolver::MitigateDamagePacket(
		OutgoingPacket, AttackerActor, DefenderActor,
		AttackerAttributes, DefenderAttributes, EffectiveInfo);

	// Stagger and hit-response gates run before application so both the
	// authoritative path and non-authority previews agree on the outcome.
	FCombatIncomingDamageResolver::EvaluateStagger(DefenderActor, DefenderAttributes, OutResult);
	FCombatIncomingDamageResolver::ApplyHitResponse(EffectiveHitResponse, bEffectiveCanApplyAilments, OutResult);

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
		*FCombatIncomingDamageResolver::FormatResult(OutResult));

	return true;
}

// Application

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

	// Fallback path bypasses GAS clamping; configure DamageApplicationGE.
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
			TEXT("ApplyOnHitRecovery: RecoveryApplicationGE is not set on %s; on-hit recovery skipped."),
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

	UCombatStatusEffectApplier* StatusManager = CombatStatus;
	if (!StatusManager)
	{
		UE_LOG(LogCombatManager, Verbose,
			TEXT("ApplyAilments skipped: no CombatStatus effect applier found for %s."),
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
