#include "Item/Library/FunctionLibraries/AffixPoolValidationLibrary.h"

#include "Engine/DataTable.h"
#include "Item/Generation/AffixGenerator.h"
#include "Item/Library/FunctionLibraries/ItemAffixSelectionFunctionLibrary.h"
#include "Item/Library/FunctionLibraries/ItemEnumFunctionLibrary.h"
#include "Item/Library/Structs/AffixStructs.h"
#include "Item/Library/Structs/ItemAttributeStructs.h"
#include "UObject/UObjectIterator.h"

DEFINE_LOG_CATEGORY_STATIC(LogAffixPoolValidation, Log, All);

namespace AffixPoolValidationPrivate
{
	/** The definition-table facts validation needs, from either row format. */
	struct FAffixDefinitionSummary
	{
		EAffixes AffixType = EAffixes::AF_Prefix;

		/** Already resolved through the rarity fallback; 0 means never spawns. */
		int32 Weight = 0;

		TSet<int32> TierNumbers;
		TArray<EItemSubType> AllowedSubTypes;
		TArray<EItemType> AllowedItemTypes;
		TArray<EItemType> ExcludedItemTypes;
	};

	using FDefinitionMap = TMap<FName, FAffixDefinitionSummary>;

	void CollectDefinitions(const UDataTable* Table, FDefinitionMap& OutDefinitions)
	{
		if (!Table)
		{
			return;
		}

		// Both row formats are accepted by the generator, so both are accepted here.
		if (Table->GetRowStruct() == FAffixData::StaticStruct())
		{
			for (const TPair<FName, uint8*>& Row : Table->GetRowMap())
			{
				const FAffixData* Definition = reinterpret_cast<const FAffixData*>(Row.Value);
				if (!Definition || Definition->AffixID.IsNone())
				{
					continue;
				}

				FAffixDefinitionSummary& Summary = OutDefinitions.FindOrAdd(Definition->AffixID);
				Summary.AffixType = Definition->AffixType;
				Summary.Weight = Definition->GetEffectiveWeight();
				Summary.AllowedSubTypes = Definition->AllowedSubTypes;
				Summary.AllowedItemTypes = Definition->AllowedItemTypes;
				Summary.ExcludedItemTypes = Definition->ExcludedItemTypes;
				for (const FAffixTier& Tier : Definition->Tiers)
				{
					Summary.TierNumbers.Add(Tier.TierNumber);
				}
			}
			return;
		}

		if (Table->GetRowStruct() == FPHAttributeData::StaticStruct())
		{
			for (const TPair<FName, uint8*>& Row : Table->GetRowMap())
			{
				const FPHAttributeData* Definition = reinterpret_cast<const FPHAttributeData*>(Row.Value);
				if (!Definition)
				{
					continue;
				}

				const FName AffixID = Definition->GetStableAffixID();
				if (AffixID.IsNone())
				{
					continue;
				}

				// Legacy tables are already one row per tier, so a single ID
				// accumulates its tiers across several rows.
				FAffixDefinitionSummary& Summary = OutDefinitions.FindOrAdd(AffixID);
				Summary.AffixType = Definition->AffixType;
				Summary.Weight = Definition->GetWeight();
				Summary.AllowedSubTypes = Definition->AllowedSubTypes;
				Summary.AllowedItemTypes = Definition->AllowedItemTypes;
				Summary.ExcludedItemTypes = Definition->ExcludedItemTypes;
				Summary.TierNumbers.Add(Definition->TierNumber);
			}
		}
	}

	/** Gear sub-types worth expecting a pool for; consumables and the like are skipped. */
	TArray<EItemSubType> GearSubTypes()
	{
		TArray<EItemSubType> SubTypes;
		const UEnum* SubTypeEnum = StaticEnum<EItemSubType>();
		if (!SubTypeEnum)
		{
			return SubTypes;
		}

		for (int32 Index = 0; Index < SubTypeEnum->NumEnums() - 1; ++Index)
		{
			const EItemSubType SubType = static_cast<EItemSubType>(SubTypeEnum->GetValueByIndex(Index));
			if (SubType == EItemSubType::IST_None)
			{
				continue;
			}

			if (UItemEnumFunctionLibrary::IsItemSubTypeAllowedForItemType(EItemType::IT_Weapon, SubType)
				|| UItemEnumFunctionLibrary::IsItemSubTypeAllowedForItemType(EItemType::IT_Armor, SubType)
				|| UItemEnumFunctionLibrary::IsItemSubTypeAllowedForItemType(EItemType::IT_Accessory, SubType))
			{
				SubTypes.Add(SubType);
			}
		}

		return SubTypes;
	}

