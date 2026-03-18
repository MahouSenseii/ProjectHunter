#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "HunterMMC_StrengthMaxHealth.generated.h"

/**
 * MaxHealth = 100 + (5 + Strength) + (12 * (PlayerLevel - 1))
 */
UCLASS()
class ALS_PROJECTHUNTER_API UHunterMMC_StrengthMaxHealth : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

public:
	UHunterMMC_StrengthMaxHealth();

	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;
};
