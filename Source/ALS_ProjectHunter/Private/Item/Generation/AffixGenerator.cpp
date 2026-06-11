#include "Item/Generation/AffixGenerator.h"
#include "Engine/DataTable.h"

DEFINE_LOG_CATEGORY_STATIC(LogAffixGenerator, Log, All);

FPHItemStats FAffixGenerator::GenerateAffixes(
	const FItemBase& BaseItem,
	int32 ItemLevel,
	EItemRarity Rarity,
	int32 Seed,
	float CorruptionChance,
	bool bForceOneCorrupted) const
{
	FPHItemStats Stats;

	// I-02 FIX (completed): a single seeded stream drives EVERY roll in this
	// function — implicits, unique affixes, prefixes, and suffixes — so a stored
	// Seed reproduces the item exactly. The unseeded RollValue() overload pulls
	// from the global RNG and must not be used here.
	FRandomStream RandStream(Seed);

	Stats.Implicits = BaseItem.ImplicitMods;
	for (FPHAttributeData& Implicit : Stats.Implicits)
	{
		Implicit.RollValue(RandStream);
		Implicit.GenerateUID();
	}

	if (Rarity == EItemRarity::IR_GradeSS || BaseItem.bIsUnique)
	{
		Stats.Prefixes = BaseItem.UniqueAffixes;
		for (FPHAttributeData& Affix : Stats.Prefixes)
		{
			Affix.RollValue(RandStream);
			Affix.GenerateUID();
		}
		Stats.bAffixesGenerated = true;
		return Stats;
	}

	int32 MinPrefixes, MaxPrefixes, MinSuffixes, MaxSuffixes;
	GetAffixCountByRarity(Rarity, MinPrefixes, MaxPrefixes, MinSuffixes, MaxSuffixes);
	const int32 NumPrefixes = RandStream.RandRange(MinPrefixes, MaxPrefixes);
	const int32 NumSuffixes = RandStream.RandRange(MinSuffixes, MaxSuffixes);

	bool bHasRolledCorrupted = false;

	Stats.Prefixes = RollAffixesWithCorruption(
		EAffixes::AF_Prefix,
		NumPrefixes,
		ItemLevel,
		BaseItem.ItemType,
		BaseItem.ItemSubType,
		CorruptionChance,
		bForceOneCorrupted && !bHasRolledCorrupted,
		bHasRolledCorrupted,
		RandStream
	);

	Stats.Suffixes = RollAffixesWithCorruption(
		EAffixes::AF_Suffix,
		NumSuffixes,
		ItemLevel,
		BaseItem.ItemType,
		BaseItem.ItemSubType,
		CorruptionChance,
		bForceOneCorrupted && !bHasRolledCorrupted,
		bHasRolledCorrupted,
		RandStream
	);

	Stats.bAffixesGenerated = true;
	return Stats;
}

UDataTable* FAffixGenerator::GetAffixDataTable(EAffixes AffixType) const
{
	switch (AffixType)
	{
		case EAffixes::AF_Prefix:
			return LoadPrefixDataTable();

		case EAffixes::AF_Suffix:
			return LoadSuffixDataTable();

		case EAffixes::AF_Enchant:
			return LoadEnchantDataTable();

		default:
			UE_LOG(LogAffixGenerator, Warning, TEXT("AffixGenerator: Unsupported affix type %d"),
				static_cast<int32>(AffixType));
			return nullptr;
	}
}

UDataTable* FAffixGenerator::LoadEnchantDataTable() const
{
	if (CachedEnchantTable && IsValid(CachedEnchantTable))
	{
		return CachedEnchantTable;
	}

	if (bEnchantLoadAttempted && !CachedEnchantTable)
	{
		return nullptr;
	}

	bEnchantLoadAttempted = true;
	CachedEnchantTable = Cast<UDataTable>(EnchantDataTablePath.TryLoad());

	if (!CachedEnchantTable)
	{
		UE_LOG(LogAffixGenerator, Error, TEXT("AffixGenerator: Failed to load ENCHANT DataTable from '%s'"),
			*EnchantDataTablePath.ToString());
	}
	else
	{
		CachedEnchantRows.Reset();
		CachedEnchantTable->GetAllRows<FPHAttributeData>("LoadEnchantDataTable", CachedEnchantRows);

		UE_LOG(LogAffixGenerator, Log, TEXT("AffixGenerator: Loaded ENCHANT DataTable with %d rows"),
			CachedEnchantRows.Num());
	}

	return CachedEnchantTable;
}