	FString SubTypeName(const EItemSubType SubType)
	{
		const UEnum* SubTypeEnum = StaticEnum<EItemSubType>();
		return SubTypeEnum ? SubTypeEnum->GetNameStringByValue(static_cast<int64>(SubType))
						   : FString::FromInt(static_cast<int32>(SubType));
	}

	FAffixPoolIssue MakeIssue(
		const EAffixPoolIssueType Type,
		const EAffixPoolIssueSeverity Severity,
		const FName PoolRow,
		const FName AffixID,
		const EItemSubType SubType,
		FString&& Detail)
	{
		FAffixPoolIssue Issue;
		Issue.Type = Type;
		Issue.Severity = Severity;
		Issue.PoolRow = PoolRow;
		Issue.AffixID = AffixID;
		Issue.SubType = SubType;
		Issue.Detail = MoveTemp(Detail);
		return Issue;
	}

	/** Check one list of pool entries against the definitions. */
	void ValidateEntries(
		const TArray<FAffixPoolEntry>& Entries,
		const EAffixes ExpectedType,
		const FName PoolRow,
		const EItemSubType SubType,
		const FDefinitionMap& Definitions,
		TSet<FName>& InOutReachable,
		TArray<FAffixPoolIssue>& OutIssues)
	{
		for (const FAffixPoolEntry& Entry : Entries)
		{
			if (Entry.AffixID.IsNone())
			{
				continue;
			}

			const FAffixDefinitionSummary* Definition = Definitions.Find(Entry.AffixID);
			if (!Definition)
			{
				OutIssues.Add(MakeIssue(
					EAffixPoolIssueType::API_MissingAffixDefinition,
					EAffixPoolIssueSeverity::APS_Error,
					PoolRow, Entry.AffixID, SubType,
					TEXT("No affix definition with this ID exists in the prefix or suffix table.")));
				continue;
			}

			// Weight 0 is a deliberate opt-out, so this is a note rather than a
			// problem - but the entry still reads as listed, which is worth
			// saying out loud when someone is scanning a pool.
			const int32 EffectiveWeight = Entry.WeightOverride >= 0
				? Entry.WeightOverride
				: Definition->Weight;

			if (EffectiveWeight == 0)
			{
				OutIssues.Add(MakeIssue(
					EAffixPoolIssueType::API_ZeroWeightEntry,
					EAffixPoolIssueSeverity::APS_Info,
					PoolRow, Entry.AffixID, SubType,
					Entry.WeightOverride == 0
						? TEXT("WeightOverride is 0, so this pool excludes the affix.")
						: TEXT("The affix's own weight is 0, so it never rolls anywhere.")));
			}
			else
			{
				InOutReachable.Add(Entry.AffixID);
			}

			if (Definition->AffixType != ExpectedType)
			{
				OutIssues.Add(MakeIssue(
					EAffixPoolIssueType::API_WrongAffixType,
					EAffixPoolIssueSeverity::APS_Error,
					PoolRow, Entry.AffixID, SubType,
					FString::Printf(TEXT("Listed under %s, but the definition is affix type %d."),
						ExpectedType == EAffixes::AF_Prefix ? TEXT("Prefixes") : TEXT("Suffixes"),
						static_cast<int32>(Definition->AffixType))));
			}

			if (Entry.ForceTier > 0 && !Definition->TierNumbers.Contains(Entry.ForceTier))
			{
				OutIssues.Add(MakeIssue(
					EAffixPoolIssueType::API_ForceTierNotFound,
					EAffixPoolIssueSeverity::APS_Error,
					PoolRow, Entry.AffixID, SubType,
					FString::Printf(TEXT("ForceTier %d, but the affix only defines tiers %s."),
						Entry.ForceTier,
						*FString::JoinBy(Definition->TierNumbers, TEXT(", "),
							[](int32 Tier) { return FString::FromInt(Tier); }))));
			}

			// The pool decides membership, so a contradicting restriction on the
			// affix is a leftover rather than a second opinion worth honouring.
			if (SubType != EItemSubType::IST_None
				&& Definition->AllowedSubTypes.Num() > 0
				&& !Definition->AllowedSubTypes.Contains(SubType))
			{
				OutIssues.Add(MakeIssue(
					EAffixPoolIssueType::API_RestrictionConflict,
					EAffixPoolIssueSeverity::APS_Warning,
					PoolRow, Entry.AffixID, SubType,
					FString::Printf(
						TEXT("Pool lists it for %s, but the affix's own AllowedSubTypes exclude that sub-type."),
						*SubTypeName(SubType))));
			}
		}
	}
}

