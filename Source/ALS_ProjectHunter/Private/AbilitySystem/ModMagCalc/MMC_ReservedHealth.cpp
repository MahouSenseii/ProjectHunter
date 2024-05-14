// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/ModMagCalc/MMC_ReservedHealth.h"

#include "AbilitySystem/PHAttributeSet.h"
#include "Kismet/KismetMathLibrary.h"

UMMC_ReserveHealth::UMMC_ReserveHealth()
{
	HealthFlatDef.AttributeToCapture = UPHAttributeSet::GetFlatReservedHealthAttribute();
	HealthFlatDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
	HealthFlatDef.bSnapshot = false;

	HealthPercentageDef.AttributeToCapture = UPHAttributeSet::GetPercentageReservedHealthAttribute();
	HealthPercentageDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
	HealthPercentageDef.bSnapshot = false;



	
	MaxHealthDef.AttributeToCapture = UPHAttributeSet::GetMaxHealthAttribute();
	MaxHealthDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
	MaxHealthDef.bSnapshot = false;
	
	
	RelevantAttributesToCapture.Add(HealthFlatDef);
	RelevantAttributesToCapture.Add(HealthPercentageDef);
	RelevantAttributesToCapture.Add(MaxHealthDef);
}

float UMMC_ReserveHealth::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
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
	GetCapturedAttributeMagnitude(HealthFlatDef, Spec, EvaluateParameters, FlatValue);
	FlatValue = FMath::Max(FlatValue, 0.0f);

	GetCapturedAttributeMagnitude(MaxHealthDef, Spec, EvaluateParameters, MaxValue);
	MaxValue = FMath::Max(MaxValue, 0.0f);
	
	GetCapturedAttributeMagnitude(HealthPercentageDef, Spec, EvaluateParameters, PercentageValue);
	PercentageValue = FMath::Max(PercentageValue, 0.0f);

	// Calculate the total reserved health, combining both flat and percentage values relative to max health
	// Get max current health and add current reserved health so i can get the true max hp
	// then times by percentageValue  and add flat value 
	const float ReservedHealth = ((MaxValue * PercentageValue) + FlatValue);
	return FMath::RoundHalfToEven(ReservedHealth);
}
