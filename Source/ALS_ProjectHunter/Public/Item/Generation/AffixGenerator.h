#pragma once

#include "CoreMinimal.h"
#include "Item/Library/Structs/ItemStructs.h"
#include "Item/Library/Enums/ItemEnums.h"
#include "Item/Library/Enums/AffixEnums.h"
#include "AffixGenerator.generated.h"

/**
 * One item sub-type's resolved affix pool, materialised as runtime rows.
 *
 * Built once per sub-type on first use: resolving the set and matching every
 * pool entry against the definition table is O(entries x definitions), which is
 * far too much to redo for every item that drops.
 */
struct FAffixSubTypePoolCache
{
	/** False when the pool table has no set for this sub-type - cached so the lookup is not retried. */
	bool bHasPool = false;

	TArray<FPHAttributeData> PrefixRowData;
	TArray<FPHAttributeData> SuffixRowData;

	// Views over the owned arrays above; see the note on CachedPrefixRows.
	TArray<FPHAttributeData*> PrefixRows;
	TArray<FPHAttributeData*> SuffixRows;
};

USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FAffixGenerator
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix Generator")
	FSoftObjectPath PrefixDataTablePath = FSoftObjectPath(TEXT("/Game/ProjectHunter/Item/Affixes/DT_Prefixes.DT_Prefixes"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix Generator")
	FSoftObjectPath SuffixDataTablePath = FSoftObjectPath(TEXT("/Game/ProjectHunter/Item/Affixes/DT_Suffixes.DT_Suffixes"));

	// Enchants are applied after normal generation and stored separately in FPHItemStats::Enchants.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix Generator")
	FSoftObjectPath EnchantDataTablePath = FSoftObjectPath(TEXT("/Game/ProjectHunter/Item/Affixes/DT_Enchants.DT_Enchants"));

	/**
	 * FAffixSet rows naming which affixes each item sub-type can roll.
	 *
	 * When a set exists for the item's sub-type it replaces the whole-table scan:
	 * the pool is the list, so nothing outside it can roll. Items whose sub-type
	 * has no set fall back to the shared tables above, which keeps loot working
	 * while the pools are still being authored.
	 *
	 * A per-base PrefixAffixTable/SuffixAffixTable override still wins over this.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix Generator")
	FSoftObjectPath AffixPoolTablePath = FSoftObjectPath(TEXT("/Game/ProjectHunter/Item/Affixes/DT_AffixPools.DT_AffixPools"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix Generator")
	int32 DefaultAffixWeight = 100;

	FPHItemStats GenerateAffixes(
		const FItemBase& BaseItem,
		int32 ItemLevel,
		EItemRarity Rarity,
		int32 Seed,
		float CorruptionChance = 0.0f,
		bool bForceOneCorrupted = false) const;

	static void GetAffixCountByRarity(
		EItemRarity Rarity,
		int32& OutMinPrefixes,
		int32& OutMaxPrefixes,
		int32& OutMinSuffixes,
		int32& OutMaxSuffixes);

	UDataTable* GetAffixDataTable(EAffixes AffixType) const;

	/**
	 * Use these exact tables instead of loading the configured paths, for tests
	 * and for swapping the affix set at runtime. Null disables that source
	 * rather than falling back to its path. Drops every cached pool, which is
	 * materialised from the definition rows.
	 */
	void SetAffixDefinitionTables(UDataTable* PrefixTable, UDataTable* SuffixTable);
	void SetAffixPoolTable(UDataTable* PoolTable);

	// Items can only hold one enchant at a time; a successful roll replaces the current enchant.
	bool ApplyEnchant(
		const FItemBase& BaseItem,
		int32 ItemLevel,
		int32 Seed,
		FPHItemStats& OutStats) const;

private:
	TArray<FPHAttributeData> RollAffixesWithCorruption(
		const TArray<FPHAttributeData*>& SourceAffixes,
		EAffixes AffixType,
		int32 Count,
		int32 ItemLevel,
		EItemType ItemType,
		EItemSubType ItemSubType,
		float CorruptionChance,
		bool bMustRollOneCorrupted,
		bool& bOutHasRolledCorrupted,
		TSet<FName>& InOutExcludedAffixes,
		TSet<FName>& InOutExcludedGroups,
		FRandomStream& RandStream) const;

	mutable UDataTable* CachedPrefixTable = nullptr;

	mutable UDataTable* CachedSuffixTable = nullptr;

	mutable UDataTable* CachedEnchantTable = nullptr;

	// Owned copies of the affix rows.
	//
	// These used to be raw FPHAttributeData* into the DataTable's own row map.
	// That is safe for the table's lifetime but NOT across mutation: reimporting
	// or editing an affix table during PIE empties and reallocates the row map,
	// leaving every cached pointer dangling. Copying the rows costs a few KB once
	// per table and removes the hazard.
	mutable TArray<FPHAttributeData> CachedPrefixRowData;
	mutable TArray<FPHAttributeData> CachedSuffixRowData;
	mutable TArray<FPHAttributeData> CachedEnchantRowData;

	// Pointer views over the owned copies above. Rebuilt whenever the owning
	// array is refilled and never appended to afterwards, so a reallocation can
	// never invalidate them.
	mutable TArray<FPHAttributeData*> CachedPrefixRows;
	mutable TArray<FPHAttributeData*> CachedSuffixRows;
	mutable TArray<FPHAttributeData*> CachedEnchantRows;

	/** Refills a pointer view from its owned copies. */
	static void RebuildRowPointers(
		TArray<FPHAttributeData>& OwnedRows,
		TArray<FPHAttributeData*>& OutPointers);

	/** Copies every row out of a DataTable into owned storage, then rebuilds the view. */
	static void CacheRowsFromTable(
		const UDataTable& Table,
		const TCHAR* Context,
		TArray<FPHAttributeData>& OutOwnedRows,
		TArray<FPHAttributeData*>& OutPointers);

	// Minimal native pools keep loot functional before designers author DataTables.
	mutable TArray<FPHAttributeData> FallbackPrefixRows;
	mutable TArray<FPHAttributeData> FallbackSuffixRows;

	mutable bool bPrefixLoadAttempted = false;

	mutable bool bSuffixLoadAttempted = false;

	mutable bool bEnchantLoadAttempted = false;

	UDataTable* LoadPrefixDataTable() const;

	UDataTable* LoadSuffixDataTable() const;

	UDataTable* LoadEnchantDataTable() const;

	void BuildFallbackRows(EAffixes AffixType) const;

	// SUB-TYPE AFFIX POOLS

	mutable UDataTable* CachedPoolTable = nullptr;

	mutable bool bPoolLoadAttempted = false;

	// Held by shared pointer so the cache entries never move: the RowData arrays
	// inside them are pointed into by their own PrefixRows/SuffixRows views.
	mutable TMap<EItemSubType, TSharedPtr<FAffixSubTypePoolCache>> CachedSubTypePools;

	UDataTable* LoadAffixPoolTable() const;

	/** Resolved pool for SubType, building it on first use. Null when pools are not in play. */
	const FAffixSubTypePoolCache* GetSubTypePool(EItemSubType SubType) const;
};
