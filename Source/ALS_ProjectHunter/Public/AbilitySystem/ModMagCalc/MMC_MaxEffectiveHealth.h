// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "MMC_MaxEffectiveHealth.generated.h"

/**
 * 
 */
UCLASS()
class ALS_PROJECTHUNTER_API UMMC_MaxEffectiveHealth : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

public:
	UMMC_MaxEffectiveHealth();
	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;

private:
	mutable FGameplayEffectAttributeCaptureDefinition MaxHealthDef;
	mutable FGameplayEffectAttributeCaptureDefinition ReservedAmountDef;

};
