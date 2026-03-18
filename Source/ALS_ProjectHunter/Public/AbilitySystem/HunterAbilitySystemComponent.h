// Copyright © 2025 MahouSensei
// Author: Quentin Davis

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
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

private:
	// AbilityActorInfoSet can be called more than once as possession/controller state changes.
	// Keep the effect delegate bound exactly once so runtime refreshes do not stack callbacks.
	bool bEffectAppliedDelegateBound = false;
	bool bSprintingTagDelegateBound = false;
	bool bSprintDegenEffectTagApplied = false;
	FTimerHandle SprintStaminaDegenTimerHandle;
};
