#include "CoreMinimal.h"
#include "Generation/Generators/PHDungeonGenerator.h"
#include "Generation/Library/FunctionLibraries/PHEncounterPlanLibrary.h"
#include "Generation/PHGenerationTags.h"
#include "Misc/AutomationTest.h"
#include "UObject/StrongObjectPtr.h"

#include <limits>

#if WITH_DEV_AUTOMATION_TESTS

namespace PHEncounterPlanTests
{
	constexpr EAutomationTestFlags TestFlags =
		EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter;

	constexpr double TileSize = 400.0;
	constexpr double Inset = 200.0;

	FPHLayoutRequest MakeRequest(const int32 Seed)
	{
		FPHLayoutRequest Request;
		Request.Seed = Seed;
		Request.GridSize = TileSize;
		Request.MinRegionSize = FVector2D(1200.0, 1200.0);
		Request.MaxRegionSize = FVector2D(2400.0, 2400.0);
		Request.RegionSpacing = 800.0;
		Request.AreaSize = FVector2D(12000.0, 12000.0);

		FPHAnchorRule Enemies;
		Enemies.SemanticTag = PHGenerationTags::Anchor_Enemy_Small.GetTag();
		Enemies.MinPerRegion = 1;
		Enemies.MaxPerRegion = 4;
		Enemies.bAllowInStartRegion = false;

		FPHAnchorRule Elites;
		Elites.SemanticTag = PHGenerationTags::Anchor_Elite.GetTag();
		Elites.MinPerRegion = 0;
		Elites.MaxPerRegion = 1;
		Elites.MaxTotal = 2;
		Elites.bAllowInStartRegion = false;

		FPHAnchorRule Chests;
		Chests.SemanticTag = PHGenerationTags::Anchor_Chest.GetTag();
		Chests.MinPerRegion = 0;
		Chests.MaxPerRegion = 1;

		Request.AnchorRules = { Enemies, Elites, Chests };
		return Request;
	}

