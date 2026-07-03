#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "HunterMMC_ArcaneShieldRegen.generated.h"

/**
 * Computes arcane shield regeneration per second from
 * ArcaneShieldRegenRate * ArcaneShieldRegenAmount. The native GE scales this
 * value by its period for smooth updates.
 */
UCLASS()
class ALS_PROJECTHUNTER_API UHunterMMC_ArcaneShieldRegen : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

public:
	UHunterMMC_ArcaneShieldRegen();

	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;
};
