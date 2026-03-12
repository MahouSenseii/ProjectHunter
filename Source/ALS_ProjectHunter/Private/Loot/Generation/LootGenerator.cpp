// Loot/Generation/LootGenerator.cpp

#include "Loot/Generation/LootGenerator.h"
#include "Item/ItemInstance.h"
#include "Engine/DataTable.h"

DEFINE_LOG_CATEGORY(LogLootGenerator);

// ═══════════════════════════════════════════════════════════════════════
// MAIN GENERATION FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════

FLootResultBatch FLootGenerator::GenerateLoot(
	const FLootTable& LootTable,
	const FLootDropSettings& Settings,
	int32 Seed,
	UObject* Outer) const
{
	FLootResultBatch Batch;
	
	if (LootTable.Entries.Num() == 0)
	{
		UE_LOG(LogLootGenerator, Warning, TEXT("GenerateLoot: Empty loot table"));
		return Batch;
	}
	
	FRandomStream RandStream(Seed != 0 ? Seed : FMath::Rand());
	Batch.Seed = RandStream.GetCurrentSeed();
	
	TArray<FLootEntry> FilteredEntries = FilterEntries(LootTable.Entries, Settings);
	
	if (FilteredEntries.Num() == 0)
	{
		UE_LOG(LogLootGenerator, Warning, TEXT("GenerateLoot: No valid entries after filtering"));
		return Batch;
	}
	
	int32 DropCount = CalculateDropCount(LootTable, Settings, RandStream);
	
	TArray<int32> SelectedIndices;
	
	switch (LootTable.SelectionMethod)
	{
		case ELootSelectionMethod::LSM_Weighted:
			SelectedIndices = SelectWeighted(FilteredEntries, DropCount, LootTable.bAllowDuplicates, RandStream);
			break;
			
		case ELootSelectionMethod::LSM_Sequential:
			SelectedIndices = SelectSequential(FilteredEntries, Settings, RandStream);
			break;
			
		case ELootSelectionMethod::LSM_GuaranteedOne:
			SelectedIndices = SelectGuaranteedOne(FilteredEntries, RandStream);
			break;
			
		case ELootSelectionMethod::LSM_All:
			SelectedIndices = SelectAll(FilteredEntries, Settings, RandStream);
			break;
			
		default:
			SelectedIndices = SelectWeighted(FilteredEntries, DropCount, LootTable.bAllowDuplicates, RandStream);
			break;
	}
	
	for (int32 Index : SelectedIndices)
	{
		if (FilteredEntries.IsValidIndex(Index))
		{
			FLootResult Result = CreateItemFromEntry(FilteredEntries[Index], Settings, RandStream, Outer);
			if (Result.IsValid())
			{
				Batch.AddResult(Result);
			}
		}
	}
	
	UE_LOG(LogLootGenerator, Verbose, TEXT("GenerateLoot: Generated %d items from %d entries (seed: %d)"),
		Batch.Results.Num(), FilteredEntries.Num(), Batch.Seed);
	
	return Batch;
}

FLootResultBatch FLootGenerator::GenerateLootFromHandle(
	const FDataTableRowHandle& TableHandle,
	const FLootDropSettings& Settings,
	int32 Seed,
	UObject* Outer) const
{
	const FLootTable* LootTable = GetLootTableFromHandle(TableHandle);
	
	if (!LootTable)
	{
		UE_LOG(LogLootGenerator, Warning, TEXT("GenerateLootFromHandle: Invalid table handle"));
		return FLootResultBatch();
	}
	
	return GenerateLoot(*LootTable, Settings, Seed, Outer);
}

FLootResultBatch FLootGenerator::GenerateLootWithSource(
	const FLootTable& LootTable,
	const FLootDropSettings& Settings,
	ELootSourceType SourceType,
	int32 Seed,
	UObject* Outer) const
{
	FLootResultBatch Batch = GenerateLoot(LootTable, Settings, Seed, Outer);
	Batch.SourceType = SourceType;
	return Batch;
}

// ═══════════════════════════════════════════════════════════════════════
// CORRUPTED LOOT GENERATION
// ═══════════════════════════════════════════════════════════════════════

