#include "AbilitySystem/ModMagnitude/HunterMMC_StrengthMaxHealth.h"

#include "AbilitySystem/HunterAttributeSet.h"
#include "AbilitySystem/Library/FunctionLibraries/PHResourceFunctionLibrary.h"
#include "GameplayEffectExtension.h"

namespace HunterMMCStrengthMaxHealthPrivate
{
	// The base value is authored per-character in DA_BaseStats and applied as the
	// attribute's base, so this contributes the stat scaling only. A non-zero base
	// here would give every pawn - player, trash, boss - the same starting pool.
	constexpr float BaseMaxValue = 0.0f;
	// Zero so a primary of 0 contributes nothing. A flat bonus here would be a
	// second hidden base on top of the one DA_BaseStats authors.
	constexpr float BasePrimaryBonus = 0.0f;
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

	return UPHResourceFunctionLibrary::CalculatePrimaryDerivedMaxValue(FPHPrimaryDerivedResourceInput(
		HunterMMCStrengthMaxHealthPrivate::BaseMaxValue,
		HunterMMCStrengthMaxHealthPrivate::BasePrimaryBonus,
		Strength,
		PlayerLevel,
		HunterMMCStrengthMaxHealthPrivate::PerLevelBonus));
}
