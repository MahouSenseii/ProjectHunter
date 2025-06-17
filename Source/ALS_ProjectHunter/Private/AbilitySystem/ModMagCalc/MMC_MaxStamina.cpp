// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/ModMagCalc/MMC_MaxStamina.h"

#include "AbilitySystem/PHAttributeSet.h"
#include "Interfaces/CombatSubInterface.h"
//check
class ICombatSubInterface;

UMMC_MaxStamina::UMMC_MaxStamina()
{
	 EnduranceDef.AttributeToCapture = UPHAttributeSet::GetEnduranceAttribute();
	 EnduranceDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
	 EnduranceDef.bSnapshot = false;

	ReservedDef.AttributeToCapture = UPHAttributeSet::GetReservedStaminaAttribute();
	ReservedDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
	ReservedDef.bSnapshot = false;

	RelevantAttributesToCapture.Add( EnduranceDef);
	RelevantAttributesToCapture.Add(ReservedDef);
}

float UMMC_MaxStamina::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	//Gather tags from source and target
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvaluateParameters;
	EvaluateParameters.SourceTags = SourceTags;
	EvaluateParameters.TargetTags = TargetTags;
	
	float Endurance = 0.f;
	float Reserved = 0.f;
	
	GetCapturedAttributeMagnitude( EnduranceDef, Spec, EvaluateParameters,  Endurance);
	Endurance = FMath::Max( Endurance, 0.0f);
	
	GetCapturedAttributeMagnitude(ReservedDef, Spec, EvaluateParameters,  Reserved);
	Reserved = FMath::Max( Reserved, 0.0f);
	
	ICombatSubInterface* CombatSubInterface =  Cast<ICombatSubInterface>(Spec.GetContext().GetSourceObject());
	const int32 PlayerLevel = CombatSubInterface->GetPlayerLevel();

	// Base hp 100 +( 5 * Endurance) + ( 6 * PlayerLevel )
	//PrimaryAttributes start at 0 so players will always start with 100 Stamina
	return (100.0f + ( 5.f *   Endurance) + (6.f *  (  PlayerLevel - 1)) - Reserved);
	
}
