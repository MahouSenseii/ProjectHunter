#include "AbilitySystem/ModMagnitude/HunterMMC_ManaRegen.h"

#include "AbilitySystem/HunterAttributeSet.h"
#include "GameplayEffectExtension.h"
#include "PHGameplayTags.h"

namespace HunterMMCManaRegenPrivate
{
	struct FCaptureDefinitions
	{
		FGameplayEffectAttributeCaptureDefinition ManaRegenRateDef;
		FGameplayEffectAttributeCaptureDefinition ManaRegenAmountDef;

		FCaptureDefinitions()
			: ManaRegenRateDef(UHunterAttributeSet::GetManaRegenRateAttribute(),   EGameplayEffectAttributeCaptureSource::Target, false)
			, ManaRegenAmountDef(UHunterAttributeSet::GetManaRegenAmountAttribute(), EGameplayEffectAttributeCaptureSource::Target, false)
		{
		}
	};

	static const FCaptureDefinitions& GetCaptureDefinitions()
	{
		static const FCaptureDefinitions Definitions;
		return Definitions;
	}
}

UHunterMMC_ManaRegen::UHunterMMC_ManaRegen()
{
	RelevantAttributesToCapture.Add(HunterMMCManaRegenPrivate::GetCaptureDefinitions().ManaRegenRateDef);
	RelevantAttributesToCapture.Add(HunterMMCManaRegenPrivate::GetCaptureDefinitions().ManaRegenAmountDef);
}

float UHunterMMC_ManaRegen::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();
	const FPHGameplayTags& Tags = FPHGameplayTags::Get();
	if (TargetTags &&
		(TargetTags->HasTagExact(Tags.Condition_Self_CannotRegenMana) ||
			TargetTags->HasTagExact(Tags.Effect_Mana_Exhausted)))
	{
		return 0.f;
	}

	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = SourceTags;
	EvaluationParameters.TargetTags = TargetTags;

	const auto& Defs = HunterMMCManaRegenPrivate::GetCaptureDefinitions();

	float Rate   = 0.f;
	float Amount = 0.f;

	GetCapturedAttributeMagnitude(Defs.ManaRegenRateDef,   Spec, EvaluationParameters, Rate);
	GetCapturedAttributeMagnitude(Defs.ManaRegenAmountDef, Spec, EvaluationParameters, Amount);

	// Per-second value. The GE coefficient scales this by its tick period.
	return FMath::Max(Rate, 0.f) * FMath::Max(Amount, 0.f);
}
