#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LootCalculationFunctionLibrary.generated.h"

UCLASS()
class ALS_PROJECTHUNTER_API ULootCalculationFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Loot|Calculation")
	static float ApplyLuckToDropChance(float BaseChance, float Luck);

	UFUNCTION(BlueprintPure, Category = "Loot|Calculation")
	static int32 ApplyMagicFindToQuantity(int32 BaseQuantity, float MagicFind);
};
