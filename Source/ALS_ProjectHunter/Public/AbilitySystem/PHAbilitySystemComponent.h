// Copyright@2024 Quentin Davis

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "PHAbilitySystemComponent.generated.h"


DECLARE_MULTICAST_DELEGATE_OneParam(FEffectAssetTags, const FGameplayTagContainer /*Asset Tags*/)

/**
 * 
 */
UCLASS()
class ALS_PROJECTHUNTER_API UPHAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:

	void AbilityActorInfoSet();

	FEffectAssetTags EffectAssetTags;

protected:

	void EffectApplied(UAbilitySystemComponent* AbilitySystemComponent, const FGameplayEffectSpec& EffectSpec, FActiveGameplayEffectHandle
	                   ActiveEffectHandle);
	
};
