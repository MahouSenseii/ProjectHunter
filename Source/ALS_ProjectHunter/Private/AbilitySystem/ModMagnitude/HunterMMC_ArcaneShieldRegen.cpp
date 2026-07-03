#include "AbilitySystem/ModMagnitude/HunterMMC_ArcaneShieldRegen.h"

#include "AbilitySystem/HunterAttributeSet.h"
#include "GameplayEffectExtension.h"

namespace HunterMMCArcaneShieldRegenPrivate
{
	struct FCaptureDefinitions
	{
		FGameplayEffectAttributeCaptureDefinition RegenRateDef;
		FGameplayEffectAttributeCaptureDefinition RegenAmountDef;

		FCaptureDefinitions()
			: RegenRateDef(UHunterAttributeSet::GetArcaneShieldRegenRateAttribute(),     EGameplayEffectAttributeCaptureSource::Target, false)
			, RegenAmountDef(UHunterAttributeSet::GetArcaneShieldRegenAmountAttribute(), EGameplayEffectAttributeCaptureSource::Target, false)
		{
		}
	};

	static const FCaptureDefinitions& GetCaptureDefinitions()
	{
		static const FCaptureDefinitions Definitions;
		return Definitions;
	}
}

UHunterMMC_ArcaneShieldRegen::UHunterMMC_ArcaneShieldRegen()
{
	RelevantAttributesToCapture.Add(HunterMMCArcaneShieldRegenPrivate::GetCaptureDefinitions().RegenRateDef);
	RelevantAttributesToCapture.Add(HunterMMCArcaneShieldRegenPrivate::GetCaptureDefinitions().RegenAmountDef);
}

float UHunterMMC_ArcaneShieldRegen::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = SourceTags;
	EvaluationParameters.TargetTags = TargetTags;

	const auto& Defs = HunterMMCArcaneShieldRegenPrivate::GetCaptureDefinitions();

	float Rate   = 0.f;
	float Amount = 0.f;

	GetCapturedAttributeMagnitude(Defs.RegenRateDef,   Spec, EvaluationParameters, Rate);
	GetCapturedAttributeMagnitude(Defs.RegenAmountDef, Spec, EvaluationParameters, Amount);

	// Per-second value. The GE coefficient scales this by its tick period.
	return FMath::Max(Rate, 0.f) * FMath::Max(Amount, 0.f);
}
