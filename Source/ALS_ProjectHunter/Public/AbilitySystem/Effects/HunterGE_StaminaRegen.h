#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "HunterGE_StaminaRegen.generated.h"

/**
 * Instant GE used by the passive-regen timer to restore Stamina.
 * Modifier: Add on HunterAttributeSet.Stamina, magnitude via Data.Recovery.Stamina SetByCaller.
 * The timer sets the magnitude each tick via SetSetByCallerMagnitude before applying.
 * Stamina regen is suppressed while Condition.Sprinting is active (handled by the timer,
 * not by tag requirements on this GE).
 */
UCLASS()
class ALS_PROJECTHUNTER_API UHunterGE_StaminaRegen : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UHunterGE_StaminaRegen();
};
