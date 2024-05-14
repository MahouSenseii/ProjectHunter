// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/ModMagCalc/MMC_MaxEffectiveHealth.h"

#include "AbilitySystem/PHAttributeSet.h"

UMMC_MaxEffectiveHealth::UMMC_MaxEffectiveHealth()
{
	MaxHealthDef.AttributeToCapture = UPHAttributeSet::GetMaxHealthAttribute();
	MaxHealthDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
	MaxHealthDef.bSnapshot = false;

	ReservedAmountDef.AttributeToCapture = UPHAttributeSet::GetReservedHealthAttribute();
	ReservedAmountDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
	ReservedAmountDef.bSnapshot = false;

	RelevantAttributesToCapture.Add(ReservedAmountDef);
	RelevantAttributesToCapture.Add(MaxHealthDef);
}

float UMMC_MaxEffectiveHealth::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
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

	
	
	GetCapturedAttributeMagnitude(MaxHealthDef, Spec, EvaluateParameters, MaxHealth);
	MaxHealth = FMath::Max(MaxHealth, 0.0f);

	return MaxHealth - ReservedAmount ;
}
