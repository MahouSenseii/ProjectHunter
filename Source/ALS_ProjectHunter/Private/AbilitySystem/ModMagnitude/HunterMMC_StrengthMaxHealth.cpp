#include "AbilitySystem/ModMagnitude/HunterMMC_StrengthMaxHealth.h"

#include "AbilitySystem/HunterAttributeSet.h"
#include "GameplayEffectExtension.h"

namespace HunterMMCStrengthMaxHealthPrivate
{
	constexpr float BaseMaxValue = 100.0f;
	constexpr float BasePrimaryBonus = 5.0f;
	constexpr float PerLevelBonus = 12.0f;

	struct FCaptureDefinitions
	{
		FGameplayEffectAttributeCaptureDefinition StrengthDef;
		FGameplayEffectAttributeCaptureDefinition PlayerLevelDef;

		FCaptureDefinitions()
			: StrengthDef(UHunterAttributeSet::GetStrengthAttribute(), EGameplayEffectAttributeCaptureSource::Target, false)
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

UHunterMMC_StrengthMaxHealth::UHunterMMC_StrengthMaxHealth()
{
	const HunterMMCStrengthMaxHealthPrivate::FCaptureDefinitions& CaptureDefinitions = HunterMMCStrengthMaxHealthPrivate::GetCaptureDefinitions();

	RelevantAttributesToCapture.Add(CaptureDefinitions.StrengthDef);
	RelevantAttributesToCapture.Add(CaptureDefinitions.PlayerLevelDef);
}

float UHunterMMC_StrengthMaxHealth::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = SourceTags;
	EvaluationParameters.TargetTags = TargetTags;

	const HunterMMCStrengthMaxHealthPrivate::FCaptureDefinitions& CaptureDefinitions = HunterMMCStrengthMaxHealthPrivate::GetCaptureDefinitions();

	float Strength = 0.0f;
	GetCapturedAttributeMagnitude(CaptureDefinitions.StrengthDef, Spec, EvaluationParameters, Strength);
	Strength = FMath::Max(Strength, 0.0f);

	float PlayerLevel = 1.0f;
	GetCapturedAttributeMagnitude(CaptureDefinitions.PlayerLevelDef, Spec, EvaluationParameters, PlayerLevel);
	PlayerLevel = FMath::Max(PlayerLevel, 1.0f);

	const float CalculatedMaxHealth =
		HunterMMCStrengthMaxHealthPrivate::BaseMaxValue +
		(HunterMMCStrengthMaxHealthPrivate::BasePrimaryBonus + Strength) +
		(HunterMMCStrengthMaxHealthPrivate::PerLevelBonus * (PlayerLevel - 1.0f));

	return FMath::Max(CalculatedMaxHealth, 0.0f);
}
