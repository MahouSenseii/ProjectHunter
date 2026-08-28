#include "Item/Library/FunctionLibraries/ItemAffixSelectionFunctionLibrary.h"
#include "Engine/DataTable.h"

DEFINE_LOG_CATEGORY_STATIC(LogAffixPool, Log, All);

namespace AffixPoolPrivate
{
	/** Append Source onto Out, collapsing repeats by AffixID with the last one winning. */
	void MergeEntries(const TArray<FAffixPoolEntry>& Source, TArray<FAffixPoolEntry>& Out)
	{
		for (const FAffixPoolEntry& Entry : Source)
		{
			if (Entry.AffixID.IsNone())
			{
				continue;
			}

			// Overwrite in place rather than appending so ordering stays a
			// function of first appearance only. Generation is seeded and
			// replicated, so pool order has to be identical on every machine.
			const int32 ExistingIndex = Out.IndexOfByPredicate(
				[&Entry](const FAffixPoolEntry& Existing)
				{
					return Existing.AffixID == Entry.AffixID;
				});

			if (ExistingIndex != INDEX_NONE)
			{
				Out[ExistingIndex] = Entry;
			}
			else
			{
				Out.Add(Entry);
			}
		}
	}

	void ResolveInto(
		const UDataTable* PoolTable,
		const FName SetRowName,
		TSet<TPair<const UDataTable*, FName>>& Visited,
		FResolvedAffixPool& OutPool,
		FAffixSetResolveDiagnostics* OutDiagnostics)
	{
		if (!PoolTable || SetRowName.IsNone())
		{
			return;
		}

		// Also the cycle guard: a set that includes an ancestor is skipped here
		// rather than recursing forever.
		bool bAlreadyVisited = false;
		Visited.Add(TPair<const UDataTable*, FName>(PoolTable, SetRowName), &bAlreadyVisited);
		if (bAlreadyVisited)
		{
			if (OutDiagnostics)
			{
				OutDiagnostics->CyclicIncludes.AddUnique(SetRowName);
			}
			return;
		}

		const FAffixSet* Set = PoolTable->FindRow<FAffixSet>(
			SetRowName, TEXT("UItemAffixSelectionFunctionLibrary::ResolveAffixSet"), /*bWarnIfRowMissing=*/false);
		if (!Set)
		{
			if (OutDiagnostics)
			{
				OutDiagnostics->MissingIncludeRows.AddUnique(SetRowName);
			}

			UE_LOG(LogAffixPool, Warning,
				TEXT("ResolveAffixSet: '%s' has no row named '%s'."),
				*GetNameSafe(PoolTable), *SetRowName.ToString());
			return;
		}

		for (const FDataTableRowHandle& Include : Set->IncludedSets)
		{
			// An include may point at another table; unset means "this one".
			const UDataTable* IncludeTable = Include.DataTable ? Include.DataTable.Get() : PoolTable;
			ResolveInto(IncludeTable, Include.RowName, Visited, OutPool, OutDiagnostics);
		}

		MergeEntries(Set->Prefixes, OutPool.Prefixes);
		MergeEntries(Set->Suffixes, OutPool.Suffixes);
		MergeEntries(Set->Implicits, OutPool.Implicits);
	}
}

FName UItemAffixSelectionFunctionLibrary::FindAffixSetRowForSubType(
	const UDataTable* PoolTable,
	const EItemSubType SubType)
{
	if (!PoolTable || SubType == EItemSubType::IST_None)
	{
		return NAME_None;
	}

	FName FoundRowName = NAME_None;

	for (const TPair<FName, uint8*>& Row : PoolTable->GetRowMap())
	{
		const FAffixSet* Set = reinterpret_cast<const FAffixSet*>(Row.Value);
		if (!Set || Set->SubType != SubType)
		{
			continue;
		}

		if (!FoundRowName.IsNone())
		{
			UE_LOG(LogAffixPool, Warning,
				TEXT("FindAffixSetRowForSubType: '%s' has more than one set for sub-type %d ('%s' and '%s'); using '%s'."),
				*GetNameSafe(PoolTable), static_cast<int32>(SubType),
				*FoundRowName.ToString(), *Row.Key.ToString(), *FoundRowName.ToString());
			continue;
		}

		FoundRowName = Row.Key;
	}

	return FoundRowName;
}

