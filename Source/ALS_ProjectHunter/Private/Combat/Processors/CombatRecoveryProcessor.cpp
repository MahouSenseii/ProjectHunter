#include "Combat/Processors/CombatRecoveryProcessor.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystem/HunterAttributeSet.h"
#include "Combat/Components/CombatManager.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

namespace CombatRecoveryProcessorPrivate
{
	struct FResourceSnapshot
	{
		FGameplayAttribute CurrentAttribute;
		float Current = 0.f;
		float EffectiveMaximum = 0.f;
		float MaximumLeechRatePercent = 0.f;
	};

	FResourceSnapshot ResolveResource(
		const ECombatRecoveryResource Resource,
		const UHunterAttributeSet* Attributes)
	{
		FResourceSnapshot Result;
		if (!Attributes)
		{
			return Result;
		}

		switch (Resource)
		{
		case ECombatRecoveryResource::Health:
			Result.CurrentAttribute = UHunterAttributeSet::GetHealthAttribute();
			Result.Current = Attributes->GetHealth();
			Result.EffectiveMaximum = Attributes->GetMaxEffectiveHealth() > 0.f
				? Attributes->GetMaxEffectiveHealth() : Attributes->GetMaxHealth();
			Result.MaximumLeechRatePercent = Attributes->GetMaxLifeLeechRatePercent();
			break;
		case ECombatRecoveryResource::Mana:
			Result.CurrentAttribute = UHunterAttributeSet::GetManaAttribute();
			Result.Current = Attributes->GetMana();
			Result.EffectiveMaximum = Attributes->GetMaxEffectiveMana() > 0.f
				? Attributes->GetMaxEffectiveMana() : Attributes->GetMaxMana();
			Result.MaximumLeechRatePercent = Attributes->GetMaxManaLeechRatePercent();
			break;
		case ECombatRecoveryResource::Stamina:
			Result.CurrentAttribute = UHunterAttributeSet::GetStaminaAttribute();
			Result.Current = Attributes->GetStamina();
			Result.EffectiveMaximum = Attributes->GetMaxEffectiveStamina() > 0.f
				? Attributes->GetMaxEffectiveStamina() : Attributes->GetMaxStamina();
			Result.MaximumLeechRatePercent = Attributes->GetMaxStaminaLeechRatePercent();
			break;
		}

		Result.Current = FMath::Max(0.f, Result.Current);
		Result.EffectiveMaximum = FMath::Max(0.f, Result.EffectiveMaximum);
		return Result;
	}
}

float UCombatRecoveryProcessor::CalculateLeechRateCap(
	const float EffectiveMaximum,
	const float MaximumRatePercent)
{
	return FMath::Max(0.f, EffectiveMaximum)
		* FMath::Clamp(MaximumRatePercent, 0.f, 100.f) / 100.f;
}

void UCombatRecoveryProcessor::QueueLeech(
	const ECombatRecoveryResource Resource,
	const float Amount,
	const float Duration)
{
	QueueRecovery(Resource, ECombatRecoverySource::Leech, Amount, Duration);
}

void UCombatRecoveryProcessor::QueueRecoup(
	const ECombatRecoveryResource Resource,
	const float Amount,
	const float Duration)
{
	QueueRecovery(Resource, ECombatRecoverySource::Recoup, Amount, Duration);
}

void UCombatRecoveryProcessor::QueueRecovery(
	const ECombatRecoveryResource Resource,
	const ECombatRecoverySource Source,
	const float Amount,
	const float Duration)
{
	const float SafeAmount = FMath::Max(0.f, Amount);
	const float SafeDuration = FMath::Max(Duration, KINDA_SMALL_NUMBER);
	if (SafeAmount <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	FRecoveryInstance& Instance = RecoveryInstances.AddDefaulted_GetRef();
	Instance.Resource = Resource;
	Instance.Source = Source;
	Instance.RemainingAmount = SafeAmount;
	Instance.RatePerSecond = SafeAmount / SafeDuration;
	EnsureTimerRunning();
}

void UCombatRecoveryProcessor::NotifyHitDamageTaken()
{
	const AActor* OwnerActor = GetOwningActor();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return;
	}

	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwnerActor);
	const UHunterAttributeSet* Attributes = ASC ? ASC->GetSet<UHunterAttributeSet>() : nullptr;
	if (!Attributes)
	{
		return;
	}

	ArcaneShieldRechargeDelayRemaining = FMath::Max(0.f, Attributes->GetArcaneShieldRechargeDelay());
	EnsureTimerRunning();
}

