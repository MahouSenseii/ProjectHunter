#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "HunterMMC_StaminaRegen.generated.h"

/**
 * Computes stamina regeneration per second from StaminaRegenRate * StaminaRegenAmount.
 * The native GE scales this value by its period for smooth updates.
 */
UCLASS()
class ALS_PROJECTHUNTER_API UHunterMMC_StaminaRegen : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

public:
	UHunterMMC_StaminaRegen();

	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;
};
