#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "HunterMMC_ManaRegen.generated.h"

/**
 * Computes mana regeneration per second from ManaRegenRate * ManaRegenAmount.
 * The native GE scales this value by its period for smooth updates.
 */
UCLASS()
class ALS_PROJECTHUNTER_API UHunterMMC_ManaRegen : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

public:
	UHunterMMC_ManaRegen();

	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;
};
