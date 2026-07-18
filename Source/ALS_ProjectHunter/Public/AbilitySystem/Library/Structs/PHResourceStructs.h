#pragma once

#include "CoreMinimal.h"
#include "PHResourceStructs.generated.h"

USTRUCT(BlueprintType)
struct FPHResourceReservationInput
{
	GENERATED_BODY()

	FPHResourceReservationInput() = default;

	FPHResourceReservationInput(
		float InRawMaxValue,
		float InFlatReservedValue,
		float InPercentageReservedValue,
		float InExistingReservedValue,
		float InMaxReservedValue);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
	float RawMaxValue = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
	float FlatReservedValue = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
	float PercentageReservedValue = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
	float ExistingReservedValue = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
	float MaxReservedValue = 0.0f;
};

USTRUCT(BlueprintType)
struct FPHResourceReservationResult
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Resource")
	float ComponentReservedValue = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Resource")
	float ReservedValue = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Resource")
	float MaxEffectiveValue = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Resource")
	bool bUsesComponentReservation = false;
};

USTRUCT(BlueprintType)
struct FPHPrimaryDerivedResourceInput
{
	GENERATED_BODY()

	FPHPrimaryDerivedResourceInput() = default;

	FPHPrimaryDerivedResourceInput(
		float InBaseMaxValue,
		float InBasePrimaryBonus,
		float InPrimaryValue,
		float InPlayerLevel,
		float InPerLevelBonus);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
	float BaseMaxValue = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
	float BasePrimaryBonus = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
	float PrimaryValue = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
	float PlayerLevel = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
	float PerLevelBonus = 0.0f;
};
