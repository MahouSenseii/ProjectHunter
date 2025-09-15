// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "MMC_MaxEffectiveStamina.generated.h"

/**
 * 
 */
UCLASS()
class ALS_PROJECTHUNTER_API UMMC_MaxEffectiveStamina : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

public:
	UMMC_MaxEffectiveStamina();

	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;

private:

	mutable FGameplayEffectAttributeCaptureDefinition MaxStaminaDef;
	mutable FGameplayEffectAttributeCaptureDefinition ReservedAmountDef;

};