bool FAffixGenerator::ApplyEnchant(
	const FItemBase& BaseItem,
	int32 ItemLevel,
	int32 Seed,
	FPHItemStats& OutStats) const
{
	LoadEnchantDataTable();
	if (CachedEnchantRows.Num() == 0)
	{
		UE_LOG(LogAffixGenerator, Warning, TEXT("AffixGenerator::ApplyEnchant: No enchants loaded."));
		return false;
	}

	// Filter pool by item type, subtype, and level — same rules as regular affixes.
	TArray<FPHAttributeData*> Pool;
	Pool.Reserve(CachedEnchantRows.Num());
	for (FPHAttributeData* Row : CachedEnchantRows)
	{
		if (!Row) continue;
		if (!Row->IsAllowedOnItemType(BaseItem.ItemType)) continue;
		if (!Row->IsAllowedOnSubType(BaseItem.ItemSubType)) continue;
		if (!Row->IsValidForItemLevel(ItemLevel)) continue;
		Pool.Add(Row);
	}

	if (Pool.Num() == 0)
	{
		UE_LOG(LogAffixGenerator, Warning,
			TEXT("AffixGenerator::ApplyEnchant: No valid enchants for item type %d at level %d."),
			static_cast<int32>(BaseItem.ItemType), ItemLevel);
		return false;
	}

	FRandomStream RandStream(Seed);
	const FPHAttributeData* Selected = SelectRandomAffix(Pool, RandStream);
	if (!Selected)
	{
		return false;
	}

	FPHAttributeData Rolled = CreateRolledAffix(*Selected, RandStream);
	Rolled.AffixType = EAffixes::AF_Enchant;

	// Items can only hold one enchant at a time — replace any existing one.
	OutStats.Enchants.Reset(1);
	OutStats.Enchants.Add(Rolled);
	return true;
}

UDataTable* FAffixGenerator::LoadPrefixDataTable() const
{
	if (CachedPrefixTable && IsValid(CachedPrefixTable))
	{
		return CachedPrefixTable;
	}

	if (bPrefixLoadAttempted && !CachedPrefixTable)
	{
		return nullptr;
	}

	bPrefixLoadAttempted = true;
	CachedPrefixTable = Cast<UDataTable>(PrefixDataTablePath.TryLoad());

	if (!CachedPrefixTable)
	{
		UE_LOG(LogAffixGenerator, Error, TEXT("AffixGenerator: Failed to load PREFIX DataTable from '%s'"),
			*PrefixDataTablePath.ToString());
	}
	else
	{
		// Cache all row pointers once so BuildAffixPoolByCorruption never calls
		// GetAllRows again. Raw pointers stay valid as long as the DataTable is alive.
		CachedPrefixRows.Reset();
		CachedPrefixTable->GetAllRows<FPHAttributeData>("LoadPrefixDataTable", CachedPrefixRows);

		UE_LOG(LogAffixGenerator, Log, TEXT("AffixGenerator: Loaded PREFIX DataTable with %d rows"),
			CachedPrefixRows.Num());
	}

	return CachedPrefixTable;
}

UDataTable* FAffixGenerator::LoadSuffixDataTable() const
{
	if (CachedSuffixTable && IsValid(CachedSuffixTable))
	{
		return CachedSuffixTable;
	}

	if (bSuffixLoadAttempted && !CachedSuffixTable)
	{
		return nullptr;
	}

	bSuffixLoadAttempted = true;
	CachedSuffixTable = Cast<UDataTable>(SuffixDataTablePath.TryLoad());

	if (!CachedSuffixTable)
	{
		UE_LOG(LogAffixGenerator, Error, TEXT("AffixGenerator: Failed to load SUFFIX DataTable from '%s'"),
			*SuffixDataTablePath.ToString());
	}
	else
	{
		// Cache all row pointers once (same rationale as CachedPrefixRows).
		CachedSuffixRows.Reset();
		CachedSuffixTable->GetAllRows<FPHAttributeData>("LoadSuffixDataTable", CachedSuffixRows);

		UE_LOG(LogAffixGenerator, Log, TEXT("AffixGenerator: Loaded SUFFIX DataTable with %d rows"),
			CachedSuffixRows.Num());
	}

	return CachedSuffixTable;
}

