#include "CoreMinimal.h"
#include "Generation/Data/PHBiomeModuleSet.h"
#include "Generation/Generators/PHDungeonGenerator.h"
#include "Generation/Library/FunctionLibraries/PHBlockoutPlanLibrary.h"
#include "Generation/PHGenerationTags.h"
#include "Misc/AutomationTest.h"
#include "UObject/StrongObjectPtr.h"

#include <limits>

#if WITH_DEV_AUTOMATION_TESTS

namespace PHBlockoutPlanTests
{
	constexpr EAutomationTestFlags TestFlags =
		EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter;

	/** Matches the BlockingStarterPack floor tile the authored module set maps. */
	constexpr double TileSize = 400.0;

	FPHLayoutRequest MakeRequest(const int32 Seed)
	{
		FPHLayoutRequest Request;
		Request.Seed = Seed;
		Request.GridSize = TileSize;
		Request.MinRegionSize = FVector2D(800.0, 800.0);
		Request.MaxRegionSize = FVector2D(2000.0, 2000.0);
		Request.RegionSpacing = 800.0;
		Request.AreaSize = FVector2D(12000.0, 12000.0);
		return Request;
	}

	FIntPoint ToTile(const FVector& Location)
	{
		return FIntPoint(FMath::FloorToInt32(Location.X / TileSize),
			FMath::FloorToInt32(Location.Y / TileSize));
	}

	TSet<FIntPoint> FloorTilesOf(const FPHBlockoutPlan& Plan)
	{
		TSet<FIntPoint> Tiles;
		for (const FPHPiecePlacement& Placement : Plan.Placements)
		{
			if (Placement.PieceTag == PHGenerationTags::Piece_Floor.GetTag())
			{
				Tiles.Add(ToTile(Placement.Transform.GetLocation()));
			}
		}
		return Tiles;
	}

	bool BuildFor(const int32 Seed, FPHGeneratedLayout& OutLayout, FPHBlockoutPlan& OutPlan)
	{
		TStrongObjectPtr<UPHDungeonGenerator> Generator(NewObject<UPHDungeonGenerator>());
		TArray<FPHGenerationIssue> Issues;
		if (!Generator->GenerateLayout(MakeRequest(Seed), OutLayout, Issues))
		{
			return false;
		}
		return UPHBlockoutPlanLibrary::BuildPlan(OutLayout, TileSize, OutPlan, Issues);
	}

