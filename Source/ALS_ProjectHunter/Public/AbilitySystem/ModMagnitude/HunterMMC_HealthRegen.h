#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "HunterMMC_HealthRegen.generated.h"

/**
 * Computes health regeneration per second from HealthRegenRate * HealthRegenAmount.
 * The native GE scales this value by its period for smooth updates.
 */
UCLASS()
class ALS_PROJECTHUNTER_API UHunterMMC_HealthRegen : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

public:
	UHunterMMC_HealthRegen();

	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;
};
