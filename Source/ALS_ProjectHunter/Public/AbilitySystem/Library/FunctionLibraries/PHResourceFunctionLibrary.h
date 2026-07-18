#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Library/Structs/PHResourceStructs.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PHResourceFunctionLibrary.generated.h"

UCLASS()
class ALS_PROJECTHUNTER_API UPHResourceFunctionLibrary final : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "ProjectHunter|AbilitySystem|Resource")
	static float CalculateComponentReservedAmount(float RawMaxValue, float FlatReservedValue, float PercentageReservedValue);

	UFUNCTION(BlueprintPure, Category = "ProjectHunter|AbilitySystem|Resource")
	static float CalculateRoundedReservedAmount(float RawMaxValue, float FlatReservedValue, float PercentageReservedValue);

	UFUNCTION(BlueprintPure, Category = "ProjectHunter|AbilitySystem|Resource")
	static float ClampReservedAmount(float ReservedValue, float RawMaxValue, float MaxReservedValue);

	UFUNCTION(BlueprintPure, Category = "ProjectHunter|AbilitySystem|Resource")
	static float CalculateMaxEffectiveValue(float RawMaxValue, float ReservedValue);

	UFUNCTION(BlueprintPure, Category = "ProjectHunter|AbilitySystem|Resource")
	static FPHResourceReservationResult ResolveResourceReservation(const FPHResourceReservationInput& Input);

	UFUNCTION(BlueprintPure, Category = "ProjectHunter|AbilitySystem|Resource")
	static float ClampWithOptionalCap(float Value, float MaxCap);

	UFUNCTION(BlueprintPure, Category = "ProjectHunter|AbilitySystem|Resource")
	static float CalculateResourceFlowAmount(float RateValue, float AmountValue);

	UFUNCTION(BlueprintPure, Category = "ProjectHunter|AbilitySystem|Resource")
	static float CalculateResourceDrainAmount(float RateValue, float AmountValue);

	UFUNCTION(BlueprintPure, Category = "ProjectHunter|AbilitySystem|Resource")
	static float CalculatePrimaryDerivedMaxValue(const FPHPrimaryDerivedResourceInput& Input);
};
