#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "HunterGE_StaminaDegen.generated.h"

/**
 * Infinite periodic GE that drains Stamina. UHunterAbilitySystemComponent
 * applies and removes this effect when the sprint condition tag changes.
 */
UCLASS()
class ALS_PROJECTHUNTER_API UHunterGE_StaminaDegen : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UHunterGE_StaminaDegen();
};
