// Item/Generation/AffixGenerator.h
#pragma once

#include "CoreMinimal.h"
#include "Item/Library/ItemStructs.h"
#include "Item/Library/ItemEnums.h"
#include "Item/Library/AffixEnums.h"
#include "AffixGenerator.generated.h"

/**
 * Affix Generator - Handles all affix generation logic
 */
USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FAffixGenerator
{
	GENERATED_BODY()

public:
	// ═══════════════════════════════════════════════
	// CONFIGURATION - SEPARATE DATATABLE PATHS
	// ═══════════════════════════════════════════════

	/** Path to PREFIX affix DataTable (DT_Prefixes) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix Generator")
	FSoftObjectPath PrefixDataTablePath = FSoftObjectPath(TEXT("/Game/Data/Items/DT_Prefixes"));

	/** Path to SUFFIX affix DataTable (DT_Suffixes) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix Generator")
	FSoftObjectPath SuffixDataTablePath = FSoftObjectPath(TEXT("/Game/Data/Items/DT_Suffixes"));

	/**
	 * Path to ENCHANT affix DataTable (DT_Enchants).
	 * Enchants are applied post-generation (enchanting bench / special currency),
	 * not during normal item generation.  Stored separately in FPHItemStats::Enchants.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix Generator")
	FSoftObjectPath EnchantDataTablePath = FSoftObjectPath(TEXT("/Game/Data/Items/DT_Enchants"));

	/** Default weight for affixes without explicit weight */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix Generator")
	int32 DefaultAffixWeight = 100;

	// ═══════════════════════════════════════════════
	// MAIN GENERATION FUNCTIONS
	// ═══════════════════════════════════════════════

	/**
	 * Generate affixes for an item with corruption support
	 * @param BaseItem - Base item data
	 * @param ItemLevel - Item level (1-100)
	 * @param Rarity - Item rarity (determines affix count)
	 * @param Seed - Random seed for generation
	 * @param CorruptionChance - Chance (0-1) for each affix to be corrupted
	 * @param bForceOneCorrupted - Force at least one corrupted affix
	 */
	FPHItemStats GenerateAffixes(
		const FItemBase& BaseItem,
		int32 ItemLevel,
		EItemRarity Rarity,
		int32 Seed,
		float CorruptionChance = 0.0f,
		bool bForceOneCorrupted = false) const;

	/**
	 * Get affix counts based on rarity
	 * @param Rarity - Item rarity
	 * @param OutMinPrefixes - Min prefix count
	 * @param OutMaxPrefixes - Max prefix count
	 * @param OutMinSuffixes - Min suffix count
	 * @param OutMaxSuffixes - Max suffix count
	 */
	static void GetAffixCountByRarity(
		EItemRarity Rarity,
		int32& OutMinPrefixes,
		int32& OutMaxPrefixes,
		int32& OutMinSuffixes,
		int32& OutMaxSuffixes);

	// ═══════════════════════════════════════════════
	// DATATABLE ACCESS - SINGLE RESPONSIBILITY
	// ═══════════════════════════════════════════════

	/**
	 * Get the appropriate DataTable for the given affix type.
	 * Supports Prefix, Suffix, and Enchant.  Returns nullptr for unsupported types.
	 * @param AffixType - Prefix, Suffix, or Enchant
	 * @return DataTable pointer or nullptr if not found
	 */
	UDataTable* GetAffixDataTable(EAffixes AffixType) const;

	/**
	 * Roll one enchant from DT_Enchants and store it in OutStats.Enchants.
	 * Replaces any existing enchant (items can only have one at a time).
	 * @param BaseItem  - Item to enchant (used for type filtering)
	 * @param ItemLevel - Used to pick the correct affix tier
	 * @param Seed      - Random seed
	 * @param OutStats  - Stats struct to write the enchant into
	 * @return True if an enchant was successfully rolled
	 */
	bool ApplyEnchant(
		const FItemBase& BaseItem,
		int32 ItemLevel,
		int32 Seed,
		FPHItemStats& OutStats) const;

private:
	// ═══════════════════════════════════════════════
	// INTERNAL GENERATION - SINGLE RESPONSIBILITY
	// ═══════════════════════════════════════════════

	/**
	 * Roll affixes with corruption support
	 * @param AffixType - Prefix or Suffix
	 * @param Count - Number of affixes to roll
	 * @param ItemLevel - Item level for filtering
	 * @param ItemType - Item type for filtering
	 * @param ItemSubType - Item subtype for filtering
	 * @param CorruptionChance - Per-affix corruption chance
	 * @param bMustRollOneCorrupted - Force one corrupted if not yet rolled
	 * @param bOutHasRolledCorrupted - Output: whether a corrupted was rolled
	 * @param RandStream - Random stream
	 */
	TArray<FPHAttributeData> RollAffixesWithCorruption(
		EAffixes AffixType,
		int32 Count,
		int32 ItemLevel,
		EItemType ItemType,
		EItemSubType ItemSubType,
		float CorruptionChance,
		bool bMustRollOneCorrupted,
		bool& bOutHasRolledCorrupted,
		FRandomStream& RandStream) const;

	/**
	 * Build available affix pool filtered by corruption and group exclusion.
	 * @param AffixType      - Prefix or Suffix (routes to correct DataTable)
	 * @param bCorruptedOnly - If true, only return negative affixes
	 * @param ExcludeAffixes - Exact AttributeNames already rolled (no duplicates)
	 * @param ExcludeGroups  - AffixGroups already represented (no same-category duplicates)
	 */
	// I-09 FIX: ExcludeAffixes is now TSet<FName> for O(1) Contains() instead of O(n).
	TArray<FPHAttributeData*> BuildAffixPoolByCorruption(
		EAffixes AffixType,
		EItemType ItemType,
		EItemSubType ItemSubType,
		int32 ItemLevel,
		bool bCorruptedOnly,
		const TSet<FName>& ExcludeAffixes,
		const TSet<FName>& ExcludeGroups = TSet<FName>()) const;

	/**
	 * Select random affix from pool (weighted)
	 */
	const FPHAttributeData* SelectRandomAffix(
		const TArray<FPHAttributeData*>& AvailableAffixes,
		FRandomStream& RandStream) const;

	/**
	 * Create rolled affix instance from DataTable row
	 */
	FPHAttributeData CreateRolledAffix(
		const FPHAttributeData& TemplateAffix,
		FRandomStream& RandStream) const;

	// ═══════════════════════════════════════════════
	// LAZY-LOADED CACHED DATA - OPTIMIZATION
	// ═══════════════════════════════════════════════

	/** Cached PREFIX DataTable (lazy-loaded) */
	mutable UDataTable* CachedPrefixTable = nullptr;

	/** Cached SUFFIX DataTable (lazy-loaded) */
	mutable UDataTable* CachedSuffixTable = nullptr;

	/** Cached ENCHANT DataTable (lazy-loaded) */
	mutable UDataTable* CachedEnchantTable = nullptr;

	/**
	 * P-1 FIX: Cached row pointers for each DataTable.
	 * GetAllRows<> scans every row on every call — caching here reduces per-item-generation
	 * cost from O(n) DataTable scan to O(1) pointer read.
	 * Populated once alongside the table load; raw pointers remain valid for the DataTable's
	 * lifetime (DataTables never shrink at runtime).
	 */
	mutable TArray<FPHAttributeData*> CachedPrefixRows;
	mutable TArray<FPHAttributeData*> CachedSuffixRows;
	mutable TArray<FPHAttributeData*> CachedEnchantRows;

	/** Track if we've attempted to load prefixes (prevents repeated failures) */
	mutable bool bPrefixLoadAttempted = false;

	/** Track if we've attempted to load suffixes (prevents repeated failures) */
	mutable bool bSuffixLoadAttempted = false;

	/** Track if we've attempted to load enchants (prevents repeated failures) */
	mutable bool bEnchantLoadAttempted = false;

	// ═══════════════════════════════════════════════
	// INTERNAL HELPERS - SINGLE RESPONSIBILITY
	// ═══════════════════════════════════════════════

	/**
	 * Load and cache PREFIX DataTable
	 * SINGLE RESPONSIBILITY: Load PREFIX table only
	 */
	UDataTable* LoadPrefixDataTable() const;

	/**
	 * Load and cache SUFFIX DataTable
	 * SINGLE RESPONSIBILITY: Load SUFFIX table only
	 */
	UDataTable* LoadSuffixDataTable() const;

	/**
	 * Load and cache ENCHANT DataTable
	 * SINGLE RESPONSIBILITY: Load ENCHANT table only
	 */
	UDataTable* LoadEnchantDataTable() const;
};