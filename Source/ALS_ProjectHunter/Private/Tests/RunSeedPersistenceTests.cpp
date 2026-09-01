#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Tower/Library/FunctionLibraries/RunSeedFunctionLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace PHRunSeedPersistenceTests
{
	constexpr EAutomationTestFlags TestFlags =
		EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPHRunSeedFixedValuesTest,
	"ProjectHunter.Run.Seed.FixedValuesAcrossProcesses",
	PHRunSeedPersistenceTests::TestFlags)

bool FPHRunSeedFixedValuesTest::RunTest(const FString&)
{
	// Run a second process with -PHSeedNameWarmup to change name registration order.
	if (FParse::Param(FCommandLine::Get(), TEXT("PHSeedNameWarmup")))
	{
		TArray<FName> PaddingNames;
		for (int32 Index = 0; Index < 256; ++Index)
		{
			PaddingNames.Emplace(*FString::Printf(TEXT("PHSeedWarmup%08dX"), Index));
		}
	}

	// Literal expectations lock the seed format; deriving both sides would hide drift.
	const FName CustomLabel(TEXT("PH.Seed.Persistence_42"));
	const int32 CustomSeed = URunSeedFunctionLibrary::DeriveSeed(777, CustomLabel, 9);
	TestEqual(TEXT("Custom label keeps its seed despite name registration order"), CustomSeed, 926037106);
	AddInfo(FString::Printf(TEXT("Persistence probe: NameTableHash=%u Seed=%d"),
		GetTypeHash(CustomLabel), CustomSeed));

	TestEqual(TEXT("NAME_None has a stable seed"),
		URunSeedFunctionLibrary::DeriveSeed(0, NAME_None, 0), 1757443740);
	TestEqual(TEXT("Zero parent and floor are stable"),
		URunSeedFunctionLibrary::DeriveFloorSeed(0, 0), 1538697843);
	TestEqual(TEXT("Negative inputs are stable"),
		URunSeedFunctionLibrary::DeriveSeed(-1234567, CustomLabel, -3), 303332926);
	TestEqual(TEXT("Maximum int32 inputs are stable"),
		URunSeedFunctionLibrary::DeriveSeed(MAX_int32, CustomLabel, MAX_int32), 475894911);
	TestEqual(TEXT("Minimum int32 inputs are stable"),
		URunSeedFunctionLibrary::DeriveSeed(MIN_int32, CustomLabel, MIN_int32), 1385948915);

	const int32 Floor = URunSeedFunctionLibrary::DeriveFloorSeed(20260829, 1);
	const int32 Encounter = URunSeedFunctionLibrary::DeriveEncounterSeed(Floor, 2);
	const int32 Monster = URunSeedFunctionLibrary::DeriveMonsterSeed(Encounter, 3);
	const int32 Modifier = URunSeedFunctionLibrary::DeriveModifierSeed(Monster);
	const int32 Reward = URunSeedFunctionLibrary::DeriveRewardSeed(Floor);
	const int32 Loot = URunSeedFunctionLibrary::DeriveLootSeed(Reward, 4);

	TestEqual(TEXT("Floor seed format is stable"), Floor, 1721569280);
	TestEqual(TEXT("Encounter seed format is stable"), Encounter, 724587559);
	TestEqual(TEXT("Monster seed format is stable"), Monster, 1928648107);
	TestEqual(TEXT("Modifier seed format is stable"), Modifier, 1279967630);
	TestEqual(TEXT("Reward seed format is stable"), Reward, 838242727);
	TestEqual(TEXT("Loot seed format is stable"), Loot, 1578881273);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPHRunSeedLabelIdentityTest,
	"ProjectHunter.Run.Seed.LabelIdentity",
	PHRunSeedPersistenceTests::TestFlags)