FLootResultBatch FLootGenerator::GenerateCorruptedLoot(
	const FLootTable& LootTable,
	const FLootDropSettings& Settings,
	int32 Seed,
	UObject* Outer) const
{
	// Force all items to have at least one corrupted (negative) affix
	FLootDropSettings CorruptedSettings = Settings;
	CorruptedSettings.bForceCorruptedDrops = true;
	CorruptedSettings.CorruptionChanceMultiplier = 1.0f;
	
	return GenerateLoot(LootTable, CorruptedSettings, Seed, Outer);
}

// ═══════════════════════════════════════════════════════════════════════
// SINGLE ITEM GENERATION
// ═══════════════════════════════════════════════════════════════════════

FLootResult FLootGenerator::CreateItemFromEntry(
	const FLootEntry& Entry,
	const FLootDropSettings& Settings,
	FRandomStream& RandStream,
	UObject* Outer) const
{
	FLootResult Result;
	
	if (!Entry.IsValid())
	{
		return Result;
	}
	
	int32 Quantity = RollQuantity(Entry, Settings, RandStream);
	int32 ItemLevel = RollItemLevel(Entry, Settings, RandStream);
	EItemRarity Rarity = DetermineRarity(Entry, Settings, RandStream);
	int32 ItemSeed = RandStream.RandHelper(INT32_MAX);
	
	// ═══════════════════════════════════════════════
	// CALCULATE CORRUPTION PARAMETERS
	// Corruption = negative affixes via ERankPoints
	// ═══════════════════════════════════════════════
	
	float FinalCorruptionChance = 0.0f;
	bool bForceCorrupted = false;
	
	if (Entry.bCanBeCorrupted)
	{
		// Base corruption chance from entry
		FinalCorruptionChance = Entry.CorruptionChancePerAffix;
		
		// Apply global multiplier
		FinalCorruptionChance *= Settings.CorruptionChanceMultiplier;
		
		// Check for forced corruption
		bForceCorrupted = Entry.bForceOneCorruptedAffix || Settings.bForceCorruptedDrops;
	}
	
	// Create item instance
	UItemInstance* Item = CreateItemInstance(
		Entry,
		ItemLevel,
		Rarity,
		FinalCorruptionChance,
		bForceCorrupted,
		ItemSeed,
		Outer
	);
	
	if (Item)
	{
		Result.Item = Item;
		Result.Quantity = Quantity;
		Result.bWasCorrupted = Item->IsCorrupted();
		
		if (Quantity > 1 && Item->IsStackable())
		{
			Item->SetQuantity(Quantity);
		}
	}
	
	return Result;
}

// ═══════════════════════════════════════════════════════════════════════
// UTILITY FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════

int32 FLootGenerator::RollQuantity(
	const FLootEntry& Entry,
	const FLootDropSettings& Settings,
	FRandomStream& RandStream) const
{
	int32 BaseQuantity = RandStream.RandRange(Entry.MinQuantity, Entry.MaxQuantity);
	
	float Multiplier = Settings.QuantityMultiplier;
	Multiplier += Settings.PlayerMagicFindBonus * 0.01f;
	
	int32 FinalQuantity = FMath::RoundToInt(BaseQuantity * Multiplier);
	
	return FMath::Max(1, FinalQuantity);
}

int32 FLootGenerator::RollItemLevel(
	const FLootEntry& Entry,
	const FLootDropSettings& Settings,
	FRandomStream& RandStream) const
{
	int32 BaseLevel;
	
	// bUseItemLevel = use source's level with variance
	// !bUseItemLevel = use entry-specific range
	if (Entry.bUseItemLevel)
	{
		const int32 MinLevel = FMath::Max(1, Settings.SourceLevel - Settings.LevelVariance);
		const int32 MaxLevel = FMath::Min(100, Settings.SourceLevel + Settings.LevelVariance);
		BaseLevel = RandStream.RandRange(MinLevel, MaxLevel);
	}
	else
	{
		BaseLevel = RandStream.RandRange(Entry.MinItemLevel, Entry.MaxItemLevel);
	}
	
	return FMath::Clamp(BaseLevel, 1, 100);
}

