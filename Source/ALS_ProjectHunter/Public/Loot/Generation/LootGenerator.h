#pragma once

#include "CoreMinimal.h"
#include "Loot/Library/Structs/LootStructs.h"
#include "LootGenerator.generated.h"

class UItemInstance;
class UObject;

DECLARE_LOG_CATEGORY_EXTERN(LogLootGenerator, Log, All);

USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FLootGenerator
{
	GENERATED_BODY()

public:
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

	FLootResultBatch GenerateCorruptedLoot(
		const FLootTable& LootTable,
		const FLootDropSettings& Settings,
		int32 Seed,
		UObject* Outer) const;

	FLootResult CreateItemFromEntry(
		const FLootEntry& Entry,
		const FLootDropSettings& Settings,
		FRandomStream& RandStream,
		UObject* Outer) const;

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
	UItemInstance* CreateItemInstance(
		const FLootEntry& Entry,
		int32 ItemLevel,
		EItemRarity Rarity,
		float CorruptionChance,
		bool bForceCorrupted,
		int32 Seed,
		UObject* Outer) const;
};

FORCEINLINE const FLootTable* FLootGenerator::GetLootTableFromHandle(const FDataTableRowHandle& Handle)
{
	if (!Handle.DataTable || Handle.RowName.IsNone())
	{
		return nullptr;
	}

	return Handle.DataTable->FindRow<FLootTable>(Handle.RowName, TEXT("FLootGenerator::GetLootTableFromHandle"));
}
