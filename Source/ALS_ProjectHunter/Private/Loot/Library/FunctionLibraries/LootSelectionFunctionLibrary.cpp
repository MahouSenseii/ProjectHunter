#include "Loot/Library/FunctionLibraries/LootSelectionFunctionLibrary.h"

TArray<FLootEntry> ULootSelectionFunctionLibrary::FilterEntries(
	const TArray<FLootEntry>& Entries,
	const FLootDropSettings& Settings)
{
	(void)Settings;

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

int32 ULootSelectionFunctionLibrary::CalculateDropCount(
	const FLootTable& Table,
	const FLootDropSettings& Settings,
	FRandomStream& RandStream)
{
	int32 Min = Table.MinSelections > 0 ? Table.MinSelections : Settings.MinDrops;
	int32 Max = Table.MaxSelections > 0 ? Table.MaxSelections : Settings.MaxDrops;

	Max = FMath::RoundToInt(Max * (1.0f + Settings.PlayerMagicFindBonus * 0.01f));
	Max = FMath::Max(Min, Max);

	return RandStream.RandRange(Min, Max);
}

TArray<int32> ULootSelectionFunctionLibrary::SelectWeighted(
	const TArray<FLootEntry>& Entries,
	int32 NumToSelect,
	bool bAllowDuplicates,
	FRandomStream& RandStream)
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
	float RemainingWeight = TotalWeight;

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
		if (!bAllowDuplicates && AvailableIndices.Num() == 0)
		{
			break;
		}

		const float CurrentTotalWeight = bAllowDuplicates ? TotalWeight : RemainingWeight;
		const float RandomValue = RandStream.FRandRange(0.0f, CurrentTotalWeight);
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
					RemainingWeight -= AvailableWeights[j];
					AvailableIndices.RemoveAtSwap(j);
					AvailableWeights.RemoveAtSwap(j);
					break;
				}
			}
		}
	}

	return Selected;
}

TArray<int32> ULootSelectionFunctionLibrary::SelectSequential(
	const TArray<FLootEntry>& Entries,
	const FLootDropSettings& Settings,
	FRandomStream& RandStream)
{
	TArray<int32> Selected;

	for (int32 i = 0; i < Entries.Num(); ++i)
	{
		const FLootEntry& Entry = Entries[i];
		const float EffectiveChance = Entry.DropChance * Settings.DropChanceMultiplier;

		if (RandStream.FRand() < EffectiveChance)
		{
			Selected.Add(i);
		}
	}

	return Selected;
}

TArray<int32> ULootSelectionFunctionLibrary::SelectGuaranteedOne(
	const TArray<FLootEntry>& Entries,
	FRandomStream& RandStream)
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

	const float RandomValue = RandStream.FRandRange(0.0f, TotalWeight);
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

TArray<int32> ULootSelectionFunctionLibrary::SelectAll(const TArray<FLootEntry>& Entries)
{
	TArray<int32> Selected;
	Selected.Reserve(Entries.Num());

	for (int32 i = 0; i < Entries.Num(); ++i)
	{
		Selected.Add(i);
	}

	return Selected;
}
