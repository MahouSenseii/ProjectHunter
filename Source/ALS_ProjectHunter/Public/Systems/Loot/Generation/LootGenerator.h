// Loot/Generation/LootGenerator.h
// Systems/Loot/Generation/LootGenerator.h
#pragma once

#include "CoreMinimal.h"
#include "Systems/Loot/Library/LootStructs.h"
#include "LootGenerator.generated.h"

// Forward declarations
class UItemInstance;
class UDataTable;
class UObject;

DECLARE_LOG_CATEGORY_EXTERN(LogLootGenerator, Log, All);

/**
 * FLootGenerator - Pure loot generation logic
 */
USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FLootGenerator
{
	GENERATED_BODY()

public:
	// ═══════════════════════════════════════════════
	// MAIN GENERATION FUNCTIONS
	// ═══════════════════════════════════════════════

	FLootResultBatch GenerateLoot(
		const FLootTable& LootTable,
		const FLootDropSettings& Settings,
		int32 Seed,
		UObject* Outer) const;

	FLootResultBatch GenerateLootFromHandle(
		const FDataTableRowHandle& TableHandle,
		const FLootDropSettings& Settings,
		int32 Seed,
		UObject* Outer) const;

	FLootResultBatch GenerateLootWithSource(
		const FLootTable& LootTable,
		const FLootDropSettings& Settings,
		ELootSourceType SourceType,
		int32 Seed,
		UObject* Outer) const;

	// ═══════════════════════════════════════════════
	// CORRUPTED LOOT GENERATION
	// ═══════════════════════════════════════════════

	/**
	 * Generate loot with guaranteed corruption (negative affixes)
	 * All items will have at least one negative affix
	 */
	FLootResultBatch GenerateCorruptedLoot(
		const FLootTable& LootTable,
		const FLootDropSettings& Settings,
		int32 Seed,
		UObject* Outer) const;

	// ═══════════════════════════════════════════════
	// SINGLE ITEM GENERATION
	// ═══════════════════════════════════════════════

	FLootResult CreateItemFromEntry(
		const FLootEntry& Entry,
		const FLootDropSettings& Settings,
		FRandomStream& RandStream,
		UObject* Outer) const;

	// ═══════════════════════════════════════════════
	// UTILITY FUNCTIONS
	// ═══════════════════════════════════════════════

	int32 RollQuantity(
		const FLootEntry& Entry,
		const FLootDropSettings& Settings,
		FRandomStream& RandStream) const;

	int32 RollItemLevel(
		const FLootEntry& Entry,
		const FLootDropSettings& Settings,
		FRandomStream& RandStream) const;

	EItemRarity DetermineRarity(
		const FLootEntry& Entry,
		const FLootDropSettings& Settings,
		FRandomStream& RandStream) const;

	static const FLootTable* GetLootTableFromHandle(const FDataTableRowHandle& Handle);

private:
	// ═══════════════════════════════════════════════
	// INTERNAL SELECTION METHODS
	// ═══════════════════════════════════════════════

	TArray<int32> SelectWeighted(
		const TArray<FLootEntry>& Entries,
		int32 NumToSelect,
		bool bAllowDuplicates,
		FRandomStream& RandStream) const;

	TArray<int32> SelectSequential(
		const TArray<FLootEntry>& Entries,
		const FLootDropSettings& Settings,
		FRandomStream& RandStream) const;

	TArray<int32> SelectGuaranteedOne(
		const TArray<FLootEntry>& Entries,
		FRandomStream& RandStream) const;

	TArray<int32> SelectAll(
		const TArray<FLootEntry>& Entries,
		const FLootDropSettings& Settings,
		FRandomStream& RandStream) const;

	TArray<FLootEntry> FilterEntries(
		const TArray<FLootEntry>& Entries,
		const FLootDropSettings& Settings) const;

	int32 CalculateDropCount(
		const FLootTable& Table,
		const FLootDropSettings& Settings,
		FRandomStream& RandStream) const;

	/**
	 * Create item instance with corruption support
	 * @param CorruptionChance - Per-affix corruption chance
	 * @param bForceCorrupted - Guarantee at least one corrupted affix
	 */
	UItemInstance* CreateItemInstance(
		const FLootEntry& Entry,
		int32 ItemLevel,
		EItemRarity Rarity,
		float CorruptionChance,
		bool bForceCorrupted,
		int32 Seed,
		UObject* Outer) const;
};

// ═══════════════════════════════════════════════════════════════════════
// INLINE IMPLEMENTATIONS
// ═══════════════════════════════════════════════════════════════════════

FORCEINLINE const FLootTable* FLootGenerator::GetLootTableFromHandle(const FDataTableRowHandle& Handle)
{
	if (!Handle.DataTable || Handle.RowName.IsNone())
	{
		return nullptr;
	}
	return Handle.DataTable->FindRow<FLootTable>(Handle.RowName, TEXT("FLootGenerator::GetLootTableFromHandle"));
}
