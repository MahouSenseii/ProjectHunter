#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "HunterGE_HealthRegen.generated.h"

/**
 * Instant GE used by the passive-regen timer to restore Health.
 * Modifier: Add on HunterAttributeSet.Health, magnitude via Data.Recovery.Health SetByCaller.
 * The timer sets the magnitude each tick via SetSetByCallerMagnitude before applying.
 */
UCLASS()
class ALS_PROJECTHUNTER_API UHunterGE_HealthRegen : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UHunterGE_HealthRegen();
};
