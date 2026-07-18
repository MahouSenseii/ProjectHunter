#include "AbilitySystem/ModMagnitude/HunterMMC_EffectiveResource.h"
#include "HunterMMCResourceShared.h"

UHunterMMC_EffectiveResource::UHunterMMC_EffectiveResource()
{
	HunterMMCResourceShared::AddReservationCaptureDefinitions(RelevantAttributesToCapture);
}

float UHunterMMC_EffectiveResource::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	float MaxValue    = 0.0f;
	float PercentValue = 0.0f;
	float FlatValue    = 0.0f;

	const HunterMMCResourceShared::FReservationCaptureDefinitions CaptureDefs =
		HunterMMCResourceShared::ResolveReservationCaptureDefinitions(GetResourceType());
	if (CaptureDefs.IsValid())
	{
		MaxValue = GetCapturedValue(Spec, *CaptureDefs.MaxValueDef, 0.0f);
		PercentValue = GetCapturedValue(Spec, *CaptureDefs.PercentageReservedDef, 0.0f);
		FlatValue = GetCapturedValue(Spec, *CaptureDefs.FlatReservedDef, 0.0f);
	}

	const float ReservedAmount = CalculateReservedAmount(MaxValue, PercentValue, FlatValue);
	return UPHResourceFunctionLibrary::CalculateMaxEffectiveValue(MaxValue, ReservedAmount);
}

float UHunterMMC_EffectiveResource::GetCapturedValue(
	const FGameplayEffectSpec& Spec,
	const FGameplayEffectAttributeCaptureDefinition& CaptureDefinition,
	float DefaultValue) const
{
	const FAggregatorEvaluateParameters EvaluationParameters = HunterMMCResourceShared::BuildEvaluationParameters(Spec);
	float CapturedValue = DefaultValue;
	GetCapturedAttributeMagnitude(CaptureDefinition, Spec, EvaluationParameters, CapturedValue);
	return CapturedValue;
}

float UHunterMMC_EffectiveResource::CalculateReservedAmount(float MaxValue, float PercentValue, float FlatValue)
{
	return HunterMMCResourceShared::CalculateReservedAmount(MaxValue, PercentValue, FlatValue);
}
