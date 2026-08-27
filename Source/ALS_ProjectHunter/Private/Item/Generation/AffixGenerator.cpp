#include "Item/Generation/AffixGenerator.h"
#include "Engine/DataTable.h"
#include "Item/Library/FunctionLibraries/ItemAffixSelectionFunctionLibrary.h"
#include "Item/Library/Structs/AffixStructs.h"

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
		Result.AffixID = FName(Attribute);
		Result.AffixName = FText::FromString(Name);
		Result.AffixGroup = FName(Group);
		Result.AttributeName = FName(Attribute);
		Result.ModifyType = ModifyType;
		Result.ModifiedLocation = EAffixScope::AS_Global;
		Result.MinValue = MinValue;
		Result.MaxValue = MaxValue;
		Result.AllowedItemTypes = MoveTemp(AllowedTypes);
		Result.SpawnWeight = 100;
		Result.PowerValue = AffixType == EAffixes::AF_Corrupted ? -10.0f : 10.0f;
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

	EAttributeDisplayFormat ConvertDisplayFormat(const EAffixDisplayFormat Format)
	{
		switch (Format)
		{
		case EAffixDisplayFormat::ADF_Percentage: return EAttributeDisplayFormat::ADF_Percent;
		case EAffixDisplayFormat::ADF_Range:
		case EAffixDisplayFormat::ADF_PercentRange: return EAttributeDisplayFormat::ADF_MinMax;
		case EAffixDisplayFormat::ADF_Skill: return EAttributeDisplayFormat::ADF_SkillGrant;
		case EAffixDisplayFormat::ADF_CustomFormat: return EAttributeDisplayFormat::ADF_CustomText;
		case EAffixDisplayFormat::ADF_FlatValue:
		default: return EAttributeDisplayFormat::ADF_Additive;
		}
	}

	void CopyRuntimeRowsFromTable(
		const UDataTable& Table,
		const TCHAR* Context,
		TArray<FPHAttributeData>& OutRows)
	{
		OutRows.Reset();

		if (Table.GetRowStruct() == FAffixData::StaticStruct())
		{
			TArray<FAffixData*> Definitions;
			Table.GetAllRows<FAffixData>(Context, Definitions);
			for (const FAffixData* Definition : Definitions)
			{
				if (!Definition)
				{
					continue;
				}

				for (const FAffixTier& Tier : Definition->Tiers)
				{
					FPHAttributeData Runtime;
					Runtime.AffixID = Definition->AffixID;
					Runtime.AffixType = Definition->AffixType;
					Runtime.AffixName = Definition->AffixName;
					Runtime.AffixGroup = Definition->TagGroup;
					Runtime.TierNumber = Tier.TierNumber;
					Runtime.PowerValue = Tier.PowerValue;
					Runtime.SpawnWeight = FMath::Max(0, FMath::RoundToInt(
						Definition->GetEffectiveWeight() * Tier.WeightMultiplier));
					Runtime.PrimaryTag = Definition->PrimaryTag;
					Runtime.SecondaryTags = Definition->SecondaryTags;
					Runtime.AllowedItemTypes = Definition->AllowedItemTypes;
					Runtime.AllowedSubTypes = Definition->AllowedSubTypes;
					Runtime.ExcludedItemTypes = Definition->ExcludedItemTypes;
					Runtime.MinItemLevel = FMath::Max(1, Tier.MinItemLevel);
					Runtime.MaxItemLevel = FMath::Max(Runtime.MinItemLevel, Tier.MaxItemLevel);
					Runtime.ModifiedAttribute = Tier.ModifiedAttribute;
					Runtime.AttributeName = Definition->AttributeName.IsNone()
						? Definition->AffixID
						: Definition->AttributeName;
					Runtime.ModifyType = Tier.ModifyType;
					Runtime.ModifiedLocation = Definition->bIsLocal ? EAffixScope::AS_Local : Definition->Scope;
					Runtime.Condition = Definition->Condition;
					Runtime.MinValue = Tier.MinValue;
					Runtime.MaxValue = Tier.MaxValue;
					Runtime.DisplayFormat = ConvertDisplayFormat(Definition->FormatType);
					Runtime.DisplayText = Definition->DisplayFormat;
					Runtime.GameplayEffect = Definition->GameplayEffect;
					OutRows.Add(MoveTemp(Runtime));
				}
			}
			return;
		}

		if (Table.GetRowStruct() == FPHAttributeData::StaticStruct())
		{
			TArray<FPHAttributeData*> LegacyRows;
			Table.GetAllRows<FPHAttributeData>(Context, LegacyRows);
			OutRows.Reserve(LegacyRows.Num());
			for (const FPHAttributeData* Row : LegacyRows)
			{
				if (Row)
				{
					OutRows.Add(*Row);
				}
			}
			return;
		}

		UE_LOG(LogAffixGenerator, Error,
			TEXT("AffixGenerator: DataTable '%s' must use FAffixData (preferred) or legacy FPHAttributeData rows."),
			*GetNameSafe(&Table));
	}

	TArray<FPHAttributeData*> ResolveConfiguredAffixTable(
		UDataTable* ConfiguredTable,
		EAffixes ExpectedType,
		const FName ItemID,
		TArray<FPHAttributeData>& OutOwnedRows)
	{
		CopyRuntimeRowsFromTable(*ConfiguredTable,
			TEXT("FAffixGenerator::ResolveConfiguredAffixTable"), OutOwnedRows);

		TArray<FPHAttributeData*> ResolvedRows;
		ResolvedRows.Reserve(OutOwnedRows.Num());

		for (FPHAttributeData& OwnedAffix : OutOwnedRows)
		{
			FPHAttributeData* Affix = &OwnedAffix;
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
	TSet<FName> ExcludedAffixes;
	TSet<FName> ExcludedGroups;

	TArray<FPHAttributeData*> PrefixSource;
	TArray<FPHAttributeData> ConfiguredPrefixRows;
	if (BaseItem.PrefixAffixTable)
	{
		PrefixSource = ResolveConfiguredAffixTable(
			BaseItem.PrefixAffixTable, EAffixes::AF_Prefix, BaseItem.ItemID, ConfiguredPrefixRows);
	}
	else
	{
		LoadPrefixDataTable();
		PrefixSource = CachedPrefixRows;
	}

	TArray<FPHAttributeData*> SuffixSource;
	TArray<FPHAttributeData> ConfiguredSuffixRows;
	if (BaseItem.SuffixAffixTable)
	{
		SuffixSource = ResolveConfiguredAffixTable(
			BaseItem.SuffixAffixTable, EAffixes::AF_Suffix, BaseItem.ItemID, ConfiguredSuffixRows);
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
		ExcludedAffixes,
		ExcludedGroups,
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
		ExcludedAffixes,
		ExcludedGroups,
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
		CacheRowsFromTable(*CachedEnchantTable, TEXT("LoadEnchantDataTable"), CachedEnchantRowData, CachedEnchantRows);

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
	TArray<FPHAttributeData> ConfiguredEnchantRows;
	if (BaseItem.EnchantAffixTable)
	{
		EnchantSource = ResolveConfiguredAffixTable(
			BaseItem.EnchantAffixTable, EAffixes::AF_Enchant, BaseItem.ItemID, ConfiguredEnchantRows);
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

void FAffixGenerator::RebuildRowPointers(
	TArray<FPHAttributeData>& OwnedRows,
	TArray<FPHAttributeData*>& OutPointers)
{
	OutPointers.Reset(OwnedRows.Num());
	for (FPHAttributeData& Row : OwnedRows)
	{
		OutPointers.Add(&Row);
	}
}

void FAffixGenerator::CacheRowsFromTable(
	const UDataTable& Table,
	const TCHAR* Context,
	TArray<FPHAttributeData>& OutOwnedRows,
	TArray<FPHAttributeData*>& OutPointers)
{
	// Copy out immediately. Holding the table's own row pointers would dangle if
	// the asset is reimported or edited while the generator is alive.
	CopyRuntimeRowsFromTable(Table, Context, OutOwnedRows);

	RebuildRowPointers(OutOwnedRows, OutPointers);
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
		CacheRowsFromTable(*CachedPrefixTable, TEXT("LoadPrefixDataTable"), CachedPrefixRowData, CachedPrefixRows);

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
		CacheRowsFromTable(*CachedSuffixTable, TEXT("LoadSuffixDataTable"), CachedSuffixRowData, CachedSuffixRows);

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

		// Move the fallback set into the owned cache so the pointer view has a
		// single, stable backing store regardless of where the rows came from.
		CachedPrefixRowData = FallbackPrefixRows;
		RebuildRowPointers(CachedPrefixRowData, CachedPrefixRows);
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

		// Move the fallback set into the owned cache so the pointer view has a
		// single, stable backing store regardless of where the rows came from.
		CachedSuffixRowData = FallbackSuffixRows;
		RebuildRowPointers(CachedSuffixRowData, CachedSuffixRows);
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
	TSet<FName>& InOutExcludedAffixes,
	TSet<FName>& InOutExcludedGroups,
	FRandomStream& RandStream) const
{
	TArray<FPHAttributeData> RolledAffixes;
	InOutExcludedAffixes.Reserve(InOutExcludedAffixes.Num() + Count);
	InOutExcludedGroups.Reserve(InOutExcludedGroups.Num() + Count);

	RolledAffixes.Reserve(Count);

	// The forced-corruption guarantee is a debt owed exactly once, not a mode.
	// Reading bMustRollOneCorrupted directly inside the loop forced every affix
	// in this batch to be corrupted. Track it as a pending flag and retire it as
	// soon as a corrupted affix actually lands - natural CorruptionChance rolls
	// can still add more corruption on top.
	bool bForcedCorruptionPending = bMustRollOneCorrupted;

	for (int32 i = 0; i < Count; ++i)
	{
		const bool bShouldBeCorrupted = bForcedCorruptionPending
			|| (CorruptionChance > 0.0f && RandStream.FRand() < CorruptionChance);

		TArray<FPHAttributeData*> AvailableAffixes = UItemAffixSelectionFunctionLibrary::BuildAffixPoolByCorruption(
			SourceAffixes, ItemType, ItemSubType, ItemLevel,
			bShouldBeCorrupted, InOutExcludedAffixes, InOutExcludedGroups
		);

		if (AvailableAffixes.Num() == 0)
		{
			if (bShouldBeCorrupted)
			{
				// No corrupted candidate is available. Fill the slot from the normal
				// pool rather than dropping the affix, and keep the debt pending so a
				// later slot can still satisfy the guarantee.
				if (bForcedCorruptionPending)
				{
					UE_LOG(LogAffixGenerator, Warning,
						TEXT("AffixGenerator: No corrupted affixes available for type %d at level %d; ")
						TEXT("filling slot %d from the normal pool and retrying the guarantee."),
						static_cast<int32>(AffixType), ItemLevel, i);
				}

				AvailableAffixes = UItemAffixSelectionFunctionLibrary::BuildAffixPoolByCorruption(
					SourceAffixes, ItemType, ItemSubType, ItemLevel,
					false, InOutExcludedAffixes, InOutExcludedGroups
				);
			}

			if (AvailableAffixes.Num() == 0)
			{
				UE_LOG(LogAffixGenerator, Warning,
					TEXT("AffixGenerator: No compatible affixes for type %d at level %d"),
					static_cast<int32>(AffixType), ItemLevel);
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
			bForcedCorruptionPending = false;
		}

		InOutExcludedAffixes.Add(SelectedAffix->GetStableAffixID());
		if (SelectedAffix->AffixGroup != NAME_None)
		{
			InOutExcludedGroups.Add(SelectedAffix->AffixGroup);
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
