// Item/Library/Structs/ItemCalculationStructs.h
#pragma once

#include "CoreMinimal.h"
#include "ItemCalculationStructs.generated.h"

USTRUCT(BlueprintType)
struct FDamageRange
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	float MinDamage = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	float MaxDamage = 0.0f;

	FDamageRange() = default;
	FDamageRange(float Min, float Max) : MinDamage(Min), MaxDamage(Max) {}

	float GetAverage() const { return (MinDamage + MaxDamage) / 2.0f; }
	float GetTotal() const { return MinDamage + MaxDamage; }
};
