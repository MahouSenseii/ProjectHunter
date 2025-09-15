// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "MMC_MaxEffectiveMana.generated.h"

/**
 * 
 */
UCLASS()
class ALS_PROJECTHUNTER_API UMMC_MaxEffectiveMana : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

public:
	UMMC_MaxEffectiveMana();

	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;

private:


	mutable FGameplayEffectAttributeCaptureDefinition MaxManaDef;
	mutable FGameplayEffectAttributeCaptureDefinition ReservedAmountDef;

};
