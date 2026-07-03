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

	/** Server-only. Stops sprinting and applies the stamina exhaustion GE so stamina regen pauses for the recovery window. Called by the AttributeSet when Stamina reaches 0. */
	void HandleStaminaDepleted();

	/** Server-only. Applies the mana exhaustion GE so mana regen pauses for the recovery window. Called by the AttributeSet when Mana reaches 0. */
	void HandleManaDepleted();

	/** Movement bridge: wall running requests the same stamina degen effect as sprinting. Also refreshes airborne stamina gates on movement mode changes. */
	void SetWallRunningStaminaDegenActive(bool bActive);

	/** Called when raw sprint/wall-traversal input changes, even if exhaustion blocks movement. */
	void NotifyStaminaMovementInputChanged();

	UFUNCTION(BlueprintPure, Category = "Exhaustion")
	bool ShouldCheckExhaustion() const { return bShouldCheckExhaustion; }
	
	// Debug / Cheat helpers
	UFUNCTION(BlueprintCallable, Category="Project Hunter|Debug|ASC")
	void Debug_StopStaminaDegen();

	UFUNCTION(BlueprintCallable, Category="Project Hunter|Debug|ASC")
	void Debug_RefillHealth();

	UFUNCTION(BlueprintCallable, Category="Project Hunter|Debug|ASC")
	void Debug_RefillStamina();

	UFUNCTION(BlueprintCallable, Category="Project Hunter|Debug|ASC")
	void Debug_ReserveHealth(float NewValue);
	
	UFUNCTION(BlueprintCallable, Category="Project Hunter|Debug|ASC")
	void Debug_DisableStaminaDrain();
	
	UFUNCTION(BlueprintCallable, Category="Project Hunter|Debug|ASC")
	void Debug_ReactivateStaminaDrain();

protected:
	virtual void AbilitySpecInputPressed(FGameplayAbilitySpec& Spec) override;
	virtual void AbilitySpecInputReleased(FGameplayAbilitySpec& Spec) override;
	virtual void NotifyAbilityActivated(const FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability) override;
	virtual void NotifyAbilityEnded(FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability, bool bWasCancelled) override;

	void EffectApplied(UAbilitySystemComponent* AbilitySystemComponent,
		const FGameplayEffectSpec& EffectSpec,
		FActiveGameplayEffectHandle ActiveEffectHandle);

	void HandleSprintingTagChanged(const FGameplayTag CallbackTag, int32 NewCount);
	void HandleStaminaExhaustedTagChanged(const FGameplayTag CallbackTag, int32 NewCount);
	void RefreshStaminaDegenEffect();
	void StartSprintStaminaDegen();
	void StopSprintStaminaDegen();
	void RefreshStaminaExhaustionRecovery();
	void CompleteStaminaExhaustionRecovery();
	void ClearStaminaExhaustionRecoveryTimer();
	void RemoveStaminaExhaustionEffect();
	bool IsStaminaMovementInputHeldForRecovery() const;
	bool IsAvatarAirborneForStamina() const;

	/** Applies the four regen GEs once on the server and grants the RegenActive tags. Idempotent. */
	void StartPassiveRegen();
	/** Removes the applied regen/degen GEs and the RegenActive tags. */
	void StopPassiveRegen();

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
	 * GameplayEffect used to drain stamina during sprinting. The native default
	 * is UHunterGE_StaminaDegen; Blueprint overrides must inherit from that class
	 * so copied regen assets cannot accidentally run as sprint drain.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sprinting")
	TSubclassOf<UGameplayEffect> SprintStaminaDrainGE;

	/*
	 * Passive Regen GEs. Blueprint overrides should inherit from the matching native class.
	 * Native defaults are infinite periodic GEs with MMC magnitudes.
	 * They read <Resource>RegenRate and <Resource>RegenAmount live.
	 * Native class fallback prevents stale SetByCaller assets from blocking regen.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Passive Regen")
	TSubclassOf<UGameplayEffect> HealthRegenGE;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Passive Regen")
	TSubclassOf<UGameplayEffect> ManaRegenGE;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Passive Regen")
	TSubclassOf<UGameplayEffect> StaminaRegenGE;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Passive Regen")
	TSubclassOf<UGameplayEffect> ArcaneShieldRegenGE;

	/**
	 * Exhaustion GEs applied when a pool hits 0. Each grants its
	 * Effect.<Resource>.Exhausted tag, which inhibits that resource's regen GE.
	 * Stamina recovery is timed by this ASC after sprint/wall-run input is released
	 * and the avatar is no longer airborne.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Exhaustion")
	TSubclassOf<UGameplayEffect> StaminaExhaustionGE;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Exhaustion")
	TSubclassOf<UGameplayEffect> ManaExhaustionGE;

	/** Debug/design switch. When false, stamina exhaustion does not stop sprinting or wall running. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Exhaustion")
	bool bShouldCheckExhaustion = true;

	/** After sprint/wall-run input is released and the avatar is not airborne, stamina exhausted is removed after this delay so regen can resume. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Exhaustion", meta = (ClampMin = "0.0", Units = "s"))
	float StaminaExhaustionRecoveryDelay = 1.0f;


private:
	// AbilityActorInfoSet can be called more than once as possession/controller state changes.
	// Keep the effect delegate bound exactly once so runtime refreshes do not stack callbacks.
	bool bEffectAppliedDelegateBound = false;
	bool bSprintingTagDelegateBound  = false;
	bool bStaminaExhaustedTagDelegateBound = false;
	bool bPassiveRegenStarted        = false;
	bool bWarnedNonNativeSprintDrainGE = false;
	bool bWarnedNonNativeHealthRegenGE = false;
	bool bWarnedNonNativeManaRegenGE = false;
	bool bWarnedNonNativeStaminaRegenGE = false;
	bool bWarnedNonNativeArcaneShieldRegenGE = false;
	bool bSprintStaminaDegenRequested = false;
	bool bWallRunningStaminaDegenRequested = false;

	TArray<FGameplayAbilitySpecHandle> InputPressedSpecHandles;
	TArray<FGameplayAbilitySpecHandle> InputReleasedSpecHandles;
	TArray<FGameplayAbilitySpecHandle> InputHeldSpecHandles;

	int32 ActivationGroupCounts[static_cast<uint8>(EPHAbilityActivationGroup::MAX)];

	// GE specs built once in StartPassiveRegen and applied immediately as infinite
	// periodic effects. Each handle is invalid until StartPassiveRegen() runs (or if
	// the GE class is null).
	FGameplayEffectSpecHandle CachedHealthRegenSpec;
	FGameplayEffectSpecHandle CachedManaRegenSpec;
	FGameplayEffectSpecHandle CachedStaminaRegenSpec;
	FGameplayEffectSpecHandle CachedArcaneShieldRegenSpec;

	// Handles for the infinite periodic resource GEs applied once in StartPassiveRegen.
	TArray<FActiveGameplayEffectHandle> ActivePassiveEffectHandles;

	FActiveGameplayEffectHandle ActiveSprintStaminaDrainHandle;
	FActiveGameplayEffectHandle ActiveStaminaExhaustionHandle;
	FTimerHandle StaminaExhaustionRecoveryTimerHandle;
	
#if !UE_BUILD_SHIPPING
	bool bDebugStaminaDrainDisabled = false;
#endif
};
