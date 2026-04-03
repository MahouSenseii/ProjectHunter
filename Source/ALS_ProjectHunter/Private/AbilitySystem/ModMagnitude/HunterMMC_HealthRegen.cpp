#include "AbilitySystem/ModMagnitude/HunterMMC_HealthRegen.h"

#include "AbilitySystem/HunterAttributeSet.h"
#include "GameplayEffectExtension.h"

namespace HunterMMCHealthRegenPrivate
{
	struct FCaptureDefinitions
	{
		FGameplayEffectAttributeCaptureDefinition HealthRegenRateDef;
		FGameplayEffectAttributeCaptureDefinition HealthRegenAmountDef;

		FCaptureDefinitions()
			: HealthRegenRateDef(UHunterAttributeSet::GetHealthRegenRateAttribute(),   EGameplayEffectAttributeCaptureSource::Target, false)
			, HealthRegenAmountDef(UHunterAttributeSet::GetHealthRegenAmountAttribute(), EGameplayEffectAttributeCaptureSource::Target, false)
		{
		}
	};

	static const FCaptureDefinitions& GetCaptureDefinitions()
	{
		static const FCaptureDefinitions Definitions;
		return Definitions;
	}
}

UHunterMMC_HealthRegen::UHunterMMC_HealthRegen()
{
	RelevantAttributesToCapture.Add(HunterMMCHealthRegenPrivate::GetCaptureDefinitions().HealthRegenRateDef);
	RelevantAttributesToCapture.Add(HunterMMCHealthRegenPrivate::GetCaptureDefinitions().HealthRegenAmountDef);
}

float UHunterMMC_HealthRegen::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = SourceTags;
	EvaluationParameters.TargetTags = TargetTags;

	const auto& Defs = HunterMMCHealthRegenPrivate::GetCaptureDefinitions();

	float Rate   = 0.f;
	float Amount = 0.f;

	GetCapturedAttributeMagnitude(Defs.HealthRegenRateDef,   Spec, EvaluationParameters, Rate);
	GetCapturedAttributeMagnitude(Defs.HealthRegenAmountDef, Spec, EvaluationParameters, Amount);

	// Magnitude = Rate * Amount per GE Period.
	// With Period = 1.0 s this equals Health restored per second.
	return FMath::Max(Rate, 0.f) * FMath::Max(Amount, 0.f);
}
