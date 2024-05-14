// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/ModMagCalc/MMC_MaxMana.h"

#include "AbilitySystem/PHAttributeSet.h"
#include "Interfaces/CombatSubInterface.h"

UMMC_MaxMana::UMMC_MaxMana()
{
	IntelligenceDef.AttributeToCapture = UPHAttributeSet::GetIntelligenceAttribute();
	IntelligenceDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
	IntelligenceDef.bSnapshot = false;

	ReservedDef.AttributeToCapture = UPHAttributeSet::GetReservedManaAttribute();
	ReservedDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
	ReservedDef.bSnapshot = false;
	
	RelevantAttributesToCapture.Add(IntelligenceDef);
	RelevantAttributesToCapture.Add(ReservedDef);
}

float UMMC_MaxMana::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	//Gather tags from source and target
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvaluateParameters;
	EvaluateParameters.SourceTags = SourceTags;
	EvaluateParameters.TargetTags = TargetTags;
	float Intelligence= 0.f;
	float Reserved = 0.f;
	GetCapturedAttributeMagnitude(IntelligenceDef, Spec, EvaluateParameters,  Intelligence);
	Intelligence = FMath::Max( Intelligence, 0.0f);

	GetCapturedAttributeMagnitude(ReservedDef, Spec, EvaluateParameters,  Reserved);
	Reserved = FMath::Max( Reserved, 0.0f);
	

	ICombatSubInterface* CombatSubInterface =  Cast<ICombatSubInterface>(Spec.GetContext().GetSourceObject());
	const int32 PlayerLevel = CombatSubInterface->GetPlayerLevel();

	// Base hp 100 +( 5 * Intelligence) + ( 6 * PlayerLevel )
	//PrimaryAttributes start at 0 so players will always start with 100 Mana
	return (100.0f + ( 5.f *   Intelligence) + ((6.f * ( 1 - PlayerLevel)) - Reserved));
}
