#include "Item/Generation/AffixGenerator.h"
#include "Engine/DataTable.h"
#include "Item/Library/FunctionLibraries/ItemAffixSelectionFunctionLibrary.h"

DEFINE_LOG_CATEGORY_STATIC(LogAffixGenerator, Log, All);

namespace
{
	TArray<EItemType> GearTypes()
	{
		return { EItemType::IT_Weapon, EItemType::IT_Armor, EItemType::IT_Accessory };
	}

	FPHAttributeData MakeFallbackAffix(
		const EAffixes AffixType,
		const TCHAR* Name,
		const TCHAR* Group,
		const TCHAR* Attribute,
		const EModifyType ModifyType,
		const float MinValue,
		const float MaxValue,
		TArray<EItemType> AllowedTypes = {})
	{
		FPHAttributeData Result;
		Result.AffixType = AffixType;
		Result.AffixName = FText::FromString(Name);
		Result.AffixGroup = FName(Group);
		Result.AttributeName = FName(Attribute);
		Result.ModifyType = ModifyType;
		Result.ModifiedLocation = EAffixScope::AS_Global;
		Result.MinValue = MinValue;
		Result.MaxValue = MaxValue;
		Result.AllowedItemTypes = MoveTemp(AllowedTypes);
		Result.DisplayFormat = ModifyType == EModifyType::MT_Increased
			? EAttributeDisplayFormat::ADF_Increase
			: ModifyType == EModifyType::MT_More
				? EAttributeDisplayFormat::ADF_More
				: ModifyType == EModifyType::MT_Less
					? EAttributeDisplayFormat::ADF_Less
					: EAttributeDisplayFormat::ADF_Additive;
		return Result;
	}

	bool IsAffixCompatibleWithPool(EAffixes ActualType, EAffixes PoolType)
	{
		if (ActualType == PoolType)
		{
			return true;
		}

		const bool bIsRandomAffixPool =
			PoolType == EAffixes::AF_Prefix || PoolType == EAffixes::AF_Suffix;
		return bIsRandomAffixPool && ActualType == EAffixes::AF_Corrupted;
	}

	TArray<FPHAttributeData*> ResolveConfiguredAffixTable(
		UDataTable* ConfiguredTable,
		EAffixes ExpectedType,
		const FName ItemID)
	{
		TArray<FPHAttributeData*> TableRows;
		ConfiguredTable->GetAllRows<FPHAttributeData>(
			TEXT("FAffixGenerator::ResolveConfiguredAffixTable"), TableRows);

		TArray<FPHAttributeData*> ResolvedRows;
		ResolvedRows.Reserve(TableRows.Num());

		for (FPHAttributeData* Affix : TableRows)
		{
			if (!Affix)
			{
				continue;
			}

			if (!IsAffixCompatibleWithPool(Affix->AffixType, ExpectedType))
			{
				UE_LOG(LogAffixGenerator, Warning,
					TEXT("AffixGenerator: Item '%s' uses table '%s' for pool %d, but it contains affix type %d."),
					*ItemID.ToString(),
					*GetNameSafe(ConfiguredTable),
					static_cast<int32>(ExpectedType),
					static_cast<int32>(Affix->AffixType));
				continue;
			}

			ResolvedRows.Add(Affix);
		}

		return ResolvedRows;
	}
}

