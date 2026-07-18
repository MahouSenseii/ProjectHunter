#pragma once

#include "AbilitySystem/HunterAttributeSet.h"
#include "AbilitySystem/Library/Enums/HunterResourceEnums.h"
#include "AbilitySystem/Library/FunctionLibraries/PHResourceFunctionLibrary.h"
#include "GameplayEffectExtension.h"

namespace HunterMMCResourceShared
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

	struct FReservationCaptureDefinitions
	{
		const FGameplayEffectAttributeCaptureDefinition* MaxValueDef = nullptr;
		const FGameplayEffectAttributeCaptureDefinition* PercentageReservedDef = nullptr;
		const FGameplayEffectAttributeCaptureDefinition* FlatReservedDef = nullptr;

		bool IsValid() const
		{
			return MaxValueDef && PercentageReservedDef && FlatReservedDef;
		}
	};

	inline const FCaptureDefinitions& GetCaptureDefinitions()
	{
		static const FCaptureDefinitions Definitions;
		return Definitions;
	}

	inline void AddReservationCaptureDefinitions(TArray<FGameplayEffectAttributeCaptureDefinition>& RelevantAttributesToCapture)
	{
		const FCaptureDefinitions& Cap = GetCaptureDefinitions();
		RelevantAttributesToCapture.Add(Cap.MaxHealthDef);
		RelevantAttributesToCapture.Add(Cap.MaxManaDef);
		RelevantAttributesToCapture.Add(Cap.MaxStaminaDef);
		RelevantAttributesToCapture.Add(Cap.FlatReservedHealthDef);
		RelevantAttributesToCapture.Add(Cap.FlatReservedManaDef);
		RelevantAttributesToCapture.Add(Cap.FlatReservedStaminaDef);
		RelevantAttributesToCapture.Add(Cap.PercentageReservedHealthDef);
		RelevantAttributesToCapture.Add(Cap.PercentageReservedManaDef);
		RelevantAttributesToCapture.Add(Cap.PercentageReservedStaminaDef);
	}

	inline FReservationCaptureDefinitions ResolveReservationCaptureDefinitions(const EHunterResourceType ResourceType)
	{
		const FCaptureDefinitions& Cap = GetCaptureDefinitions();

		switch (ResourceType)
		{
		case EHunterResourceType::Health:
			return { &Cap.MaxHealthDef, &Cap.PercentageReservedHealthDef, &Cap.FlatReservedHealthDef };

		case EHunterResourceType::Mana:
			return { &Cap.MaxManaDef, &Cap.PercentageReservedManaDef, &Cap.FlatReservedManaDef };

		case EHunterResourceType::Stamina:
			return { &Cap.MaxStaminaDef, &Cap.PercentageReservedStaminaDef, &Cap.FlatReservedStaminaDef };

		default:
			return {};
		}
	}

	inline FAggregatorEvaluateParameters BuildEvaluationParameters(const FGameplayEffectSpec& Spec)
	{
		FAggregatorEvaluateParameters EvaluationParameters;
		EvaluationParameters.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
		EvaluationParameters.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();
		return EvaluationParameters;
	}

	inline float CalculateReservedAmount(float MaxValue, float PercentValue, float FlatValue)
	{
		return UPHResourceFunctionLibrary::CalculateRoundedReservedAmount(MaxValue, FlatValue, PercentValue);
	}
}
