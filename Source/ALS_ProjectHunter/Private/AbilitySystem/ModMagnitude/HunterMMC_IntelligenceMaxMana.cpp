#include "AbilitySystem/ModMagnitude/HunterMMC_IntelligenceMaxMana.h"

#include "AbilitySystem/HunterAttributeSet.h"
#include "AbilitySystem/Library/FunctionLibraries/PHResourceFunctionLibrary.h"
#include "GameplayEffectExtension.h"

namespace HunterMMCIntelligenceMaxManaPrivate
{
	// The base value is authored per-character in DA_BaseStats and applied as the
	// attribute's base, so this contributes the stat scaling only. A non-zero base
	// here would give every pawn - player, trash, boss - the same starting pool.
	constexpr float BaseMaxValue = 0.0f;
	// Zero so a primary of 0 contributes nothing. A flat bonus here would be a
	// second hidden base on top of the one DA_BaseStats authors.
	constexpr float BasePrimaryBonus = 0.0f;
	constexpr float PerLevelBonus = 12.f;

	struct FCaptureDefinitions
	{
		FGameplayEffectAttributeCaptureDefinition IntelligenceDef;
		FGameplayEffectAttributeCaptureDefinition PlayerLevelDef;

		FCaptureDefinitions()
			: IntelligenceDef(UHunterAttributeSet::GetIntelligenceAttribute(), EGameplayEffectAttributeCaptureSource::Target, false)
			, PlayerLevelDef(UHunterAttributeSet::GetPlayerLevelAttribute(), EGameplayEffectAttributeCaptureSource::Target, false)
		{
		}
	};

	const FCaptureDefinitions& GetCaptureDefinitions()
	{
		static const FCaptureDefinitions Definitions;
		return Definitions;
	}
}

UHunterMMC_IntelligenceMaxMana::UHunterMMC_IntelligenceMaxMana()
{
	const HunterMMCIntelligenceMaxManaPrivate::FCaptureDefinitions& Definitions =
		HunterMMCIntelligenceMaxManaPrivate::GetCaptureDefinitions();
	RelevantAttributesToCapture.Add(Definitions.IntelligenceDef);
	RelevantAttributesToCapture.Add(Definitions.PlayerLevelDef);
}

float UHunterMMC_IntelligenceMaxMana::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = SourceTags;
	EvaluationParameters.TargetTags = TargetTags;

	const HunterMMCIntelligenceMaxManaPrivate::FCaptureDefinitions& Definitions =
		HunterMMCIntelligenceMaxManaPrivate::GetCaptureDefinitions();

	float Intelligence = 0.f;
	GetCapturedAttributeMagnitude(Definitions.IntelligenceDef, Spec, EvaluationParameters, Intelligence);

	float PlayerLevel = 0.f;
	GetCapturedAttributeMagnitude(Definitions.PlayerLevelDef, Spec, EvaluationParameters, PlayerLevel);

	return UPHResourceFunctionLibrary::CalculatePrimaryDerivedMaxValue(FPHPrimaryDerivedResourceInput(
		HunterMMCIntelligenceMaxManaPrivate::BaseMaxValue,
		HunterMMCIntelligenceMaxManaPrivate::BasePrimaryBonus,
		FMath::Max(Intelligence, 0.f),
		FMath::Max(PlayerLevel, 0.f),
		HunterMMCIntelligenceMaxManaPrivate::PerLevelBonus));
}
