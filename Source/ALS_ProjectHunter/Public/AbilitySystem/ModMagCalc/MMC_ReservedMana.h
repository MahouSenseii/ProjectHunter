// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "MMC_ReservedMana.generated.h"

/**
 * 
 */
UCLASS()
class ALS_PROJECTHUNTER_API UMMC_ReservedMana : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

public:

	UMMC_ReservedMana();

	
	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;

private:

	FGameplayEffectAttributeCaptureDefinition ManaFlatDef;
	FGameplayEffectAttributeCaptureDefinition ManaPercentageDef;
	FGameplayEffectAttributeCaptureDefinition MaxManaDef;
	
};