EItemRarity FLootGenerator::DetermineRarity(
	const FLootEntry& Entry,
	const FLootDropSettings& Settings,
	FRandomStream& RandStream) const
{
	if (Entry.OverrideRarity != EItemRarity::IR_None)
	{
		return Entry.OverrideRarity;
	}
	
	if (Settings.MinimumItemRarity != EItemRarity::IR_None)
	{
		return Settings.MinimumItemRarity;
	}
	
	float UpgradeChance = Settings.RarityBonusChance;
	UpgradeChance += Settings.PlayerLuckBonus * 0.005f;
	
	EItemRarity BaseRarity = EItemRarity::IR_GradeF;
	
	switch (Settings.SourceRarity)
	{
		case EDropRarity::DR_Common:
			BaseRarity = EItemRarity::IR_GradeF;
			break;
		case EDropRarity::DR_Uncommon:
			BaseRarity = EItemRarity::IR_GradeE;
			break;
		case EDropRarity::DR_Rare:
			BaseRarity = EItemRarity::IR_GradeD;
			break;
		case EDropRarity::DR_Epic:
			BaseRarity = EItemRarity::IR_GradeC;
			break;
		case EDropRarity::DR_Legendary:
			BaseRarity = EItemRarity::IR_GradeB;
			break;
		case EDropRarity::DR_Mythical:
			BaseRarity = EItemRarity::IR_GradeA;
			break;
	}
	
	if (UpgradeChance > 0.0f && RandStream.FRand() < UpgradeChance)
	{
		int32 RarityInt = static_cast<int32>(BaseRarity);
		int32 MaxRarity = static_cast<int32>(EItemRarity::IR_GradeS);
		
		if (RarityInt < MaxRarity)
		{
			BaseRarity = static_cast<EItemRarity>(RarityInt + 1);
		}
	}
	
	return BaseRarity;
}

// ═══════════════════════════════════════════════════════════════════════
// INTERNAL HELPERS
// ═══════════════════════════════════════════════════════════════════════

int32 FLootGenerator::CalculateDropCount(
	const FLootTable& Table,
	const FLootDropSettings& Settings,
	FRandomStream& RandStream) const
{
	int32 Min = Table.MinSelections > 0 ? Table.MinSelections : Settings.MinDrops;
	int32 Max = Table.MaxSelections > 0 ? Table.MaxSelections : Settings.MaxDrops;
	
	Max = FMath::RoundToInt(Max * (1.0f + Settings.PlayerMagicFindBonus * 0.01f));
	Max = FMath::Max(Min, Max);
	
	return RandStream.RandRange(Min, Max);
}

UItemInstance* FLootGenerator::CreateItemInstance(
	const FLootEntry& Entry,
	int32 ItemLevel,
	EItemRarity Rarity,
	float CorruptionChance,
	bool bForceCorrupted,
	int32 Seed,
	UObject* Outer) const
{
	UItemInstance* Item = NewObject<UItemInstance>(Outer);
	
	if (!Item)
	{
		UE_LOG(LogLootGenerator, Error, TEXT("Failed to create ItemInstance"));
		return nullptr;
	}
	
	Item->SetSeed(Seed);
	
	if (Entry.ItemRowHandle.DataTable)
	{
		// ═══════════════════════════════════════════════
		// CORRUPTION: Use InitializeWithCorruption for corruption params
		// Corruption = negative affixes (ERankPoints < 0)
		// ═══════════════════════════════════════════════
		Item->InitializeWithCorruption(
			Entry.ItemRowHandle,
			ItemLevel,
			Rarity,
			Entry.bGenerateAffixes,
			CorruptionChance,     
			bForceCorrupted        // Force at least one corrupted
		);
	}
	else if (Entry.ItemClass)
	{
		UE_LOG(LogLootGenerator, Warning, TEXT("Class-based item creation not yet implemented"));
	}
	
	return Item;
}

// ═══════════════════════════════════════════════════════════════════════
// SELECTION METHODS
// ═══════════════════════════════════════════════════════════════════════

