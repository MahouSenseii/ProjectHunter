#include "Loot/Library/FunctionLibraries/LootFunctionLibrary.h"

#include "Loot/Library/FunctionLibraries/LootCalculationFunctionLibrary.h"
#include "Loot/Library/FunctionLibraries/LootCorruptionFunctionLibrary.h"
#include "Loot/Library/FunctionLibraries/LootRarityFunctionLibrary.h"
#include "Loot/Library/FunctionLibraries/LootSourceFunctionLibrary.h"
#include "Loot/Library/FunctionLibraries/LootTableFunctionLibrary.h"

FText ULootFunctionLibrary::GetDropRarityDisplayName(const EDropRarity Rarity)
{
	return ULootRarityFunctionLibrary::GetDropRarityDisplayName(Rarity);
}

FLinearColor ULootFunctionLibrary::GetDropRarityColor(const EDropRarity Rarity)
{
	return ULootRarityFunctionLibrary::GetDropRarityColor(Rarity);
}

EItemRarity ULootFunctionLibrary::DropRarityToItemRarity(const EDropRarity DropRarity)
{
	return ULootRarityFunctionLibrary::DropRarityToItemRarity(DropRarity);
}

float ULootFunctionLibrary::GetRarityMultiplier(const EDropRarity Rarity)
{
	return ULootRarityFunctionLibrary::GetRarityMultiplier(Rarity);
}

bool ULootFunctionLibrary::IsValidLootTableHandle(const FDataTableRowHandle& Handle)
{
	return ULootTableFunctionLibrary::IsValidLootTableHandle(Handle);
}

float ULootFunctionLibrary::GetLootTableTotalWeight(const FDataTableRowHandle& Handle)
{
	return ULootTableFunctionLibrary::GetLootTableTotalWeight(Handle);
}

int32 ULootFunctionLibrary::GetLootTableEntryCount(const FDataTableRowHandle& Handle)
{
	return ULootTableFunctionLibrary::GetLootTableEntryCount(Handle);
}

int32 ULootFunctionLibrary::GetCorruptedEntryCount(const FDataTableRowHandle& Handle)
{
	return ULootTableFunctionLibrary::GetCorruptedEntryCount(Handle);
}

float ULootFunctionLibrary::GetEntryDropPercentage(const FLootEntry& Entry, const float TotalTableWeight)
{
	return ULootTableFunctionLibrary::GetEntryDropPercentage(Entry, TotalTableWeight);
}

bool ULootFunctionLibrary::IsValidLootEntry(const FLootEntry& Entry)
{
	return ULootTableFunctionLibrary::IsValidLootEntry(Entry);
}

FText ULootFunctionLibrary::GetCorruptionTypeName(const ECorruptionType Type)
{
	return ULootCorruptionFunctionLibrary::GetCorruptionTypeName(Type);
}

FLinearColor ULootFunctionLibrary::GetCorruptionTypeColor(const ECorruptionType Type)
{
	return ULootCorruptionFunctionLibrary::GetCorruptionTypeColor(Type);
}

float ULootFunctionLibrary::GetCorruptionSeverity(const ECorruptionType Type)
{
	return ULootCorruptionFunctionLibrary::GetCorruptionSeverity(Type);
}

FText ULootFunctionLibrary::GetLootSourceTypeName(const ELootSourceType Type)
{
	return ULootSourceFunctionLibrary::GetLootSourceTypeName(Type);
}

FLootDropSettings ULootFunctionLibrary::GetDefaultSettingsForSourceType(const ELootSourceType Type)
{
	return ULootSourceFunctionLibrary::GetDefaultSettingsForSourceType(Type);
}

float ULootFunctionLibrary::ApplyLuckToDropChance(const float BaseChance, const float Luck)
{
	return ULootCalculationFunctionLibrary::ApplyLuckToDropChance(BaseChance, Luck);
}

int32 ULootFunctionLibrary::ApplyMagicFindToQuantity(const int32 BaseQuantity, const float MagicFind)
{
	return ULootCalculationFunctionLibrary::ApplyMagicFindToQuantity(BaseQuantity, MagicFind);
}
