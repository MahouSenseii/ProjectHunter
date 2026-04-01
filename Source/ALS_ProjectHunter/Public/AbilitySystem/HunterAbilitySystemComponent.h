// Copyright © 2025 MahouSensei
// Author: Quentin Davis

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
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

protected:
	void EffectApplied(UAbilitySystemComponent* AbilitySystemComponent,
		const FGameplayEffectSpec& EffectSpec, 
		FActiveGameplayEffectHandle ActiveEffectHandle);
	void HandleSprintingTagChanged(const FGameplayTag CallbackTag, int32 NewCount);
	void StartSprintStaminaDegen();
	void StopSprintStaminaDegen();
	void TickSprintStaminaDegen();
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

private:
	// AbilityActorInfoSet can be called more than once as possession/controller state changes.
	// Keep the effect delegate bound exactly once so runtime refreshes do not stack callbacks.
	bool bEffectAppliedDelegateBound = false;
	bool bSprintingTagDelegateBound = false;
	bool bSprintDegenEffectTagApplied = false;
	FTimerHandle SprintStaminaDegenTimerHandle;

	// N-13 FIX: Cache degen parameters computed in StartSprintStaminaDegen so
	// TickSprintStaminaDegen does not re-query the AttributeSet every 0.1 s.
	// Updated whenever sprint starts (which is when attributes are first validated).
	float CachedDegenRate   = 0.f;
	float CachedDegenAmount = 0.f;

	// OPT-SPRINT: Cache the GE spec handle so TickSprintStaminaDegen reuses it
	// instead of allocating a new context + spec every 0.1 s while sprinting.
	FGameplayEffectSpecHandle CachedSprintDrainSpec;
};
