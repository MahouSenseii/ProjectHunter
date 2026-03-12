// Item/Library/ItemFunctionLibrary.h
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Item/Library/ItemEnums.h"
#include "Item/Library/AffixEnums.h"
#include "Item/Library/ItemStructs.h"
#include "ItemFunctionLibrary.generated.h"

// Forward declarations
class UItemInstance;

/**
 * Damage range struct for min-max damage calculations (PoE2 style)
 */
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

/**
 * Blueprint Function Library for Item System - Hunter Manga Style
 * Provides utility functions for formatting, calculations, and item operations
 * 
 * Supports:
 * - Grade F-SS rarity formatting
 * - Hunter stat-based calculations
 * - Weight-based inventory helpers
 * - PoE2-style damage calculations
 * - Affix formatting and display
 */
UCLASS()
class ALS_PROJECTHUNTER_API UItemFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// ═══════════════════════════════════════════════
	// RARITY & DISPLAY (Hunter Manga)
	// ═══════════════════════════════════════════════

	/**
	 * Get color for rarity grade (F-SS)
	 * @param Rarity - The item rarity (Grade F-SS)
	 * @return Linear color for the rarity
	 */
	UFUNCTION(BlueprintPure, Category = "Item|Display", meta = (
		DisplayName = "Get Rarity Color",
		Keywords = "rarity color grade hunter"))
	static FLinearColor GetRarityColor(EItemRarity Rarity);

	/**
	 * Get rarity display name (Grade F-SS)
	 * @param Rarity - The item rarity
	 * @return Display name (e.g., "Grade F (Common)", "Grade SS (EX-Rank)")
	 */
	UFUNCTION(BlueprintPure, Category = "Item|Display", meta = (
		DisplayName = "Get Rarity Name",
		Keywords = "rarity name grade display"))
	static FText GetRarityDisplayName(EItemRarity Rarity);

	/**
	 * Get affix count range for rarity
	 * @param Rarity - Item rarity
	 * @return Text describing affix range (e.g., "2-3 Affixes")
	 */
	UFUNCTION(BlueprintPure, Category = "Item|Display", meta = (
		DisplayName = "Get Affix Count Text",
		Keywords = "affix count rarity display"))
	static FText GetAffixCountText(EItemRarity Rarity);

	// ═══════════════════════════════════════════════
	// AFFIX FORMATTING
	// ═══════════════════════════════════════════════

	/**
	 * Format an affix value for display in tooltip
	 * @param Value - The rolled stat value
	 * @param Format - How to display the value
	 * @param AttributeName - The attribute being modified
	 * @param MinValue - For MinMax format, the minimum value
	 * @param MaxValue - For MinMax format, the maximum value
	 * @param CustomText - For CustomText format, the custom text
	 * @return Formatted display string (e.g., "+15 to Strength", "Adds 10-20 Fire Damage")
	 */
	UFUNCTION(BlueprintPure, Category = "Item|Formatting", meta = (
		DisplayName = "Format Affix Value",
		Keywords = "format affix tooltip display"))
	static FString FormatAffixValue(
		float Value,
		EAttributeDisplayFormat Format,
		FName AttributeName,
		float MinValue = 0.0f,
		float MaxValue = 0.0f,
		const FText& CustomText = FText::GetEmpty());

	/**
	 * Format an affix from FPHAttributeData
	 * @param Affix - The attribute data to format
	 * @return Formatted display string
	 */
	UFUNCTION(BlueprintPure, Category = "Item|Formatting", meta = (
		DisplayName = "Format Affix Text",
		Keywords = "format affix tooltip"))
	static FString FormatAffixText(const FPHAttributeData& Affix);

	/**
	 * Format modify type symbol ("+", "+%", "% More", etc.)
	 * @param ModifyType - The modification type
	 * @return Symbol string
	 */
	UFUNCTION(BlueprintPure, Category = "Item|Formatting", meta = (
		DisplayName = "Get Modify Type Symbol",
		Keywords = "modify type symbol format"))
	static FString GetModifyTypeSymbol(EModifyType ModifyType);

	// ═══════════════════════════════════════════════
	// RANK POINTS / TIER FUNCTIONS
	// ═══════════════════════════════════════════════

	/**
	 * Convert ERankPoints enum to integer value
	 * @param Points - The rank points enum value
	 * @return Integer value (-10 to +10)
	 */
	UFUNCTION(BlueprintPure, Category = "Item|RankPoints", meta = (
		DisplayName = "Get Rank Points Value",
		Keywords = "rank points tier value"))
	static int32 GetRankPointsValue(ERankPoints Points);

	/**
	 * Get tier name from rank points
	 * @param Points - The rank points enum value
	 * @return Tier name (e.g., "Tier 1", "Tier 5", "Cursed")
	 */
	UFUNCTION(BlueprintPure, Category = "Item|RankPoints", meta = (
		DisplayName = "Get Tier Name",
		Keywords = "tier name rank"))
	static FText GetTierName(ERankPoints Points);

	/**
	 * Compare two affixes by rank points (for sorting)
	 * @param AffixA - First affix
	 * @param AffixB - Second affix
	 * @return True if AffixA has higher rank than AffixB
	 */
	UFUNCTION(BlueprintPure, Category = "Item|RankPoints", meta = (
		DisplayName = "Compare Affix Rank",
		Keywords = "compare rank tier affix"))
	static bool CompareAffixRank(const FPHAttributeData& AffixA, const FPHAttributeData& AffixB);

	// ═══════════════════════════════════════════════
	// NAME GENERATION (Hunter Manga Style)
	// ═══════════════════════════════════════════════

	/**
	 * Generate item name from affixes (PoE-style)
	 * Grade F/E: "Iron Sword"
	 * Grade D/C/B: "Flaming Iron Sword of the Bear"
	 * Grade A/S: "Demon-Slaying Blade"
	 * Grade SS: "[Shadow Monarch's Dagger]"
	 * @param ItemStats - The item's stats (affixes)
	 * @param ItemBase - The base item data
	 * @param Rarity - Item rarity
	 * @return Generated name
	 */
	UFUNCTION(BlueprintPure, Category = "Item|Names", meta = (
		DisplayName = "Generate Item Name",
		Keywords = "name generate affix hunter"))
	static FText GenerateItemName(
		const FPHItemStats& ItemStats,
		const FItemBase& ItemBase,
		EItemRarity Rarity);

	/**
	 * Generate a random legendary name (for Grade A/S items)
	 * @param Seed - Random seed for consistent generation
	 * @return Random name (e.g., "Demon's Fang", "Shadow Whisper", "Eternal Frost")
	 */
	UFUNCTION(BlueprintPure, Category = "Item|Names", meta = (
		DisplayName = "Generate Legendary Name",
		Keywords = "name generate legendary rare unique random"))
	static FText GenerateLegendaryName(int32 Seed);

	/**
	 * Generate prefix name from affix (e.g., "Flaming", "Heavy", "Cursed")
	 * @param Affix - The prefix affix
	 * @return Prefix name
	 */
	UFUNCTION(BlueprintPure, Category = "Item|Names", meta = (
		DisplayName = "Get Prefix Name",
		Keywords = "prefix name affix"))
	static FText GetPrefixName(const FPHAttributeData& Affix);

	/**
	 * Generate suffix name from affix (e.g., "of the Bear", "of Fire", "of Swiftness")
	 * @param Affix - The suffix affix
	 * @return Suffix name
	 */
	UFUNCTION(BlueprintPure, Category = "Item|Names", meta = (
		DisplayName = "Get Suffix Name",
		Keywords = "suffix name affix"))
	static FText GetSuffixName(const FPHAttributeData& Affix);

	// ═══════════════════════════════════════════════
	// DAMAGE CALCULATION (PoE2 Style)
	// ═══════════════════════════════════════════════

	/**
	 * Calculate final damage with all modifiers (PoE2 formula)
	 * Formula: Base → +Flat → ×Increased → ×More
	 * @param BaseDamage - Base damage range (min-max)
	 * @param FlatAdded - Flat damage to add
	 * @param IncreasedPercent - Additive increased (sum all Increased)
	 * @param MorePercent - Multiplicative more (separate multiplier)
	 * @return Final calculated damage range
	 */
	UFUNCTION(BlueprintPure, Category = "Item|Damage", meta = (
		DisplayName = "Calculate Final Damage (PoE2)",
		Keywords = "damage calculate final modifier poe"))
	static FDamageRange CalculateFinalDamage(
		FDamageRange BaseDamage,
		float FlatAdded = 0.0f,
		float IncreasedPercent = 0.0f,
		float MorePercent = 0.0f);

	/**
	 * Calculate DPS (Damage Per Second)
	 * @param DamageRange - The damage range (min-max)
	 * @param AttackSpeed - Attacks per second
	 * @return Average DPS
	 */
	UFUNCTION(BlueprintPure, Category = "Item|Damage", meta = (
		DisplayName = "Calculate DPS",
		Keywords = "dps damage per second calculate"))
	static float CalculateDPS(FDamageRange DamageRange, float AttackSpeed);

	/**
	 * Calculate critical strike damage
	 * @param BaseDamage - Base damage range
	 * @param CritMultiplier - Critical strike multiplier (default 150%)
	 * @return Critical damage range
	 */
	UFUNCTION(BlueprintPure, Category = "Item|Damage", meta = (
		DisplayName = "Calculate Critical Damage",
		Keywords = "critical crit damage calculate"))
	static FDamageRange CalculateCriticalDamage(
		FDamageRange BaseDamage,
		float CritMultiplier = 1.5f);

	// ═══════════════════════════════════════════════
	// DEFENSE CALCULATION (Hunter Manga)
	// ═══════════════════════════════════════════════

	/**
	 * Calculate final resistance with all modifiers
	 * Capped at 100% (over-cap not allowed)
	 * @param BaseResistance - Base resistance value
	 * @param FlatAdded - Flat resistance to add
	 * @param IncreasedPercent - Increased resistance percentage
	 * @return Final resistance (0-100%)
	 */
	UFUNCTION(BlueprintPure, Category = "Item|Defense", meta = (
		DisplayName = "Calculate Final Resistance",
		Keywords = "resistance calculate final defense"))
	static float CalculateFinalResistance(
		float BaseResistance,
		float FlatAdded = 0.0f,
		float IncreasedPercent = 0.0f);

	/**
	 * Calculate damage reduction from armor (PoE2 formula)
	 * @param Armor - Total armor value
	 * @param IncomingDamage - Incoming damage amount
	 * @return Damage reduction percentage (0.0 to 1.0)
	 */
	UFUNCTION(BlueprintPure, Category = "Item|Defense", meta = (
		DisplayName = "Calculate Armor Reduction",
		Keywords = "armor damage reduction calculate defense"))
	static float CalculateArmorReduction(float Armor, float IncomingDamage);

	// ═══════════════════════════════════════════════
	// WEIGHT & INVENTORY (Hunter Manga)
	// ═══════════════════════════════════════════════

	/**
	 * Calculate max carry weight from hunter strength
	 * @param Strength - Hunter's strength stat
	 * @param WeightPerStrength - Weight per point of strength (default 10.0)
	 * @return Maximum carry weight
	 */
	UFUNCTION(BlueprintPure, Category = "Item|Weight", meta = (
		DisplayName = "Calculate Max Weight From Strength",
		Keywords = "weight carry strength calculate hunter"))
	static float CalculateMaxWeightFromStrength(
		int32 Strength,
		float WeightPerStrength = 10.0f);

	/**
	 * Check if overweight (for movement penalties)
	 * @param CurrentWeight - Current total weight
	 * @param MaxWeight - Maximum carry weight
	 * @return Overweight percentage (0.0 = not overweight, 1.0 = 100% over)
	 */
	UFUNCTION(BlueprintPure, Category = "Item|Weight", meta = (
		DisplayName = "Get Overweight Percentage",
		Keywords = "overweight weight percentage penalty"))
	static float GetOverweightPercentage(float CurrentWeight, float MaxWeight);

	// ═══════════════════════════════════════════════
	// ITEM VALIDATION & REQUIREMENTS
	// ═══════════════════════════════════════════════

	/**
	 * Check if hunter meets item stat requirements
	 * @param Requirements - Item's stat requirements
	 * @param HunterLevel - Hunter's level
	 * @param Strength - Hunter's strength
	 * @param Dexterity - Hunter's dexterity
	 * @param Intelligence - Hunter's intelligence
	 * @param Endurance - Hunter's endurance
	 * @param Affliction - Hunter's affliction
	 * @param Luck - Hunter's luck
	 * @param Covenant - Hunter's covenant
	 * @return True if hunter meets all requirements
	 */
	UFUNCTION(BlueprintPure, Category = "Item|Requirements", meta = (
		DisplayName = "Meets Item Requirements",
		Keywords = "requirements meet stat check hunter"))
	static bool MeetsItemRequirements(
		const FItemStatRequirement& Requirements,
		int32 HunterLevel,
		int32 Strength,
		int32 Dexterity,
		int32 Intelligence,
		int32 Endurance,
		int32 Affliction,
		int32 Luck,
		int32 Covenant);

	/**
	 * Get required level for item (from stat requirements)
	 * @param Requirements - Item's stat requirements
	 * @return Required level
	 */
	UFUNCTION(BlueprintPure, Category = "Item|Requirements", meta = (
		DisplayName = "Get Required Level",
		Keywords = "requirements level stat"))
	static int32 GetRequiredLevel(const FItemStatRequirement& Requirements);

	// ═══════════════════════════════════════════════
	// AFFIX GENERATION HELPERS
	// ═══════════════════════════════════════════════

	/**
	 * Determine number of affixes based on rarity (F-SS grades)
	 * @param Rarity - Item rarity (Grade F-SS)
	 * @param OutMinPrefixes - Minimum prefixes
	 * @param OutMaxPrefixes - Maximum prefixes
	 * @param OutMinSuffixes - Minimum suffixes
	 * @param OutMaxSuffixes - Maximum suffixes
	 */
	UFUNCTION(BlueprintPure, Category = "Item|Generation", meta = (
		DisplayName = "Get Affix Count By Rarity",
		Keywords = "affix count rarity grade hunter"))
	static void GetAffixCountByRarity(
		EItemRarity Rarity,
		int32& OutMinPrefixes,
		int32& OutMaxPrefixes,
		int32& OutMinSuffixes,
		int32& OutMaxSuffixes);

	/**
	 * Calculate item value multiplier from rarity
	 * @param Rarity - Item rarity (Grade F-SS)
	 * @return Value multiplier (1.0 to 1000.0 for EX-Rank)
	 */
	UFUNCTION(BlueprintPure, Category = "Item|Economy", meta = (
		DisplayName = "Get Rarity Value Multiplier",
		Keywords = "rarity value multiplier grade"))
	static float GetRarityValueMultiplier(EItemRarity Rarity);

	// ═══════════════════════════════════════════════
	// ITEM COMPARISON
	// ═══════════════════════════════════════════════

	/**
	 * Compare two items by total damage
	 * @param ItemA - First item base data
	 * @param ItemB - Second item base data
	 * @return -1 if A < B, 0 if A == B, 1 if A > B
	 */
	UFUNCTION(BlueprintPure, Category = "Item|Comparison", meta = (
		DisplayName = "Compare Item Damage",
		Keywords = "compare damage item"))
	static int32 CompareItemDamage(const FItemBase& ItemA, const FItemBase& ItemB);

	/**
	 * Compare two items by total value
	 * @param ItemA - First item base data
	 * @param ItemB - Second item base data
	 * @return -1 if A < B, 0 if A == B, 1 if A > B
	 */
	UFUNCTION(BlueprintPure, Category = "Item|Comparison", meta = (
		DisplayName = "Compare Item Value",
		Keywords = "compare value price item"))
	static int32 CompareItemValue(const FItemBase& ItemA, const FItemBase& ItemB);

	/**
	 * Compare two item instances by calculated value
	 * @param ItemA - First item instance
	 * @param ItemB - Second item instance
	 * @return -1 if A < B, 0 if A == B, 1 if A > B
	 */
	UFUNCTION(BlueprintPure, Category = "Item|Comparison", meta = (
		DisplayName = "Compare Item Instance Value",
		Keywords = "compare value instance"))
	static int32 CompareItemInstanceValue(const UItemInstance* ItemA, const UItemInstance* ItemB);

	/**
	 * Compare two item instances by rarity
	 * @param ItemA - First item instance
	 * @param ItemB - Second item instance
	 * @return -1 if A < B, 0 if A == B, 1 if A > B
	 */
	UFUNCTION(BlueprintPure, Category = "Item|Comparison", meta = (
		DisplayName = "Compare Item Instance Rarity",
		Keywords = "compare rarity instance grade"))
	static int32 CompareItemInstanceRarity(const UItemInstance* ItemA, const UItemInstance* ItemB);

	/**
	 * Compare two item instances by weight
	 * @param ItemA - First item instance
	 * @param ItemB - Second item instance
	 * @return -1 if A < B, 0 if A == B, 1 if A > B
	 */
	UFUNCTION(BlueprintPure, Category = "Item|Comparison", meta = (
		DisplayName = "Compare Item Instance Weight",
		Keywords = "compare weight instance"))
	static int32 CompareItemInstanceWeight(const UItemInstance* ItemA, const UItemInstance* ItemB);

	// ═══════════════════════════════════════════════
	// UTILITY FUNCTIONS
	// ═══════════════════════════════════════════════

	/**
	 * Convert damage type to resistance type
	 * @param DamageType - Damage type
	 * @return Corresponding resistance type
	 */
	UFUNCTION(BlueprintPure, Category = "Item|Utility", meta = (
		DisplayName = "Damage Type To Resistance",
		Keywords = "damage resistance convert type"))
	static EDefenseType DamageTypeToResistance(EDamageType DamageType);

	/**
	 * Get item type display name
	 * @param ItemType - Item type
	 * @return Display name (e.g., "Weapon", "Armor", "Consumable")
	 */
	UFUNCTION(BlueprintPure, Category = "Item|Utility", meta = (
		DisplayName = "Get Item Type Name",
		Keywords = "item type name display"))
	static FText GetItemTypeName(EItemType ItemType);

	/**
	 * Get item subtype display name
	 * @param SubType - Item subtype
	 * @return Display name (e.g., "Sword", "Katana", "Helmet")
	 */
	UFUNCTION(BlueprintPure, Category = "Item|Utility", meta = (
		DisplayName = "Get Item SubType Name",
		Keywords = "item subtype name display"))
	static FText GetItemSubTypeName(EItemSubType SubType);
};