	bool BuildFor(const int32 Seed, FPHGeneratedLayout& OutLayout, FPHEncounterPlan& OutPlan)
	{
		TStrongObjectPtr<UPHDungeonGenerator> Generator(NewObject<UPHDungeonGenerator>());
		TArray<FPHGenerationIssue> Issues;
		if (!Generator->GenerateLayout(MakeRequest(Seed), OutLayout, Issues))
		{
			return false;
		}
		return UPHEncounterPlanLibrary::BuildEncounterPlan(OutLayout, Inset, OutPlan, Issues);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHEncounterPlanMatchesAnchorsTest,
	"ProjectHunter.Generation.Encounter.MatchesEnemyAnchors", PHEncounterPlanTests::TestFlags)

bool FPHEncounterPlanMatchesAnchorsTest::RunTest(const FString&)
{
	using namespace PHEncounterPlanTests;

	for (int32 Seed = 1; Seed <= 150; ++Seed)
	{
		FPHGeneratedLayout Layout;
		FPHEncounterPlan Plan;
		if (!BuildFor(Seed, Layout, Plan))
		{
			AddError(FString::Printf(TEXT("Seed %d produced no encounter plan."), Seed));
			return false;
		}

		// Count enemy anchors independently; the plan must account for every one and nothing else.
		int32 ExpectedEnemies = 0;
		TMap<int32, int32> ExpectedPerRegion;
		for (const FPHGeneratedAnchor& Anchor : Layout.Anchors)
		{
			const bool bEnemy =
				Anchor.SemanticTag.MatchesTag(PHGenerationTags::Anchor_Enemy)
				|| Anchor.SemanticTag.MatchesTag(PHGenerationTags::Anchor_Elite)
				|| Anchor.SemanticTag.MatchesTag(PHGenerationTags::Anchor_Boss);
			if (bEnemy)
			{
				++ExpectedEnemies;
				++ExpectedPerRegion.FindOrAdd(Anchor.RegionID);
			}
		}

		if (Plan.TotalEnemyCount != ExpectedEnemies)
		{
			AddError(FString::Printf(TEXT("Seed %d: plan counted %d enemies, anchors gave %d."),
				Seed, Plan.TotalEnemyCount, ExpectedEnemies));
			return false;
		}

		if (Plan.Placements.Num() != ExpectedPerRegion.Num())
		{
			AddError(FString::Printf(TEXT("Seed %d: %d placements for %d enemy-bearing regions."),
				Seed, Plan.Placements.Num(), ExpectedPerRegion.Num()));
			return false;
		}

		for (const FPHEncounterPlacement& Placement : Plan.Placements)
		{
			const int32* Expected = ExpectedPerRegion.Find(Placement.RegionID);
			if (!Expected || *Expected != Placement.EnemyCount)
			{
				AddError(FString::Printf(TEXT("Seed %d: region %d count mismatch."),
					Seed, Placement.RegionID));
				return false;
			}

			// A chest-only region must never produce an encounter request.
			if (Placement.EnemyCount <= 0)
			{
				AddError(FString::Printf(TEXT("Seed %d: empty placement for region %d."),
					Seed, Placement.RegionID));
				return false;
			}
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHEncounterVolumeInsideRegionTest,
	"ProjectHunter.Generation.Encounter.VolumeStaysInsideItsRegion", PHEncounterPlanTests::TestFlags)

bool FPHEncounterVolumeInsideRegionTest::RunTest(const FString&)
{
	using namespace PHEncounterPlanTests;

	for (int32 Seed = 1; Seed <= 150; ++Seed)
	{
		FPHGeneratedLayout Layout;
		FPHEncounterPlan Plan;
		if (!BuildFor(Seed, Layout, Plan))
		{
			AddError(FString::Printf(TEXT("Seed %d produced no encounter plan."), Seed));
			return false;
		}

		for (const FPHEncounterPlacement& Placement : Plan.Placements)
		{
			const FPHGeneratedRegion* Region = Layout.Regions.FindByPredicate(
				[&Placement](const FPHGeneratedRegion& R) { return R.RegionID == Placement.RegionID; });
			if (!Region)
			{
				AddError(FString::Printf(TEXT("Seed %d: placement references a missing region."), Seed));
				return false;
			}

			// Spawn candidates must never be handed out beyond the room, or the encounter owner
			// will trace into walls and report spawn failures.
			if (!Region->Bounds.IsInsideOrOn(Placement.SpawnBounds))
			{
				AddError(FString::Printf(TEXT("Seed %d: encounter volume escapes region %d."),
					Seed, Placement.RegionID));
				return false;
			}

			if (Placement.SpawnBounds.GetSize().X <= 0.0 || Placement.SpawnBounds.GetSize().Y <= 0.0)
			{
				AddError(FString::Printf(TEXT("Seed %d: region %d produced a degenerate volume."),
					Seed, Placement.RegionID));
				return false;
			}
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHEncounterStartRegionTest,
	"ProjectHunter.Generation.Encounter.FlagsTheStartRegion", PHEncounterPlanTests::TestFlags)

bool FPHEncounterStartRegionTest::RunTest(const FString&)
{
	using namespace PHEncounterPlanTests;

	// The rules above bar enemies from the start region, so no placement should claim to be one.
	// This is what lets a caller keep the entry room quiet without inspecting anchors itself.
	int32 Checked = 0;
	for (int32 Seed = 1; Seed <= 100; ++Seed)
	{
		FPHGeneratedLayout Layout;
		FPHEncounterPlan Plan;
		if (!BuildFor(Seed, Layout, Plan))
		{
			continue;
		}

		const FPHGeneratedAnchor* Start = Layout.Anchors.FindByPredicate(
			[&Layout](const FPHGeneratedAnchor& A) { return A.AnchorID == Layout.PlayerStartAnchorID; });
		if (!Start)
		{
			AddError(FString::Printf(TEXT("Seed %d has no start anchor."), Seed));
			return false;
		}

		for (const FPHEncounterPlacement& Placement : Plan.Placements)
		{
			const bool bExpected = (Placement.RegionID == Start->RegionID);
			if (Placement.bIsStartRegion != bExpected)
			{
				AddError(FString::Printf(TEXT("Seed %d mislabelled the start region."), Seed));
				return false;
			}
			++Checked;
		}
	}

	TestTrue(TEXT("Some placements were actually examined"), Checked > 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHEncounterNoAnchorsTest,
	"ProjectHunter.Generation.Encounter.NoAnchorsMeansNoEncounters", PHEncounterPlanTests::TestFlags)

bool FPHEncounterNoAnchorsTest::RunTest(const FString&)
{
	using namespace PHEncounterPlanTests;
	TStrongObjectPtr<UPHDungeonGenerator> Generator(NewObject<UPHDungeonGenerator>());

	// A layout with no anchor rules carries only the two endpoints, neither of which is an enemy.
	FPHLayoutRequest Bare = MakeRequest(4);
	Bare.AnchorRules.Reset();

	FPHGeneratedLayout Layout;
	TArray<FPHGenerationIssue> Issues;
	TestTrue(TEXT("The bare layout generates"), Generator->GenerateLayout(Bare, Layout, Issues));

	FPHEncounterPlan Plan;
	TestTrue(TEXT("Planning succeeds with nothing to place"),
		UPHEncounterPlanLibrary::BuildEncounterPlan(Layout, Inset, Plan, Issues));
	TestEqual(TEXT("No encounter requests"), Plan.Placements.Num(), 0);
	TestEqual(TEXT("No enemies counted"), Plan.TotalEnemyCount, 0);

	TestFalse(TEXT("A negative inset is refused"),
		UPHEncounterPlanLibrary::BuildEncounterPlan(Layout, -1.0, Plan, Issues));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHEncounterFiniteInsetTest,
	"ProjectHunter.Generation.Encounter.RejectsNonfiniteInset", PHEncounterPlanTests::TestFlags)

bool FPHEncounterFiniteInsetTest::RunTest(const FString&)
{
	using namespace PHEncounterPlanTests;
	FPHGeneratedLayout Layout;
	FPHEncounterPlan Plan;
	if (!TestTrue(TEXT("A normal encounter plan builds"), BuildFor(1, Layout, Plan)))
	{
		return false;
	}
	TestTrue(TEXT("The fixture has enemy anchors to plan"), Plan.TotalEnemyCount > 0);

	TArray<FPHGenerationIssue> Issues;
	for (const double InvalidInset : { std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::infinity() })
	{
		TestFalse(TEXT("Nonfinite inset is refused instead of silently using full room bounds"),
			UPHEncounterPlanLibrary::BuildEncounterPlan(Layout, InvalidInset, Plan, Issues));
		TestTrue(TEXT("Nonfinite inset reports InvalidRequest"), Issues.ContainsByPredicate(
			[](const FPHGenerationIssue& Issue) { return Issue.Code == EPHGenerationIssueCode::InvalidRequest; }));
		TestEqual(TEXT("A refused inset leaves no partial encounter plan"), Plan.Placements.Num(), 0);
		TestEqual(TEXT("A refused inset leaves no enemy budget"), Plan.TotalEnemyCount, 0);
	}

	TestTrue(TEXT("Zero inset remains supported"),
		UPHEncounterPlanLibrary::BuildEncounterPlan(Layout, 0.0, Plan, Issues));
	TestTrue(TEXT("A finite oversized inset keeps the documented full-room fallback"),
		UPHEncounterPlanLibrary::BuildEncounterPlan(Layout, 100000.0, Plan, Issues));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
