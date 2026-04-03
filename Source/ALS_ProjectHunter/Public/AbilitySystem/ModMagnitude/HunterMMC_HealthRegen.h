#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "HunterMMC_HealthRegen.generated.h"

/**
 * Computes the health regeneration magnitude per GE period.
 * Returns HealthRegenRate * HealthRegenAmount.
 * Set the GE Period to 1.0 s so the result equals HP restored per second.
 */
UCLASS()
class ALS_PROJECTHUNTER_API UHunterMMC_HealthRegen : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

public:
	UHunterMMC_HealthRegen();

	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;
};
