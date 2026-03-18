#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "HunterMMC_IntelligenceManaRegen.generated.h"

/**
 * Returns the Intelligence-driven mana regeneration bonus.
 */
UCLASS()
class ALS_PROJECTHUNTER_API UHunterMMC_IntelligenceManaRegen : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

public:
	UHunterMMC_IntelligenceManaRegen();

	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;
};
