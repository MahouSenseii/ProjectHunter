#include "AbilitySystem/ModMagnitude/HunterMMC_StaminaRegen.h"

#include "AbilitySystem/HunterAttributeSet.h"
#include "GameplayEffectExtension.h"
#include "PHGameplayTags.h"

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
	const FPHGameplayTags& Tags = FPHGameplayTags::Get();
	if (TargetTags &&
		(TargetTags->HasTagExact(Tags.Condition_Self_CannotRegenStamina) ||
			TargetTags->HasTagExact(Tags.Effect_Stamina_Exhausted)))
	{
		return 0.f;
	}

	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = SourceTags;
	EvaluationParameters.TargetTags = TargetTags;

	const auto& Defs = HunterMMCStaminaRegenPrivate::GetCaptureDefinitions();

	float Rate   = 0.f;
	float Amount = 0.f;

	GetCapturedAttributeMagnitude(Defs.StaminaRegenRateDef,   Spec, EvaluationParameters, Rate);
	GetCapturedAttributeMagnitude(Defs.StaminaRegenAmountDef, Spec, EvaluationParameters, Amount);

	// Per-second value. The GE coefficient scales this by its tick period.
	return FMath::Max(Rate, 0.f) * FMath::Max(Amount, 0.f);
}
