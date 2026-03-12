// Item/Generation/AffixGenerator.cpp

#include "Item/Generation/AffixGenerator.h"
#include "Engine/DataTable.h"

// ═══════════════════════════════════════════════════════════════════════
// MAIN GENERATION
// ═══════════════════════════════════════════════════════════════════════

FPHItemStats FAffixGenerator::GenerateAffixes(
	const FItemBase& BaseItem,
	int32 ItemLevel,
	EItemRarity Rarity,
	int32 Seed,
	float CorruptionChance,
	bool bForceOneCorrupted) const
{
	FPHItemStats Stats;
	
	// Copy implicits from base item
	Stats.Implicits = BaseItem.ImplicitMods;
	for (FPHAttributeData& Implicit : Stats.Implicits)
	{
		Implicit.RollValue();
		Implicit.GenerateUID();
	}
	
	// Grade SS (EX-Rank): Use unique affixes from base item
	if (Rarity == EItemRarity::IR_GradeSS || BaseItem.bIsUnique)
	{
		Stats.Prefixes = BaseItem.UniqueAffixes;
		for (FPHAttributeData& Affix : Stats.Prefixes)
		{
			Affix.RollValue();
			Affix.GenerateUID();
		}
		Stats.bAffixesGenerated = true;
		return Stats;
	}
	
	// Get affix counts based on rarity
	int32 MinPrefixes, MaxPrefixes, MinSuffixes, MaxSuffixes;
	GetAffixCountByRarity(Rarity, MinPrefixes, MaxPrefixes, MinSuffixes, MaxSuffixes);
	
	// Roll random counts
	FRandomStream RandStream(Seed);
	const int32 NumPrefixes = RandStream.RandRange(MinPrefixes, MaxPrefixes);
	const int32 NumSuffixes = RandStream.RandRange(MinSuffixes, MaxSuffixes);
	
	// Track if we've rolled a corrupted affix (for bForceOneCorrupted)
	bool bHasRolledCorrupted = false;
	
	// Generate prefixes
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
	
	// Generate suffixes
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

// ═══════════════════════════════════════════════════════════════════════
// DATATABLE ACCESS - ROUTES TO CORRECT TABLE
// ═══════════════════════════════════════════════════════════════════════

UDataTable* FAffixGenerator::GetAffixDataTable(EAffixes AffixType) const
{
	// SINGLE RESPONSIBILITY: Route to correct DataTable based on type
	switch (AffixType)
	{
		case EAffixes::AF_Prefix:
			return LoadPrefixDataTable();
			
		case EAffixes::AF_Suffix:
			return LoadSuffixDataTable();
			
		default:
			UE_LOG(LogTemp, Warning, TEXT("AffixGenerator: Unsupported affix type %d"), 
				static_cast<int32>(AffixType));
			return nullptr;
	}
}

UDataTable* FAffixGenerator::LoadPrefixDataTable() const
{
	// OPTIMIZATION: Return cached table if valid
	if (CachedPrefixTable && IsValid(CachedPrefixTable))
	{
		return CachedPrefixTable;
	}
	
	// OPTIMIZATION: Don't retry loading if already failed
	if (bPrefixLoadAttempted && !CachedPrefixTable)
	{
		return nullptr;
	}
	
	bPrefixLoadAttempted = true;
	CachedPrefixTable = Cast<UDataTable>(PrefixDataTablePath.TryLoad());
	
	if (!CachedPrefixTable)
	{
		UE_LOG(LogTemp, Error, TEXT("AffixGenerator: Failed to load PREFIX DataTable from '%s'"),
			*PrefixDataTablePath.ToString());
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("AffixGenerator: Loaded PREFIX DataTable with %d rows"),
			CachedPrefixTable->GetRowNames().Num());
	}
	
	return CachedPrefixTable;
}

UDataTable* FAffixGenerator::LoadSuffixDataTable() const
{
	// OPTIMIZATION: Return cached table if valid
	if (CachedSuffixTable && IsValid(CachedSuffixTable))
	{
		return CachedSuffixTable;
	}
	
	// OPTIMIZATION: Don't retry loading if already failed
	if (bSuffixLoadAttempted && !CachedSuffixTable)
	{
		return nullptr;
	}
	
	bSuffixLoadAttempted = true;
	CachedSuffixTable = Cast<UDataTable>(SuffixDataTablePath.TryLoad());
	
	if (!CachedSuffixTable)
	{
		UE_LOG(LogTemp, Error, TEXT("AffixGenerator: Failed to load SUFFIX DataTable from '%s'"),
			*SuffixDataTablePath.ToString());
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("AffixGenerator: Loaded SUFFIX DataTable with %d rows"),
			CachedSuffixTable->GetRowNames().Num());
	}
	
	return CachedSuffixTable;
}