FString FAffixPoolIssue::ToString() const
{
	using namespace AffixPoolValidationPrivate;

	const UEnum* TypeEnum = StaticEnum<EAffixPoolIssueType>();
	const FString TypeName = TypeEnum
		? TypeEnum->GetDisplayNameTextByValue(static_cast<int64>(Type)).ToString()
		: FString::FromInt(static_cast<int32>(Type));

	FString Location;
	if (!PoolRow.IsNone())
	{
		Location += FString::Printf(TEXT(" [pool %s]"), *PoolRow.ToString());
	}
	if (!AffixID.IsNone())
	{
		Location += FString::Printf(TEXT(" [affix %s]"), *AffixID.ToString());
	}
	if (SubType != EItemSubType::IST_None)
	{
		Location += FString::Printf(TEXT(" [%s]"), *SubTypeName(SubType));
	}

	return FString::Printf(TEXT("%s:%s %s"), *TypeName, *Location, *Detail);
}

int32 FAffixPoolValidationReport::CountBySeverity(const EAffixPoolIssueSeverity Severity) const
{
	int32 Count = 0;
	for (const FAffixPoolIssue& Issue : Issues)
	{
		if (Issue.Severity == Severity)
		{
			++Count;
		}
	}
	return Count;
}

FString FAffixPoolValidationReport::ToString() const
{
	TArray<FString> Lines;
	Lines.Add(FString::Printf(
		TEXT("Affix pool validation: %d sets, %d sub-types covered, %d/%d affixes reachable"),
		SetsChecked, SubTypesWithPools, AffixesReachable, AffixesDefined));
	Lines.Add(FString::Printf(TEXT("  %d error(s), %d warning(s), %d note(s)"),
		CountBySeverity(EAffixPoolIssueSeverity::APS_Error),
		CountBySeverity(EAffixPoolIssueSeverity::APS_Warning),
		CountBySeverity(EAffixPoolIssueSeverity::APS_Info)));

	// Errors first: they are the ones that change what can roll.
	for (const EAffixPoolIssueSeverity Severity :
		{ EAffixPoolIssueSeverity::APS_Error,
		  EAffixPoolIssueSeverity::APS_Warning,
		  EAffixPoolIssueSeverity::APS_Info })
	{
		for (const FAffixPoolIssue& Issue : Issues)
		{
			if (Issue.Severity == Severity)
			{
				Lines.Add(TEXT("  ") + Issue.ToString());
			}
		}
	}

	if (Issues.IsEmpty())
	{
		Lines.Add(TEXT("  No issues found."));
	}

	return FString::Join(Lines, TEXT("\n"));
}

