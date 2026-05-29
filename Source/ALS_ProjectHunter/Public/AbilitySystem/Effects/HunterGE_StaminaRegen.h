#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "HunterGE_StaminaRegen.generated.h"

/**
 * Instant GE used by the passive-regen timer to restore Stamina.
 */
UCLASS()
class ALS_PROJECTHUNTER_API UHunterGE_StaminaRegen : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UHunterGE_StaminaRegen();
};
