#include "AbilitySystem/ModMagnitude/HunterMMC_StaminaDegen.h"

#include "AbilitySystem/HunterAttributeSet.h"
#include "GameplayEffectExtension.h"

namespace HunterMMCStaminaDegenPrivate
{
	struct FCaptureDefinitions
	{
		FGameplayEffectAttributeCaptureDefinition StaminaDegenRateDef;
		FGameplayEffectAttributeCaptureDefinition StaminaDegenAmountDef;

		FCaptureDefinitions()
			: StaminaDegenRateDef(UHunterAttributeSet::GetStaminaDegenRateAttribute(),     EGameplayEffectAttributeCaptureSource::Target, false)
			, StaminaDegenAmountDef(UHunterAttributeSet::GetStaminaDegenAmountAttribute(), EGameplayEffectAttributeCaptureSource::Target, false)
		{
		}
	};

	static const FCaptureDefinitions& GetCaptureDefinitions()
	{
		static const FCaptureDefinitions Definitions;
		return Definitions;
	}
}

UHunterMMC_StaminaDegen::UHunterMMC_StaminaDegen()
{
	RelevantAttributesToCapture.Add(HunterMMCStaminaDegenPrivate::GetCaptureDefinitions().StaminaDegenRateDef);
	RelevantAttributesToCapture.Add(HunterMMCStaminaDegenPrivate::GetCaptureDefinitions().StaminaDegenAmountDef);
}

float UHunterMMC_StaminaDegen::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = SourceTags;
	EvaluationParameters.TargetTags = TargetTags;

	const auto& Defs = HunterMMCStaminaDegenPrivate::GetCaptureDefinitions();

	float Rate   = 0.f;
	float Amount = 0.f;

	GetCapturedAttributeMagnitude(Defs.StaminaDegenRateDef,   Spec, EvaluationParameters, Rate);
	GetCapturedAttributeMagnitude(Defs.StaminaDegenAmountDef, Spec, EvaluationParameters, Amount);

	// Negative so an Additive periodic modifier drains Stamina. The GE coefficient
	// scales this per-second value by the tick period for smoother updates.
	return -(FMath::Max(Rate, 0.f) * FMath::Max(Amount, 0.f));
}
