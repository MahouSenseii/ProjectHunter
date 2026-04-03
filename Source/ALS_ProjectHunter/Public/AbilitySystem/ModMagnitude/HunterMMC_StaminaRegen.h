#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "HunterMMC_StaminaRegen.generated.h"

/**
 * Computes the stamina regeneration magnitude per GE period.
 * Returns StaminaRegenRate * StaminaRegenAmount.
 * Set the GE Period to 1.0 s so the result equals HP restored per second.
 */
UCLASS()
class ALS_PROJECTHUNTER_API UHunterMMC_StaminaRegen : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

public:
	UHunterMMC_StaminaRegen();

	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;
};
