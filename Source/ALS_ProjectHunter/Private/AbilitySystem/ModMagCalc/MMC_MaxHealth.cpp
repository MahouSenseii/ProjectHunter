// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/ModMagCalc/MMC_MaxHealth.h"

#include "AbilitySystem/PHAttributeSet.h"
#include "Interfaces/CombatSubInterface.h"

UMMC_MaxHealth::UMMC_MaxHealth()
{
	StrengthDef.AttributeToCapture = UPHAttributeSet::GetStrengthAttribute();
	StrengthDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
	StrengthDef.bSnapshot = false;
	
	RelevantAttributesToCapture.Add(StrengthDef);
}

float UMMC_MaxHealth::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	//Gather tags from source and target
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvaluateParameters;
	EvaluateParameters.SourceTags = SourceTags;
	EvaluateParameters.TargetTags = TargetTags;
	float Strength = 0.f;
	
	GetCapturedAttributeMagnitude(StrengthDef, Spec, EvaluateParameters, Strength);
	Strength = FMath::Max(Strength, 0.0f);
	
	ICombatSubInterface* CombatSubInterface =  Cast<ICombatSubInterface>(Spec.GetContext().GetSourceObject());
	const int32 PlayerLevel = CombatSubInterface->GetPlayerLevel();

	// Base hp 100 +( 5 * Strength) + ( 6 * ( 1 - PlayerLevel) )
	//PrimaryAttributes start at 0 so players will always start with 100 Health
	
	return (100.0f + (5.f *   Strength) + (12.f *  ( PlayerLevel - 1)));
}