	FPHGeneratedLayout MakeRoomFixture(const double Height = 800.0)
	{
		FPHGeneratedLayout Layout;
		Layout.Seed = 1;
		Layout.GenerationVersion = 1;
		Layout.Bounds = FBox(FVector::ZeroVector, FVector(800.0, 800.0, Height));
		FPHGeneratedRegion& Region = Layout.Regions.AddDefaulted_GetRef();
		Region.RegionID = 0;
		Region.Bounds = Layout.Bounds;

		FPHGeneratedAnchor& Start = Layout.Anchors.AddDefaulted_GetRef();
		Start.AnchorID = 0;
		Start.RegionID = 0;
		Start.SemanticTag = PHGenerationTags::Anchor_PlayerStart.GetTag();
		Start.Transform = FTransform(FVector(200.0, 200.0, 0.0));
		FPHGeneratedAnchor& Exit = Layout.Anchors.AddDefaulted_GetRef();
		Exit.AnchorID = 1;
		Exit.RegionID = 0;
		Exit.SemanticTag = PHGenerationTags::Anchor_Exit.GetTag();
		Exit.Transform = FTransform(FVector(600.0, 600.0, 0.0));
		Layout.PlayerStartAnchorID = 0;
		Layout.ExitAnchorID = 1;
		return Layout;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHBlockoutWalkableTest,
	"ProjectHunter.Generation.Blockout.BuiltFloorIsWalkable", PHBlockoutPlanTests::TestFlags)

bool FPHBlockoutWalkableTest::RunTest(const FString&)
{
	using namespace PHBlockoutPlanTests;

	// The point of the corridors: a layout that is connected on the graph must also be walkable
	// as built geometry. Flood filling the actual floor tiles is what proves that.
	for (int32 Seed = 1; Seed <= 100; ++Seed)
	{
		FPHGeneratedLayout Layout;
		FPHBlockoutPlan Plan;
		if (!BuildFor(Seed, Layout, Plan))
		{
			AddError(FString::Printf(TEXT("Seed %d produced no plan."), Seed));
			return false;
		}

		const TSet<FIntPoint> Tiles = FloorTilesOf(Plan);
		const FIntPoint Start = ToTile(Plan.PlayerStart.GetLocation());
		const FIntPoint Exit = ToTile(Plan.Exit.GetLocation());

		if (!Tiles.Contains(Start) || !Tiles.Contains(Exit))
		{
			AddError(FString::Printf(TEXT("Seed %d put an endpoint off the floor."), Seed));
			return false;
		}

		TSet<FIntPoint> Reached;
		TArray<FIntPoint> Pending;
		Pending.Add(Start);
		Reached.Add(Start);
		for (int32 Index = 0; Index < Pending.Num(); ++Index)
		{
			const FIntPoint Current = Pending[Index];
			const FIntPoint Neighbours[] = {
				Current + FIntPoint(1, 0), Current + FIntPoint(-1, 0),
				Current + FIntPoint(0, 1), Current + FIntPoint(0, -1) };
			for (const FIntPoint& Next : Neighbours)
			{
				if (Tiles.Contains(Next) && !Reached.Contains(Next))
				{
					Reached.Add(Next);
					Pending.Add(Next);
				}
			}
		}

		if (!Reached.Contains(Exit))
		{
			AddError(FString::Printf(TEXT("Seed %d: the exit cannot be walked to from the start."),
				Seed));
			return false;
		}

		if (Reached.Num() != Tiles.Num())
		{
			AddError(FString::Printf(TEXT("Seed %d: %d of %d floor tiles are cut off."),
				Seed, Tiles.Num() - Reached.Num(), Tiles.Num()));
			return false;
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHBlockoutWallsSealTest,
	"ProjectHunter.Generation.Blockout.WallsSealTheFloor", PHBlockoutPlanTests::TestFlags)

bool FPHBlockoutWallsSealTest::RunTest(const FString&)
{
	using namespace PHBlockoutPlanTests;

	for (int32 Seed = 1; Seed <= 50; ++Seed)
	{
		FPHGeneratedLayout Layout;
		FPHBlockoutPlan Plan;
		if (!BuildFor(Seed, Layout, Plan))
		{
			AddError(FString::Printf(TEXT("Seed %d produced no plan."), Seed));
			return false;
		}

		const TSet<FIntPoint> Tiles = FloorTilesOf(Plan);

		// Every open edge needs exactly one wall: none means the floor leaks into the void,
		// two means overlapping geometry.
		int32 ExpectedWalls = 0;
		for (const FIntPoint& Tile : Tiles)
		{
			const FIntPoint Neighbours[] = {
				Tile + FIntPoint(1, 0), Tile + FIntPoint(-1, 0),
				Tile + FIntPoint(0, 1), Tile + FIntPoint(0, -1) };
			for (const FIntPoint& Next : Neighbours)
			{
				if (!Tiles.Contains(Next))
				{
					++ExpectedWalls;
				}
			}
		}

		if (Plan.WallCount != ExpectedWalls)
		{
			AddError(FString::Printf(TEXT("Seed %d: %d walls for %d open edges."),
				Seed, Plan.WallCount, ExpectedWalls));
			return false;
		}

		TSet<FString> WallKeys;
		for (const FPHPiecePlacement& Placement : Plan.Placements)
		{
			if (Placement.PieceTag != PHGenerationTags::Piece_Wall_Straight.GetTag())
			{
				continue;
			}
			const FString Key = Placement.Transform.GetLocation().ToString()
				+ Placement.Transform.Rotator().ToString();
			if (WallKeys.Contains(Key))
			{
				AddError(FString::Printf(TEXT("Seed %d placed two walls in one spot."), Seed));
				return false;
			}
			WallKeys.Add(Key);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHBlockoutOffGridTest,
	"ProjectHunter.Generation.Blockout.RefusesOffTileLayout", PHBlockoutPlanTests::TestFlags)

bool FPHBlockoutOffGridTest::RunTest(const FString&)
{
	using namespace PHBlockoutPlanTests;
	TStrongObjectPtr<UPHDungeonGenerator> Generator(NewObject<UPHDungeonGenerator>());

	// A layout generated on a 100 grid cannot be tiled by a 400 kit; rooms would end mid-tile.
	FPHLayoutRequest Request = MakeRequest(9);
	Request.GridSize = 100.0;

	FPHGeneratedLayout Layout;
	TArray<FPHGenerationIssue> Issues;
	TestTrue(TEXT("The 100-grid layout itself generates"),
		Generator->GenerateLayout(Request, Layout, Issues));

	FPHBlockoutPlan Plan;
	TestFalse(TEXT("Planning it on a 400 tile is refused"),
		UPHBlockoutPlanLibrary::BuildPlan(Layout, TileSize, Plan, Issues));
	TestTrue(TEXT("The mismatch is reported as off-grid"),
		Issues.ContainsByPredicate([](const FPHGenerationIssue& Issue)
		{
			return Issue.Code == EPHGenerationIssueCode::RegionOffGrid;
		}));
	TestEqual(TEXT("No partial plan is returned"), Plan.Placements.Num(), 0);

	TestFalse(TEXT("A non-positive tile size is refused"),
		UPHBlockoutPlanLibrary::BuildPlan(Layout, 0.0, Plan, Issues));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHBlockoutDeterminismTest,
	"ProjectHunter.Generation.Blockout.PlanIsDeterministic", PHBlockoutPlanTests::TestFlags)

bool FPHBlockoutDeterminismTest::RunTest(const FString&)
{
	using namespace PHBlockoutPlanTests;

	// Placement order must not depend on set iteration order, or two builds of one seed differ.
	for (int32 Seed = 1; Seed <= 25; ++Seed)
	{
		FPHGeneratedLayout LayoutA, LayoutB;
		FPHBlockoutPlan PlanA, PlanB;
		BuildFor(Seed, LayoutA, PlanA);
		BuildFor(Seed, LayoutB, PlanB);

		if (PlanA.Placements.Num() != PlanB.Placements.Num())
		{
			AddError(FString::Printf(TEXT("Seed %d produced different placement counts."), Seed));
			return false;
		}

		for (int32 Index = 0; Index < PlanA.Placements.Num(); ++Index)
		{
			if (!PlanA.Placements[Index].Transform.Equals(PlanB.Placements[Index].Transform)
				|| PlanA.Placements[Index].PieceTag != PlanB.Placements[Index].PieceTag)
			{
				AddError(FString::Printf(TEXT("Seed %d placement %d differs between builds."),
					Seed, Index));
				return false;
			}
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHBlockoutAuthoredSetTest,
	"ProjectHunter.Generation.Blockout.AuthoredModuleSetIsUsable", PHBlockoutPlanTests::TestFlags)

bool FPHBlockoutAuthoredSetTest::RunTest(const FString&)
{
	// Covers the authored asset itself, so a bad edit to it fails here rather than in the editor.
	UPHBiomeModuleSet* Set = LoadObject<UPHBiomeModuleSet>(nullptr,
		TEXT("/Game/ProjectHunter/World/Tower/Generation/Biomes/DA_Biome_Blockout"));
	if (!Set)
	{
		AddError(TEXT("DA_Biome_Blockout could not be loaded."));
		return false;
	}

	TArray<FPHGenerationIssue> Issues;
	TestTrue(TEXT("The authored blockout set validates"), Set->ValidateModuleSet(Issues));

	TArray<FGameplayTag> Missing;
	TestTrue(TEXT("It covers what the blockout plan asks for"),
		Set->HasPieces({ PHGenerationTags::Piece_Floor.GetTag(),
			PHGenerationTags::Piece_Wall_Straight.GetTag(),
			PHGenerationTags::Piece_Pillar.GetTag() }, Missing));

	FPHModuleEntry Floor;
	TestTrue(TEXT("Its floor resolves"),
		Set->ResolvePiece(PHGenerationTags::Piece_Floor.GetTag(), Floor));
	TestEqual(TEXT("Its floor tiles at the size the plan assumes"),
		Floor.Footprint.X, PHBlockoutPlanTests::TileSize);
	TestEqual(TEXT("Its floor is square"), Floor.Footprint.X, Floor.Footprint.Y);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHBlockoutCorridorVarietyTest,
	"ProjectHunter.Generation.Blockout.CorridorsVaryInWidth", PHBlockoutPlanTests::TestFlags)

bool FPHBlockoutCorridorVarietyTest::RunTest(const FString&)
{
	using namespace PHBlockoutPlanTests;
	TStrongObjectPtr<UPHDungeonGenerator> Generator(NewObject<UPHDungeonGenerator>());

	TSet<int32> ObservedCorridorCounts;
	for (int32 Seed = 1; Seed <= 60; ++Seed)
	{
		FPHGeneratedLayout Layout;
		TArray<FPHGenerationIssue> Issues;
		if (!Generator->GenerateLayout(MakeRequest(Seed), Layout, Issues))
		{
			AddError(FString::Printf(TEXT("Seed %d produced no layout."), Seed));
			return false;
		}

		FPHBlockoutPlan Narrow;
		FPHBlockoutPlan Wide;
		if (!UPHBlockoutPlanLibrary::BuildPlan(Layout, TileSize, Narrow, Issues, false, 1, 1)
			|| !UPHBlockoutPlanLibrary::BuildPlan(Layout, TileSize, Wide, Issues, false, 1, 3))
		{
			AddError(FString::Printf(TEXT("Seed %d failed to plan."), Seed));
			return false;
		}

		// Widening may only add floor: a wider corridor must never remove walkable space.
		if (Wide.FloorTileCount < Narrow.FloorTileCount)
		{
			AddError(FString::Printf(TEXT("Seed %d lost floor when corridors widened."), Seed));
			return false;
		}

		// Still one connected space, which is the property widening could most easily break.
		const TSet<FIntPoint> Tiles = FloorTilesOf(Wide);
		const FIntPoint Start = ToTile(Wide.PlayerStart.GetLocation());
		TSet<FIntPoint> Reached;
		TArray<FIntPoint> Pending;
		if (Tiles.Contains(Start))
		{
			Pending.Add(Start);
			Reached.Add(Start);
		}
		for (int32 Index = 0; Index < Pending.Num(); ++Index)
		{
			const FIntPoint Current = Pending[Index];
			const FIntPoint Neighbours[] = {
				Current + FIntPoint(1, 0), Current + FIntPoint(-1, 0),
				Current + FIntPoint(0, 1), Current + FIntPoint(0, -1) };
			for (const FIntPoint& Next : Neighbours)
			{
				if (Tiles.Contains(Next) && !Reached.Contains(Next))
				{
					Reached.Add(Next);
					Pending.Add(Next);
				}
			}
		}
		if (Reached.Num() != Tiles.Num() || !Tiles.Contains(ToTile(Wide.Exit.GetLocation())))
		{
			AddError(FString::Printf(TEXT("Seed %d: widened corridors broke connectivity."), Seed));
			return false;
		}

		// Determinism must survive the extra draws.
		FPHBlockoutPlan Repeat;
		UPHBlockoutPlanLibrary::BuildPlan(Layout, TileSize, Repeat, Issues, false, 1, 3);
		if (Repeat.FloorTileCount != Wide.FloorTileCount
			|| Repeat.CorridorTileCount != Wide.CorridorTileCount)
		{
			AddError(FString::Printf(TEXT("Seed %d: corridor draws are not deterministic."), Seed));
			return false;
		}

		ObservedCorridorCounts.Add(Wide.CorridorTileCount - Narrow.CorridorTileCount);
	}

	// If every seed widened by the same amount, the range is not actually being drawn from.
	TestTrue(TEXT("Corridor widening varies across seeds"), ObservedCorridorCounts.Num() > 3);

	FPHGeneratedLayout Layout;
	TArray<FPHGenerationIssue> Issues;
	Generator->GenerateLayout(MakeRequest(1), Layout, Issues);
	FPHBlockoutPlan Bad;
	TestFalse(TEXT("An inverted width range is refused"),
		UPHBlockoutPlanLibrary::BuildPlan(Layout, TileSize, Bad, Issues, false, 4, 2));
	TestFalse(TEXT("A zero width is refused"),
		UPHBlockoutPlanLibrary::BuildPlan(Layout, TileSize, Bad, Issues, false, 0, 2));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHBlockoutEndpointTileTest,
	"ProjectHunter.Generation.Blockout.EndpointsStandOnBuiltTiles", PHBlockoutPlanTests::TestFlags)

bool FPHBlockoutEndpointTileTest::RunTest(const FString&)
{
	using namespace PHBlockoutPlanTests;

	// A region centre is not a safe pose: an even-sized room centres on the seam between two tiles,
	// so anything spawned there straddles geometry rather than standing on it. That is how a player
	// ends up outside the floor they were meant to start on, and it is invisible from the layout,
	// which considers the pose perfectly inside its region.
	for (int32 Seed = 1; Seed <= 250; ++Seed)
	{
		FPHGeneratedLayout Layout;
		FPHBlockoutPlan Plan;
		if (!BuildFor(Seed, Layout, Plan))
		{
			AddError(FString::Printf(TEXT("Seed %d produced no plan."), Seed));
			return false;
		}

		const TSet<FIntPoint> Built(Plan.FloorTiles);

		if (!Built.Contains(Plan.PlayerStartTile) || !Built.Contains(Plan.ExitTile))
		{
			AddError(FString::Printf(
				TEXT("Seed %d put an endpoint on a tile the plan never built: start %s, exit %s."),
				Seed, *Plan.PlayerStartTile.ToString(), *Plan.ExitTile.ToString()));
			return false;
		}

		// Published tiles and published poses must agree, or a caller reading one gets a different
		// answer from the other.
		auto IsTileCentre = [this, Seed](const FTransform& Pose, const FIntPoint& Tile,
			const TCHAR* Label)
		{
			const FVector Expected((Tile.X + 0.5) * TileSize, (Tile.Y + 0.5) * TileSize,
				Pose.GetLocation().Z);
			if (!Pose.GetLocation().Equals(Expected, 0.01))
			{
				AddError(FString::Printf(TEXT("Seed %d: the %s pose %s is not the centre of %s."),
					Seed, Label, *Pose.GetLocation().ToString(), *Tile.ToString()));
				return false;
			}
			return true;
		};

		if (!IsTileCentre(Plan.PlayerStart, Plan.PlayerStartTile, TEXT("entry"))
			|| !IsTileCentre(Plan.Exit, Plan.ExitTile, TEXT("exit")))
		{
			return false;
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHBlockoutStraightCorridorTest,
	"ProjectHunter.Generation.Blockout.NeighboursGetStraightCorridors",
	PHBlockoutPlanTests::TestFlags)

bool FPHBlockoutStraightCorridorTest::RunTest(const FString&)
{
	using namespace PHBlockoutPlanTests;

	// Hand-built rather than generated, so the geometry under test is exact: two rooms that share
	// three rows of frontage and are separated by four columns of gap.
	FPHGeneratedLayout Layout;
	Layout.Seed = 4242;
	Layout.GenerationVersion = 1;

	auto AddRegion = [&Layout](const int32 ID, const FVector& Min, const FVector& Max)
	{
		FPHGeneratedRegion& Region = Layout.Regions.AddDefaulted_GetRef();
		Region.RegionID = ID;
		Region.Bounds = FBox(Min, Max);
		Layout.Bounds += Region.Bounds;
	};

	AddRegion(0, FVector(0.0, 0.0, 0.0), FVector(1600.0, 1600.0, 400.0));
	AddRegion(1, FVector(3200.0, 400.0, 0.0), FVector(4800.0, 2000.0, 400.0));

	FPHGeneratedConnection& Connection = Layout.Connections.AddDefaulted_GetRef();
	Connection.ConnectionID = 0;
	Connection.FromRegionID = 0;
	Connection.ToRegionID = 1;

	FPHGeneratedAnchor& Start = Layout.Anchors.AddDefaulted_GetRef();
	Start.AnchorID = 0;
	Start.RegionID = 0;
	Start.Transform = FTransform(Layout.Regions[0].Bounds.GetCenter());
	Start.SemanticTag = PHGenerationTags::Anchor_PlayerStart.GetTag();

	FPHGeneratedAnchor& Exit = Layout.Anchors.AddDefaulted_GetRef();
	Exit.AnchorID = 1;
	Exit.RegionID = 1;
	Exit.Transform = FTransform(Layout.Regions[1].Bounds.GetCenter());
	Exit.SemanticTag = PHGenerationTags::Anchor_Exit.GetTag();

	Layout.PlayerStartAnchorID = 0;
	Layout.ExitAnchorID = 1;

	FPHBlockoutPlan Plan;
	TArray<FPHGenerationIssue> Issues;
	if (!UPHBlockoutPlanLibrary::BuildPlan(Layout, TileSize, Plan, Issues, false, 1, 1))
	{
		AddError(TEXT("The hand-built layout should plan."));
		return false;
	}

	// Room 0 owns columns 0-3, room 1 owns columns 8-11, so only 4-7 can be corridor. A dog-leg
	// between the two centres would instead run a column of tiles up or down the map.
	TestEqual(TEXT("The corridor spans exactly the gap"), Plan.CorridorTiles.Num(), 4);

	TSet<int32> Rows;
	for (const FIntPoint& Tile : Plan.CorridorTiles)
	{
		Rows.Add(Tile.Y);
		if (Tile.X < 4 || Tile.X > 7)
		{
			AddError(FString::Printf(TEXT("Corridor tile %s is outside the gap columns 4-7."),
				*Tile.ToString()));
			return false;
		}
	}

	TestEqual(TEXT("A single-width corridor keeps to one row"), Rows.Num(), 1);

	// The row has to be inside the frontage both rooms share, or a mouth opens onto a wall.
	const int32 Row = Rows.Array()[0];
	TestTrue(FString::Printf(TEXT("Row %d lies inside the shared frontage 1-3"), Row),
		Row >= 1 && Row <= 3);

	// And the whole thing still has to be walkable end to end.
	const TSet<FIntPoint> Tiles = FloorTilesOf(Plan);
	TSet<FIntPoint> Reached;
	TArray<FIntPoint> Pending;
	Pending.Add(Plan.PlayerStartTile);
	Reached.Add(Plan.PlayerStartTile);
	for (int32 Index = 0; Index < Pending.Num(); ++Index)
	{
		const FIntPoint Current = Pending[Index];
		const FIntPoint Neighbours[] = {
			Current + FIntPoint(1, 0), Current + FIntPoint(-1, 0),
			Current + FIntPoint(0, 1), Current + FIntPoint(0, -1) };
		for (const FIntPoint& Next : Neighbours)
		{
			if (Tiles.Contains(Next) && !Reached.Contains(Next))
			{
				Reached.Add(Next);
				Pending.Add(Next);
			}
		}
	}
	TestTrue(TEXT("The exit is reachable from the entry"), Reached.Contains(Plan.ExitTile));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHBlockoutGrowthCorridorLengthTest,
	"ProjectHunter.Generation.Blockout.GrowthShortensCorridors", PHBlockoutPlanTests::TestFlags)

bool FPHBlockoutGrowthCorridorLengthTest::RunTest(const FString&)
{
	using namespace PHBlockoutPlanTests;
	TStrongObjectPtr<UPHDungeonGenerator> Generator(NewObject<UPHDungeonGenerator>());

	// The measurable claim behind "smarter placement": rooms grown off one another are joined by
	// short passages, where scattered rooms need long ones to reach each other. Corridor tiles as a
	// share of the floor is the number that shows it - a floor that is mostly corridor is a maze of
	// passages with rooms attached, not a plan of rooms with passages between them.
	auto MeasureCorridorShare = [&](const EPHRegionPlacement Placement)
	{
		int64 CorridorTiles = 0;
		int64 FloorTiles = 0;
		for (int32 Seed = 1; Seed <= 100; ++Seed)
		{
			FPHLayoutRequest Request = MakeRequest(Seed);
			Request.RegionPlacement = Placement;

			FPHGeneratedLayout Layout;
			TArray<FPHGenerationIssue> Issues;
			if (!Generator->GenerateLayout(Request, Layout, Issues))
			{
				AddError(FString::Printf(TEXT("Seed %d produced no layout."), Seed));
				return 1.0f;
			}

			FPHBlockoutPlan Plan;
			if (!UPHBlockoutPlanLibrary::BuildPlan(Layout, TileSize, Plan, Issues, false, 1, 2))
			{
				AddError(FString::Printf(TEXT("Seed %d produced no plan."), Seed));
				return 1.0f;
			}

			CorridorTiles += Plan.CorridorTileCount;
			FloorTiles += Plan.FloorTileCount;
		}
		return (FloorTiles > 0) ? static_cast<float>(CorridorTiles) / static_cast<float>(FloorTiles) : 1.0f;
	};

	const float GrowthShare = MeasureCorridorShare(EPHRegionPlacement::Growth);
	const float ScatterShare = MeasureCorridorShare(EPHRegionPlacement::Scatter);

	AddInfo(FString::Printf(TEXT("Corridor share of the floor: growth %.3f, scatter %.3f."),
		GrowthShare, ScatterShare));

	TestTrue(FString::Printf(
		TEXT("Growth spends less of the floor on corridor than scatter (%.3f vs %.3f)"),
		GrowthShare, ScatterShare), GrowthShare < ScatterShare);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHBlockoutLightCoverageTest,
	"ProjectHunter.Generation.Blockout.EveryFloorTileIsLit", PHBlockoutPlanTests::TestFlags)

bool FPHBlockoutLightCoverageTest::RunTest(const FString&)
{
	using namespace PHBlockoutPlanTests;
	TStrongObjectPtr<UPHDungeonGenerator> Generator(NewObject<UPHDungeonGenerator>());

	// The claim one-light-per-room could never make. A room lit only at its centre is dark at the
	// corners once it is more than a couple of tiles across, and that is exactly what showed up in
	// play; coverage is the property worth asserting, not the light count.
	const TArray<int32> Spacings = { 2, 3, 5 };
	for (const int32 Spacing : Spacings)
	{
		int32 TotalLights = 0;
		int32 TotalTiles = 0;

		for (int32 Seed = 1; Seed <= 60; ++Seed)
		{
			FPHGeneratedLayout Layout;
			TArray<FPHGenerationIssue> Issues;
			if (!Generator->GenerateLayout(MakeRequest(Seed), Layout, Issues))
			{
				AddError(FString::Printf(TEXT("Seed %d produced no layout."), Seed));
				return false;
			}

			FPHBlockoutPlan Plan;
			if (!UPHBlockoutPlanLibrary::BuildPlan(
				Layout, TileSize, Plan, Issues, false, 1, 2, TileSize, Spacing))
			{
				AddError(FString::Printf(TEXT("Seed %d produced no plan."), Seed));
				return false;
			}

			if (Plan.LightPoses.IsEmpty())
			{
				AddError(FString::Printf(TEXT("Seed %d planned no lights at spacing %d."),
					Seed, Spacing));
				return false;
			}

			for (const FIntPoint& Tile : Plan.FloorTiles)
			{
				const FVector2D TileCentre((Tile.X + 0.5) * TileSize, (Tile.Y + 0.5) * TileSize);

				double NearestSquared = TNumericLimits<double>::Max();
				for (const FTransform& Light : Plan.LightPoses)
				{
					NearestSquared = FMath::Min(NearestSquared, FVector2D::DistSquared(
						TileCentre, FVector2D(Light.GetLocation().X, Light.GetLocation().Y)));
				}

				const double Allowed = FMath::Square(Spacing * TileSize);
				if (NearestSquared > Allowed + UE_KINDA_SMALL_NUMBER)
				{
					AddError(FString::Printf(
						TEXT("Seed %d spacing %d: tile %s is %.0f units from the nearest light, over %.0f."),
						Seed, Spacing, *Tile.ToString(),
						FMath::Sqrt(NearestSquared), FMath::Sqrt(Allowed)));
					return false;
				}
			}

			TotalLights += Plan.LightPoses.Num();
			TotalTiles += Plan.FloorTileCount;
		}

		AddInfo(FString::Printf(TEXT("Spacing %d: %d lights over %d floor tiles (1 per %.1f)."),
			Spacing, TotalLights, TotalTiles,
			static_cast<float>(TotalTiles) / static_cast<float>(FMath::Max(1, TotalLights))));
	}

	// Lights mount on the ceiling plane. They used to be planned a quarter of a wall
	// course below it, which hung every fixture about a metre under the ceiling - the
	// defect the user reported. Ceiling pieces are the reference: a light's Z must
	// match the ceiling placed over its own tile.
	{
		FPHGeneratedLayout CeilingLayout;
		TArray<FPHGenerationIssue> CeilingIssues;
		Generator->GenerateLayout(MakeRequest(7), CeilingLayout, CeilingIssues);

		FPHBlockoutPlan Lit;
		if (UPHBlockoutPlanLibrary::BuildPlan(
			CeilingLayout, TileSize, Lit, CeilingIssues, /*bIncludeCeiling*/ true, 1, 2, TileSize, 3)
			&& !Lit.LightPoses.IsEmpty())
		{
			TArray<double> CeilingHeights;
			for (const FPHPiecePlacement& Placement : Lit.Placements)
			{
				if (Placement.PieceTag == PHGenerationTags::Piece_Ceiling.GetTag())
				{
					CeilingHeights.AddUnique(Placement.Transform.GetLocation().Z);
				}
			}

			if (TestFalse(TEXT("The lit plan placed ceilings to compare against"),
				CeilingHeights.IsEmpty()))
			{
				for (const FTransform& Light : Lit.LightPoses)
				{
					const double LightZ = Light.GetLocation().Z;
					const bool bOnACeiling = CeilingHeights.ContainsByPredicate(
						[LightZ](const double Height)
						{
							return FMath::IsNearlyEqual(Height, LightZ, 0.01);
						});
					if (!bOnACeiling)
					{
						AddError(FString::Printf(
							TEXT("A light sits at Z=%.1f, which matches no ceiling height. "
							     "Lights must mount on the ceiling plane."), LightZ));
						break;
					}
				}
			}
		}
	}

	// Zero is the documented way to plan no lighting at all, not an accident of the loop bounds.
	FPHGeneratedLayout Layout;
	TArray<FPHGenerationIssue> Issues;
	Generator->GenerateLayout(MakeRequest(1), Layout, Issues);
	FPHBlockoutPlan Unlit;
	UPHBlockoutPlanLibrary::BuildPlan(Layout, TileSize, Unlit, Issues, false, 1, 2, TileSize, 0);
	TestEqual(TEXT("Spacing 0 plans no lights"), Unlit.LightPoses.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHBlockoutWholeWallCoursesTest,
	"ProjectHunter.Generation.Blockout.RejectsFractionalWallCourses", PHBlockoutPlanTests::TestFlags)

bool FPHBlockoutWholeWallCoursesTest::RunTest(const FString&)
{
	using namespace PHBlockoutPlanTests;
	FPHBlockoutPlan Plan;
	TArray<FPHGenerationIssue> Issues;
	TestTrue(TEXT("Two whole wall courses are buildable"),
		UPHBlockoutPlanLibrary::BuildPlan(MakeRoomFixture(800.0), TileSize, Plan, Issues, true, 1, 1, 400.0));
	TestEqual(TEXT("All eight exposed edges receive two courses"), Plan.WallCount, 16);

	for (const double Height : { 900.0, 1000.0 })
	{
		TestFalse(FString::Printf(TEXT("Height %.0f cannot be sealed by 400-unit courses"), Height),
			UPHBlockoutPlanLibrary::BuildPlan(MakeRoomFixture(Height), TileSize, Plan, Issues, true, 1, 1, 400.0));
		TestTrue(TEXT("Incompatible courses report an issue"), !Issues.IsEmpty());
		TestEqual(TEXT("An incompatible course leaves no partial placements"), Plan.Placements.Num(), 0);
		TestEqual(TEXT("An incompatible course leaves no floor tiles"), Plan.FloorTiles.Num(), 0);
	}

	TestTrue(TEXT("Zero wall height retains the tile-sized default"),
		UPHBlockoutPlanLibrary::BuildPlan(MakeRoomFixture(), TileSize, Plan, Issues, false, 1, 1, 0.0));
	TestTrue(TEXT("A negative wall-height sentinel retains the existing tile-sized fallback"),
		UPHBlockoutPlanLibrary::BuildPlan(MakeRoomFixture(), TileSize, Plan, Issues, false, 1, 1, -1.0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHBlockoutEnvelopeTest,
	"ProjectHunter.Generation.Blockout.RefusesCorridorsOutsideEnvelope", PHBlockoutPlanTests::TestFlags)

bool FPHBlockoutEnvelopeTest::RunTest(const FString&)
{
	using namespace PHBlockoutPlanTests;
	FPHGeneratedLayout Layout = MakeRoomFixture(400.0);
	Layout.Bounds = FBox(FVector::ZeroVector, FVector(1600.0, 400.0, 400.0));
	Layout.Regions[0].Bounds = FBox(FVector::ZeroVector, FVector(400.0, 400.0, 400.0));
	FPHGeneratedRegion& Second = Layout.Regions.AddDefaulted_GetRef();
	Second.RegionID = 1;
	Second.Bounds = FBox(FVector(1200.0, 0.0, 0.0), FVector(1600.0, 400.0, 400.0));
	Layout.Anchors[1].RegionID = 1;
	Layout.Anchors[1].Transform = FTransform(FVector(1400.0, 200.0, 0.0));
	FPHGeneratedConnection& Connection = Layout.Connections.AddDefaulted_GetRef();
	Connection.ConnectionID = 0;
	Connection.FromRegionID = 0;
	Connection.ToRegionID = 1;

	FPHBlockoutPlan Plan;
	TArray<FPHGenerationIssue> Issues;
	TestTrue(TEXT("The one-tile corridor fits the envelope"),
		UPHBlockoutPlanLibrary::BuildPlan(Layout, TileSize, Plan, Issues, false, 1, 1));
	TestEqual(TEXT("The narrow corridor joins both rooms across two gap tiles"), Plan.CorridorTileCount, 2);

	TestFalse(TEXT("A three-tile corridor cannot fit a one-tile-high envelope"),
		UPHBlockoutPlanLibrary::BuildPlan(Layout, TileSize, Plan, Issues, false, 3, 3));
	TestTrue(TEXT("The unsupported width reports an issue"), !Issues.IsEmpty());
	TestEqual(TEXT("A refused corridor leaves no partial placements"), Plan.Placements.Num(), 0);
	TestEqual(TEXT("A refused corridor leaves no partial floor tiles"), Plan.FloorTiles.Num(), 0);

	// The same impossible corridor, marked optional, is dropped instead of taking the floor with
	// it. A loop is an enrichment on a spanning tree, and one pair of rooms that barely face each
	// other should not make an otherwise sound floor unbuildable - it was costing one seed in four
	// on the current kit, where MinCorridorWidth is 2.
	Layout.Connections[0].bOptional = true;
	TestTrue(TEXT("An optional corridor that cannot fit is dropped, not fatal"),
		UPHBlockoutPlanLibrary::BuildPlan(Layout, TileSize, Plan, Issues, false, 3, 3));
	TestEqual(TEXT("Dropping it leaves the rooms themselves built"), Plan.FloorTiles.Num(), 2);
	TestEqual(TEXT("Dropping it adds no corridor"), Plan.CorridorTileCount, 0);

	// A required connection still refuses, so the distinction is doing the work rather than the
	// planner having quietly become permissive.
	Layout.Connections[0].bOptional = false;
	TestFalse(TEXT("A required corridor that cannot fit still refuses"),
		UPHBlockoutPlanLibrary::BuildPlan(Layout, TileSize, Plan, Issues, false, 3, 3));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHBlockoutFiniteSettingsTest,
	"ProjectHunter.Generation.Blockout.RejectsNonfiniteSettings", PHBlockoutPlanTests::TestFlags)

bool FPHBlockoutFiniteSettingsTest::RunTest(const FString&)
{
	using namespace PHBlockoutPlanTests;
	// A planar room and disabled lighting keep the old implementation from constructing NaN
	// transforms while the test checks whether settings are refused before geometry arithmetic.
	const FPHGeneratedLayout Layout = MakeRoomFixture(0.0);

	auto ExpectRejected = [&](const TCHAR* Label, const double TestTileSize, const double TestWallHeight)
	{
		FPHBlockoutPlan Plan;
		TArray<FPHGenerationIssue> Issues;
		TestFalse(Label, UPHBlockoutPlanLibrary::BuildPlan(Layout, TestTileSize, Plan, Issues, false, 1, 1, TestWallHeight, 0));
		TestTrue(FString::Printf(TEXT("%s reports InvalidRequest before geometry arithmetic"), Label), Issues.ContainsByPredicate(
			[](const FPHGenerationIssue& Issue) { return Issue.Code == EPHGenerationIssueCode::InvalidRequest; }));
		TestEqual(TEXT("Invalid settings leave no partial placements"), Plan.Placements.Num(), 0);
	};
	ExpectRejected(TEXT("NaN tile size"), std::numeric_limits<double>::quiet_NaN(), 400.0);
	ExpectRejected(TEXT("Infinite tile size"), std::numeric_limits<double>::infinity(), 400.0);
	ExpectRejected(TEXT("NaN wall course"), TileSize, std::numeric_limits<double>::quiet_NaN());
	ExpectRejected(TEXT("Infinite wall course"), TileSize, std::numeric_limits<double>::infinity());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHBlockoutWorkBudgetTest,
	"ProjectHunter.Generation.Blockout.RefusesUnboundedConstructionWork", PHBlockoutPlanTests::TestFlags)

bool FPHBlockoutWorkBudgetTest::RunTest(const FString&)
{
	using namespace PHBlockoutPlanTests;
	FPHGeneratedLayout HugeRoom = MakeRoomFixture();
	HugeRoom.Bounds.Max.X = 400000000.0;
	HugeRoom.Bounds.Max.Y = 400000000.0;
	HugeRoom.Regions[0].Bounds = HugeRoom.Bounds;
	FPHBlockoutPlan Plan;
	TArray<FPHGenerationIssue> Issues;
	TestFalse(TEXT("A trillion-tile room is refused before raster allocation"),
		UPHBlockoutPlanLibrary::BuildPlan(HugeRoom, TileSize, Plan, Issues));
	TestEqual(TEXT("An oversized raster leaves no partial plan"), Plan.Placements.Num(), 0);
	TestTrue(TEXT("The raster refusal is diagnosed"), !Issues.IsEmpty());

	TestFalse(TEXT("A million wall courses are refused before placement allocation"),
		UPHBlockoutPlanLibrary::BuildPlan(MakeRoomFixture(400000000.0), TileSize, Plan, Issues));
	TestEqual(TEXT("Oversized wall work leaves no partial plan"), Plan.Placements.Num(), 0);
	TestTrue(TEXT("The wall-work refusal is diagnosed"), !Issues.IsEmpty());

	TestFalse(TEXT("An overflowing random-width range is refused"),
		UPHBlockoutPlanLibrary::BuildPlan(MakeRoomFixture(), TileSize, Plan, Issues, false, 1, MAX_int32));
	TestTrue(TEXT("The width refusal is diagnosed"), !Issues.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHBlockoutCurrentKitSeedSweepTest,
	"ProjectHunter.Generation.Blockout.CurrentKit500SeedConstruction", PHBlockoutPlanTests::TestFlags)

bool FPHBlockoutCurrentKitSeedSweepTest::RunTest(const FString&)
{
	using namespace PHBlockoutPlanTests;
	TStrongObjectPtr<UPHDungeonGenerator> Generator(NewObject<UPHDungeonGenerator>());
	FPHLayoutRequest Request;
	Request.GridSize = TileSize;
	Request.MinRegionCount = 7;
	Request.MaxRegionCount = 14;
	Request.MinRegionSize = FVector2D(1200.0, 1200.0);
	Request.MaxRegionSize = FVector2D(2800.0, 2800.0);
	Request.RegionSpacing = 800.0;
	Request.AreaSize = FVector2D(12000.0, 12000.0);
	Request.RegionPlacement = EPHRegionPlacement::Growth;
	Request.RegionHeight = 400.0;
	Request.MaxHeightStacks = 3;
	FPHAnchorRule Enemies;
	Enemies.SemanticTag = PHGenerationTags::Anchor_Enemy_Small.GetTag();
	Enemies.MinPerRegion = 2;
	Enemies.MaxPerRegion = 4;
	Enemies.bAllowInStartRegion = false;
	Request.AnchorRules = { Enemies };

	int32 GeneratedCount = 0;
	int32 RefusedCount = 0;
	for (int32 Seed = 1; Seed <= 500; ++Seed)
	{
		Request.Seed = Seed;
		FPHGeneratedLayout Layout;
		TArray<FPHGenerationIssue> Issues;
		if (!Generator->GenerateLayout(Request, Layout, Issues))
		{
			++RefusedCount;
			TestTrue(TEXT("A refused logical layout supplies a diagnostic"), !Issues.IsEmpty());
			continue;
		}
		++GeneratedCount;
		if (!TestEqual(TEXT("The sweep exercises the current v3 generator"), Layout.GenerationVersion, 3))
		{
			return false;
		}

		FPHBlockoutPlan Plan;
		if (!UPHBlockoutPlanLibrary::BuildPlan(Layout, TileSize, Plan, Issues, true, 1, 2, 400.0, 3))
		{
			AddError(FString::Printf(TEXT("Current-kit seed %d generated a layout that could not build: %s"),
				Seed, Issues.IsEmpty() ? TEXT("no diagnostic") : *Issues[0].Message));
			return false;
		}

		const TSet<FIntPoint> Tiles = FloorTilesOf(Plan);
		for (const FIntPoint& Tile : Tiles)
		{
			const FVector Minimum(Tile.X * TileSize, Tile.Y * TileSize, Layout.Bounds.Min.Z);
			const FVector Maximum((Tile.X + 1) * TileSize, (Tile.Y + 1) * TileSize, Layout.Bounds.Min.Z);
			if (!Layout.Bounds.IsInsideOrOn(Minimum) || !Layout.Bounds.IsInsideOrOn(Maximum))
			{
				AddError(FString::Printf(TEXT("Current-kit seed %d builds tile %s outside its envelope."), Seed, *Tile.ToString()));
				return false;
			}
		}

		TSet<FIntPoint> Reached;
		TArray<FIntPoint> Pending;
		const FIntPoint Start = ToTile(Plan.PlayerStart.GetLocation());
		const FIntPoint Exit = ToTile(Plan.Exit.GetLocation());
		if (Tiles.Contains(Start))
		{
			Reached.Add(Start);
			Pending.Add(Start);
		}
		for (int32 Index = 0; Index < Pending.Num(); ++Index)
		{
			const FIntPoint Tile = Pending[Index];
			const FIntPoint Neighbours[] = { Tile + FIntPoint(1, 0), Tile + FIntPoint(-1, 0),
				Tile + FIntPoint(0, 1), Tile + FIntPoint(0, -1) };
			for (const FIntPoint& Next : Neighbours)
			{
				if (Tiles.Contains(Next) && !Reached.Contains(Next))
				{
					Reached.Add(Next);
					Pending.Add(Next);
				}
			}
		}
		if (Reached.Num() != Tiles.Num() || !Reached.Contains(Exit))
		{
			AddError(FString::Printf(TEXT("Current-kit seed %d builds disconnected floor or endpoints."), Seed));
			return false;
		}

		FPHGeneratedLayout RepeatedLayout;
		FPHBlockoutPlan Repeated;
		if (!Generator->GenerateLayout(Request, RepeatedLayout, Issues)
			|| !UPHBlockoutPlanLibrary::BuildPlan(RepeatedLayout, TileSize, Repeated, Issues, true, 1, 2, 400.0, 3)
			|| Plan.FloorTiles != Repeated.FloorTiles || Plan.CorridorTiles != Repeated.CorridorTiles
			|| Plan.Placements.Num() != Repeated.Placements.Num() || Plan.LightPoses.Num() != Repeated.LightPoses.Num()
			|| !Plan.PlayerStart.Equals(Repeated.PlayerStart) || !Plan.Exit.Equals(Repeated.Exit)
			|| Layout.Anchors.Num() != RepeatedLayout.Anchors.Num())
		{
			AddError(FString::Printf(TEXT("Current-kit seed %d cannot reproduce its plan."), Seed));
			return false;
		}
		for (int32 Index = 0; Index < Plan.Placements.Num(); ++Index)
		{
			if (Plan.Placements[Index].PieceTag != Repeated.Placements[Index].PieceTag
				|| !Plan.Placements[Index].Transform.Equals(Repeated.Placements[Index].Transform))
			{
				AddError(FString::Printf(TEXT("Current-kit seed %d changes construction placement %d on replay."), Seed, Index));
				return false;
			}
		}
		for (int32 Index = 0; Index < Plan.LightPoses.Num(); ++Index)
		{
			if (!Plan.LightPoses[Index].Equals(Repeated.LightPoses[Index]))
			{
				AddError(FString::Printf(TEXT("Current-kit seed %d changes light %d on replay."), Seed, Index));
				return false;
			}
		}
		for (int32 Index = 0; Index < Layout.Anchors.Num(); ++Index)
		{
			const FPHGeneratedAnchor& First = Layout.Anchors[Index];
			const FPHGeneratedAnchor& Second = RepeatedLayout.Anchors[Index];
			if (First.AnchorID != Second.AnchorID || First.RegionID != Second.RegionID
				|| First.SemanticTag != Second.SemanticTag || !First.Transform.Equals(Second.Transform))
			{
				AddError(FString::Printf(TEXT("Current-kit seed %d changes anchor %d on replay."), Seed, Index));
				return false;
			}
		}
	}
	AddInfo(FString::Printf(TEXT("Current-kit sweep: %d of 500 seeds generated; %d logical refusals; every accepted layout built, connected and replayed."),
		GeneratedCount, RefusedCount));
	TestTrue(TEXT("The sweep produced actual layouts to verify"), GeneratedCount > 0);
	return true;
}


#endif // WITH_DEV_AUTOMATION_TESTS
