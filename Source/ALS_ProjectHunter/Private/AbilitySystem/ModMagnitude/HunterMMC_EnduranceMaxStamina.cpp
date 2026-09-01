#include "AbilitySystem/ModMagnitude/HunterMMC_EnduranceMaxStamina.h"

#include "AbilitySystem/HunterAttributeSet.h"
#include "AbilitySystem/Library/FunctionLibraries/PHResourceFunctionLibrary.h"
#include "GameplayEffectExtension.h"

namespace HunterMMCEnduranceMaxStaminaPrivate
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

	float PlayerLevel = 0.0f;
	GetCapturedAttributeMagnitude(CaptureDefinitions.PlayerLevelDef, Spec, EvaluationParameters, PlayerLevel);
	PlayerLevel = FMath::Max(PlayerLevel, 0.0f);

	return UPHResourceFunctionLibrary::CalculatePrimaryDerivedMaxValue(FPHPrimaryDerivedResourceInput(
		HunterMMCEnduranceMaxStaminaPrivate::BaseMaxValue,
		HunterMMCEnduranceMaxStaminaPrivate::BasePrimaryBonus,
		Endurance,
		PlayerLevel,
		HunterMMCEnduranceMaxStaminaPrivate::PerLevelBonus));
}
