// Automation coverage for affix generation, focused on the corruption guarantee.
//
// The generator is driven through its public GenerateAffixes entry point using a
// DataTable built in the test, so nothing depends on shipped content. Both the
// prefix and suffix pools are given a mix of normal and corrupted rows.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Engine/DataTable.h"
#include "UObject/StrongObjectPtr.h"

#include "Item/Generation/AffixGenerator.h"
#include "Item/Library/Structs/ItemStructs.h"
#include "Item/Library/Structs/ItemAttributeStructs.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

namespace PHAffixTests
{
	constexpr EAutomationTestFlags TestFlags =
		EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter;

	FPHAttributeData MakeRow(
		const EAffixes AffixType,
		const TCHAR* AttributeName,
		const TCHAR* AffixGroup)
	{
		FPHAttributeData Row;
		Row.AffixType = AffixType;
		Row.AffixName = FText::FromString(AttributeName);
		Row.AttributeName = FName(AttributeName);
		Row.AffixGroup = FName(AffixGroup);
		Row.ModifyType = EModifyType::MT_Add;
		Row.ModifiedLocation = EAffixScope::AS_Global;
		Row.MinValue = 1.f;
		Row.MaxValue = 10.f;
		Row.MinItemLevel = 1;
		Row.MaxItemLevel = 100;
		return Row;
	}

	/**
	 * Pool with plenty of normal rows and plenty of corrupted rows, all in
	 * distinct affix groups so group exclusion never starves the roll.
	 */
	UDataTable* BuildAffixTable(const EAffixes PoolType, const int32 NormalCount, const int32 CorruptedCount)
	{
		UDataTable* Table = NewObject<UDataTable>(GetTransientPackage());
		Table->RowStruct = FPHAttributeData::StaticStruct();

		for (int32 Index = 0; Index < NormalCount; ++Index)
		{
			const FString Name = FString::Printf(TEXT("Normal_%d"), Index);
			FPHAttributeData Row = MakeRow(PoolType, *Name, *FString::Printf(TEXT("GroupN%d"), Index));
			Table->AddRow(FName(*Name), Row);
		}

		for (int32 Index = 0; Index < CorruptedCount; ++Index)
		{
			const FString Name = FString::Printf(TEXT("Corrupted_%d"), Index);
			FPHAttributeData Row = MakeRow(EAffixes::AF_Corrupted, *Name, *FString::Printf(TEXT("GroupC%d"), Index));
			Table->AddRow(FName(*Name), Row);
		}

		return Table;
	}

