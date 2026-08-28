#pragma once

#include "CoreMinimal.h"
#include "TimerManager.h"
#include "UObject/Object.h"
#include "CombatRecoveryProcessor.generated.h"

class UAbilitySystemComponent;
class UCombatManager;
class UHunterAttributeSet;

enum class ECombatRecoveryResource : uint8
{
	Health,
	Mana,
	Stamina
};

enum class ECombatRecoverySource : uint8
{
	Leech,
	Recoup
};

/** Timed recovery state owned by one CombatManager. */
UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class ALS_PROJECTHUNTER_API UCombatRecoveryProcessor final : public UObject
{
	GENERATED_BODY()

public:
	void QueueLeech(ECombatRecoveryResource Resource, float Amount, float Duration);
	void QueueRecoup(ECombatRecoveryResource Resource, float Amount, float Duration);
	void NotifyHitDamageTaken();
	void Shutdown();

	/** Public for deterministic automation tests; runtime calls this from a private timer. */
	void Advance(float DeltaSeconds);

	int32 GetPendingRecoveryCount() const { return RecoveryInstances.Num(); }
	bool IsArcaneShieldRechargePending() const { return ArcaneShieldRechargeDelayRemaining >= 0.f; }

	static float CalculateLeechRateCap(float EffectiveMaximum, float MaximumRatePercent);

private:
	struct FRecoveryInstance
	{
		ECombatRecoveryResource Resource = ECombatRecoveryResource::Health;
		ECombatRecoverySource Source = ECombatRecoverySource::Leech;
		float RemainingAmount = 0.f;
		float RatePerSecond = 0.f;
	};

	void QueueRecovery(
		ECombatRecoveryResource Resource,
		ECombatRecoverySource Source,
		float Amount,
		float Duration);
	void AdvanceResource(
		ECombatRecoveryResource Resource,
		float DeltaSeconds,
		UAbilitySystemComponent* ASC,
		const UHunterAttributeSet* Attributes);
	void AdvanceArcaneShieldRecharge(
		float DeltaSeconds,
		UAbilitySystemComponent* ASC,
		const UHunterAttributeSet* Attributes);
	void EnsureTimerRunning();
	void StopTimerIfIdle();
	void HandleTimerTick();

	UCombatManager* GetOwningCombatManager() const;
	AActor* GetOwningActor() const;

	TArray<FRecoveryInstance> RecoveryInstances;
	FTimerHandle RecoveryTimerHandle;
	float ArcaneShieldRechargeDelayRemaining = -1.f;

	static constexpr float ProcessingIntervalSeconds = 0.05f;
};
