#include "AbilitySystem/ModMagnitude/HunterMMC_StaminaRegen.h"

#include "AbilitySystem/HunterAttributeSet.h"
#include "GameplayEffectExtension.h"

namespace HunterMMCStaminaRegenPrivate
{
	struct FCaptureDefinitions
	{
		FGameplayEffectAttributeCaptureDefinition StaminaRegenRateDef;
		FGameplayEffectAttributeCaptureDefinition StaminaRegenAmountDef;

		FCaptureDefinitions()
			: StaminaRegenRateDef(UHunterAttributeSet::GetStaminaRegenRateAttribute(),   EGameplayEffectAttributeCaptureSource::Target, false)
			, StaminaRegenAmountDef(UHunterAttributeSet::GetStaminaRegenAmountAttribute(), EGameplayEffectAttributeCaptureSource::Target, false)
		{
		}
	};

	static const FCaptureDefinitions& GetCaptureDefinitions()
	{
		static const FCaptureDefinitions Definitions;
		return Definitions;
	}
}

UHunterMMC_StaminaRegen::UHunterMMC_StaminaRegen()
{
	RelevantAttributesToCapture.Add(HunterMMCStaminaRegenPrivate::GetCaptureDefinitions().StaminaRegenRateDef);
	RelevantAttributesToCapture.Add(HunterMMCStaminaRegenPrivate::GetCaptureDefinitions().StaminaRegenAmountDef);
}

float UHunterMMC_StaminaRegen::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = SourceTags;
	EvaluationParameters.TargetTags = TargetTags;

	const auto& Defs = HunterMMCStaminaRegenPrivate::GetCaptureDefinitions();

	float Rate   = 0.f;
	float Amount = 0.f;

	GetCapturedAttributeMagnitude(Defs.StaminaRegenRateDef,   Spec, EvaluationParameters, Rate);
	GetCapturedAttributeMagnitude(Defs.StaminaRegenAmountDef, Spec, EvaluationParameters, Amount);

	// Magnitude = Rate * Amount per GE Period.
	// With Period = 1.0 s this equals Stamina restored per second.
	return FMath::Max(Rate, 0.f) * FMath::Max(Amount, 0.f);
}
