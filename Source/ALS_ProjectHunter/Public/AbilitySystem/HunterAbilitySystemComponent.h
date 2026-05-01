// Copyright © 2025 MahouSensei
// Author: Quentin Davis

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Abilities/PHGameplayAbility.h"
#include "GameplayEffect.h"
#include "TimerManager.h"
#include "HunterAbilitySystemComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FEffectAssetTags, const FGameplayTagContainer& /*Asset Tags*/)

// Declare log category
DECLARE_LOG_CATEGORY_EXTERN(LogHunterGAS, Log, All);

class UHunterAttributeSet;

/**
 * Minimal custom ASC for Project Hunter
 * Only handles core GAS initialization and effect application broadcasting
 */
UCLASS()
class ALS_PROJECTHUNTER_API UHunterAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

	// ========================================
	// FUNCTIONS
	// ========================================
public:
	UHunterAbilitySystemComponent();
	
	virtual void AbilityActorInfoSet();
	virtual void InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor) override;

	UFUNCTION(BlueprintCallable, Category = "Project Hunter|Abilities")
	void AbilityInputTagPressed(const FGameplayTag& InputTag);

	UFUNCTION(BlueprintCallable, Category = "Project Hunter|Abilities")
	void AbilityInputTagReleased(const FGameplayTag& InputTag);

	UFUNCTION(BlueprintCallable, Category = "Project Hunter|Abilities")
	void ProcessAbilityInput(float DeltaTime, bool bGamePaused);

	UFUNCTION(BlueprintCallable, Category = "Project Hunter|Abilities")
	void ClearAbilityInput();

	void TryActivateAbilitiesOnSpawn();
	void CancelInputActivatedAbilities(bool bReplicateCancelAbility);

	typedef TFunctionRef<bool(const UPHGameplayAbility* PHAbility, FGameplayAbilitySpecHandle Handle)> TShouldCancelAbilityFunc;
	void CancelAbilitiesByFunc(TShouldCancelAbilityFunc ShouldCancelFunc, bool bReplicateCancelAbility);

	bool IsActivationGroupBlocked(EPHAbilityActivationGroup Group) const;
	void AddAbilityToActivationGroup(EPHAbilityActivationGroup Group, UPHGameplayAbility* PHAbility);
	void RemoveAbilityFromActivationGroup(EPHAbilityActivationGroup Group, UPHGameplayAbility* PHAbility);
	void CancelActivationGroupAbilities(EPHAbilityActivationGroup Group, UPHGameplayAbility* IgnorePHAbility, bool bReplicateCancelAbility);

protected:
	virtual void AbilitySpecInputPressed(FGameplayAbilitySpec& Spec) override;
	virtual void AbilitySpecInputReleased(FGameplayAbilitySpec& Spec) override;
	virtual void NotifyAbilityActivated(const FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability) override;
	virtual void NotifyAbilityEnded(FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability, bool bWasCancelled) override;

	void EffectApplied(UAbilitySystemComponent* AbilitySystemComponent,
		const FGameplayEffectSpec& EffectSpec,
		FActiveGameplayEffectHandle ActiveEffectHandle);
	void HandleSprintingTagChanged(const FGameplayTag CallbackTag, int32 NewCount);
	void StartSprintStaminaDegen();
	void StopSprintStaminaDegen();
	void TickSprintStaminaDegen();

	/** Builds regen GE specs and starts the passive-regen accumulator timer. Idempotent. */
	void StartPassiveRegen();
	/** Clears the passive-regen timer and releases cached specs. */
	void StopPassiveRegen();
	/**
	 * Fixed 0.1 s base tick. Per-stat accumulators track elapsed time.
	 * A stat heals when its accumulator reaches its RegenRate (seconds between ticks).
	 */
	void TickPassiveRegen();

	const UHunterAttributeSet* GetHunterAttributeSet() const;

#if !UE_BUILD_SHIPPING
	void ShowEffectDebug(const FGameplayEffectSpec& EffectSpec, 
		const FGameplayTagContainer& TagContainer) const;
#endif
	
	// ========================================
	// VARIABLES
	// ========================================
