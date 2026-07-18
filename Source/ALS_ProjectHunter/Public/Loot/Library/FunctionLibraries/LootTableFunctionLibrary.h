#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Loot/Library/Structs/LootStructs.h"
#include "LootTableFunctionLibrary.generated.h"

UCLASS()
class ALS_PROJECTHUNTER_API ULootTableFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Loot|Validation")
	static bool IsValidLootTableHandle(const FDataTableRowHandle& Handle);

	UFUNCTION(BlueprintPure, Category = "Loot|Calculation")
	static float GetLootTableTotalWeight(const FDataTableRowHandle& Handle);

	UFUNCTION(BlueprintPure, Category = "Loot|Info")
	static int32 GetLootTableEntryCount(const FDataTableRowHandle& Handle);

	UFUNCTION(BlueprintPure, Category = "Loot|Corruption")
	static int32 GetCorruptedEntryCount(const FDataTableRowHandle& Handle);

	UFUNCTION(BlueprintPure, Category = "Loot|Calculation")
	static float GetEntryDropPercentage(const FLootEntry& Entry, float TotalTableWeight);

	UFUNCTION(BlueprintPure, Category = "Loot|Validation")
	static bool IsValidLootEntry(const FLootEntry& Entry);
};