FAffixPoolValidationReport UAffixPoolValidationLibrary::ValidateAffixPools(
	const UDataTable* PoolTable,
	const UDataTable* PrefixTable,
	const UDataTable* SuffixTable)
{
	using namespace AffixPoolValidationPrivate;

	FAffixPoolValidationReport Report;

	FDefinitionMap Definitions;
	CollectDefinitions(PrefixTable, Definitions);
	CollectDefinitions(SuffixTable, Definitions);
	Report.AffixesDefined = Definitions.Num();

	if (!PoolTable)
	{
		Report.Issues.Add(MakeIssue(
			EAffixPoolIssueType::API_EmptyPool, EAffixPoolIssueSeverity::APS_Warning,
			NAME_None, NAME_None, EItemSubType::IST_None,
			TEXT("No affix pool table; every sub-type falls back to the whole shared table.")));
		return Report;
	}

	if (PoolTable->GetRowStruct() != FAffixSet::StaticStruct())
	{
		Report.Issues.Add(MakeIssue(
			EAffixPoolIssueType::API_MissingAffixDefinition, EAffixPoolIssueSeverity::APS_Error,
			NAME_None, NAME_None, EItemSubType::IST_None,
			FString::Printf(TEXT("Pool table '%s' does not use FAffixSet rows."), *GetNameSafe(PoolTable))));
		return Report;
	}

	Report.SetsChecked = PoolTable->GetRowMap().Num();

	// Which set serves each sub-type, flagging any sub-type claimed twice.
	TMap<EItemSubType, FName> SubTypeToRow;
	for (const TPair<FName, uint8*>& Row : PoolTable->GetRowMap())
	{
		const FAffixSet* Set = reinterpret_cast<const FAffixSet*>(Row.Value);
		if (!Set || Set->SubType == EItemSubType::IST_None)
		{
			continue;
		}

		if (const FName* Existing = SubTypeToRow.Find(Set->SubType))
		{
			Report.Issues.Add(MakeIssue(
				EAffixPoolIssueType::API_DuplicateSubTypeSet, EAffixPoolIssueSeverity::APS_Error,
				Row.Key, NAME_None, Set->SubType,
				FString::Printf(TEXT("Sub-type is already served by set '%s'; only one of them is used."),
					*Existing->ToString())));
			continue;
		}

		SubTypeToRow.Add(Set->SubType, Row.Key);
	}

	Report.SubTypesWithPools = SubTypeToRow.Num();

	TSet<FName> ReachableAffixes;

	for (const TPair<EItemSubType, FName>& Mapping : SubTypeToRow)
	{
		FResolvedAffixPool Pool;
		FAffixSetResolveDiagnostics Diagnostics;
		UItemAffixSelectionFunctionLibrary::ResolveAffixSet(
			PoolTable, Mapping.Value, Pool, &Diagnostics);

		for (const FName& MissingRow : Diagnostics.MissingIncludeRows)
		{
			Report.Issues.Add(MakeIssue(
				EAffixPoolIssueType::API_MissingIncludeRow, EAffixPoolIssueSeverity::APS_Error,
				Mapping.Value, NAME_None, Mapping.Key,
				FString::Printf(TEXT("IncludedSets references row '%s', which does not exist."),
					*MissingRow.ToString())));
		}

		for (const FName& CyclicRow : Diagnostics.CyclicIncludes)
		{
			Report.Issues.Add(MakeIssue(
				EAffixPoolIssueType::API_IncludeCycle, EAffixPoolIssueSeverity::APS_Warning,
				Mapping.Value, NAME_None, Mapping.Key,
				FString::Printf(TEXT("Set '%s' is reachable from its own includes; the repeat was skipped."),
					*CyclicRow.ToString())));
		}

		if (Pool.Prefixes.IsEmpty() && Pool.Suffixes.IsEmpty())
		{
			Report.Issues.Add(MakeIssue(
				EAffixPoolIssueType::API_EmptyPool, EAffixPoolIssueSeverity::APS_Error,
				Mapping.Value, NAME_None, Mapping.Key,
				TEXT("Resolves to no prefixes and no suffixes, so this sub-type rolls nothing.")));
		}

		ValidateEntries(Pool.Prefixes, EAffixes::AF_Prefix, Mapping.Value, Mapping.Key,
			Definitions, ReachableAffixes, Report.Issues);
		ValidateEntries(Pool.Suffixes, EAffixes::AF_Suffix, Mapping.Value, Mapping.Key,
			Definitions, ReachableAffixes, Report.Issues);
	}

	Report.AffixesReachable = ReachableAffixes.Num();

	// The headline check: an affix no sub-type pool reaches can never roll,
	// which is exactly the mistake per-sub-type lists make easy and silent.
	for (const TPair<FName, FAffixDefinitionSummary>& Definition : Definitions)
	{
		if (!ReachableAffixes.Contains(Definition.Key))
		{
			Report.Issues.Add(MakeIssue(
				EAffixPoolIssueType::API_UnreachableAffix, EAffixPoolIssueSeverity::APS_Warning,
				NAME_None, Definition.Key, EItemSubType::IST_None,
				TEXT("Defined but listed in no sub-type's pool, so it can never roll.")));
		}
	}

	for (const EItemSubType SubType : GearSubTypes())
	{
		if (!SubTypeToRow.Contains(SubType))
		{
			Report.Issues.Add(MakeIssue(
				EAffixPoolIssueType::API_SubTypeWithoutPool, EAffixPoolIssueSeverity::APS_Info,
				NAME_None, NAME_None, SubType,
				TEXT("No pool yet; falls back to the whole shared affix table.")));
		}
	}

	return Report;
}

FAffixPoolValidationReport UAffixPoolValidationLibrary::ValidateConfiguredAffixPools()
{
	const FAffixGenerator Defaults;

	const UDataTable* PoolTable   = Cast<UDataTable>(Defaults.AffixPoolTablePath.TryLoad());
	const UDataTable* PrefixTable = Cast<UDataTable>(Defaults.PrefixDataTablePath.TryLoad());
	const UDataTable* SuffixTable = Cast<UDataTable>(Defaults.SuffixDataTablePath.TryLoad());

	return ValidateAffixPools(PoolTable, PrefixTable, SuffixTable);
}