TArray<FPHAttributeData> FAffixGenerator::RollAffixesWithCorruption(
	EAffixes AffixType,
	int32 Count,
	int32 ItemLevel,
	EItemType ItemType,
	EItemSubType ItemSubType,
	float CorruptionChance,
	bool bMustRollOneCorrupted,
	bool& bOutHasRolledCorrupted,
	FRandomStream& RandStream) const
{
	TArray<FPHAttributeData> RolledAffixes;
	// TSet for O(1) Contains() lookups instead of O(n) TArray::Contains.
	TSet<FName> ExcludedAffixes;
	// Group exclusion: prevents two affixes from the same AffixGroup rolling on
	// the same item (e.g. two different "fire damage" affixes).  Mirrors the
	// POE2 affix-conflict system.  Affixes with AffixGroup == NAME_None are exempt.
	TSet<FName> ExcludedGroups;
	ExcludedAffixes.Reserve(Count);
	ExcludedGroups.Reserve(Count);

	RolledAffixes.Reserve(Count);

	for (int32 i = 0; i < Count; ++i)
	{
		const bool bShouldBeCorrupted = bMustRollOneCorrupted
			|| (CorruptionChance > 0.0f && RandStream.FRand() < CorruptionChance);

		TArray<FPHAttributeData*> AvailableAffixes = BuildAffixPoolByCorruption(
			AffixType, ItemType, ItemSubType, ItemLevel,
			bShouldBeCorrupted, ExcludedAffixes, ExcludedGroups
		);

		if (AvailableAffixes.Num() == 0)
		{
			if (bShouldBeCorrupted)
			{
				AvailableAffixes = BuildAffixPoolByCorruption(
					AffixType, ItemType, ItemSubType, ItemLevel,
					false, ExcludedAffixes, ExcludedGroups
				);
			}

			if (AvailableAffixes.Num() == 0)
			{
				UE_LOG(LogAffixGenerator, Warning, TEXT("AffixGenerator: No available affixes for type %d at level %d"),
					static_cast<int32>(AffixType), ItemLevel);
				continue;
			}
		}

		const FPHAttributeData* SelectedAffix = SelectRandomAffix(AvailableAffixes, RandStream);
		if (!SelectedAffix)
		{
			continue;
		}

		FPHAttributeData RolledAffix = CreateRolledAffix(*SelectedAffix, RandStream);
		RolledAffixes.Add(RolledAffix);

		if (RolledAffix.IsCorruptedAffix())
		{
			bOutHasRolledCorrupted = true;
		}

		ExcludedAffixes.Add(SelectedAffix->AttributeName);
		if (SelectedAffix->AffixGroup != NAME_None)
		{
			ExcludedGroups.Add(SelectedAffix->AffixGroup);
		}
	}

	return RolledAffixes;
}

TArray<FPHAttributeData*> FAffixGenerator::BuildAffixPoolByCorruption(
	EAffixes AffixType,
	EItemType ItemType,
	EItemSubType ItemSubType,
	int32 ItemLevel,
	bool bCorruptedOnly,
	const TSet<FName>& ExcludeAffixes,
	const TSet<FName>& ExcludeGroups) const
{
	TArray<FPHAttributeData*> Pool;

	// Ensure the table (and its row cache) is loaded, then use the cached row
	// pointers directly instead of calling GetAllRows<> on every invocation.
	// GetAllRows performs a full table scan each call; with many items generating
	// affixes per frame this was a significant hotspot.
	UDataTable* AffixTable = GetAffixDataTable(AffixType);
	if (!AffixTable)
	{
		return Pool;
	}

	const TArray<FPHAttributeData*>& AllAffixes =
		(AffixType == EAffixes::AF_Prefix)  ? CachedPrefixRows  :
		(AffixType == EAffixes::AF_Enchant) ? CachedEnchantRows :
		                                       CachedSuffixRows;

	Pool.Reserve(AllAffixes.Num() / 4);

	for (FPHAttributeData* Affix : AllAffixes)
	{
		if (!Affix)
		{
			continue;
		}

		// Exclude by exact affix name (prevents exact duplicate)
		if (ExcludeAffixes.Contains(Affix->AttributeName))
		{
			continue;
		}

		// Exclude by group (prevents two affixes from the same category, e.g. two fire-damage mods)
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

		const bool bIsNegative = Affix->IsCorruptedAffix();

		if (bCorruptedOnly && !bIsNegative)
		{
			continue;
		}

		if (!bCorruptedOnly && bIsNegative)
		{
			continue;
		}

		Pool.Add(Affix);
	}

	return Pool;
}