void UCombatRecoveryProcessor::Advance(const float DeltaSeconds)
{
	AActor* OwnerActor = GetOwningActor();
	if (!OwnerActor || !OwnerActor->HasAuthority() || DeltaSeconds <= 0.f)
	{
		return;
	}

	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwnerActor);
	const UHunterAttributeSet* Attributes = ASC ? ASC->GetSet<UHunterAttributeSet>() : nullptr;
	if (!ASC || !Attributes)
	{
		Shutdown();
		return;
	}

	AdvanceResource(ECombatRecoveryResource::Health, DeltaSeconds, ASC, Attributes);
	AdvanceResource(ECombatRecoveryResource::Mana, DeltaSeconds, ASC, Attributes);
	AdvanceResource(ECombatRecoveryResource::Stamina, DeltaSeconds, ASC, Attributes);
	AdvanceArcaneShieldRecharge(DeltaSeconds, ASC, Attributes);
	StopTimerIfIdle();
}

void UCombatRecoveryProcessor::AdvanceResource(
	const ECombatRecoveryResource Resource,
	const float DeltaSeconds,
	UAbilitySystemComponent* ASC,
	const UHunterAttributeSet* Attributes)
{
	using namespace CombatRecoveryProcessorPrivate;

	const FResourceSnapshot Snapshot = ResolveResource(Resource, Attributes);
	if (!Snapshot.CurrentAttribute.IsValid() || Snapshot.EffectiveMaximum <= 0.f
		|| Snapshot.Current >= Snapshot.EffectiveMaximum - KINDA_SMALL_NUMBER)
	{
		RecoveryInstances.RemoveAll([Resource](const FRecoveryInstance& Instance)
		{
			return Instance.Resource == Resource;
		});
		return;
	}

	float DesiredLeech = 0.f;
	float DesiredRecoup = 0.f;
	for (const FRecoveryInstance& Instance : RecoveryInstances)
	{
		if (Instance.Resource != Resource)
		{
			continue;
		}

		const float Desired = FMath::Min(
			Instance.RemainingAmount,
			Instance.RatePerSecond * DeltaSeconds);
		(Instance.Source == ECombatRecoverySource::Leech ? DesiredLeech : DesiredRecoup) += Desired;
	}

	const float LeechCapThisStep = CalculateLeechRateCap(
		Snapshot.EffectiveMaximum,
		Snapshot.MaximumLeechRatePercent) * DeltaSeconds;
	if (DesiredLeech > KINDA_SMALL_NUMBER && LeechCapThisStep <= KINDA_SMALL_NUMBER)
	{
		RecoveryInstances.RemoveAll([Resource](const FRecoveryInstance& Instance)
		{
			return Instance.Resource == Resource
				&& Instance.Source == ECombatRecoverySource::Leech;
		});
		DesiredLeech = 0.f;
	}
	const float LeechScale = DesiredLeech > KINDA_SMALL_NUMBER
		? FMath::Min(1.f, LeechCapThisStep / DesiredLeech)
		: 0.f;
	const float DesiredTotal = DesiredLeech * LeechScale + DesiredRecoup;
	if (DesiredTotal <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const float AppliedAmount = FMath::Min(
		DesiredTotal,
		Snapshot.EffectiveMaximum - Snapshot.Current);
	ASC->SetNumericAttributeBase(Snapshot.CurrentAttribute, Snapshot.Current + AppliedAmount);

	if (AppliedAmount + KINDA_SMALL_NUMBER < DesiredTotal)
	{
		RecoveryInstances.RemoveAll([Resource](const FRecoveryInstance& Instance)
		{
			return Instance.Resource == Resource;
		});
		return;
	}

	for (FRecoveryInstance& Instance : RecoveryInstances)
	{
		if (Instance.Resource != Resource)
		{
			continue;
		}

		float Consumed = FMath::Min(
			Instance.RemainingAmount,
			Instance.RatePerSecond * DeltaSeconds);
		if (Instance.Source == ECombatRecoverySource::Leech)
		{
			Consumed *= LeechScale;
		}
		Instance.RemainingAmount = FMath::Max(0.f, Instance.RemainingAmount - Consumed);
	}

	RecoveryInstances.RemoveAll([](const FRecoveryInstance& Instance)
	{
		return Instance.RemainingAmount <= KINDA_SMALL_NUMBER;
	});
}

void UCombatRecoveryProcessor::AdvanceArcaneShieldRecharge(
	float DeltaSeconds,
	UAbilitySystemComponent* ASC,
	const UHunterAttributeSet* Attributes)
{
	if (ArcaneShieldRechargeDelayRemaining < 0.f)
	{
		return;
	}

	const float EffectiveMaximum = FMath::Max(
		Attributes->GetMaxEffectiveArcaneShield() > 0.f
			? Attributes->GetMaxEffectiveArcaneShield() : Attributes->GetMaxArcaneShield(),
		0.f);
	const float CurrentShield = FMath::Max(0.f, Attributes->GetArcaneShield());
	if (EffectiveMaximum <= 0.f || CurrentShield >= EffectiveMaximum - KINDA_SMALL_NUMBER)
	{
		ArcaneShieldRechargeDelayRemaining = -1.f;
		return;
	}

	if (ArcaneShieldRechargeDelayRemaining > 0.f)
	{
		const float TimeSpentWaiting = FMath::Min(DeltaSeconds, ArcaneShieldRechargeDelayRemaining);
		ArcaneShieldRechargeDelayRemaining -= TimeSpentWaiting;
		DeltaSeconds -= TimeSpentWaiting;
	}

	if (DeltaSeconds <= 0.f)
	{
		return;
	}

	const float RechargePerSecond = EffectiveMaximum
		* FMath::Clamp(Attributes->GetArcaneShieldRechargeRate(), 0.f, 100.f) / 100.f;
	if (RechargePerSecond <= KINDA_SMALL_NUMBER)
	{
		ArcaneShieldRechargeDelayRemaining = -1.f;
		return;
	}

	const float NewShield = FMath::Min(
		EffectiveMaximum,
		CurrentShield + RechargePerSecond * DeltaSeconds);
	ASC->SetNumericAttributeBase(UHunterAttributeSet::GetArcaneShieldAttribute(), NewShield);
	if (NewShield >= EffectiveMaximum - KINDA_SMALL_NUMBER)
	{
		ArcaneShieldRechargeDelayRemaining = -1.f;
	}
}

void UCombatRecoveryProcessor::EnsureTimerRunning()
{
	UCombatManager* Manager = GetOwningCombatManager();
	UWorld* World = Manager ? Manager->GetWorld() : nullptr;
	AActor* OwnerActor = GetOwningActor();
	if (!World || !OwnerActor || !OwnerActor->HasAuthority()
		|| World->GetTimerManager().IsTimerActive(RecoveryTimerHandle))
	{
		return;
	}

	World->GetTimerManager().SetTimer(
		RecoveryTimerHandle,
		this,
		&UCombatRecoveryProcessor::HandleTimerTick,
		ProcessingIntervalSeconds,
		true);
}

void UCombatRecoveryProcessor::StopTimerIfIdle()
{
	if (RecoveryInstances.Num() > 0 || ArcaneShieldRechargeDelayRemaining >= 0.f)
	{
		return;
	}

	if (UCombatManager* Manager = GetOwningCombatManager())
	{
		if (UWorld* World = Manager->GetWorld())
		{
			World->GetTimerManager().ClearTimer(RecoveryTimerHandle);
		}
	}
}

void UCombatRecoveryProcessor::HandleTimerTick()
{
	Advance(ProcessingIntervalSeconds);
}

void UCombatRecoveryProcessor::Shutdown()
{
	RecoveryInstances.Reset();
	ArcaneShieldRechargeDelayRemaining = -1.f;
	if (UCombatManager* Manager = GetOwningCombatManager())
	{
		if (UWorld* World = Manager->GetWorld())
		{
			World->GetTimerManager().ClearTimer(RecoveryTimerHandle);
		}
	}
}

UCombatManager* UCombatRecoveryProcessor::GetOwningCombatManager() const
{
	return Cast<UCombatManager>(GetOuter());
}

AActor* UCombatRecoveryProcessor::GetOwningActor() const
{
	const UCombatManager* Manager = GetOwningCombatManager();
	return Manager ? Manager->GetOwner() : nullptr;
}
