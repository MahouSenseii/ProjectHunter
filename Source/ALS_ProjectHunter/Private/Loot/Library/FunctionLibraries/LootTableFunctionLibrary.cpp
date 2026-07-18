#include "Loot/Library/FunctionLibraries/LootTableFunctionLibrary.h"

#include "Loot/Generation/LootGenerator.h"

bool ULootTableFunctionLibrary::IsValidLootTableHandle(const FDataTableRowHandle& Handle)
{
	return FLootGenerator::GetLootTableFromHandle(Handle) != nullptr;
}

float ULootTableFunctionLibrary::GetLootTableTotalWeight(const FDataTableRowHandle& Handle)
{
	const FLootTable* Table = FLootGenerator::GetLootTableFromHandle(Handle);
	return Table ? Table->GetTotalWeight() : 0.0f;
}

int32 ULootTableFunctionLibrary::GetLootTableEntryCount(const FDataTableRowHandle& Handle)
{
	const FLootTable* Table = FLootGenerator::GetLootTableFromHandle(Handle);
	return Table ? Table->Entries.Num() : 0;
}

int32 ULootTableFunctionLibrary::GetCorruptedEntryCount(const FDataTableRowHandle& Handle)
{
	const FLootTable* Table = FLootGenerator::GetLootTableFromHandle(Handle);
	return Table ? Table->GetCorruptedEntries().Num() : 0;
}

float ULootTableFunctionLibrary::GetEntryDropPercentage(const FLootEntry& Entry, const float TotalTableWeight)
{
	if (TotalTableWeight <= 0.0f)
	{
		return 0.0f;
	}

	return Entry.GetEffectiveWeight() / TotalTableWeight * 100.0f;
}

bool ULootTableFunctionLibrary::IsValidLootEntry(const FLootEntry& Entry)
{
	return Entry.IsValid();
}