// ═══════════════════════════════════════════════════════════════════════
// INTERNAL GENERATION - CORRUPTION SUPPORT
// ═══════════════════════════════════════════════════════════════════════

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
	TArray<FName> ExcludedAffixes; // Prevent duplicates
	
	// OPTIMIZATION: Pre-allocate array size
	RolledAffixes.Reserve(Count);
	
	for (int32 i = 0; i < Count; ++i)
	{
		// Determine if this affix should be corrupted
		const bool bShouldBeCorrupted = bMustRollOneCorrupted 
			|| (CorruptionChance > 0.0f && RandStream.FRand() < CorruptionChance);
		
		// Build affix pool (filtered by corruption type)
		TArray<FPHAttributeData*> AvailableAffixes = BuildAffixPoolByCorruption(
			AffixType, ItemType, ItemSubType, ItemLevel,
			bShouldBeCorrupted, ExcludedAffixes
		);
		
		// If we forced corruption but got no negative affixes, try positive ones instead
		if (AvailableAffixes.Num() == 0)
		{
			if (bShouldBeCorrupted)
			{
				AvailableAffixes = BuildAffixPoolByCorruption(
					AffixType, ItemType, ItemSubType, ItemLevel,
					false, ExcludedAffixes
				);
			}
			
			if (AvailableAffixes.Num() == 0)
			{
				UE_LOG(LogTemp, Warning, TEXT("AffixGenerator: No available affixes for type %d at level %d"),
					static_cast<int32>(AffixType), ItemLevel);
				continue;
			}
		}
		
		// Select random affix from pool
		const FPHAttributeData* SelectedAffix = SelectRandomAffix(AvailableAffixes, RandStream);
		if (!SelectedAffix)
		{
			continue;
		}
		
		// Create rolled instance with random value
		FPHAttributeData RolledAffix = CreateRolledAffix(*SelectedAffix, RandStream);
		RolledAffixes.Add(RolledAffix);
		
		// Track if we've rolled a corrupted affix
		if (RolledAffix.IsCorruptedAffix())
		{
			bOutHasRolledCorrupted = true;
		}
		
		// Exclude this affix from future rolls (prevent duplicates)
		ExcludedAffixes.Add(SelectedAffix->AttributeName);
	}
	
	return RolledAffixes;
}

TArray<FPHAttributeData*> FAffixGenerator::BuildAffixPoolByCorruption(
	EAffixes AffixType,
	EItemType ItemType,
	EItemSubType ItemSubType,
	int32 ItemLevel,
	bool bCorruptedOnly,
	const TArray<FName>& ExcludeAffixes) const
{
	TArray<FPHAttributeData*> Pool;
	
	// SINGLE RESPONSIBILITY: Get correct DataTable for type
	UDataTable* AffixTable = GetAffixDataTable(AffixType);
	if (!AffixTable)
	{
		return Pool;
	}
	
	// Get all affixes from the correct table
	TArray<FPHAttributeData*> AllAffixes;
	AffixTable->GetAllRows<FPHAttributeData>("BuildAffixPoolByCorruption", AllAffixes);
	
	// OPTIMIZATION: Pre-allocate reasonable size
	Pool.Reserve(AllAffixes.Num() / 4);
	
	// Filter affixes
	for (FPHAttributeData* Affix : AllAffixes)
	{
		if (!Affix)
		{
			continue;
		}
		
		// NOTE: No need to filter by AffixType since we're already using the correct table
		
		// Exclude already rolled affixes (prevent duplicates)
		if (ExcludeAffixes.Contains(Affix->AttributeName))
		{
			continue;
		}
		
		// Filter by item type
		if (!Affix->IsAllowedOnItemType(ItemType))
		{
			continue;
		}
		
		// Filter by item subtype
		if (!Affix->IsAllowedOnSubType(ItemSubType))
		{
			continue;
		}
		
		// Filter by item level
		if (!Affix->IsValidForItemLevel(ItemLevel))
		{
			continue;
		}
		
		// ═══════════════════════════════════════════════
		// CORRUPTION FILTER
		// ═══════════════════════════════════════════════
		const bool bIsNegative = Affix->IsCorruptedAffix();
		
		if (bCorruptedOnly && !bIsNegative)
		{
			continue; // We want corrupted, this is positive
		}
		
		if (!bCorruptedOnly && bIsNegative)
		{
			continue; // We want positive, this is corrupted
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
	
	// Calculate total weight
	int32 TotalWeight = 0;
	for (const FPHAttributeData* Affix : AvailableAffixes)
	{
		TotalWeight += Affix->GetWeight();
	}
	
	// Fallback to uniform random if no valid weights
	if (TotalWeight <= 0)
	{
		return AvailableAffixes[RandStream.RandRange(0, AvailableAffixes.Num() - 1)];
	}
	
	// Weighted random selection
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
	RolledAffix.RollValue();
	RolledAffix.GenerateUID();
	return RolledAffix;
}

// ═══════════════════════════════════════════════════════════════════════
// AFFIX COUNT HELPERS
// ═══════════════════════════════════════════════════════════════════════

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