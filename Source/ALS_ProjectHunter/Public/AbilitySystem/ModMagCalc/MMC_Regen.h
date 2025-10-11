// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "MMC_Regen.generated.h"

enum class EVitalRegenType : uint8;
/**
 * 
 */
UCLASS()
class ALS_PROJECTHUNTER_API UMMC_Regen : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()
	
public:

	UMMC_Regen();

	/** Which regen amount attribute to read from */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Regen")
	EVitalRegenType RegenType;

	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;

private:
	FGameplayEffectAttributeCaptureDefinition HealthRegenAmountDef;
	FGameplayEffectAttributeCaptureDefinition ManaRegenAmountDef;
	FGameplayEffectAttributeCaptureDefinition StaminaRegenAmountDef;
	FGameplayEffectAttributeCaptureDefinition ArcaneShieldRegenAmountDef;

	// Rate captures (NEW)
	FGameplayEffectAttributeCaptureDefinition HealthRegenRateDef;
	FGameplayEffectAttributeCaptureDefinition ManaRegenRateDef;
	FGameplayEffectAttributeCaptureDefinition StaminaRegenRateDef;
	FGameplayEffectAttributeCaptureDefinition ArcaneShieldRegenRateDef;
};
