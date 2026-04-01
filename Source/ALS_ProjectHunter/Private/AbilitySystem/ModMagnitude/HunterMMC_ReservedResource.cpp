// N-18 FIX: Removed duplicated FCaptureDefinitions, GetCapturedValue, and
// CalculateReservedAmount.  All three are now shared via HunterMMCResourceShared.h
// which is also included by HunterMMC_EffectiveResource.cpp.
#include "AbilitySystem/ModMagnitude/HunterMMC_ReservedResource.h"
#include "HunterMMCResourceShared.h"

UHunterMMC_ReservedResource::UHunterMMC_ReservedResource()
{
	const HunterMMCResourceShared::FCaptureDefinitions& Cap = HunterMMCResourceShared::GetCaptureDefinitions();

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

float UHunterMMC_ReservedResource::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	const HunterMMCResourceShared::FCaptureDefinitions& Cap = HunterMMCResourceShared::GetCaptureDefinitions();

	float MaxValue     = 0.0f;
	float PercentValue = 0.0f;
	float FlatValue    = 0.0f;

	switch (GetResourceType())
	{
	case EHunterReservedResourceType::Health:
		MaxValue     = GetCapturedValue(Spec, Cap.MaxHealthDef, 0.0f);
		PercentValue = GetCapturedValue(Spec, Cap.PercentageReservedHealthDef, 0.0f);
		FlatValue    = GetCapturedValue(Spec, Cap.FlatReservedHealthDef, 0.0f);
		break;

	case EHunterReservedResourceType::Mana:
		MaxValue     = GetCapturedValue(Spec, Cap.MaxManaDef, 0.0f);
		PercentValue = GetCapturedValue(Spec, Cap.PercentageReservedManaDef, 0.0f);
		FlatValue    = GetCapturedValue(Spec, Cap.FlatReservedManaDef, 0.0f);
		break;

	case EHunterReservedResourceType::Stamina:
		MaxValue     = GetCapturedValue(Spec, Cap.MaxStaminaDef, 0.0f);
		PercentValue = GetCapturedValue(Spec, Cap.PercentageReservedStaminaDef, 0.0f);
		FlatValue    = GetCapturedValue(Spec, Cap.FlatReservedStaminaDef, 0.0f);
		break;

	default:
		break;
	}

	return CalculateReservedAmount(MaxValue, PercentValue, FlatValue);
}

float UHunterMMC_ReservedResource::GetCapturedValue(
	const FGameplayEffectSpec& Spec,
	const FGameplayEffectAttributeCaptureDefinition& CaptureDefinition,
	float DefaultValue) const
{
	const FAggregatorEvaluateParameters EvaluationParameters = HunterMMCResourceShared::BuildEvaluationParameters(Spec);
	float CapturedValue = DefaultValue;
	GetCapturedAttributeMagnitude(CaptureDefinition, Spec, EvaluationParameters, CapturedValue);
	return CapturedValue;
}

float UHunterMMC_ReservedResource::CalculateReservedAmount(float MaxValue, float PercentValue, float FlatValue)
{
	return HunterMMCResourceShared::CalculateReservedAmount(MaxValue, PercentValue, FlatValue);
}
