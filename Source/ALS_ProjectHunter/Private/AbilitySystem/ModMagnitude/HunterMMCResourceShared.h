// HunterMMCResourceShared.h
// N-18 FIX: Shared internal header for HunterMMC_EffectiveResource and
// HunterMMC_ReservedResource.  Both MMCs operate on the same nine attribute
// capture definitions and have identical GetCapturedValue /
// CalculateReservedAmount implementations.  Consolidating them here
// eliminates ~120 lines of duplicated code between the two .cpp files.
//
// IMPORTANT: This is a PRIVATE implementation detail — never include this
// outside the ModMagnitude compilation units.
#pragma once

#include "AbilitySystem/HunterAttributeSet.h"
#include "GameplayEffectExtension.h"

namespace HunterMMCResourceShared
{
	// ─────────────────────────────────────────────────────────────────────
	// Capture definitions (shared singleton)
	// ─────────────────────────────────────────────────────────────────────

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

	/** Return the process-lifetime singleton capture definitions. */
	inline const FCaptureDefinitions& GetCaptureDefinitions()
	{
		static const FCaptureDefinitions Definitions;
		return Definitions;
	}

	// ─────────────────────────────────────────────────────────────────────
	// Shared helpers
	// ─────────────────────────────────────────────────────────────────────

	/**
	 * Build the aggregator evaluation parameters from a GE spec.
	 * Call this from within a UGameplayModMagnitudeCalculation subclass, then
	 * invoke GetCapturedAttributeMagnitude(... , EvaluationParameters, ...) on
	 * 'this' — keeping the protected call inside the derived-class context.
	 */
	inline FAggregatorEvaluateParameters BuildEvaluationParameters(const FGameplayEffectSpec& Spec)
	{
		FAggregatorEvaluateParameters EvaluationParameters;
		EvaluationParameters.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
		EvaluationParameters.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();
		return EvaluationParameters;
	}

	/**
	 * Compute the rounded reserved amount from max, percent, and flat values.
	 * Reserved = RoundHalfToEven((Max * (Percent / 100)) + Flat), clamped >= 0.
	 */
	inline float CalculateReservedAmount(float MaxValue, float PercentValue, float FlatValue)
	{
		const float SafeMaxValue     = FMath::Max(MaxValue,     0.0f);
		const float SafePercentValue = FMath::Max(PercentValue, 0.0f);
		const float SafeFlatValue    = FMath::Max(FlatValue,    0.0f);
		const float ReservedRaw = (SafeMaxValue * (SafePercentValue / 100.0f)) + SafeFlatValue;
		return static_cast<float>(FMath::RoundHalfToEven(ReservedRaw));
	}

} // namespace HunterMMCResourceShared
