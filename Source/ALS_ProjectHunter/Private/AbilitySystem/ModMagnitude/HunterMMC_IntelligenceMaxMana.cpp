#include "AbilitySystem/ModMagnitude/HunterMMC_IntelligenceMaxMana.h"

#include "AbilitySystem/HunterAttributeSet.h"
#include "AbilitySystem/Library/FunctionLibraries/PHResourceFunctionLibrary.h"
#include "GameplayEffectExtension.h"

namespace HunterMMCIntelligenceMaxManaPrivate
{
	constexpr float BaseMaxValue = 100.f;
	constexpr float BasePrimaryBonus = 5.f;
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

	float PlayerLevel = 1.f;
	GetCapturedAttributeMagnitude(Definitions.PlayerLevelDef, Spec, EvaluationParameters, PlayerLevel);

	return UPHResourceFunctionLibrary::CalculatePrimaryDerivedMaxValue(FPHPrimaryDerivedResourceInput(
		HunterMMCIntelligenceMaxManaPrivate::BaseMaxValue,
		HunterMMCIntelligenceMaxManaPrivate::BasePrimaryBonus,
		FMath::Max(Intelligence, 0.f),
		FMath::Max(PlayerLevel, 1.f),
		HunterMMCIntelligenceMaxManaPrivate::PerLevelBonus));
}
