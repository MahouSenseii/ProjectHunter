#include "AbilitySystem/ModMagnitude/HunterMMC_IntelligenceManaRegen.h"

#include "AbilitySystem/HunterAttributeSet.h"
#include "GameplayEffectExtension.h"

namespace HunterMMCIntelligenceManaRegenPrivate
{
	struct FCaptureDefinitions
	{
		FGameplayEffectAttributeCaptureDefinition IntelligenceDef;

		FCaptureDefinitions()
			: IntelligenceDef(UHunterAttributeSet::GetIntelligenceAttribute(), EGameplayEffectAttributeCaptureSource::Target, false)
		{
		}
	};

	static const FCaptureDefinitions& GetCaptureDefinitions()
	{
		static const FCaptureDefinitions Definitions;
		return Definitions;
	}
}

UHunterMMC_IntelligenceManaRegen::UHunterMMC_IntelligenceManaRegen()
{
	RelevantAttributesToCapture.Add(HunterMMCIntelligenceManaRegenPrivate::GetCaptureDefinitions().IntelligenceDef);
}

float UHunterMMC_IntelligenceManaRegen::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = SourceTags;
	EvaluationParameters.TargetTags = TargetTags;

	float Intelligence = 0.0f;
	GetCapturedAttributeMagnitude(
		HunterMMCIntelligenceManaRegenPrivate::GetCaptureDefinitions().IntelligenceDef,
		Spec,
		EvaluationParameters,
		Intelligence);

	return FMath::Max(Intelligence, 0.0f);
}
