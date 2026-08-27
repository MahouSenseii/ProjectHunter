// Automation coverage for the run seed chain and positional damage.
//
// The seed chain is pure math, so it tests without a world. Floor lifecycle and
// party state live on URunSubsystem, which needs a game instance and a world -
// those are covered by the seed-determinism properties here plus manual PIE
// verification, and are called out as such in the report.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"

#include "Combat/Library/FunctionLibraries/CombatFunctionLibrary.h"
#include "Tower/Library/FunctionLibraries/RunSeedFunctionLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace PHRunSeedTests
{
	constexpr EAutomationTestFlags TestFlags =
		EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPHRunSeedDeterminismTest,
	"ProjectHunter.Run.Seed.SameSeedSameStructure",
	PHRunSeedTests::TestFlags)

bool FPHRunSeedDeterminismTest::RunTest(const FString&)
{
	// The core contract: one run seed reproduces the whole procedural structure.
	constexpr int32 RunSeed = 20260826;

	for (int32 Floor = 1; Floor <= 5; ++Floor)
	{
		const int32 FloorA = URunSeedFunctionLibrary::DeriveFloorSeed(RunSeed, Floor);
		const int32 FloorB = URunSeedFunctionLibrary::DeriveFloorSeed(RunSeed, Floor);
		TestEqual(TEXT("Floor seed is stable for one run seed"), FloorA, FloorB);

		for (int32 Encounter = 0; Encounter < 3; ++Encounter)
		{
			const int32 EncA = URunSeedFunctionLibrary::DeriveEncounterSeed(FloorA, Encounter);
			const int32 EncB = URunSeedFunctionLibrary::DeriveEncounterSeed(FloorB, Encounter);
			TestEqual(TEXT("Encounter seed is stable"), EncA, EncB);

			for (int32 Monster = 0; Monster < 3; ++Monster)
			{
				const int32 MonA = URunSeedFunctionLibrary::DeriveMonsterSeed(EncA, Monster);
				const int32 MonB = URunSeedFunctionLibrary::DeriveMonsterSeed(EncB, Monster);
				TestEqual(TEXT("Monster seed is stable"), MonA, MonB);
				TestEqual(TEXT("Modifier seed is stable"),
					URunSeedFunctionLibrary::DeriveModifierSeed(MonA),
					URunSeedFunctionLibrary::DeriveModifierSeed(MonB));
			}
		}

		const int32 RewardA = URunSeedFunctionLibrary::DeriveRewardSeed(FloorA);
		TestEqual(TEXT("Reward seed is stable"),
			RewardA, URunSeedFunctionLibrary::DeriveRewardSeed(FloorB));
		TestEqual(TEXT("Loot seed is stable"),
			URunSeedFunctionLibrary::DeriveLootSeed(RewardA, 0),
			URunSeedFunctionLibrary::DeriveLootSeed(RewardA, 0));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPHRunSeedDivergenceTest,
	"ProjectHunter.Run.Seed.DifferentInputsDiverge",
	PHRunSeedTests::TestFlags)

bool FPHRunSeedDivergenceTest::RunTest(const FString&)
{
	// A different run seed must produce a different run.
	TestNotEqual(TEXT("Different run seeds give different floors"),
		URunSeedFunctionLibrary::DeriveFloorSeed(1, 1),
		URunSeedFunctionLibrary::DeriveFloorSeed(2, 1));

	// Floors within one run must differ from each other.
	constexpr int32 RunSeed = 777;
	TSet<int32> FloorSeeds;
	for (int32 Floor = 1; Floor <= 20; ++Floor)
	{
		FloorSeeds.Add(URunSeedFunctionLibrary::DeriveFloorSeed(RunSeed, Floor));
	}
	TestEqual(TEXT("Twenty floors produce twenty distinct seeds"), FloorSeeds.Num(), 20);

	// Sibling streams at the same index must not collide. Without the label in
	// the hash, EncounterSeed(S,1) and MonsterSeed(S,1) would be identical and
	// two unrelated systems would draw the same numbers.
	constexpr int32 Parent = 4242;
	const int32 Encounter = URunSeedFunctionLibrary::DeriveEncounterSeed(Parent, 1);
	const int32 Monster = URunSeedFunctionLibrary::DeriveMonsterSeed(Parent, 1);
	const int32 Loot = URunSeedFunctionLibrary::DeriveLootSeed(Parent, 1);
	TestNotEqual(TEXT("Encounter and monster streams do not collide"), Encounter, Monster);
	TestNotEqual(TEXT("Encounter and loot streams do not collide"), Encounter, Loot);
	TestNotEqual(TEXT("Monster and loot streams do not collide"), Monster, Loot);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPHRunSeedNeverZeroTest,
	"ProjectHunter.Run.Seed.NeverReturnsZero",
	PHRunSeedTests::TestFlags)

bool FPHRunSeedNeverZeroTest::RunTest(const FString&)
{
	// FRandomStream(0) reseeds itself from the global RNG, which would silently
	// break determinism at exactly the point the chain is supposed to guarantee
	// it. No derivation may ever hand back zero.
	for (int32 Parent = -50; Parent <= 50; ++Parent)
	{
		for (int32 Index = 0; Index < 20; ++Index)
		{
			TestNotEqual(TEXT("Floor seed is non-zero"),
				URunSeedFunctionLibrary::DeriveFloorSeed(Parent, Index), 0);
			TestNotEqual(TEXT("Encounter seed is non-zero"),
				URunSeedFunctionLibrary::DeriveEncounterSeed(Parent, Index), 0);
			TestNotEqual(TEXT("Monster seed is non-zero"),
				URunSeedFunctionLibrary::DeriveMonsterSeed(Parent, Index), 0);
		}
	}

	TestNotEqual(TEXT("Zero parent still yields a usable seed"),
		URunSeedFunctionLibrary::DeriveFloorSeed(0, 0), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPHRunSeedStreamsReproduceRollsTest,
	"ProjectHunter.Run.Seed.StreamsReproduceRolls",
	PHRunSeedTests::TestFlags)

bool FPHRunSeedStreamsReproduceRollsTest::RunTest(const FString&)
{
	// Two streams built from the same derived seed must draw the same numbers -
	// this is what makes monster tier and modifier rolls reproducible.
	FRandomStream A = URunSeedFunctionLibrary::MakeFloorStream(999, 3);
	FRandomStream B = URunSeedFunctionLibrary::MakeFloorStream(999, 3);

	for (int32 Draw = 0; Draw < 32; ++Draw)
	{
		TestEqual(TEXT("Streams draw identical floats"), A.FRand(), B.FRand(), KINDA_SMALL_NUMBER);
		TestEqual(TEXT("Streams draw identical ints"), A.RandRange(0, 1000), B.RandRange(0, 1000));
	}

	FRandomStream Different = URunSeedFunctionLibrary::MakeFloorStream(1000, 3);
	FRandomStream Baseline = URunSeedFunctionLibrary::MakeFloorStream(999, 3);
	bool bAnyDifference = false;
	for (int32 Draw = 0; Draw < 32 && !bAnyDifference; ++Draw)
	{
		bAnyDifference = !FMath::IsNearlyEqual(Different.FRand(), Baseline.FRand(), KINDA_SMALL_NUMBER);
	}
	TestTrue(TEXT("A different run seed produces a different roll sequence"), bAnyDifference);
	return true;
}

// ---------------------------------------------------------------------------
// Positional damage
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPHPositionalMultiplierTest,
	"ProjectHunter.Combat.Positional.MultiplierByDirection",
	PHRunSeedTests::TestFlags)

bool FPHPositionalMultiplierTest::RunTest(const FString&)
{
	FCombatPositionalRules Rules;
	Rules.BackDamageMultiplier = 1.5f;
	Rules.FlankDamageMultiplier = 1.2f;

	TestEqual(TEXT("Front hits are unmodified"),
		UCombatFunctionLibrary::GetPositionalDamageMultiplier(EHitDirection::Front, Rules), 1.f, 0.001f);
	TestEqual(TEXT("Flank hits use the flank ratio"),
		UCombatFunctionLibrary::GetPositionalDamageMultiplier(EHitDirection::Flank, Rules), 1.2f, 0.001f);
	TestEqual(TEXT("Rear hits use the back ratio"),
		UCombatFunctionLibrary::GetPositionalDamageMultiplier(EHitDirection::Rear, Rules), 1.5f, 0.001f);

	// The toggle must neutralise every direction, not just the front.
	Rules.bEnablePositionalDamage = false;
	TestEqual(TEXT("Disabling positional damage neutralises rear hits"),
		UCombatFunctionLibrary::GetPositionalDamageMultiplier(EHitDirection::Rear, Rules), 1.f, 0.001f);
	TestEqual(TEXT("Disabling positional damage neutralises flank hits"),
		UCombatFunctionLibrary::GetPositionalDamageMultiplier(EHitDirection::Flank, Rules), 1.f, 0.001f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPHPositionalNullActorTest,
	"ProjectHunter.Combat.Positional.InvalidActorsNeverAwardBonus",
	PHRunSeedTests::TestFlags)

bool FPHPositionalNullActorTest::RunTest(const FString&)
{
	// An ambiguous or invalid geometry case must resolve to Front, never Rear -
	// the bonus should never be handed out by accident.
	const FCombatPositionalRules Rules;
	TestEqual(TEXT("Null actors resolve to a front hit"),
		UCombatFunctionLibrary::GetHitDirection(nullptr, nullptr, Rules), EHitDirection::Front);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
