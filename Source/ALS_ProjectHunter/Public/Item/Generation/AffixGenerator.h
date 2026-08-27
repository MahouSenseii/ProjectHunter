#pragma once

#include "CoreMinimal.h"
#include "Item/Library/Structs/ItemStructs.h"
#include "Item/Library/Enums/ItemEnums.h"
#include "Item/Library/Enums/AffixEnums.h"
#include "AffixGenerator.generated.h"

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
};
