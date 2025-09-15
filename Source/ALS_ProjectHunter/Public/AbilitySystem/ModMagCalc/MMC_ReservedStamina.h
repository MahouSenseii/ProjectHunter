// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "MMC_ReservedStamina.generated.h"

/**
 * 
 */
UCLASS()
class ALS_PROJECTHUNTER_API UMMC_ReservedStamina : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

public:

	UMMC_ReservedStamina();

	
	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;

private:
	
	mutable FGameplayEffectAttributeCaptureDefinition StaminaFlatDef;
	mutable FGameplayEffectAttributeCaptureDefinition StaminaPercentageDef;
	mutable FGameplayEffectAttributeCaptureDefinition MaxStaminaDef;
	mutable FGameplayEffectAttributeCaptureDefinition ReservedStaminaDef;


};
