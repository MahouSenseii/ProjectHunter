#include "AbilitySystem/ModMagnitude/HunterMMC_EnduranceStaminaDegen.h"

#include "AbilitySystem/HunterAttributeSet.h"
#include "GameplayEffectExtension.h"
#include "Stats/Library/FunctionLibraries/PrimaryAttributeRules.h"

namespace HunterMMCEnduranceStaminaDegenPrivate
{
	struct FCaptureDefinitions
	{
		FGameplayEffectAttributeCaptureDefinition EnduranceDef;

		FCaptureDefinitions()
			: EnduranceDef(UHunterAttributeSet::GetEnduranceAttribute(), EGameplayEffectAttributeCaptureSource::Target, false)
		{
		}
	};

	static const FCaptureDefinitions& GetCaptureDefinitions()
	{
		static const FCaptureDefinitions Definitions;
		return Definitions;
	}
}

UHunterMMC_EnduranceStaminaDegen::UHunterMMC_EnduranceStaminaDegen()
{
	RelevantAttributesToCapture.Add(HunterMMCEnduranceStaminaDegenPrivate::GetCaptureDefinitions().EnduranceDef);
}

float UHunterMMC_EnduranceStaminaDegen::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = SourceTags;
	EvaluationParameters.TargetTags = TargetTags;

	float Endurance = 0.0f;
	GetCapturedAttributeMagnitude(
		HunterMMCEnduranceStaminaDegenPrivate::GetCaptureDefinitions().EnduranceDef,
		Spec,
		EvaluationParameters,
		Endurance);

	return FPrimaryAttributeRules::Resolve(0.f, 0.f, 0.f, Endurance, 0.f, 0.f, 0.f)
		.StaminaDegenMultiplier;
}
