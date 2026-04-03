#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "HunterMMC_ManaRegen.generated.h"

/**
 * Computes the mana regeneration magnitude per GE period.
 * Returns ManaRegenRate * ManaRegenAmount.
 * Set the GE Period to 1.0 s so the result equals Mana restored per second.
 */
UCLASS()
class ALS_PROJECTHUNTER_API UHunterMMC_ManaRegen : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

public:
	UHunterMMC_ManaRegen();

	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;
};