TArray<int32> FLootGenerator::SelectWeighted(
	const TArray<FLootEntry>& Entries,
	int32 NumToSelect,
	bool bAllowDuplicates,
	FRandomStream& RandStream) const
{
	TArray<int32> Selected;
	
	if (Entries.Num() == 0 || NumToSelect <= 0)
	{
		return Selected;
	}
	
	float TotalWeight = 0.0f;
	for (const FLootEntry& Entry : Entries)
	{
		TotalWeight += Entry.GetEffectiveWeight();
	}
	
	if (TotalWeight <= 0.0f)
	{
		return Selected;
	}
	
	TArray<int32> AvailableIndices;
	TArray<float> AvailableWeights;
	
	if (!bAllowDuplicates)
	{
		for (int32 i = 0; i < Entries.Num(); ++i)
		{
			AvailableIndices.Add(i);
			AvailableWeights.Add(Entries[i].GetEffectiveWeight());
		}
	}
	
	for (int32 i = 0; i < NumToSelect; ++i)
	{
		float CurrentTotalWeight = bAllowDuplicates ? TotalWeight : 0.0f;
		
		if (!bAllowDuplicates)
		{
			for (float W : AvailableWeights)
			{
				CurrentTotalWeight += W;
			}
			
			if (AvailableIndices.Num() == 0)
			{
				break;
			}
		}
		
		float RandomValue = RandStream.FRandRange(0.0f, CurrentTotalWeight);
		float CurrentWeight = 0.0f;
		
		if (bAllowDuplicates)
		{
			for (int32 j = 0; j < Entries.Num(); ++j)
			{
				CurrentWeight += Entries[j].GetEffectiveWeight();
				if (RandomValue < CurrentWeight)
				{
					Selected.Add(j);
					break;
				}
			}
		}
		else
		{
			for (int32 j = 0; j < AvailableIndices.Num(); ++j)
			{
				CurrentWeight += AvailableWeights[j];
				if (RandomValue < CurrentWeight)
				{
					Selected.Add(AvailableIndices[j]);
					AvailableIndices.RemoveAtSwap(j);
					AvailableWeights.RemoveAtSwap(j);
					break;
				}
			}
		}
	}
	
	return Selected;
}

TArray<int32> FLootGenerator::SelectSequential(
	const TArray<FLootEntry>& Entries,
	const FLootDropSettings& Settings,
	FRandomStream& RandStream) const
{
	TArray<int32> Selected;
	
	for (int32 i = 0; i < Entries.Num(); ++i)
	{
		const FLootEntry& Entry = Entries[i];
		float EffectiveChance = Entry.DropChance * Settings.DropChanceMultiplier;
		
		if (RandStream.FRand() < EffectiveChance)
		{
			Selected.Add(i);
		}
	}
	
	return Selected;
}

TArray<int32> FLootGenerator::SelectGuaranteedOne(
	const TArray<FLootEntry>& Entries,
	FRandomStream& RandStream) const
{
	TArray<int32> Selected;
	
	if (Entries.Num() == 0)
	{
		return Selected;
	}
	
	float TotalWeight = 0.0f;
	for (const FLootEntry& Entry : Entries)
	{
		TotalWeight += Entry.GetEffectiveWeight();
	}
	
	if (TotalWeight <= 0.0f)
	{
		Selected.Add(RandStream.RandRange(0, Entries.Num() - 1));
		return Selected;
	}
	
	float RandomValue = RandStream.FRandRange(0.0f, TotalWeight);
	float CurrentWeight = 0.0f;
	
	for (int32 i = 0; i < Entries.Num(); ++i)
	{
		CurrentWeight += Entries[i].GetEffectiveWeight();
		if (RandomValue < CurrentWeight)
		{
			Selected.Add(i);
			break;
		}
	}
	
	return Selected;
}

TArray<int32> FLootGenerator::SelectAll(
	const TArray<FLootEntry>& Entries,
	const FLootDropSettings& Settings,
	FRandomStream& RandStream) const
{
	TArray<int32> Selected;
	
	for (int32 i = 0; i < Entries.Num(); ++i)
	{
		const FLootEntry& Entry = Entries[i];
		float EffectiveChance = Entry.DropChance * Settings.DropChanceMultiplier;
		
		if (RandStream.FRand() < EffectiveChance)
		{
			Selected.Add(i);
		}
	}
	
	return Selected;
}

TArray<FLootEntry> FLootGenerator::FilterEntries(
	const TArray<FLootEntry>& Entries,
	const FLootDropSettings& Settings) const
{
	TArray<FLootEntry> Filtered;
	Filtered.Reserve(Entries.Num());
	
	for (const FLootEntry& Entry : Entries)
	{
		if (!Entry.IsValid())
		{
			continue;
		}
		
		Filtered.Add(Entry);
	}
	
	return Filtered;
}