#include "Loot/Generation/LootGenerator.h"
#include "Loot/Library/FunctionLibraries/LootRarityFunctionLibrary.h"
#include "Loot/Library/FunctionLibraries/LootSelectionFunctionLibrary.h"
#include "Item/ItemInstance.h"
#include "Engine/DataTable.h"

DEFINE_LOG_CATEGORY(LogLootGenerator);

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

	const TArray<FLootEntry> FilteredEntries = ULootSelectionFunctionLibrary::FilterEntries(LootTable.Entries, Settings);

	if (FilteredEntries.Num() == 0)
	{
		UE_LOG(LogLootGenerator, Warning, TEXT("GenerateLoot: No valid entries after filtering"));
		return Batch;
	}

	const int32 DropCount = ULootSelectionFunctionLibrary::CalculateDropCount(LootTable, Settings, RandStream);

	TArray<int32> SelectedIndices;

	switch (LootTable.SelectionMethod)
	{
		case ELootSelectionMethod::LSM_Weighted:
			SelectedIndices = ULootSelectionFunctionLibrary::SelectWeighted(FilteredEntries, DropCount, LootTable.bAllowDuplicates, RandStream);
			break;

		case ELootSelectionMethod::LSM_Sequential:
			SelectedIndices = ULootSelectionFunctionLibrary::SelectSequential(FilteredEntries, Settings, RandStream);
			break;

		case ELootSelectionMethod::LSM_GuaranteedOne:
			SelectedIndices = ULootSelectionFunctionLibrary::SelectGuaranteedOne(FilteredEntries, RandStream);
			break;

		case ELootSelectionMethod::LSM_All:
			SelectedIndices = ULootSelectionFunctionLibrary::SelectAll(FilteredEntries);
			break;

		default:
			SelectedIndices = ULootSelectionFunctionLibrary::SelectWeighted(FilteredEntries, DropCount, LootTable.bAllowDuplicates, RandStream);
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

FLootResultBatch FLootGenerator::GenerateCorruptedLoot(
	const FLootTable& LootTable,
	const FLootDropSettings& Settings,
	int32 Seed,
	UObject* Outer) const
{
	FLootDropSettings CorruptedSettings = Settings;
	CorruptedSettings.bForceCorruptedDrops = true;
	CorruptedSettings.CorruptionChanceMultiplier = 1.0f;

	return GenerateLoot(LootTable, CorruptedSettings, Seed, Outer);
}

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

	const int32 Quantity = RollQuantity(Entry, Settings, RandStream);
	const int32 ItemLevel = RollItemLevel(Entry, Settings, RandStream);
	const EItemRarity Rarity = DetermineRarity(Entry, Settings, RandStream);
	const int32 ItemSeed = RandStream.RandHelper(INT32_MAX);

	float FinalCorruptionChance = 0.0f;
	bool bForceCorrupted = false;

	if (Entry.bCanBeCorrupted)
	{
		FinalCorruptionChance = Entry.CorruptionChancePerAffix;
		FinalCorruptionChance *= Settings.CorruptionChanceMultiplier;
		bForceCorrupted = Entry.bForceOneCorruptedAffix || Settings.bForceCorruptedDrops;
	}

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

int32 FLootGenerator::RollQuantity(
	const FLootEntry& Entry,
	const FLootDropSettings& Settings,
	FRandomStream& RandStream) const
{
	const int32 BaseQuantity = RandStream.RandRange(Entry.MinQuantity, Entry.MaxQuantity);
	const float Multiplier = Settings.QuantityMultiplier + Settings.PlayerMagicFindBonus * 0.01f;
	const int32 FinalQuantity = FMath::RoundToInt(BaseQuantity * Multiplier);

	return FMath::Max(1, FinalQuantity);
}

int32 FLootGenerator::RollItemLevel(
	const FLootEntry& Entry,
	const FLootDropSettings& Settings,
	FRandomStream& RandStream) const
{
	int32 BaseLevel;

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

	EItemRarity BaseRarity = ULootRarityFunctionLibrary::DropRarityToItemRarity(Settings.SourceRarity);
	if (BaseRarity == EItemRarity::IR_None)
	{
		BaseRarity = EItemRarity::IR_GradeF;
	}

	constexpr int32 MinimumGrade = static_cast<int32>(EItemRarity::IR_GradeF);
	constexpr int32 MaximumGeneratedGrade = static_cast<int32>(EItemRarity::IR_GradeS);
	const int32 ConfiguredMinimum = static_cast<int32>(Settings.MinimumItemRarity);
	if (ConfiguredMinimum >= MinimumGrade && ConfiguredMinimum <= MaximumGeneratedGrade)
	{
		BaseRarity = static_cast<EItemRarity>(FMath::Max(
			static_cast<int32>(BaseRarity), ConfiguredMinimum));
	}

	float UpgradeChance = Settings.RarityBonusChance;
	UpgradeChance += Settings.PlayerLuckBonus * 0.005f;
	UpgradeChance = FMath::Clamp(UpgradeChance, 0.f, 1.f);

	if (UpgradeChance > 0.0f)
	{
		constexpr float DecayFactor = 0.35f;
		float CurrentChance = UpgradeChance;
		while (CurrentChance > 0.0f && RandStream.FRand() < CurrentChance)
		{
			const int32 RarityInt = static_cast<int32>(BaseRarity);
			if (RarityInt >= MaximumGeneratedGrade)
			{
				break;
			}

			BaseRarity = static_cast<EItemRarity>(RarityInt + 1);
			CurrentChance *= DecayFactor;
		}
	}

	return BaseRarity;
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
	UItemInstance* Item = Entry.ItemClass.Get()
		? NewObject<UItemInstance>(Outer, Entry.ItemClass.Get())
		: NewObject<UItemInstance>(Outer);

	if (!Item)
	{
		UE_LOG(LogLootGenerator, Error, TEXT("Failed to create ItemInstance"));
		return nullptr;
	}

	Item->SetDeterministicSeedAndIdentity(Seed);

	if (Entry.ItemRowHandle.DataTable)
	{
		Item->InitializeWithCorruption(
			Entry.ItemRowHandle,
			ItemLevel,
			Rarity,
			Entry.bGenerateAffixes,
			CorruptionChance,
			bForceCorrupted
		);
	}
	else if (Entry.ItemClass.Get())
	{
		// Item subclasses may author their base row handle on the class defaults.
		// They still pass through the same initialization pipeline as table entries.
		if (!Item->BaseItemHandle.DataTable || Item->BaseItemHandle.RowName.IsNone())
		{
			UE_LOG(LogLootGenerator, Error,
				TEXT("Class-based item '%s' has no valid BaseItemHandle; drop rejected."),
				*GetNameSafe(Entry.ItemClass.Get()));
			return nullptr;
		}

		Item->InitializeWithCorruption(
			Item->BaseItemHandle,
			ItemLevel,
			Rarity,
			Entry.bGenerateAffixes,
			CorruptionChance,
			bForceCorrupted);
	}

	return Item;
}
