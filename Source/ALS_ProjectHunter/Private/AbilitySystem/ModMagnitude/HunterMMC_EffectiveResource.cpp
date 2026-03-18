#include "AbilitySystem/ModMagnitude/HunterMMC_EffectiveResource.h"

#include "AbilitySystem/HunterAttributeSet.h"
#include "GameplayEffectExtension.h"

namespace HunterMMCEffectiveResourcePrivate
{
	struct FCaptureDefinitions
	{
		FGameplayEffectAttributeCaptureDefinition MaxHealthDef;
		FGameplayEffectAttributeCaptureDefinition MaxManaDef;
		FGameplayEffectAttributeCaptureDefinition MaxStaminaDef;
		FGameplayEffectAttributeCaptureDefinition FlatReservedHealthDef;
		FGameplayEffectAttributeCaptureDefinition FlatReservedManaDef;
		FGameplayEffectAttributeCaptureDefinition FlatReservedStaminaDef;
		FGameplayEffectAttributeCaptureDefinition PercentageReservedHealthDef;
		FGameplayEffectAttributeCaptureDefinition PercentageReservedManaDef;
		FGameplayEffectAttributeCaptureDefinition PercentageReservedStaminaDef;

		FCaptureDefinitions()
			: MaxHealthDef(UHunterAttributeSet::GetMaxHealthAttribute(), EGameplayEffectAttributeCaptureSource::Target, false)
			, MaxManaDef(UHunterAttributeSet::GetMaxManaAttribute(), EGameplayEffectAttributeCaptureSource::Target, false)
			, MaxStaminaDef(UHunterAttributeSet::GetMaxStaminaAttribute(), EGameplayEffectAttributeCaptureSource::Target, false)
			, FlatReservedHealthDef(UHunterAttributeSet::GetFlatReservedHealthAttribute(), EGameplayEffectAttributeCaptureSource::Target, false)
			, FlatReservedManaDef(UHunterAttributeSet::GetFlatReservedManaAttribute(), EGameplayEffectAttributeCaptureSource::Target, false)
			, FlatReservedStaminaDef(UHunterAttributeSet::GetFlatReservedStaminaAttribute(), EGameplayEffectAttributeCaptureSource::Target, false)
			, PercentageReservedHealthDef(UHunterAttributeSet::GetPercentageReservedHealthAttribute(), EGameplayEffectAttributeCaptureSource::Target, false)
			, PercentageReservedManaDef(UHunterAttributeSet::GetPercentageReservedManaAttribute(), EGameplayEffectAttributeCaptureSource::Target, false)
			, PercentageReservedStaminaDef(UHunterAttributeSet::GetPercentageReservedStaminaAttribute(), EGameplayEffectAttributeCaptureSource::Target, false)
		{
		}
	};

	static const FCaptureDefinitions& GetCaptureDefinitions()
	{
		static const FCaptureDefinitions Definitions;
		return Definitions;
	}
}

UHunterMMC_EffectiveResource::UHunterMMC_EffectiveResource()
{
	const HunterMMCEffectiveResourcePrivate::FCaptureDefinitions& CaptureDefinitions = HunterMMCEffectiveResourcePrivate::GetCaptureDefinitions();

	RelevantAttributesToCapture.Add(CaptureDefinitions.MaxHealthDef);
	RelevantAttributesToCapture.Add(CaptureDefinitions.MaxManaDef);
	RelevantAttributesToCapture.Add(CaptureDefinitions.MaxStaminaDef);
	RelevantAttributesToCapture.Add(CaptureDefinitions.FlatReservedHealthDef);
	RelevantAttributesToCapture.Add(CaptureDefinitions.FlatReservedManaDef);
	RelevantAttributesToCapture.Add(CaptureDefinitions.FlatReservedStaminaDef);
	RelevantAttributesToCapture.Add(CaptureDefinitions.PercentageReservedHealthDef);
	RelevantAttributesToCapture.Add(CaptureDefinitions.PercentageReservedManaDef);
	RelevantAttributesToCapture.Add(CaptureDefinitions.PercentageReservedStaminaDef);
}

float UHunterMMC_EffectiveResource::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	const HunterMMCEffectiveResourcePrivate::FCaptureDefinitions& CaptureDefinitions = HunterMMCEffectiveResourcePrivate::GetCaptureDefinitions();

	float MaxValue = 0.0f;
	float PercentValue = 0.0f;
	float FlatValue = 0.0f;

	switch (GetResourceType())
	{
	case EHunterEffectiveResourceType::Health:
		MaxValue = GetCapturedValue(Spec, CaptureDefinitions.MaxHealthDef, 0.0f);
		PercentValue = GetCapturedValue(Spec, CaptureDefinitions.PercentageReservedHealthDef, 0.0f);
		FlatValue = GetCapturedValue(Spec, CaptureDefinitions.FlatReservedHealthDef, 0.0f);
		break;

	case EHunterEffectiveResourceType::Mana:
		MaxValue = GetCapturedValue(Spec, CaptureDefinitions.MaxManaDef, 0.0f);
		PercentValue = GetCapturedValue(Spec, CaptureDefinitions.PercentageReservedManaDef, 0.0f);
		FlatValue = GetCapturedValue(Spec, CaptureDefinitions.FlatReservedManaDef, 0.0f);
		break;

	case EHunterEffectiveResourceType::Stamina:
		MaxValue = GetCapturedValue(Spec, CaptureDefinitions.MaxStaminaDef, 0.0f);
		PercentValue = GetCapturedValue(Spec, CaptureDefinitions.PercentageReservedStaminaDef, 0.0f);
		FlatValue = GetCapturedValue(Spec, CaptureDefinitions.FlatReservedStaminaDef, 0.0f);
		break;

	default:
		break;
	}

	const float ReservedAmount = CalculateReservedAmount(MaxValue, PercentValue, FlatValue);
	return FMath::Max(FMath::Max(MaxValue, 0.0f) - ReservedAmount, 0.0f);
}

float UHunterMMC_EffectiveResource::GetCapturedValue(
	const FGameplayEffectSpec& Spec,
	const FGameplayEffectAttributeCaptureDefinition& CaptureDefinition,
	float DefaultValue) const
{
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = SourceTags;
	EvaluationParameters.TargetTags = TargetTags;

	float CapturedValue = DefaultValue;
	GetCapturedAttributeMagnitude(CaptureDefinition, Spec, EvaluationParameters, CapturedValue);
	return CapturedValue;
}

float UHunterMMC_EffectiveResource::CalculateReservedAmount(float MaxValue, float PercentValue, float FlatValue)
{
	const float SafeMaxValue = FMath::Max(MaxValue, 0.0f);
	const float SafePercentValue = FMath::Max(PercentValue, 0.0f);
	const float SafeFlatValue = FMath::Max(FlatValue, 0.0f);
	const float ReservedRaw = (SafeMaxValue * (SafePercentValue / 100.0f)) + SafeFlatValue;

	return static_cast<float>(FMath::RoundHalfToEven(ReservedRaw));
}