FPHItemStats FAffixGenerator::GenerateAffixes(
	const FItemBase& BaseItem,
	int32 ItemLevel,
	EItemRarity Rarity,
	int32 Seed,
	float CorruptionChance,
	bool bForceOneCorrupted) const
{
	FPHItemStats Stats;

	FRandomStream RandStream(Seed);

	Stats.Implicits = BaseItem.ImplicitMods;
	for (FPHAttributeData& Implicit : Stats.Implicits)
	{
		Implicit.RollValue(RandStream);
		Implicit.GenerateUID(RandStream);
	}

	if (BaseItem.bIsUnique)
	{
		Stats.Prefixes = BaseItem.UniqueAffixes;
		for (FPHAttributeData& Affix : Stats.Prefixes)
		{
			Affix.RollValue(RandStream);
			Affix.GenerateUID(RandStream);
		}
		Stats.bAffixesGenerated = true;
		return Stats;
	}

	int32 MinPrefixes, MaxPrefixes, MinSuffixes, MaxSuffixes;
	GetAffixCountByRarity(Rarity, MinPrefixes, MaxPrefixes, MinSuffixes, MaxSuffixes);
	const int32 NumPrefixes = RandStream.RandRange(MinPrefixes, MaxPrefixes);
	const int32 NumSuffixes = RandStream.RandRange(MinSuffixes, MaxSuffixes);

	bool bHasRolledCorrupted = false;

	TArray<FPHAttributeData*> PrefixSource;
	if (BaseItem.PrefixAffixTable)
	{
		PrefixSource = ResolveConfiguredAffixTable(
			BaseItem.PrefixAffixTable, EAffixes::AF_Prefix, BaseItem.ItemID);
	}
	else
	{
		LoadPrefixDataTable();
		PrefixSource = CachedPrefixRows;
	}

	TArray<FPHAttributeData*> SuffixSource;
	if (BaseItem.SuffixAffixTable)
	{
		SuffixSource = ResolveConfiguredAffixTable(
			BaseItem.SuffixAffixTable, EAffixes::AF_Suffix, BaseItem.ItemID);
	}
	else
	{
		LoadSuffixDataTable();
		SuffixSource = CachedSuffixRows;
	}

	Stats.Prefixes = RollAffixesWithCorruption(
		PrefixSource,
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
		SuffixSource,
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
	TArray<FPHAttributeData*> EnchantSource;
	if (BaseItem.EnchantAffixTable)
	{
		EnchantSource = ResolveConfiguredAffixTable(
			BaseItem.EnchantAffixTable, EAffixes::AF_Enchant, BaseItem.ItemID);
	}
	else
	{
		LoadEnchantDataTable();
		EnchantSource = CachedEnchantRows;
	}

	if (EnchantSource.Num() == 0)
	{
		UE_LOG(LogAffixGenerator, Warning,
			TEXT("AffixGenerator::ApplyEnchant: No enchant candidates configured or loaded."));
		return false;
	}

	const TSet<FName> EmptyExcludedAffixes;
	const TSet<FName> EmptyExcludedGroups;
	TArray<FPHAttributeData*> Pool = UItemAffixSelectionFunctionLibrary::BuildAffixPoolByCorruption(
		EnchantSource,
		BaseItem.ItemType,
		BaseItem.ItemSubType,
		ItemLevel,
		false,
		EmptyExcludedAffixes,
		EmptyExcludedGroups);

	if (Pool.Num() == 0)
	{
		UE_LOG(LogAffixGenerator, Warning,
			TEXT("AffixGenerator::ApplyEnchant: No valid enchants for item type %d at level %d."),
			static_cast<int32>(BaseItem.ItemType), ItemLevel);
		return false;
	}

	FRandomStream RandStream(Seed);
	const FPHAttributeData* Selected = UItemAffixSelectionFunctionLibrary::SelectWeightedAffix(Pool, RandStream);
	if (!Selected)
	{
		return false;
	}

	FPHAttributeData Rolled = UItemAffixSelectionFunctionLibrary::CreateRolledAffix(*Selected, RandStream);
	Rolled.AffixType = EAffixes::AF_Enchant;

	// Items can only hold one enchant at a time - replace any existing one.
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
		BuildFallbackRows(EAffixes::AF_Prefix);
		UE_LOG(LogAffixGenerator, Warning, TEXT("AffixGenerator: Failed to load PREFIX DataTable from '%s'; using %d native starter affixes."),
			*PrefixDataTablePath.ToString(), CachedPrefixRows.Num());
	}
	else
	{
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
		BuildFallbackRows(EAffixes::AF_Suffix);
		UE_LOG(LogAffixGenerator, Warning, TEXT("AffixGenerator: Failed to load SUFFIX DataTable from '%s'; using %d native starter affixes."),
			*SuffixDataTablePath.ToString(), CachedSuffixRows.Num());
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

void FAffixGenerator::BuildFallbackRows(const EAffixes AffixType) const
{
	if (AffixType == EAffixes::AF_Prefix)
	{
		if (!FallbackPrefixRows.IsEmpty())
		{
			return;
		}

		FallbackPrefixRows.Reserve(6);
		FallbackPrefixRows.Add(MakeFallbackAffix(EAffixes::AF_Prefix, TEXT("Stalwart"), TEXT("Life"),
			TEXT("MaxHealth"), EModifyType::MT_Add, 15.f, 50.f, GearTypes()));
		FallbackPrefixRows.Add(MakeFallbackAffix(EAffixes::AF_Prefix, TEXT("Tempered"), TEXT("Armour"),
			TEXT("ArmourFlatBonus"), EModifyType::MT_Add, 20.f, 80.f, { EItemType::IT_Armor }));
		FallbackPrefixRows.Add(MakeFallbackAffix(EAffixes::AF_Prefix, TEXT("Jagged"), TEXT("PhysicalDamage"),
			TEXT("PhysicalPercentDamage"), EModifyType::MT_Increased, 8.f, 18.f, { EItemType::IT_Weapon }));
		FallbackPrefixRows.Add(MakeFallbackAffix(EAffixes::AF_Prefix, TEXT("Ember-touched"), TEXT("FireDamage"),
			TEXT("FireFlatDamage"), EModifyType::MT_Add, 3.f, 12.f, { EItemType::IT_Weapon }));
		FallbackPrefixRows.Add(MakeFallbackAffix(EAffixes::AF_Prefix, TEXT("Mighty"), TEXT("Strength"),
			TEXT("Strength"), EModifyType::MT_Add, 2.f, 8.f, GearTypes()));
		FallbackPrefixRows.Add(MakeFallbackAffix(EAffixes::AF_Corrupted, TEXT("Brittle"), TEXT("CorruptedDamage"),
			TEXT("GlobalMoreDamage"), EModifyType::MT_Less, 5.f, 12.f, GearTypes()));

		CachedPrefixRows.Reset(FallbackPrefixRows.Num());
		for (FPHAttributeData& Row : FallbackPrefixRows)
		{
			CachedPrefixRows.Add(&Row);
		}
		return;
	}

	if (AffixType == EAffixes::AF_Suffix)
	{
		if (!FallbackSuffixRows.IsEmpty())
		{
			return;
		}

		FallbackSuffixRows.Reserve(6);
		FallbackSuffixRows.Add(MakeFallbackAffix(EAffixes::AF_Suffix, TEXT("of Cinders"), TEXT("FireResistance"),
			TEXT("FireResistanceFlatBonus"), EModifyType::MT_Add, 5.f, 18.f, GearTypes()));
		FallbackSuffixRows.Add(MakeFallbackAffix(EAffixes::AF_Suffix, TEXT("of Rime"), TEXT("IceResistance"),
			TEXT("IceResistanceFlatBonus"), EModifyType::MT_Add, 5.f, 18.f, GearTypes()));
		FallbackSuffixRows.Add(MakeFallbackAffix(EAffixes::AF_Suffix, TEXT("of Precision"), TEXT("CriticalChance"),
			TEXT("CritChance"), EModifyType::MT_Add, 1.f, 5.f,
			{ EItemType::IT_Weapon, EItemType::IT_Accessory }));
		FallbackSuffixRows.Add(MakeFallbackAffix(EAffixes::AF_Suffix, TEXT("of Haste"), TEXT("AttackSpeed"),
			TEXT("AttackSpeed"), EModifyType::MT_More, 4.f, 12.f, { EItemType::IT_Weapon }));
		FallbackSuffixRows.Add(MakeFallbackAffix(EAffixes::AF_Suffix, TEXT("of Momentum"), TEXT("MovementSpeed"),
			TEXT("MovementSpeed"), EModifyType::MT_More, 3.f, 8.f,
			{ EItemType::IT_Armor, EItemType::IT_Accessory }));
		FallbackSuffixRows.Add(MakeFallbackAffix(EAffixes::AF_Corrupted, TEXT("of Agony"), TEXT("CorruptedDefense"),
			TEXT("GlobalDamageTakenMultiplier"), EModifyType::MT_More, 5.f, 12.f, GearTypes()));

		CachedSuffixRows.Reset(FallbackSuffixRows.Num());
		for (FPHAttributeData& Row : FallbackSuffixRows)
		{
			CachedSuffixRows.Add(&Row);
		}
	}
}

TArray<FPHAttributeData> FAffixGenerator::RollAffixesWithCorruption(
	const TArray<FPHAttributeData*>& SourceAffixes,
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
	// Prevent duplicate exclusive affix groups on the same item; NAME_None is exempt.
	TSet<FName> ExcludedGroups;
	ExcludedAffixes.Reserve(Count);
	ExcludedGroups.Reserve(Count);

	RolledAffixes.Reserve(Count);

	for (int32 i = 0; i < Count; ++i)
	{
		const bool bShouldBeCorrupted = bMustRollOneCorrupted
			|| (CorruptionChance > 0.0f && RandStream.FRand() < CorruptionChance);

		TArray<FPHAttributeData*> AvailableAffixes = UItemAffixSelectionFunctionLibrary::BuildAffixPoolByCorruption(
			SourceAffixes, ItemType, ItemSubType, ItemLevel,
			bShouldBeCorrupted, ExcludedAffixes, ExcludedGroups
		);

		if (AvailableAffixes.Num() == 0)
		{
			if (bShouldBeCorrupted && !bMustRollOneCorrupted)
			{
				AvailableAffixes = UItemAffixSelectionFunctionLibrary::BuildAffixPoolByCorruption(
					SourceAffixes, ItemType, ItemSubType, ItemLevel,
					false, ExcludedAffixes, ExcludedGroups
				);
			}

			if (AvailableAffixes.Num() == 0)
			{
				if (bMustRollOneCorrupted)
				{
					UE_LOG(LogAffixGenerator, Error,
						TEXT("AffixGenerator: No required corrupted affixes for type %d at level %d"),
						static_cast<int32>(AffixType), ItemLevel);
				}
				else
				{
					UE_LOG(LogAffixGenerator, Warning,
						TEXT("AffixGenerator: No compatible affixes for type %d at level %d"),
						static_cast<int32>(AffixType), ItemLevel);
				}
				continue;
			}
		}

		const FPHAttributeData* SelectedAffix = UItemAffixSelectionFunctionLibrary::SelectWeightedAffix(AvailableAffixes, RandStream);
		if (!SelectedAffix)
		{
			continue;
		}

		FPHAttributeData RolledAffix = UItemAffixSelectionFunctionLibrary::CreateRolledAffix(*SelectedAffix, RandStream);
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
