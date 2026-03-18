#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "HunterMMC_EnduranceMaxStamina.generated.h"

/**
 * MaxStamina = 100 + (5 + Endurance) + (12 * (PlayerLevel - 1))
 */
UCLASS()
class ALS_PROJECTHUNTER_API UHunterMMC_EnduranceMaxStamina : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

public:
	UHunterMMC_EnduranceMaxStamina();

	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;
};