FString UAffixPoolValidationLibrary::DescribeSubTypePool(const EItemSubType SubType)
{
	using namespace AffixPoolValidationPrivate;

	const FAffixGenerator Defaults;
	const UDataTable* PoolTable = Cast<UDataTable>(Defaults.AffixPoolTablePath.TryLoad());
	if (!PoolTable)
	{
		return FString::Printf(TEXT("No affix pool table at '%s'."),
			*Defaults.AffixPoolTablePath.ToString());
	}

	const FName SetRowName = UItemAffixSelectionFunctionLibrary::FindAffixSetRowForSubType(PoolTable, SubType);
	if (SetRowName.IsNone())
	{
		return FString::Printf(
			TEXT("%s has no pool; it falls back to the whole shared affix table."),
			*SubTypeName(SubType));
	}

	FResolvedAffixPool Pool;
	UItemAffixSelectionFunctionLibrary::ResolveAffixSet(PoolTable, SetRowName, Pool);

	TArray<FString> Lines;
	Lines.Add(FString::Printf(TEXT("%s -> set '%s': %d prefixes, %d suffixes, %d implicits"),
		*SubTypeName(SubType), *SetRowName.ToString(),
		Pool.Prefixes.Num(), Pool.Suffixes.Num(), Pool.Implicits.Num()));

	auto AppendEntries = [&Lines](const TCHAR* Label, const TArray<FAffixPoolEntry>& Entries)
	{
		if (Entries.IsEmpty())
		{
			return;
		}

		Lines.Add(FString::Printf(TEXT("  %s:"), Label));
		for (const FAffixPoolEntry& Entry : Entries)
		{
			FString Detail;
			if (Entry.WeightOverride > 0)
			{
				Detail += FString::Printf(TEXT(" weight=%d"), Entry.WeightOverride);
			}
			if (Entry.ForceTier > 0)
			{
				Detail += FString::Printf(TEXT(" tier=%d"), Entry.ForceTier);
			}
			if (Entry.MinItemLevelOverride > 0)
			{
				Detail += FString::Printf(TEXT(" minILvl=%d"), Entry.MinItemLevelOverride);
			}

			Lines.Add(FString::Printf(TEXT("    %s%s"), *Entry.AffixID.ToString(), *Detail));
		}
	};

	AppendEntries(TEXT("Prefixes"), Pool.Prefixes);
	AppendEntries(TEXT("Suffixes"), Pool.Suffixes);
	AppendEntries(TEXT("Implicits"), Pool.Implicits);

	return FString::Join(Lines, TEXT("\n"));
}

static FAutoConsoleCommand GValidateAffixPoolsCommand(
	TEXT("ph.AffixPools.Validate"),
	TEXT("Cross-check DT_AffixPools against the affix definition tables."),
	FConsoleCommandDelegate::CreateStatic([]()
	{
		const FAffixPoolValidationReport Report =
			UAffixPoolValidationLibrary::ValidateConfiguredAffixPools();

		if (Report.HasErrors())
		{
			UE_LOG(LogAffixPoolValidation, Error, TEXT("\n%s"), *Report.ToString());
		}
		else
		{
			UE_LOG(LogAffixPoolValidation, Display, TEXT("\n%s"), *Report.ToString());
		}
	}));

static FAutoConsoleCommand GDescribeAffixPoolCommand(
	TEXT("ph.AffixPools.Describe"),
	TEXT("Print the resolved affix list for one item sub-type, e.g. ph.AffixPools.Describe IST_Bow"),
	FConsoleCommandWithArgsDelegate::CreateStatic([](const TArray<FString>& Args)
	{
		if (Args.Num() < 1)
		{
			UE_LOG(LogAffixPoolValidation, Warning,
				TEXT("Usage: ph.AffixPools.Describe <EItemSubType, e.g. IST_Bow>"));
			return;
		}

		const UEnum* SubTypeEnum = StaticEnum<EItemSubType>();
		const int64 Value = SubTypeEnum ? SubTypeEnum->GetValueByNameString(Args[0]) : INDEX_NONE;
		if (Value == INDEX_NONE)
		{
			UE_LOG(LogAffixPoolValidation, Warning,
				TEXT("'%s' is not an EItemSubType name (expected e.g. IST_Bow)."), *Args[0]);
			return;
		}

		UE_LOG(LogAffixPoolValidation, Display, TEXT("\n%s"),
			*UAffixPoolValidationLibrary::DescribeSubTypePool(static_cast<EItemSubType>(Value)));
	}));
