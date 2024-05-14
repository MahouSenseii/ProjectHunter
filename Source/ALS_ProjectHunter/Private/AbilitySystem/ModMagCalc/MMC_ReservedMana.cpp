// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/ModMagCalc/MMC_ReservedMana.h"

#include "AbilitySystem/PHAttributeSet.h"
#include "Kismet/KismetMathLibrary.h"

UMMC_ReservedMana::UMMC_ReservedMana()
{
	ManaFlatDef.AttributeToCapture = UPHAttributeSet::GetFlatReservedManaAttribute();
	ManaFlatDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
	ManaFlatDef.bSnapshot = false;

	ManaPercentageDef.AttributeToCapture = UPHAttributeSet::GetPercentageReservedManaAttribute();
	ManaPercentageDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
	ManaPercentageDef.bSnapshot = false;

	MaxManaDef.AttributeToCapture = UPHAttributeSet::GetMaxManaAttribute();
	MaxManaDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
	MaxManaDef.bSnapshot = false;
	
	RelevantAttributesToCapture.Add(ManaFlatDef);
	RelevantAttributesToCapture.Add(ManaPercentageDef);
	RelevantAttributesToCapture.Add(MaxManaDef);
}

float UMMC_ReservedMana::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
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
	GetCapturedAttributeMagnitude(ManaFlatDef, Spec, EvaluateParameters, FlatValue);
	FlatValue = FMath::Max(FlatValue, 0.0f);
    
	GetCapturedAttributeMagnitude(MaxManaDef, Spec, EvaluateParameters, MaxValue);
	MaxValue = FMath::Max(MaxValue, 0.0f);
    
	GetCapturedAttributeMagnitude(ManaPercentageDef, Spec, EvaluateParameters, PercentageValue);
	PercentageValue = FMath::Max(PercentageValue, 0.0f);
    
	// Calculate the total reserved stamina, combining both flat and percentage values relative to max stamina
	return UKismetMathLibrary::Round((MaxValue * PercentageValue) + FlatValue);
}
