#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "HunterMMC_EnduranceStaminaDegen.generated.h"

/**
 * Returns the Endurance-driven stamina degeneration multiplier.
 * One Endurance reduces drain by 1%, capped at 75% reduction.
 */
UCLASS()
class ALS_PROJECTHUNTER_API UHunterMMC_EnduranceStaminaDegen : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

public:
	UHunterMMC_EnduranceStaminaDegen();

	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;
};
