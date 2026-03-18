#include "AbilitySystem/ModMagnitude/HunterMMC_EnduranceMaxStamina.h"

#include "AbilitySystem/HunterAttributeSet.h"
#include "GameplayEffectExtension.h"

namespace HunterMMCEnduranceMaxStaminaPrivate
{
	constexpr float BaseMaxValue = 100.0f;
	constexpr float BasePrimaryBonus = 5.0f;
	constexpr float PerLevelBonus = 12.0f;

	struct FCaptureDefinitions
	{
		FGameplayEffectAttributeCaptureDefinition EnduranceDef;
		FGameplayEffectAttributeCaptureDefinition PlayerLevelDef;

		FCaptureDefinitions()
			: EnduranceDef(UHunterAttributeSet::GetEnduranceAttribute(), EGameplayEffectAttributeCaptureSource::Target, false)
			, PlayerLevelDef(UHunterAttributeSet::GetPlayerLevelAttribute(), EGameplayEffectAttributeCaptureSource::Target, false)
		{
		}
	};

	static const FCaptureDefinitions& GetCaptureDefinitions()
	{
		static const FCaptureDefinitions Definitions;
		return Definitions;
	}
}

UHunterMMC_EnduranceMaxStamina::UHunterMMC_EnduranceMaxStamina()
{
	const HunterMMCEnduranceMaxStaminaPrivate::FCaptureDefinitions& CaptureDefinitions = HunterMMCEnduranceMaxStaminaPrivate::GetCaptureDefinitions();

	RelevantAttributesToCapture.Add(CaptureDefinitions.EnduranceDef);
	RelevantAttributesToCapture.Add(CaptureDefinitions.PlayerLevelDef);
}

float UHunterMMC_EnduranceMaxStamina::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = SourceTags;
	EvaluationParameters.TargetTags = TargetTags;

	const HunterMMCEnduranceMaxStaminaPrivate::FCaptureDefinitions& CaptureDefinitions = HunterMMCEnduranceMaxStaminaPrivate::GetCaptureDefinitions();

	float Endurance = 0.0f;
	GetCapturedAttributeMagnitude(CaptureDefinitions.EnduranceDef, Spec, EvaluationParameters, Endurance);
	Endurance = FMath::Max(Endurance, 0.0f);

	float PlayerLevel = 1.0f;
	GetCapturedAttributeMagnitude(CaptureDefinitions.PlayerLevelDef, Spec, EvaluationParameters, PlayerLevel);
	PlayerLevel = FMath::Max(PlayerLevel, 1.0f);

	const float CalculatedMaxStamina =
		HunterMMCEnduranceMaxStaminaPrivate::BaseMaxValue +
		(HunterMMCEnduranceMaxStaminaPrivate::BasePrimaryBonus + Endurance) +
		(HunterMMCEnduranceMaxStaminaPrivate::PerLevelBonus * (PlayerLevel - 1.0f));

	return FMath::Max(CalculatedMaxStamina, 0.0f);
}
