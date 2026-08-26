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
		FRandomStream& RandStream) const;

	mutable UDataTable* CachedPrefixTable = nullptr;

	mutable UDataTable* CachedSuffixTable = nullptr;

	mutable UDataTable* CachedEnchantTable = nullptr;

	// DataTable row pointers remain valid for the loaded table lifetime and avoid repeated GetAllRows scans.
	mutable TArray<FPHAttributeData*> CachedPrefixRows;
	mutable TArray<FPHAttributeData*> CachedSuffixRows;
	mutable TArray<FPHAttributeData*> CachedEnchantRows;

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
