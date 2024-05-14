// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/ModMagCalc/MMC_ReservedStamina.h"

#include "AbilitySystem/PHAttributeSet.h"
#include "Kismet/KismetMathLibrary.h"


UMMC_ReservedStamina::UMMC_ReservedStamina()
{
	StaminaFlatDef.AttributeToCapture = UPHAttributeSet::GetFlatReservedStaminaAttribute();
	StaminaFlatDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
	StaminaFlatDef.bSnapshot = false;

	StaminaPercentageDef.AttributeToCapture = UPHAttributeSet::GetPercentageReservedStaminaAttribute();
	StaminaPercentageDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
	StaminaPercentageDef.bSnapshot = false;

	MaxStaminaDef.AttributeToCapture = UPHAttributeSet::GetMaxStaminaAttribute();
	MaxStaminaDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
	MaxStaminaDef.bSnapshot = false;
	
	RelevantAttributesToCapture.Add(StaminaFlatDef);
	RelevantAttributesToCapture.Add(StaminaPercentageDef);
	RelevantAttributesToCapture.Add(MaxStaminaDef);
}

float UMMC_ReservedStamina::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	// Gather tags from source and target for conditional logic if necessary
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvaluateParameters;
	EvaluateParameters.SourceTags = SourceTags;
	EvaluateParameters.TargetTags = TargetTags;

	// Initialize variables to hold attribute magnitudes
	float FlatValue = 0.0f;
	float PercentageValue = 0.0f;
	float MaxValue = 0.0f;

	// Retrieve the magnitude of each captured attribute, ensuring non-negative values
	GetCapturedAttributeMagnitude(StaminaFlatDef, Spec, EvaluateParameters, FlatValue);
	FlatValue = FMath::Max(FlatValue, 0.0f);

	GetCapturedAttributeMagnitude(MaxStaminaDef, Spec, EvaluateParameters, MaxValue);
	MaxValue = FMath::Max(MaxValue, 0.0f);

	GetCapturedAttributeMagnitude(StaminaPercentageDef, Spec, EvaluateParameters, PercentageValue);
	PercentageValue = FMath::Max(PercentageValue, 0.0f);

	// Calculate the total reserved stamina, combining both flat and percentage values relative to max stamina
	float CalculatedStamina = (MaxValue * PercentageValue) + FlatValue;

	// Implement custom rounding logic if necessary. Below rounds up on 0.5 exactly.
	float RoundedStamina = floor(CalculatedStamina + 0.5f);

	return RoundedStamina;
}