const FPHAttributeData* FAffixGenerator::SelectRandomAffix(
	const TArray<FPHAttributeData*>& AvailableAffixes,
	FRandomStream& RandStream) const
{
	if (AvailableAffixes.Num() == 0)
	{
		return nullptr;
	}

	int32 TotalWeight = 0;
	for (const FPHAttributeData* Affix : AvailableAffixes)
	{
		TotalWeight += Affix->GetWeight();
	}

	if (TotalWeight <= 0)
	{
		return AvailableAffixes[RandStream.RandRange(0, AvailableAffixes.Num() - 1)];
	}

	const int32 RandomValue = RandStream.RandRange(0, TotalWeight - 1);
	int32 CurrentWeight = 0;

	for (const FPHAttributeData* Affix : AvailableAffixes)
	{
		CurrentWeight += Affix->GetWeight();
		if (RandomValue < CurrentWeight)
		{
			return Affix;
		}
	}

	return AvailableAffixes.Last();
}

FPHAttributeData FAffixGenerator::CreateRolledAffix(
	const FPHAttributeData& TemplateAffix,
	FRandomStream& RandStream) const
{
	FPHAttributeData RolledAffix = TemplateAffix;
	// Forward the seeded RandStream to RollValue so the affix roll is
	// deterministic and reproducible from a fixed seed. The no-arg RollValue()
	// consumes FMath::RandRange (global RNG), breaking seed-based item replay.
	RolledAffix.RollValue(RandStream);
	RolledAffix.GenerateUID();
	return RolledAffix;
}

void FAffixGenerator::GetAffixCountByRarity(
	EItemRarity Rarity,
	int32& OutMinPrefixes,
	int32& OutMaxPrefixes,
	int32& OutMinSuffixes,
	int32& OutMaxSuffixes)
{
	switch (Rarity)
	{
		case EItemRarity::IR_GradeF:
			OutMinPrefixes = 0;
			OutMaxPrefixes = 0;
			OutMinSuffixes = 0;
			OutMaxSuffixes = 0;
			break;

		case EItemRarity::IR_GradeE:
			OutMinPrefixes = 0;
			OutMaxPrefixes = 1;
			OutMinSuffixes = 0;
			OutMaxSuffixes = 1;
			break;

		case EItemRarity::IR_GradeD:
			OutMinPrefixes = 1;
			OutMaxPrefixes = 1;
			OutMinSuffixes = 0;
			OutMaxSuffixes = 1;
			break;

		case EItemRarity::IR_GradeC:
			OutMinPrefixes = 1;
			OutMaxPrefixes = 2;
			OutMinSuffixes = 1;
			OutMaxSuffixes = 1;
			break;

		case EItemRarity::IR_GradeB:
			OutMinPrefixes = 1;
			OutMaxPrefixes = 2;
			OutMinSuffixes = 1;
			OutMaxSuffixes = 2;
			break;

		case EItemRarity::IR_GradeA:
			OutMinPrefixes = 2;
			OutMaxPrefixes = 3;
			OutMinSuffixes = 2;
			OutMaxSuffixes = 2;
			break;

		case EItemRarity::IR_GradeS:
			OutMinPrefixes = 2;
			OutMaxPrefixes = 3;
			OutMinSuffixes = 2;
			OutMaxSuffixes = 3;
			break;

		case EItemRarity::IR_GradeSS:
		default:
			OutMinPrefixes = 3;
			OutMaxPrefixes = 3;
			OutMinSuffixes = 3;
			OutMaxSuffixes = 3;
			break;
	}
}