bool FPHRunSeedLabelIdentityTest::RunTest(const FString&)
{
	const int32 UpperCase = URunSeedFunctionLibrary::DeriveSeed(4242, FName(TEXT("ENCOUNTER")), 1);
	const int32 LowerCase = URunSeedFunctionLibrary::DeriveSeed(4242, FName(TEXT("encounter")), 1);
	TestEqual(TEXT("Label casing does not change the seed"), UpperCase, LowerCase);
	TestEqual(TEXT("Case variants use the fixed canonical seed"), UpperCase, 1702813310);

	const FName NumberedLabel(TEXT("PH.Seed.Label"), 43);
	const FName TextLabel(TEXT("ph.seed.label_42"));
	TestEqual(TEXT("Numeric suffix fixtures represent the same FName"), NumberedLabel, TextLabel);
	TestEqual(TEXT("Equivalent numbered labels produce the same seed"),
		URunSeedFunctionLibrary::DeriveSeed(42, NumberedLabel),
		URunSeedFunctionLibrary::DeriveSeed(42, TextLabel));
	TestNotEqual(TEXT("The numeric suffix participates in the seed"),
		URunSeedFunctionLibrary::DeriveSeed(42, NumberedLabel),
		URunSeedFunctionLibrary::DeriveSeed(42, FName(TEXT("PH.Seed.Label"))));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPHRunSeedBranchIsolationTest,
	"ProjectHunter.Run.Seed.DecorationDoesNotConsumeGameplayRandomness",
	PHRunSeedPersistenceTests::TestFlags)

bool FPHRunSeedBranchIsolationTest::RunTest(const FString&)
{
	for (int32 RunSeed = -500; RunSeed < 500; ++RunSeed)
	{
		const int32 Floor = URunSeedFunctionLibrary::DeriveFloorSeed(RunSeed, 3);
		const int32 EncounterSeed = URunSeedFunctionLibrary::DeriveEncounterSeed(Floor, 2);
		const int32 MonsterSeed = URunSeedFunctionLibrary::DeriveMonsterSeed(EncounterSeed, 4);
		const int32 ModifierSeed = URunSeedFunctionLibrary::DeriveModifierSeed(MonsterSeed);
		const int32 RewardSeed = URunSeedFunctionLibrary::DeriveRewardSeed(Floor);
		const int32 LootSeed = URunSeedFunctionLibrary::DeriveLootSeed(RewardSeed, 1);
		const int32 DecorationSeed = URunSeedFunctionLibrary::DeriveSeed(Floor, FName(TEXT("Decoration")));

		TestTrue(TEXT("Every branch remains a usable positive seed"),
			Floor > 0 && EncounterSeed > 0 && MonsterSeed > 0 && ModifierSeed > 0
			&& RewardSeed > 0 && LootSeed > 0 && DecorationSeed > 0);

		FRandomStream Decoration(DecorationSeed);
		for (int32 Draw = 0; Draw < 16; ++Draw)
		{
			const int32 DecorationIndex = Decoration.RandRange(0, MAX_int16);
			URunSeedFunctionLibrary::DeriveSeed(Floor, FName(TEXT("Decoration")), DecorationIndex);
		}

		// Re-derive through the public API after unrelated branch work, rather
		// than comparing two streams initialized from one already-computed seed.
		const int32 ReplayedFloor = URunSeedFunctionLibrary::DeriveFloorSeed(RunSeed, 3);
		const int32 ReplayedEncounter = URunSeedFunctionLibrary::DeriveEncounterSeed(ReplayedFloor, 2);
		const int32 ReplayedMonster = URunSeedFunctionLibrary::DeriveMonsterSeed(ReplayedEncounter, 4);
		const int32 ReplayedReward = URunSeedFunctionLibrary::DeriveRewardSeed(ReplayedFloor);
		TestEqual(TEXT("Decoration work leaves the floor seed unchanged"), ReplayedFloor, Floor);
		TestEqual(TEXT("Decoration work leaves the encounter seed unchanged"), ReplayedEncounter, EncounterSeed);
		TestEqual(TEXT("Decoration work leaves the monster seed unchanged"), ReplayedMonster, MonsterSeed);
		TestEqual(TEXT("Decoration work leaves the modifier seed unchanged"),
			URunSeedFunctionLibrary::DeriveModifierSeed(ReplayedMonster), ModifierSeed);
		TestEqual(TEXT("Decoration work leaves the reward seed unchanged"), ReplayedReward, RewardSeed);
		TestEqual(TEXT("Decoration work leaves the loot seed unchanged"),
			URunSeedFunctionLibrary::DeriveLootSeed(ReplayedReward, 1), LootSeed);
	}
	return true;
}

#endif
