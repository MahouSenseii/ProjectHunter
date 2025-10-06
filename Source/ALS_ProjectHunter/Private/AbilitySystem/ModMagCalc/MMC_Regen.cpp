// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/ModMagCalc/MMC_Regen.h"

#include "AbilitySystem/PHAttributeSet.h"
#include "Library/PHCharacterEnumLibrary.h"

UMMC_Regen::UMMC_Regen()
{
	RegenType = EVitalRegenType::Health;

	// Define capture for Health regen amount
	HealthRegenAmountDef.AttributeToCapture = UPHAttributeSet::GetHealthRegenAmountAttribute();
	HealthRegenAmountDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
	HealthRegenAmountDef.bSnapshot = false;

	// Define capture for Mana regen amount
	ManaRegenAmountDef.AttributeToCapture = UPHAttributeSet::GetManaRegenAmountAttribute();
	ManaRegenAmountDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
	ManaRegenAmountDef.bSnapshot = false;

	// Define capture for Stamina regen amount
	StaminaRegenAmountDef.AttributeToCapture = UPHAttributeSet::GetStaminaRegenAmountAttribute();
	StaminaRegenAmountDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
	StaminaRegenAmountDef.bSnapshot = false;

	// Define capture for ArcaneShield regen amount
	ArcaneShieldRegenAmountDef.AttributeToCapture = UPHAttributeSet::GetArcaneShieldRegenAmountAttribute();
	ArcaneShieldRegenAmountDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
	ArcaneShieldRegenAmountDef.bSnapshot = false;

	// Add relevant captures
	RelevantAttributesToCapture.Add(HealthRegenAmountDef);
	RelevantAttributesToCapture.Add(ManaRegenAmountDef);
	RelevantAttributesToCapture.Add(StaminaRegenAmountDef);
	RelevantAttributesToCapture.Add(ArcaneShieldRegenAmountDef);
}

float UMMC_Regen::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = SourceTags;
	EvaluationParameters.TargetTags = TargetTags;

	float RegenAmount = 0.0f;

	// Get the appropriate regen amount based on type
	switch (RegenType)
	{
	case EVitalRegenType::Health:
		GetCapturedAttributeMagnitude(HealthRegenAmountDef, Spec, EvaluationParameters, RegenAmount);
		break;
	case EVitalRegenType::Mana:
		GetCapturedAttributeMagnitude(ManaRegenAmountDef, Spec, EvaluationParameters, RegenAmount);
		break;
	case EVitalRegenType::Stamina:
		GetCapturedAttributeMagnitude(StaminaRegenAmountDef, Spec, EvaluationParameters, RegenAmount);
		break;
	case EVitalRegenType::ArcaneShield:
		GetCapturedAttributeMagnitude(ArcaneShieldRegenAmountDef, Spec, EvaluationParameters, RegenAmount);
		break;
	}

	return RegenAmount;
}
