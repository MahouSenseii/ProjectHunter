// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/ModMagCalc/MMC_MaxEffectiveMana.h"

#include "AbilitySystem/PHAttributeSet.h"

UMMC_MaxEffectiveMana::UMMC_MaxEffectiveMana()
{
	MaxManaDef.AttributeToCapture = UPHAttributeSet::GetMaxManaAttribute();
	MaxManaDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
	MaxManaDef.bSnapshot = false;

	ReservedAmountDef.AttributeToCapture = UPHAttributeSet::GetReservedManaAttribute();
	ReservedAmountDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
	ReservedAmountDef.bSnapshot = false;

	RelevantAttributesToCapture.Add(ReservedAmountDef);
	RelevantAttributesToCapture.Add(MaxManaDef);
}

float UMMC_MaxEffectiveMana::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	//Gather tags from source and target
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvaluateParameters;
	EvaluateParameters.SourceTags = SourceTags;
	EvaluateParameters.TargetTags = TargetTags;

	float MaxHealth = 0.0f;
	float ReservedAmount = 0.0f;

	GetCapturedAttributeMagnitude(ReservedAmountDef, Spec, EvaluateParameters, ReservedAmount);
	ReservedAmount = FMath::Max(ReservedAmount, 0.0f);

	
	
	GetCapturedAttributeMagnitude(MaxManaDef, Spec, EvaluateParameters, MaxHealth);
	MaxHealth = FMath::Max(MaxHealth, 0.0f);

	return MaxHealth - ReservedAmount ;
}

