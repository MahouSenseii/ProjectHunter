#include "AbilitySystem/ModMagnitude/HunterMMC_HealthRegen.h"

#include "AbilitySystem/HunterAttributeSet.h"
#include "AbilitySystem/Library/FunctionLibraries/PHResourceFunctionLibrary.h"
#include "GameplayEffectExtension.h"
#include "PHGameplayTags.h"

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
	const FPHGameplayTags& Tags = FPHGameplayTags::Get();
	if (TargetTags && TargetTags->HasTagExact(Tags.Condition_Self_CannotRegenHP))
	{
		return 0.f;
	}

	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = SourceTags;
	EvaluationParameters.TargetTags = TargetTags;

	const auto& Defs = HunterMMCHealthRegenPrivate::GetCaptureDefinitions();

	float Rate   = 0.f;
	float Amount = 0.f;

	GetCapturedAttributeMagnitude(Defs.HealthRegenRateDef,   Spec, EvaluationParameters, Rate);
	GetCapturedAttributeMagnitude(Defs.HealthRegenAmountDef, Spec, EvaluationParameters, Amount);

	// Per-second value. The GE coefficient scales this by its tick period.
	return UPHResourceFunctionLibrary::CalculateResourceFlowAmount(Rate, Amount);
}
