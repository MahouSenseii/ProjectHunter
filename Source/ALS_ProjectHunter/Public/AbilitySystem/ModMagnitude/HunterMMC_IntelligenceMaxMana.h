#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "HunterMMC_IntelligenceMaxMana.generated.h"

/** Calculates max mana from Intelligence and PlayerLevel. */
UCLASS()
class ALS_PROJECTHUNTER_API UHunterMMC_IntelligenceMaxMana : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

public:
	UHunterMMC_IntelligenceMaxMana();

	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;
};
