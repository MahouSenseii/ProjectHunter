#include "AbilitySystem/Library/FunctionLibraries/PHResourceFunctionLibrary.h"

float UPHResourceFunctionLibrary::CalculateComponentReservedAmount(
	const float RawMaxValue,
	const float FlatReservedValue,
	const float PercentageReservedValue)
{
	return FMath::Max(FlatReservedValue, 0.0f) +
		(FMath::Max(PercentageReservedValue, 0.0f) * 0.01f * FMath::Max(RawMaxValue, 0.0f));
}

float UPHResourceFunctionLibrary::CalculateRoundedReservedAmount(
	const float RawMaxValue,
	const float FlatReservedValue,
	const float PercentageReservedValue)
{
	return static_cast<float>(FMath::RoundHalfToEven(
		CalculateComponentReservedAmount(RawMaxValue, FlatReservedValue, PercentageReservedValue)));
}

float UPHResourceFunctionLibrary::ClampReservedAmount(
	const float ReservedValue,
	const float RawMaxValue,
	const float MaxReservedValue)
{
	const float SafeRawMaxValue = FMath::Max(RawMaxValue, 0.0f);
	const float ReservedCap = MaxReservedValue > 0.0f
		? FMath::Min(FMath::Max(MaxReservedValue, 0.0f), SafeRawMaxValue)
		: SafeRawMaxValue;

	return FMath::Clamp(ReservedValue, 0.0f, ReservedCap);
}

float UPHResourceFunctionLibrary::CalculateMaxEffectiveValue(const float RawMaxValue, const float ReservedValue)
{
	const float SafeRawMaxValue = FMath::Max(RawMaxValue, 0.0f);
	return FMath::Clamp(SafeRawMaxValue - FMath::Max(ReservedValue, 0.0f), 0.0f, SafeRawMaxValue);
}

FPHResourceReservationResult UPHResourceFunctionLibrary::ResolveResourceReservation(const FPHResourceReservationInput& Input)
{
	FPHResourceReservationResult Result;
	Result.ComponentReservedValue = CalculateComponentReservedAmount(
		Input.RawMaxValue,
		Input.FlatReservedValue,
		Input.PercentageReservedValue);
	Result.bUsesComponentReservation = !FMath::IsNearlyZero(Result.ComponentReservedValue, KINDA_SMALL_NUMBER);
	Result.ReservedValue = ClampReservedAmount(
		Result.bUsesComponentReservation ? Result.ComponentReservedValue : Input.ExistingReservedValue,
		Input.RawMaxValue,
		Input.MaxReservedValue);
	Result.MaxEffectiveValue = CalculateMaxEffectiveValue(Input.RawMaxValue, Result.ReservedValue);

	return Result;
}

float UPHResourceFunctionLibrary::ClampWithOptionalCap(const float Value, const float MaxCap)
{
	const float ClampedValue = FMath::Max(Value, 0.0f);
	return MaxCap > 0.0f ? FMath::Min(ClampedValue, MaxCap) : ClampedValue;
}

float UPHResourceFunctionLibrary::CalculateResourceFlowAmount(const float RateValue, const float AmountValue)
{
	return FMath::Max(RateValue, 0.0f) * FMath::Max(AmountValue, 0.0f);
}

float UPHResourceFunctionLibrary::CalculateResourceDrainAmount(const float RateValue, const float AmountValue)
{
	return -CalculateResourceFlowAmount(RateValue, AmountValue);
}

float UPHResourceFunctionLibrary::CalculatePrimaryDerivedMaxValue(const FPHPrimaryDerivedResourceInput& Input)
{
	const float SafePrimaryValue = FMath::Max(Input.PrimaryValue, 0.0f);
	const float SafePlayerLevel = FMath::Max(Input.PlayerLevel, 1.0f);
	const float CalculatedMaxValue =
		Input.BaseMaxValue +
		(Input.BasePrimaryBonus + SafePrimaryValue) +
		(Input.PerLevelBonus * (SafePlayerLevel - 1.0f));

	return FMath::Max(CalculatedMaxValue, 0.0f);
}
