#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Loot/Library/Structs/LootStructs.h"
#include "LootFunctionLibrary.generated.h"

UCLASS()
class ALS_PROJECTHUNTER_API ULootFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Loot|Display", meta = (DisplayName = "Get Drop Rarity Name", Keywords = "rarity name display"))
	static FText GetDropRarityDisplayName(EDropRarity Rarity);

	UFUNCTION(BlueprintPure, Category = "Loot|Display", meta = (DisplayName = "Get Drop Rarity Color", Keywords = "rarity color"))
	static FLinearColor GetDropRarityColor(EDropRarity Rarity);

	UFUNCTION(BlueprintPure, Category = "Loot|Conversion", meta = (DisplayName = "Drop Rarity To Item Rarity"))
	static EItemRarity DropRarityToItemRarity(EDropRarity DropRarity);

	UFUNCTION(BlueprintPure, Category = "Loot|Calculation", meta = (DisplayName = "Get Rarity Multiplier"))
	static float GetRarityMultiplier(EDropRarity Rarity);

	UFUNCTION(BlueprintPure, Category = "Loot|Validation", meta = (DisplayName = "Is Valid Loot Table Handle"))
	static bool IsValidLootTableHandle(const FDataTableRowHandle& Handle);

	UFUNCTION(BlueprintPure, Category = "Loot|Calculation", meta = (DisplayName = "Get Loot Table Total Weight"))
	static float GetLootTableTotalWeight(const FDataTableRowHandle& Handle);

	UFUNCTION(BlueprintPure, Category = "Loot|Info", meta = (DisplayName = "Get Loot Table Entry Count"))
	static int32 GetLootTableEntryCount(const FDataTableRowHandle& Handle);

	UFUNCTION(BlueprintPure, Category = "Loot|Corruption", meta = (DisplayName = "Get Corrupted Entry Count"))
	static int32 GetCorruptedEntryCount(const FDataTableRowHandle& Handle);

	UFUNCTION(BlueprintPure, Category = "Loot|Calculation", meta = (DisplayName = "Get Entry Drop Percentage"))
	static float GetEntryDropPercentage(const FLootEntry& Entry, float TotalTableWeight);

	UFUNCTION(BlueprintPure, Category = "Loot|Validation", meta = (DisplayName = "Is Valid Loot Entry"))
	static bool IsValidLootEntry(const FLootEntry& Entry);

	UFUNCTION(BlueprintPure, Category = "Loot|Corruption", meta = (DisplayName = "Get Corruption Type Name"))
	static FText GetCorruptionTypeName(ECorruptionType Type);

	UFUNCTION(BlueprintPure, Category = "Loot|Corruption", meta = (DisplayName = "Get Corruption Type Color"))
	static FLinearColor GetCorruptionTypeColor(ECorruptionType Type);

	UFUNCTION(BlueprintPure, Category = "Loot|Corruption", meta = (DisplayName = "Get Corruption Severity"))
	static float GetCorruptionSeverity(ECorruptionType Type);

	UFUNCTION(BlueprintPure, Category = "Loot|Display", meta = (DisplayName = "Get Loot Source Name"))
	static FText GetLootSourceTypeName(ELootSourceType Type);

	UFUNCTION(BlueprintPure, Category = "Loot|Settings", meta = (DisplayName = "Get Default Settings For Source"))
	static FLootDropSettings GetDefaultSettingsForSourceType(ELootSourceType Type);

	UFUNCTION(BlueprintPure, Category = "Loot|Calculation", meta = (DisplayName = "Apply Luck To Drop Chance"))
	static float ApplyLuckToDropChance(float BaseChance, float Luck);

	UFUNCTION(BlueprintPure, Category = "Loot|Calculation", meta = (DisplayName = "Apply Magic Find To Quantity"))
	static int32 ApplyMagicFindToQuantity(int32 BaseQuantity, float MagicFind);
};
