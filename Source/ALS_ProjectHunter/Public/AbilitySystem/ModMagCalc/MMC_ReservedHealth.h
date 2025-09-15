
#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "MMC_ReservedHealth.generated.h"

/**
 * 
 */
UCLASS()
class ALS_PROJECTHUNTER_API UMMC_ReserveHealth : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

public:

	UMMC_ReserveHealth();

	
	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;

private:

	mutable FGameplayEffectAttributeCaptureDefinition HealthFlatDef;
	mutable FGameplayEffectAttributeCaptureDefinition HealthPercentageDef;
	mutable FGameplayEffectAttributeCaptureDefinition MaxHealthDef;
	mutable FGameplayEffectAttributeCaptureDefinition ReservedHealthDef;

};
