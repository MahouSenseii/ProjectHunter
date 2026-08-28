#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Engine/DataTable.h"
#include "Item/Generation/AffixGenerator.h"
#include "Item/Library/FunctionLibraries/AffixPoolValidationLibrary.h"
#include "Item/Library/FunctionLibraries/ItemAffixSelectionFunctionLibrary.h"
#include "Item/Library/Structs/AffixStructs.h"
#include "Item/Library/Structs/ItemStructs.h"

namespace AffixPoolTestHelpers
{
	UDataTable* MakePoolTable()
	{
		UDataTable* Table = NewObject<UDataTable>();
		Table->RowStruct = FAffixSet::StaticStruct();
		return Table;
	}

	/** WeightOverride defaults to the -1 sentinel; 0 would exclude the entry. */
	FAffixPoolEntry MakeEntry(const TCHAR* AffixID, const int32 WeightOverride = -1)
	{
		FAffixPoolEntry Entry;
		Entry.AffixID = AffixID;
		Entry.WeightOverride = WeightOverride;
		return Entry;
	}

	FDataTableRowHandle MakeHandle(UDataTable* Table, const TCHAR* RowName)
	{
		FDataTableRowHandle Handle;
		Handle.DataTable = Table;
		Handle.RowName = RowName;
		return Handle;
	}

	const FAffixPoolEntry* FindEntry(const TArray<FAffixPoolEntry>& Entries, const TCHAR* AffixID)
	{
		const FName Wanted(AffixID);
		return Entries.FindByPredicate(
			[Wanted](const FAffixPoolEntry& Entry) { return Entry.AffixID == Wanted; });
	}