	int32 CountCorrupted(const TArray<FPHAttributeData>& Affixes)
	{
		int32 Count = 0;
		for (const FPHAttributeData& Affix : Affixes)
		{
			if (Affix.IsCorruptedAffix())
			{
				++Count;
			}
		}
		return Count;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPHAffixForcedCorruptionTest,
	"ProjectHunter.Items.Affixes.ForcedCorruptionRollsExactlyOne",
	PHAffixTests::TestFlags)

bool FPHAffixForcedCorruptionTest::RunTest(const FString&)
{
	using namespace PHAffixTests;

	// C-02 regression guard. bForceOneCorrupted used to be re-read on every
	// iteration of the roll loop, which turned "guarantee one" into "make them
	// all corrupted". With CorruptionChance at zero, exactly one corrupted affix
	// may appear across the whole item.
	const TStrongObjectPtr<UDataTable> PrefixTable(BuildAffixTable(EAffixes::AF_Prefix, 8, 8));
	const TStrongObjectPtr<UDataTable> SuffixTable(BuildAffixTable(EAffixes::AF_Suffix, 8, 8));

	FItemBase BaseItem;
	BaseItem.ItemID = FName(TEXT("TestItem"));
	BaseItem.ItemType = EItemType::IT_Weapon;
	BaseItem.PrefixAffixTable = PrefixTable.Get();
	BaseItem.SuffixAffixTable = SuffixTable.Get();

	const FAffixGenerator Generator;

	// SS rarity rolls 3 prefixes and 3 suffixes, so a leaking force flag would
	// be unmistakable: 6 corrupted affixes instead of 1.
	for (int32 Seed = 1; Seed <= 40; ++Seed)
	{
		const FPHItemStats Stats = Generator.GenerateAffixes(
			BaseItem, /*ItemLevel*/ 50, EItemRarity::IR_GradeSS, Seed,
			/*CorruptionChance*/ 0.f, /*bForceOneCorrupted*/ true);

		const int32 CorruptedPrefixes = CountCorrupted(Stats.Prefixes);
		const int32 CorruptedSuffixes = CountCorrupted(Stats.Suffixes);
		const int32 TotalCorrupted = CorruptedPrefixes + CorruptedSuffixes;

		TestEqual(
			*FString::Printf(TEXT("Seed %d guarantees exactly one corrupted affix"), Seed),
			TotalCorrupted, 1);

		TestTrue(
			*FString::Printf(TEXT("Seed %d still rolls a full set of prefixes"), Seed),
			Stats.Prefixes.Num() == 3);
		TestTrue(
			*FString::Printf(TEXT("Seed %d still rolls a full set of suffixes"), Seed),
			Stats.Suffixes.Num() == 3);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPHAffixNoForcedCorruptionTest,
	"ProjectHunter.Items.Affixes.NoCorruptionWhenNotRequested",
	PHAffixTests::TestFlags)

bool FPHAffixNoForcedCorruptionTest::RunTest(const FString&)
{
	using namespace PHAffixTests;

	const TStrongObjectPtr<UDataTable> PrefixTable(BuildAffixTable(EAffixes::AF_Prefix, 8, 8));
	const TStrongObjectPtr<UDataTable> SuffixTable(BuildAffixTable(EAffixes::AF_Suffix, 8, 8));

	FItemBase BaseItem;
	BaseItem.ItemID = FName(TEXT("TestItem"));
	BaseItem.ItemType = EItemType::IT_Weapon;
	BaseItem.PrefixAffixTable = PrefixTable.Get();
	BaseItem.SuffixAffixTable = SuffixTable.Get();

	const FAffixGenerator Generator;

	for (int32 Seed = 1; Seed <= 20; ++Seed)
	{
		const FPHItemStats Stats = Generator.GenerateAffixes(
			BaseItem, 50, EItemRarity::IR_GradeSS, Seed, 0.f, false);

		TestEqual(
			*FString::Printf(TEXT("Seed %d produces no corrupted prefixes"), Seed),
			CountCorrupted(Stats.Prefixes), 0);
		TestEqual(
			*FString::Printf(TEXT("Seed %d produces no corrupted suffixes"), Seed),
			CountCorrupted(Stats.Suffixes), 0);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPHAffixNaturalCorruptionStacksTest,
	"ProjectHunter.Items.Affixes.NaturalCorruptionCanExceedTheGuarantee",
	PHAffixTests::TestFlags)

bool FPHAffixNaturalCorruptionStacksTest::RunTest(const FString&)
{
	using namespace PHAffixTests;

	// The guarantee is a floor, not a ceiling: with CorruptionChance at 100%
	// every affix is corrupted, and the forced flag must not suppress that.
	const TStrongObjectPtr<UDataTable> PrefixTable(BuildAffixTable(EAffixes::AF_Prefix, 8, 8));
	const TStrongObjectPtr<UDataTable> SuffixTable(BuildAffixTable(EAffixes::AF_Suffix, 8, 8));

	FItemBase BaseItem;
	BaseItem.ItemID = FName(TEXT("TestItem"));
	BaseItem.ItemType = EItemType::IT_Weapon;
	BaseItem.PrefixAffixTable = PrefixTable.Get();
	BaseItem.SuffixAffixTable = SuffixTable.Get();

	const FAffixGenerator Generator;
	const FPHItemStats Stats = Generator.GenerateAffixes(
		BaseItem, 50, EItemRarity::IR_GradeSS, /*Seed*/ 7,
		/*CorruptionChance*/ 1.f, /*bForceOneCorrupted*/ true);

	const int32 TotalCorrupted = CountCorrupted(Stats.Prefixes) + CountCorrupted(Stats.Suffixes);
	TestTrue(TEXT("Natural corruption still stacks beyond the guaranteed one"), TotalCorrupted > 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPHAffixDeterministicSeedTest,
	"ProjectHunter.Items.Affixes.SameSeedProducesSameItem",
	PHAffixTests::TestFlags)

bool FPHAffixDeterministicSeedTest::RunTest(const FString&)
{
	using namespace PHAffixTests;

	const TStrongObjectPtr<UDataTable> PrefixTable(BuildAffixTable(EAffixes::AF_Prefix, 8, 8));
	const TStrongObjectPtr<UDataTable> SuffixTable(BuildAffixTable(EAffixes::AF_Suffix, 8, 8));

	FItemBase BaseItem;
	BaseItem.ItemID = FName(TEXT("TestItem"));
	BaseItem.ItemType = EItemType::IT_Weapon;
	BaseItem.PrefixAffixTable = PrefixTable.Get();
	BaseItem.SuffixAffixTable = SuffixTable.Get();

	const FAffixGenerator Generator;

	const FPHItemStats A = Generator.GenerateAffixes(BaseItem, 50, EItemRarity::IR_GradeA, 20260826, 0.f, false);
	const FPHItemStats B = Generator.GenerateAffixes(BaseItem, 50, EItemRarity::IR_GradeA, 20260826, 0.f, false);

	TestEqual(TEXT("Same seed yields the same prefix count"), A.Prefixes.Num(), B.Prefixes.Num());
	TestEqual(TEXT("Same seed yields the same suffix count"), A.Suffixes.Num(), B.Suffixes.Num());

	for (int32 Index = 0; Index < A.Prefixes.Num(); ++Index)
	{
		TestEqual(TEXT("Same seed selects the same prefix"),
			A.Prefixes[Index].AttributeName, B.Prefixes[Index].AttributeName);
		TestEqual(TEXT("Same seed rolls the same prefix value"),
			A.Prefixes[Index].RolledStatValue, B.Prefixes[Index].RolledStatValue, KINDA_SMALL_NUMBER);
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
