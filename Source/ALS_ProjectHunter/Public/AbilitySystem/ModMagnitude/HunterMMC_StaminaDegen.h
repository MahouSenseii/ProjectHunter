#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "HunterMMC_StaminaDegen.generated.h"

/**
 * Computes the per-second stamina DRAIN for the sprint degen GE.
 *
 * Returns a NEGATIVE value, -(StaminaDegenRate * StaminaDegenAmount), so an
 * Additive periodic modifier reduces Stamina. The GE scales this value by its
 * period so faster ticks feel responsive without changing the authored rate.
 * The regen and degen GEs run simultaneously and GAS nets them: if regen >=
 * degen the character can sprint indefinitely.
 */
UCLASS()
class ALS_PROJECTHUNTER_API UHunterMMC_StaminaDegen : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

public:
	UHunterMMC_StaminaDegen();

	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;
};