bool UItemAffixSelectionFunctionLibrary::ResolveAffixSet(
	const UDataTable* PoolTable,
	const FName SetRowName,
	FResolvedAffixPool& OutPool,
	FAffixSetResolveDiagnostics* OutDiagnostics)
{
	OutPool = FResolvedAffixPool();

	if (!PoolTable || SetRowName.IsNone())
	{
		return false;
	}

	if (PoolTable->GetRowStruct() != FAffixSet::StaticStruct())
	{
		UE_LOG(LogAffixPool, Error,
			TEXT("ResolveAffixSet: '%s' must use FAffixSet rows."), *GetNameSafe(PoolTable));
		return false;
	}

	TSet<TPair<const UDataTable*, FName>> Visited;
	AffixPoolPrivate::ResolveInto(PoolTable, SetRowName, Visited, OutPool, OutDiagnostics);

	return !OutPool.IsEmpty();
}

TArray<FPHAttributeData*> UItemAffixSelectionFunctionLibrary::BuildAffixPoolByCorruption(
	const TArray<FPHAttributeData*>& SourceAffixes,
	const EItemType ItemType,
	const EItemSubType ItemSubType,
	const int32 ItemLevel,
	const bool bCorruptedOnly,
	const TSet<FName>& ExcludeAffixes,
	const TSet<FName>& ExcludeGroups)
{
	TArray<FPHAttributeData*> Pool;
	Pool.Reserve(SourceAffixes.Num() / 4);

	for (FPHAttributeData* Affix : SourceAffixes)
	{
		if (!Affix)
		{
			continue;
		}

		if (ExcludeAffixes.Contains(Affix->GetStableAffixID()))
		{
			continue;
		}

		if (Affix->AffixGroup != NAME_None && ExcludeGroups.Contains(Affix->AffixGroup))
		{
			continue;
		}

		if (!Affix->IsAllowedOnItemType(ItemType))
		{
			continue;
		}

		if (!Affix->IsAllowedOnSubType(ItemSubType))
		{
			continue;
		}

		if (!Affix->IsValidForItemLevel(ItemLevel))
		{
			continue;
		}

		// A zero weight disables an affix outright, so it must not reach the
		// pool at all - SelectWeightedAffix would otherwise resurrect it if
		// every candidate happened to be disabled.
		if (!Affix->CanEverSpawn())
		{
			continue;
		}

		const bool bIsCorrupted = Affix->IsCorruptedAffix();
		if (bCorruptedOnly && !bIsCorrupted)
		{
			continue;
		}

		if (!bCorruptedOnly && bIsCorrupted)
		{
			continue;
		}

		Pool.Add(Affix);
	}

	return Pool;
}

const FPHAttributeData* UItemAffixSelectionFunctionLibrary::SelectWeightedAffix(
	const TArray<FPHAttributeData*>& AvailableAffixes,
	FRandomStream& RandStream)
{
	if (AvailableAffixes.Num() == 0)
	{
		return nullptr;
	}

	int32 TotalWeight = 0;
	for (const FPHAttributeData* Affix : AvailableAffixes)
	{
		TotalWeight += Affix ? FMath::Max(0, Affix->GetWeight()) : 0;
	}

	// Every candidate is disabled. Returning one anyway would make a zero weight
	// mean "spawns whenever nothing else can", which is the opposite of never.
	if (TotalWeight <= 0)
	{
		return nullptr;
	}

	const int32 RandomValue = RandStream.RandRange(0, TotalWeight - 1);
	int32 CurrentWeight = 0;

	for (const FPHAttributeData* Affix : AvailableAffixes)
	{
		if (!Affix)
		{
			continue;
		}

		CurrentWeight += FMath::Max(0, Affix->GetWeight());
		if (RandomValue < CurrentWeight)
		{
			return Affix;
		}
	}

	return AvailableAffixes.Last();
}

FPHAttributeData UItemAffixSelectionFunctionLibrary::CreateRolledAffix(
	const FPHAttributeData& TemplateAffix,
	FRandomStream& RandStream)
{
	FPHAttributeData RolledAffix = TemplateAffix;
	RolledAffix.RollValue(RandStream);
	RolledAffix.GenerateUID(RandStream);
	return RolledAffix;
}