	/** A single-tier prefix definition, enough for the generator to roll it. */
	void AddPrefixDefinition(UDataTable* Table, const TCHAR* AffixID)
	{
		FAffixData Definition;
		Definition.AffixID = AffixID;
		Definition.AttributeName = AffixID;
		Definition.AffixType = EAffixes::AF_Prefix;
		Definition.AffixName = FText::FromString(AffixID);
		Definition.Weight = 100;

		FAffixTier Tier;
		Tier.TierNumber = 1;
		Tier.MinItemLevel = 1;
		Tier.MaxItemLevel = 100;
		Tier.MinValue = 5.0f;
		Tier.MaxValue = 5.0f;
		Definition.Tiers.Add(Tier);

		Table->AddRow(FName(AffixID), Definition);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAffixPoolIncludeResolutionTest,
	"ProjectHunter.Item.Affix.PoolIncludesResolve",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAffixPoolIncludeResolutionTest::RunTest(const FString& Parameters)
{
	using namespace AffixPoolTestHelpers;

	UDataTable* Table = MakePoolTable();

	FAffixSet Shared;
	Shared.SubType = EItemSubType::IST_None;
	Shared.Prefixes.Add(MakeEntry(TEXT("SharedLife"), 100));
	Shared.Prefixes.Add(MakeEntry(TEXT("SharedArmour"), 100));
	Shared.Suffixes.Add(MakeEntry(TEXT("SharedResist"), 100));
	Table->AddRow(TEXT("Shared"), Shared);

	FAffixSet Boots;
	Boots.SubType = EItemSubType::IST_Boots;
	Boots.IncludedSets.Add(MakeHandle(Table, TEXT("Shared")));
	Boots.Prefixes.Add(MakeEntry(TEXT("BootsMoveSpeed"), 50));
	// Re-listing an inherited affix must override it, not add a second copy -
	// a duplicate would silently double that affix's weight.
	Boots.Prefixes.Add(MakeEntry(TEXT("SharedLife"), 250));
	Table->AddRow(TEXT("Boots"), Boots);

	FResolvedAffixPool Pool;
	TestTrue(TEXT("Boots set resolves"),
		UItemAffixSelectionFunctionLibrary::ResolveAffixSet(Table, TEXT("Boots"), Pool));

	TestEqual(TEXT("Inherited plus own prefixes, overridden entry not duplicated"),
		Pool.Prefixes.Num(), 3);
	TestEqual(TEXT("Suffixes inherited from the shared set"), Pool.Suffixes.Num(), 1);

	if (const FAffixPoolEntry* Life = FindEntry(Pool.Prefixes, TEXT("SharedLife")))
	{
		TestEqual(TEXT("Sub-type entry wins over the included one"), Life->WeightOverride, 250);
	}
	else
	{
		AddError(TEXT("Inherited affix SharedLife missing from the resolved pool"));
	}

	TestNotNull(TEXT("Sub-type's own affix present"),
		FindEntry(Pool.Prefixes, TEXT("BootsMoveSpeed")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAffixPoolCycleTest,
	"ProjectHunter.Item.Affix.PoolIncludeCycleIsSafe",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAffixPoolCycleTest::RunTest(const FString& Parameters)
{
	using namespace AffixPoolTestHelpers;

	UDataTable* Table = MakePoolTable();

	FAffixSet SetA;
	SetA.SubType = EItemSubType::IST_Sword;
	SetA.IncludedSets.Add(MakeHandle(Table, TEXT("B")));
	SetA.Prefixes.Add(MakeEntry(TEXT("AffixA")));
	Table->AddRow(TEXT("A"), SetA);

	FAffixSet SetB;
	SetB.IncludedSets.Add(MakeHandle(Table, TEXT("A")));
	SetB.Prefixes.Add(MakeEntry(TEXT("AffixB")));
	Table->AddRow(TEXT("B"), SetB);

	FResolvedAffixPool Pool;
	TestTrue(TEXT("A mutually-including pair still resolves"),
		UItemAffixSelectionFunctionLibrary::ResolveAffixSet(Table, TEXT("A"), Pool));
	TestEqual(TEXT("Each set contributes exactly once"), Pool.Prefixes.Num(), 2);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAffixPoolSubTypeLookupTest,
	"ProjectHunter.Item.Affix.PoolSubTypeLookup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAffixPoolSubTypeLookupTest::RunTest(const FString& Parameters)
{
	using namespace AffixPoolTestHelpers;

	UDataTable* Table = MakePoolTable();

	FAffixSet Shared;
	Shared.SubType = EItemSubType::IST_None;
	Shared.Prefixes.Add(MakeEntry(TEXT("SharedLife")));
	Table->AddRow(TEXT("Shared"), Shared);

	FAffixSet Boots;
	Boots.SubType = EItemSubType::IST_Boots;
	Boots.Prefixes.Add(MakeEntry(TEXT("BootsMoveSpeed")));
	Table->AddRow(TEXT("Boots"), Boots);

	TestEqual(TEXT("Finds the set for a mapped sub-type"),
		UItemAffixSelectionFunctionLibrary::FindAffixSetRowForSubType(Table, EItemSubType::IST_Boots),
		FName(TEXT("Boots")));

	TestEqual(TEXT("Shared building blocks are never selected directly"),
		UItemAffixSelectionFunctionLibrary::FindAffixSetRowForSubType(Table, EItemSubType::IST_None),
		FName(NAME_None));

	TestEqual(TEXT("A sub-type with no set falls through"),
		UItemAffixSelectionFunctionLibrary::FindAffixSetRowForSubType(Table, EItemSubType::IST_Wand),
		FName(NAME_None));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAffixPoolRestrictsGenerationTest,
	"ProjectHunter.Item.Affix.PoolIsAuthoritativeForGeneration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAffixPoolRestrictsGenerationTest::RunTest(const FString& Parameters)
{
	using namespace AffixPoolTestHelpers;

	UDataTable* PrefixTable = NewObject<UDataTable>();
	PrefixTable->RowStruct = FAffixData::StaticStruct();
	AddPrefixDefinition(PrefixTable, TEXT("InPool"));
	AddPrefixDefinition(PrefixTable, TEXT("OutOfPool"));

	UDataTable* PoolTable = MakePoolTable();
	FAffixSet SwordSet;
	SwordSet.SubType = EItemSubType::IST_Sword;
	SwordSet.Prefixes.Add(MakeEntry(TEXT("InPool")));
	PoolTable->AddRow(TEXT("Sword"), SwordSet);

	FItemBase Base;
	Base.ItemID = TEXT("TestSword");
	Base.ItemType = EItemType::IT_Weapon;
	Base.ItemSubType = EItemSubType::IST_Sword;

	FAffixGenerator Generator;
	Generator.SetAffixDefinitionTables(PrefixTable, /*SuffixTable=*/nullptr);
	Generator.SetAffixPoolTable(PoolTable);

	// Both affixes are eligible by item type and level, so anything picking from
	// the definition table instead of the pool would roll OutOfPool about half
	// the time. Sweeping seeds is what makes that a failure rather than luck.
	int32 InPoolRolls = 0;
	for (int32 Seed = 1; Seed <= 64; ++Seed)
	{
		const FPHItemStats Stats = Generator.GenerateAffixes(Base, 50, EItemRarity::IR_GradeD, Seed);
		for (const FPHAttributeData& Prefix : Stats.Prefixes)
		{
			if (Prefix.GetStableAffixID() == FName(TEXT("OutOfPool")))
			{
				AddError(FString::Printf(
					TEXT("Seed %d rolled 'OutOfPool', which the sub-type's pool does not list"), Seed));
				return false;
			}

			++InPoolRolls;
		}
	}

	TestTrue(TEXT("The pooled affix still rolls"), InPoolRolls > 0);

	// A sub-type with no set keeps working off the shared definition table,
	// so half-authored pools cannot stop loot generating.
	FItemBase Wand = Base;
	Wand.ItemID = TEXT("TestWand");
	Wand.ItemSubType = EItemSubType::IST_Wand;

	bool bWandRolledSomething = false;
	for (int32 Seed = 1; Seed <= 16 && !bWandRolledSomething; ++Seed)
	{
		bWandRolledSomething =
			Generator.GenerateAffixes(Wand, 50, EItemRarity::IR_GradeD, Seed).Prefixes.Num() > 0;
	}
	TestTrue(TEXT("Sub-type without a pool falls back to the shared table"), bWandRolledSomething);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAffixPoolValidationTest,
	"ProjectHunter.Item.Affix.PoolValidationFindsAuthoringMistakes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAffixPoolValidationTest::RunTest(const FString& Parameters)
{
	using namespace AffixPoolTestHelpers;

	UDataTable* PrefixTable = NewObject<UDataTable>();
	PrefixTable->RowStruct = FAffixData::StaticStruct();
	AddPrefixDefinition(PrefixTable, TEXT("Listed"));
	AddPrefixDefinition(PrefixTable, TEXT("NeverListed"));

	UDataTable* PoolTable = MakePoolTable();

	FAffixSet Sword;
	Sword.SubType = EItemSubType::IST_Sword;
	Sword.Prefixes.Add(MakeEntry(TEXT("Listed")));
	Sword.Prefixes.Add(MakeEntry(TEXT("TypoedAffixID")));
	Sword.IncludedSets.Add(MakeHandle(PoolTable, TEXT("DoesNotExist")));

	// The definition only has tier 1.
	FAffixPoolEntry ForcedTier = MakeEntry(TEXT("Listed"));
	ForcedTier.ForceTier = 9;
	Sword.Suffixes.Add(ForcedTier);

	PoolTable->AddRow(TEXT("Sword"), Sword);

	const FAffixPoolValidationReport Report =
		UAffixPoolValidationLibrary::ValidateAffixPools(PoolTable, PrefixTable, /*SuffixTable=*/nullptr);

	auto HasIssue = [&Report](const EAffixPoolIssueType Type, const TCHAR* AffixID)
	{
		const FName Wanted = AffixID ? FName(AffixID) : NAME_None;
		return Report.Issues.ContainsByPredicate(
			[Type, Wanted, AffixID](const FAffixPoolIssue& Issue)
			{
				return Issue.Type == Type && (!AffixID || Issue.AffixID == Wanted);
			});
	};

	TestTrue(TEXT("Reports the affix no pool lists"),
		HasIssue(EAffixPoolIssueType::API_UnreachableAffix, TEXT("NeverListed")));

	TestTrue(TEXT("Reports a pool entry with no matching definition"),
		HasIssue(EAffixPoolIssueType::API_MissingAffixDefinition, TEXT("TypoedAffixID")));

	TestTrue(TEXT("Reports an include pointing at a missing row"),
		HasIssue(EAffixPoolIssueType::API_MissingIncludeRow, nullptr));

	TestTrue(TEXT("Reports a ForceTier the affix does not define"),
		HasIssue(EAffixPoolIssueType::API_ForceTierNotFound, nullptr));

	TestTrue(TEXT("Reports a prefix listed under Suffixes"),
		HasIssue(EAffixPoolIssueType::API_WrongAffixType, nullptr));

	TestFalse(TEXT("Does not report the affix that is listed"),
		HasIssue(EAffixPoolIssueType::API_UnreachableAffix, TEXT("Listed")));

	TestEqual(TEXT("Counts reachable affixes"), Report.AffixesReachable, 1);
	TestEqual(TEXT("Counts defined affixes"), Report.AffixesDefined, 2);
	TestTrue(TEXT("Authoring mistakes surface as errors"), Report.HasErrors());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAffixPoolValidationCleanTest,
	"ProjectHunter.Item.Affix.PoolValidationPassesCleanData",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAffixPoolValidationCleanTest::RunTest(const FString& Parameters)
{
	using namespace AffixPoolTestHelpers;

	UDataTable* PrefixTable = NewObject<UDataTable>();
	PrefixTable->RowStruct = FAffixData::StaticStruct();
	AddPrefixDefinition(PrefixTable, TEXT("SharedLife"));
	AddPrefixDefinition(PrefixTable, TEXT("SwordDamage"));

	UDataTable* PoolTable = MakePoolTable();

	FAffixSet Shared;
	Shared.SubType = EItemSubType::IST_None;
	Shared.Prefixes.Add(MakeEntry(TEXT("SharedLife")));
	PoolTable->AddRow(TEXT("Shared"), Shared);

	FAffixSet Sword;
	Sword.SubType = EItemSubType::IST_Sword;
	Sword.IncludedSets.Add(MakeHandle(PoolTable, TEXT("Shared")));
	Sword.Prefixes.Add(MakeEntry(TEXT("SwordDamage")));
	PoolTable->AddRow(TEXT("Sword"), Sword);

	const FAffixPoolValidationReport Report =
		UAffixPoolValidationLibrary::ValidateAffixPools(PoolTable, PrefixTable, nullptr);

	TestFalse(TEXT("Well-formed pools produce no errors"), Report.HasErrors());
	TestEqual(TEXT("An affix reached only through an include still counts as reachable"),
		Report.AffixesReachable, 2);

	// Sub-types still awaiting a pool are notes, never errors - authoring is incremental.
	const int32 Warnings = Report.CountBySeverity(EAffixPoolIssueSeverity::APS_Warning);
	TestEqual(TEXT("No warnings on clean data"), Warnings, 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAffixZeroWeightNeverSpawnsTest,
	"ProjectHunter.Item.Affix.ZeroWeightNeverSpawns",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAffixZeroWeightNeverSpawnsTest::RunTest(const FString& Parameters)
{
	FPHAttributeData Disabled;
	Disabled.AffixID = TEXT("Disabled");
	Disabled.SpawnWeight = 0;
	TestEqual(TEXT("Weight 0 stays 0 instead of falling through to the rank formula"),
		Disabled.GetWeight(), 0);
	TestFalse(TEXT("A zero-weight affix cannot spawn"), Disabled.CanEverSpawn());

	FPHAttributeData Derived;
	Derived.AffixID = TEXT("Derived");
	TestTrue(TEXT("The -1 sentinel still derives a weight from RankPoints"),
		Derived.GetWeight() > 0);

	FPHAttributeData Explicit;
	Explicit.AffixID = TEXT("Explicit");
	Explicit.SpawnWeight = 250;
	TestEqual(TEXT("An explicit weight is used verbatim"), Explicit.GetWeight(), 250);

	// The selector must not resurrect disabled affixes when every candidate is
	// disabled - that would make 0 mean "spawns only when nothing else can".
	TArray<FPHAttributeData*> AllDisabled;
	AllDisabled.Add(&Disabled);
	FRandomStream Stream(1234);
	TestNull(TEXT("An all-disabled pool selects nothing"),
		UItemAffixSelectionFunctionLibrary::SelectWeightedAffix(AllDisabled, Stream));

	// And a disabled affix must never reach the pool in the first place.
	const TArray<FPHAttributeData*> Source{ &Disabled, &Explicit };
	const TArray<FPHAttributeData*> Pool =
		UItemAffixSelectionFunctionLibrary::BuildAffixPoolByCorruption(
			Source, EItemType::IT_Weapon, EItemSubType::IST_Sword, 1,
			/*bCorruptedOnly=*/false, TSet<FName>(), TSet<FName>());

	TestEqual(TEXT("Only the enabled affix reaches the pool"), Pool.Num(), 1);
	if (Pool.Num() == 1)
	{
		TestEqual(TEXT("The surviving affix is the enabled one"),
			Pool[0]->AffixID, FName(TEXT("Explicit")));
	}

	FAffixData Definition;
	Definition.Weight = 0;
	TestEqual(TEXT("Authored weight 0 does not fall back to the rarity weight"),
		Definition.GetEffectiveWeight(), 0);

	Definition.Weight = -1;
	TestTrue(TEXT("Authored weight -1 derives from rarity"),
		Definition.GetEffectiveWeight() > 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAffixPoolWeightOverrideOptsOutTest,
	"ProjectHunter.Item.Affix.PoolWeightOverrideOptsOut",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAffixPoolWeightOverrideOptsOutTest::RunTest(const FString& Parameters)
{
	using namespace AffixPoolTestHelpers;

	UDataTable* PrefixTable = NewObject<UDataTable>();
	PrefixTable->RowStruct = FAffixData::StaticStruct();
	AddPrefixDefinition(PrefixTable, TEXT("Kept"));
	AddPrefixDefinition(PrefixTable, TEXT("OptedOut"));

	UDataTable* PoolTable = MakePoolTable();

	FAffixSet Shared;
	Shared.SubType = EItemSubType::IST_None;
	Shared.Prefixes.Add(MakeEntry(TEXT("Kept")));
	Shared.Prefixes.Add(MakeEntry(TEXT("OptedOut")));
	PoolTable->AddRow(TEXT("Shared"), Shared);

	// A sub-type takes the shared block but drops one affix by weighting it 0.
	FAffixSet Sword;
	Sword.SubType = EItemSubType::IST_Sword;
	Sword.IncludedSets.Add(MakeHandle(PoolTable, TEXT("Shared")));
	Sword.Prefixes.Add(MakeEntry(TEXT("OptedOut"), /*WeightOverride=*/0));
	PoolTable->AddRow(TEXT("Sword"), Sword);

	FItemBase Base;
	Base.ItemID = TEXT("TestSword");
	Base.ItemType = EItemType::IT_Weapon;
	Base.ItemSubType = EItemSubType::IST_Sword;

	FAffixGenerator Generator;
	Generator.SetAffixDefinitionTables(PrefixTable, nullptr);
	Generator.SetAffixPoolTable(PoolTable);

	int32 KeptRolls = 0;
	for (int32 Seed = 1; Seed <= 64; ++Seed)
	{
		const FPHItemStats Stats = Generator.GenerateAffixes(Base, 50, EItemRarity::IR_GradeD, Seed);
		for (const FPHAttributeData& Prefix : Stats.Prefixes)
		{
			if (Prefix.GetStableAffixID() == FName(TEXT("OptedOut")))
			{
				AddError(FString::Printf(
					TEXT("Seed %d rolled 'OptedOut', which this sub-type weighted to 0"), Seed));
				return false;
			}

			++KeptRolls;
		}
	}

	TestTrue(TEXT("The affix that was kept still rolls"), KeptRolls > 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAffixLocalGlobalAreComplementaryTest,
	"ProjectHunter.Item.Affix.LocalAndGlobalAreComplementary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAffixLocalGlobalAreComplementaryTest::RunTest(const FString& Parameters)
{
	// Reachable authoring state: Scope defaults to Global while bAffectsBaseStats
	// is ticked. Both predicates accepting it would double-apply the modifier.
	FPHAttributeData Ambiguous;
	Ambiguous.ModifiedLocation = EAffixScope::AS_Global;
	Ambiguous.bAffectsBaseItemStats = true;

	TestTrue(TEXT("Folding into base item stats makes it local"), Ambiguous.IsLocal());
	TestFalse(TEXT("...and therefore not global"), Ambiguous.IsGlobal());

	FPHAttributeData PlainGlobal;
	PlainGlobal.ModifiedLocation = EAffixScope::AS_Global;
	TestFalse(TEXT("A plain global affix is not local"), PlainGlobal.IsLocal());
	TestTrue(TEXT("A plain global affix is global"), PlainGlobal.IsGlobal());

	FPHAttributeData PlainLocal;
	PlainLocal.ModifiedLocation = EAffixScope::AS_Local;
	TestTrue(TEXT("A plain local affix is local"), PlainLocal.IsLocal());
	TestFalse(TEXT("A plain local affix is not global"), PlainLocal.IsGlobal());

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
