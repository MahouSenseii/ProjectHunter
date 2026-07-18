#include "AbilitySystem/ModMagnitude/HunterMMC_ReservedResource.h"
#include "HunterMMCResourceShared.h"

UHunterMMC_ReservedResource::UHunterMMC_ReservedResource()
{
	HunterMMCResourceShared::AddReservationCaptureDefinitions(RelevantAttributesToCapture);
}

float UHunterMMC_ReservedResource::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	float MaxValue     = 0.0f;
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
