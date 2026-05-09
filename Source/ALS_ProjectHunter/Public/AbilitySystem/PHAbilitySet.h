#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "AbilitySystem/Library/AbilitySetStructs.h"
#include "PHAbilitySet.generated.h"

class UHunterAbilitySystemComponent;

/**
 * Data asset that grants a group of PH gameplay abilities, effects, and optional
 * attribute sets. This is the Lyra-style migration point for ability grants.
 */
UCLASS(BlueprintType, Const)
class ALS_PROJECTHUNTER_API UPHAbilitySet : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	void GiveToAbilitySystem(UHunterAbilitySystemComponent* HunterASC, FPHAbilitySet_GrantedHandles* OutGrantedHandles, UObject* SourceObject = nullptr) const;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Abilities", meta = (TitleProperty = "Ability"))
	TArray<FPHAbilitySet_GameplayAbility> GrantedGameplayAbilities;

	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Effects", meta = (TitleProperty = "GameplayEffect"))
	TArray<FPHAbilitySet_GameplayEffect> GrantedGameplayEffects;

	UPROPERTY(EditDefaultsOnly, Category = "Attribute Sets", meta = (TitleProperty = "AttributeSet"))
	TArray<FPHAbilitySet_AttributeSet> GrantedAttributes;
};
