
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

	FGameplayEffectAttributeCaptureDefinition HealthFlatDef;
	FGameplayEffectAttributeCaptureDefinition HealthPercentageDef;
	FGameplayEffectAttributeCaptureDefinition MaxHealthDef;
	FGameplayEffectAttributeCaptureDefinition ReservedHealthDef;
};
