// Loot/Library/LootFunctionLibrary.h
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Loot/Library/LootEnum.h"
#include "Loot/Library/LootStruct.h"
#include "LootFunctionLibrary.generated.h"

/**
 * ULootFunctionLibrary - Static utility functions for loot system
 * 
 * SINGLE RESPONSIBILITY: Provide stateless utility functions
 * 
 * Provides:
 * - Rarity conversions and display
 * - Loot table validation
 * - Weight calculations
 * - Corruption utilities
 */
UCLASS()
class ALS_PROJECTHUNTER_API ULootFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// ═══════════════════════════════════════════════
	// RARITY UTILITIES
	// ═══════════════════════════════════════════════

	/**
	 * Get display name for drop rarity
	 * @param Rarity - Drop rarity enum
	 * @return Human-readable name
	 */
	UFUNCTION(BlueprintPure, Category = "Loot|Display", meta = (
		DisplayName = "Get Drop Rarity Name",
		Keywords = "rarity name display"))
	static FText GetDropRarityDisplayName(EDropRarity Rarity);

	/**
	 * Get color for drop rarity (for UI)
	 * @param Rarity - Drop rarity
	 * @return Color for rarity tier
	 */
	UFUNCTION(BlueprintPure, Category = "Loot|Display", meta = (
		DisplayName = "Get Drop Rarity Color",
		Keywords = "rarity color"))
	static FLinearColor GetDropRarityColor(EDropRarity Rarity);

	/**
	 * Convert drop rarity to item rarity
	 * @param DropRarity - Source drop rarity
	 * @return Equivalent item rarity
	 */
	UFUNCTION(BlueprintPure, Category = "Loot|Conversion", meta = (
		DisplayName = "Drop Rarity To Item Rarity"))
	static EItemRarity DropRarityToItemRarity(EDropRarity DropRarity);

	/**
	 * Get rarity multiplier (affects quantity/quality)
	 * @param Rarity - Drop rarity
	 * @return Multiplier value (1.0 - 5.0)
	 */
	UFUNCTION(BlueprintPure, Category = "Loot|Calculation", meta = (
		DisplayName = "Get Rarity Multiplier"))
	static float GetRarityMultiplier(EDropRarity Rarity);

	// ═══════════════════════════════════════════════
	// LOOT TABLE UTILITIES
	// ═══════════════════════════════════════════════

	/**
	 * Check if loot table handle is valid
	 * @param Handle - DataTable row handle
	 * @return True if valid loot table
	 */
	UFUNCTION(BlueprintPure, Category = "Loot|Validation", meta = (
		DisplayName = "Is Valid Loot Table Handle"))
	static bool IsValidLootTableHandle(const FDataTableRowHandle& Handle);

	/**
	 * Get total weight of all entries in table
	 * @param Handle - Loot table handle
	 * @return Total weight (0 if invalid)
	 */
	UFUNCTION(BlueprintPure, Category = "Loot|Calculation", meta = (
		DisplayName = "Get Loot Table Total Weight"))
	static float GetLootTableTotalWeight(const FDataTableRowHandle& Handle);

	/**
	 * Get count of entries in loot table
	 * @param Handle - Loot table handle
	 * @return Entry count
	 */
	UFUNCTION(BlueprintPure, Category = "Loot|Info", meta = (
		DisplayName = "Get Loot Table Entry Count"))
	static int32 GetLootTableEntryCount(const FDataTableRowHandle& Handle);

	/**
	 * Get count of corrupted entries in table
	 * @param Handle - Loot table handle
	 * @return Corrupted entry count
	 */
	UFUNCTION(BlueprintPure, Category = "Loot|Corruption", meta = (
		DisplayName = "Get Corrupted Entry Count"))
	static int32 GetCorruptedEntryCount(const FDataTableRowHandle& Handle);

	// ═══════════════════════════════════════════════
	// LOOT ENTRY UTILITIES
	// ═══════════════════════════════════════════════

	/**
	 * Calculate drop probability as percentage
	 * @param Entry - Loot entry
	 * @param TotalTableWeight - Total weight of containing table
	 * @return Drop chance as percentage (0-100)
	 */
	UFUNCTION(BlueprintPure, Category = "Loot|Calculation", meta = (
		DisplayName = "Get Entry Drop Percentage"))
	static float GetEntryDropPercentage(const FLootEntry& Entry, float TotalTableWeight);

	/**
	 * Check if entry is valid (has item reference)
	 * @param Entry - Loot entry to check
	 * @return True if valid
	 */
	UFUNCTION(BlueprintPure, Category = "Loot|Validation", meta = (
		DisplayName = "Is Valid Loot Entry"))
	static bool IsValidLootEntry(const FLootEntry& Entry);

	// ═══════════════════════════════════════════════
	// CORRUPTION UTILITIES
	// ═══════════════════════════════════════════════

	/**
	 * Get display name for corruption type
	 * @param Type - Corruption type
	 * @return Display name
	 */
	UFUNCTION(BlueprintPure, Category = "Loot|Corruption", meta = (
		DisplayName = "Get Corruption Type Name"))
	static FText GetCorruptionTypeName(ECorruptionType Type);

	/**
	 * Get color for corruption type
	 * @param Type - Corruption type
	 * @return UI color
	 */
	UFUNCTION(BlueprintPure, Category = "Loot|Corruption", meta = (
		DisplayName = "Get Corruption Type Color"))
	static FLinearColor GetCorruptionTypeColor(ECorruptionType Type);

	/**
	 * Get corruption severity (0-1 scale)
	 * @param Type - Corruption type
	 * @return Severity value
	 */
	UFUNCTION(BlueprintPure, Category = "Loot|Corruption", meta = (
		DisplayName = "Get Corruption Severity"))
	static float GetCorruptionSeverity(ECorruptionType Type);

	// ═══════════════════════════════════════════════
	// SOURCE TYPE UTILITIES
	// ═══════════════════════════════════════════════

	/**
	 * Get display name for loot source type
	 * @param Type - Source type
	 * @return Display name
	 */
	UFUNCTION(BlueprintPure, Category = "Loot|Display", meta = (
		DisplayName = "Get Loot Source Name"))
	static FText GetLootSourceTypeName(ELootSourceType Type);

	/**
	 * Get default drop settings for source type
	 * @param Type - Source type
	 * @return Recommended default settings
	 */
	UFUNCTION(BlueprintPure, Category = "Loot|Settings", meta = (
		DisplayName = "Get Default Settings For Source"))
	static FLootDropSettings GetDefaultSettingsForSourceType(ELootSourceType Type);

	// ═══════════════════════════════════════════════
	// MATH UTILITIES
	// ═══════════════════════════════════════════════

	/**
	 * Apply luck modifier to drop chance
	 * @param BaseChance - Original drop chance
	 * @param Luck - Luck stat value
	 * @return Modified drop chance
	 */
	UFUNCTION(BlueprintPure, Category = "Loot|Calculation", meta = (
		DisplayName = "Apply Luck To Drop Chance"))
	static float ApplyLuckToDropChance(float BaseChance, float Luck);

	/**
	 * Apply magic find modifier to quantity
	 * @param BaseQuantity - Original quantity
	 * @param MagicFind - Magic find stat value
	 * @return Modified quantity
	 */
	UFUNCTION(BlueprintPure, Category = "Loot|Calculation", meta = (
		DisplayName = "Apply Magic Find To Quantity"))
	static int32 ApplyMagicFindToQuantity(int32 BaseQuantity, float MagicFind);
};