public:
	FEffectAssetTags EffectAssetTags;

	/**
	 * C-1 FIX: GameplayEffect used to drain stamina during sprinting.
	 * Must be an Instant GE with a SetByCaller modifier on
	 * Data.Damage.Stamina (negative magnitude = drain).
	 * Configure this in the Blueprint derived class.
	 * If left null the system falls back to SetNumericAttributeBase (legacy path).
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sprinting")
	TSubclassOf<UGameplayEffect> SprintStaminaDrainGE;

	/* ─────────────────────────────────────────────────────────────────────
	 * Passive Regen GEs  (optional — falls back to SetNumericAttributeBase if null)
	 * Each must be an Instant GE with a single SetByCaller Add modifier:
	 *   HealthRegenGE  → HunterAttributeSet.Health,  Data.Recovery.Health  (positive)
	 *   ManaRegenGE    → HunterAttributeSet.Mana,    Data.Recovery.Mana    (positive)
	 *   StaminaRegenGE → HunterAttributeSet.Stamina, Data.Recovery.Stamina (positive)
	 * Rate (seconds between heals) and Amount (heal per tick) are read live
	 * from the AttributeSet each tick so equipment changes take effect immediately.
	 * ───────────────────────────────────────────────────────────────────── */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Passive Regen")
	TSubclassOf<UGameplayEffect> HealthRegenGE;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Passive Regen")
	TSubclassOf<UGameplayEffect> ManaRegenGE;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Passive Regen")
	TSubclassOf<UGameplayEffect> StaminaRegenGE;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Passive Regen")
	TSubclassOf<UGameplayEffect> ArcaneShieldRegenGE;


private:
	// AbilityActorInfoSet can be called more than once as possession/controller state changes.
	// Keep the effect delegate bound exactly once so runtime refreshes do not stack callbacks.
	bool bEffectAppliedDelegateBound  = false;
	bool bSprintingTagDelegateBound   = false;
	bool bSprintDegenEffectTagApplied = false;
	bool bPassiveRegenStarted         = false;

	TArray<FGameplayAbilitySpecHandle> InputPressedSpecHandles;
	TArray<FGameplayAbilitySpecHandle> InputReleasedSpecHandles;
	TArray<FGameplayAbilitySpecHandle> InputHeldSpecHandles;

	int32 ActivationGroupCounts[static_cast<uint8>(EPHAbilityActivationGroup::MAX)];

	// ── Sprint stamina degen ──────────────────────────────────────────────
	FTimerHandle SprintStaminaDegenTimerHandle;

	// N-13 FIX: Cache degen parameters computed in StartSprintStaminaDegen so
	// TickSprintStaminaDegen does not re-query the AttributeSet every 0.1 s.
	// Updated whenever sprint starts (which is when attributes are first validated).
	float CachedDegenRate   = 0.f;
	float CachedDegenAmount = 0.f;

	// OPT-SPRINT: Cache the GE spec handle so TickSprintStaminaDegen reuses it
	// instead of allocating a new context + spec every 0.1 s while sprinting.
	FGameplayEffectSpecHandle CachedSprintDrainSpec;

	// ── Passive regen ─────────────────────────────────────────────────────
	FTimerHandle PassiveRegenTimerHandle;

	// GE specs built once in StartPassiveRegen; only SetByCaller magnitude changes per tick.
	// Each handle is invalid until StartPassiveRegen() runs (or if the GE class is null).
	FGameplayEffectSpecHandle CachedHealthRegenSpec;
	FGameplayEffectSpecHandle CachedManaRegenSpec;
	FGameplayEffectSpecHandle CachedStaminaRegenSpec;
	FGameplayEffectSpecHandle CachedArcaneShieldRegenSpec;

	// Per-stat accumulators — track elapsed time since the last heal.
	// A stat heals when its accumulator >= its RegenRate attribute (seconds between ticks).
	float HealthRegenAccumulator       = 0.f;
	float ManaRegenAccumulator         = 0.f;
	float StaminaRegenAccumulator      = 0.f;
	float ArcaneShieldRegenAccumulator = 0.f;
};
