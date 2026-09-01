#include "CoreMinimal.h"
#include "Generation/Generators/PHDungeonGenerator.h"
#include "Generation/Library/FunctionLibraries/PHBlockoutPlanLibrary.h"
#include "Generation/Library/FunctionLibraries/PHDecorationPlanLibrary.h"
#include "Generation/PHGenerationTags.h"
#include "Misc/AutomationTest.h"
#include "Tower/Library/FunctionLibraries/RunSeedFunctionLibrary.h"
#include "UObject/StrongObjectPtr.h"

#include <limits>

#if WITH_DEV_AUTOMATION_TESTS

namespace PHDecorationPlanTests
{
	constexpr EAutomationTestFlags TestFlags =
		EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter;

	constexpr double TileSize = 400.0;

	UE_DEFINE_GAMEPLAY_TAG_STATIC(NonPropTag, "Test.Generation.NotAProp");

	FPHLayoutRequest MakeRequest(const int32 Seed)
	{
		FPHLayoutRequest Request;
		Request.Seed = Seed;
		Request.GridSize = TileSize;
		Request.MinRegionSize = FVector2D(1200.0, 1200.0);
		Request.MaxRegionSize = FVector2D(2400.0, 2400.0);
		Request.RegionSpacing = 800.0;
		Request.AreaSize = FVector2D(12000.0, 12000.0);
		return Request;
	}

	TArray<FPHPropRule> MakeRules()
	{
		FPHPropRule Barrels;
		Barrels.PropTag = PHGenerationTags::Prop_Barrel.GetTag();
		Barrels.Placement = EPHPropPlacement::AgainstWall;
		Barrels.ChancePerTile = 0.35f;

		FPHPropRule Campfires;
		Campfires.PropTag = PHGenerationTags::Prop_Campfire.GetTag();
		Campfires.Placement = EPHPropPlacement::OpenFloor;
		Campfires.ChancePerTile = 0.2f;
		Campfires.MaxTotal = 3;

		FPHPropRule Debris;
		Debris.PropTag = PHGenerationTags::Prop_Debris.GetTag();
		Debris.Placement = EPHPropPlacement::Anywhere;
		Debris.ChancePerTile = 0.15f;
		Debris.bAvoidCorridors = false;

		return { Barrels, Campfires, Debris };
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

	FString BlockoutFingerprint(const FPHBlockoutPlan& Plan)
	{
		FString Text = FString::Printf(TEXT("F%d W%d C%d"),
			Plan.FloorTileCount, Plan.WallCount, Plan.CorridorTileCount);
		for (const FPHPiecePlacement& Placement : Plan.Placements)
		{
			Text += FString::Printf(TEXT("|%s %s"), *Placement.PieceTag.ToString(),
				*Placement.Transform.GetLocation().ToString());
		}
		return Text;
	}

	FIntPoint ToTile(const FVector& Location)
	{
		return FIntPoint(FMath::FloorToInt32(Location.X / TileSize),
			FMath::FloorToInt32(Location.Y / TileSize));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHDecorationDoesNotChangeFloorTest,
	"ProjectHunter.Generation.Decoration.DoesNotChangeTheFloor", PHDecorationPlanTests::TestFlags)

bool FPHDecorationDoesNotChangeFloorTest::RunTest(const FString&)
{
	using namespace PHDecorationPlanTests;

	// GAME_DESIGN §38 states this as a hard rule: adding, removing, or reweighting props must
	// never move geometry. Decoration draws from its own stream, and this is what proves it.
	for (int32 Seed = 1; Seed <= 60; ++Seed)
	{
		FPHGeneratedLayout Layout;
		FPHBlockoutPlan Plan;
		if (!BuildFor(Seed, Layout, Plan))
		{
			AddError(FString::Printf(TEXT("Seed %d produced no plan."), Seed));
			return false;
		}

		const FString Before = BlockoutFingerprint(Plan);
		const int32 DecorationSeed = URunSeedFunctionLibrary::DeriveSeed(
			Layout.Seed, FName(TEXT("Decoration")), 0);

		FPHDecorationPlan Light;
		FPHDecorationPlan Heavy;
		TArray<FPHGenerationIssue> Issues;

		TArray<FPHPropRule> Sparse = MakeRules();
		for (FPHPropRule& Rule : Sparse)
		{
			Rule.ChancePerTile = 0.05f;
		}
		TArray<FPHPropRule> Dense = MakeRules();
		for (FPHPropRule& Rule : Dense)
		{
			Rule.ChancePerTile = 0.9f;
		}

		UPHDecorationPlanLibrary::BuildDecorationPlan(Layout, Plan, Sparse, DecorationSeed, Light, Issues);
		UPHDecorationPlanLibrary::BuildDecorationPlan(Layout, Plan, Dense, DecorationSeed, Heavy, Issues);

		if (BlockoutFingerprint(Plan) != Before)
		{
			AddError(FString::Printf(TEXT("Seed %d: decoration mutated the blockout."), Seed));
			return false;
		}

		if (Heavy.PropCount <= Light.PropCount)
		{
			AddError(FString::Printf(
				TEXT("Seed %d: raising density did not add props (%d vs %d)."),
				Seed, Heavy.PropCount, Light.PropCount));
			return false;
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHDecorationPlacementRulesTest,
	"ProjectHunter.Generation.Decoration.RespectsPlacementRules", PHDecorationPlanTests::TestFlags)

bool FPHDecorationPlacementRulesTest::RunTest(const FString&)
{
	using namespace PHDecorationPlanTests;

	for (int32 Seed = 1; Seed <= 60; ++Seed)
	{
		FPHGeneratedLayout Layout;
		FPHBlockoutPlan Plan;
		if (!BuildFor(Seed, Layout, Plan))
		{
			AddError(FString::Printf(TEXT("Seed %d produced no plan."), Seed));
			return false;
		}

		FPHDecorationPlan Decoration;
		TArray<FPHGenerationIssue> Issues;
		if (!UPHDecorationPlanLibrary::BuildDecorationPlan(Layout, Plan, MakeRules(),
			URunSeedFunctionLibrary::DeriveSeed(Layout.Seed, FName(TEXT("Decoration")), 0),
			Decoration, Issues))
		{
			AddError(FString::Printf(TEXT("Seed %d: decoration planning failed."), Seed));
			return false;
		}

		const TSet<FIntPoint> Floor(Plan.FloorTiles);
		const TSet<FIntPoint> Corridors(Plan.CorridorTiles);
		const FIntPoint StartTile = ToTile(Plan.PlayerStart.GetLocation());
		const FIntPoint ExitTile = ToTile(Plan.Exit.GetLocation());

		TSet<FIntPoint> Used;
		int32 Campfires = 0;
		for (const FPHPiecePlacement& Prop : Decoration.Placements)
		{
			const FIntPoint Tile = ToTile(Prop.Transform.GetLocation());

			if (!Floor.Contains(Tile))
			{
				AddError(FString::Printf(TEXT("Seed %d put a prop off the floor."), Seed));
				return false;
			}

			if (Used.Contains(Tile))
			{
				AddError(FString::Printf(TEXT("Seed %d stacked two props on one tile."), Seed));
				return false;
			}
			Used.Add(Tile);

			if (Tile == StartTile || Tile == ExitTile)
			{
				AddError(FString::Printf(TEXT("Seed %d decorated an endpoint tile."), Seed));
				return false;
			}

			const FIntPoint Neighbours[] = {
				Tile + FIntPoint(1, 0), Tile + FIntPoint(-1, 0),
				Tile + FIntPoint(0, 1), Tile + FIntPoint(0, -1) };
			int32 OpenSides = 0;
			for (const FIntPoint& Next : Neighbours)
			{
				if (!Floor.Contains(Next))
				{
					++OpenSides;
				}
			}

			if (Prop.PieceTag == PHGenerationTags::Prop_Barrel.GetTag())
			{
				if (OpenSides == 0)
				{
					AddError(FString::Printf(TEXT("Seed %d put a wall prop in open floor."), Seed));
					return false;
				}
				if (Corridors.Contains(Tile))
				{
					AddError(FString::Printf(TEXT("Seed %d put a barrel in a corridor."), Seed));
					return false;
				}
			}

			if (Prop.PieceTag == PHGenerationTags::Prop_Campfire.GetTag())
			{
				++Campfires;
				if (OpenSides != 0)
				{
					AddError(FString::Printf(TEXT("Seed %d put an open-floor prop against a wall."),
						Seed));
					return false;
				}
			}
		}

		if (Campfires > 3)
		{
			AddError(FString::Printf(TEXT("Seed %d placed %d campfires over its cap of 3."),
				Seed, Campfires));
			return false;
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHDecorationDeterminismTest,
	"ProjectHunter.Generation.Decoration.IsDeterministic", PHDecorationPlanTests::TestFlags)

bool FPHDecorationDeterminismTest::RunTest(const FString&)
{
	using namespace PHDecorationPlanTests;

	for (int32 Seed = 1; Seed <= 40; ++Seed)
	{
		FPHGeneratedLayout Layout;
		FPHBlockoutPlan Plan;
		BuildFor(Seed, Layout, Plan);

		const int32 DecorationSeed = URunSeedFunctionLibrary::DeriveSeed(
			Layout.Seed, FName(TEXT("Decoration")), 0);

		FPHDecorationPlan First, Second;
		TArray<FPHGenerationIssue> Issues;
		UPHDecorationPlanLibrary::BuildDecorationPlan(Layout, Plan, MakeRules(), DecorationSeed, First, Issues);
		UPHDecorationPlanLibrary::BuildDecorationPlan(Layout, Plan, MakeRules(), DecorationSeed, Second, Issues);

		if (First.Placements.Num() != Second.Placements.Num())
		{
			AddError(FString::Printf(TEXT("Seed %d produced different prop counts."), Seed));
			return false;
		}

		for (int32 Index = 0; Index < First.Placements.Num(); ++Index)
		{
			if (!First.Placements[Index].Transform.Equals(Second.Placements[Index].Transform)
				|| First.Placements[Index].PieceTag != Second.Placements[Index].PieceTag)
			{
				AddError(FString::Printf(TEXT("Seed %d prop %d differs between runs."), Seed, Index));
				return false;
			}
		}

		// A different decoration seed must actually change the dressing.
		FPHDecorationPlan Other;
		UPHDecorationPlanLibrary::BuildDecorationPlan(Layout, Plan, MakeRules(),
			DecorationSeed + 1, Other, Issues);
		if (Other.PropCount == First.PropCount && First.Placements.Num() > 4
			&& Other.Placements.Num() > 0
			&& Other.Placements[0].Transform.Equals(First.Placements[0].Transform))
		{
			AddError(FString::Printf(TEXT("Seed %d ignored the decoration seed."), Seed));
			return false;
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHDecorationBadRulesTest,
	"ProjectHunter.Generation.Decoration.RefusesInvalidRules", PHDecorationPlanTests::TestFlags)

bool FPHDecorationBadRulesTest::RunTest(const FString&)
{
	using namespace PHDecorationPlanTests;
	FPHGeneratedLayout Layout;
	FPHBlockoutPlan Plan;
	BuildFor(3, Layout, Plan);

	auto ExpectRefused = [this, &Layout, &Plan](const TCHAR* Label, const FPHPropRule& Rule)
	{
		FPHDecorationPlan Decoration;
		TArray<FPHGenerationIssue> Issues;
		TestFalse(FString::Printf(TEXT("%s is refused"), Label),
			UPHDecorationPlanLibrary::BuildDecorationPlan(Layout, Plan, { Rule }, 7, Decoration, Issues));
		TestEqual(FString::Printf(TEXT("%s places nothing"), Label), Decoration.Placements.Num(), 0);
	};

	FPHPropRule NoTag;
	ExpectRefused(TEXT("A rule with no tag"), NoTag);

	FPHPropRule RootTag;
	RootTag.PropTag = PHGenerationTags::Prop.GetTag();
	ExpectRefused(TEXT("A rule using the Prop root"), RootTag);

	FPHPropRule WrongFamily;
	WrongFamily.PropTag = PHGenerationTags::Piece_Floor.GetTag();
	ExpectRefused(TEXT("A rule pointing at a construction piece"), WrongFamily);

	FPHPropRule Unregistered;
	Unregistered.PropTag = NonPropTag;
	ExpectRefused(TEXT("A rule outside the Prop hierarchy"), Unregistered);

	FPHPropRule NegativeCap;
	NegativeCap.PropTag = PHGenerationTags::Prop_Barrel.GetTag();
	NegativeCap.MaxTotal = -1;
	ExpectRefused(TEXT("A rule with a negative cap"), NegativeCap);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHDecorationClusteringTest,
	"ProjectHunter.Generation.Decoration.ClustersInsteadOfSprinkling",
	PHDecorationPlanTests::TestFlags)

bool FPHDecorationClusteringTest::RunTest(const FString&)
{
	using namespace PHDecorationPlanTests;

	// An even per-tile chance produces uniform noise, which is what makes a floor read as randomly
	// generated rather than lived in. The measure is the share of props that have another prop on
	// one of the eight tiles around them: things that accumulated somewhere touch each other, and
	// things sprinkled independently mostly do not.
	//
	// Deliberately not a nearest-neighbour or Clark-Evans index. Those normalise against a Poisson
	// expectation, and props here sit one per tile on a grid - a hard-core process with a built-in
	// minimum distance - so the index reads about 1.4 even for placement that is perfectly uniform.
	// Two earlier versions of this test chased that number instead of measuring the thing claimed.
	auto TouchingShare = [](const TArray<FIntPoint>& Tiles)
	{
		if (Tiles.Num() < 2)
		{
			return 0.0f;
		}

		const TSet<FIntPoint> Occupied(Tiles);
		int32 Touching = 0;
		for (const FIntPoint& Tile : Tiles)
		{
			bool bHasNeighbour = false;
			for (int32 X = -1; X <= 1 && !bHasNeighbour; ++X)
			{
				for (int32 Y = -1; Y <= 1 && !bHasNeighbour; ++Y)
				{
					if ((X != 0 || Y != 0) && Occupied.Contains(Tile + FIntPoint(X, Y)))
					{
						bHasNeighbour = true;
					}
				}
			}
			Touching += bHasNeighbour ? 1 : 0;
		}
		return static_cast<float>(Touching) / static_cast<float>(Tiles.Num());
	};

	auto Measure = [&](const int32 Clusters, float& OutShare, int32& OutCount)
	{
		int32 Touching = 0;
		OutCount = 0;

		for (int32 Seed = 1; Seed <= 60; ++Seed)
		{
			FPHGeneratedLayout Layout;
			FPHBlockoutPlan Plan;
			if (!BuildFor(Seed, Layout, Plan))
			{
				AddError(FString::Printf(TEXT("Seed %d produced no plan."), Seed));
				return false;
			}

			// One rule, placed anywhere, so eligibility cannot confound the measurement, and no
			// corner bias or density jitter so the difference is clustering alone.
			FPHPropRule Debris;
			Debris.PropTag = PHGenerationTags::Prop_Debris.GetTag();
			Debris.Placement = EPHPropPlacement::Anywhere;
			Debris.ChancePerTile = 0.12f;
			Debris.bAvoidCorridors = false;
			Debris.ClustersPerRegion = Clusters;
			Debris.CornerBias = 0.0f;
			Debris.RegionDensityJitter = 0.0f;

			FPHDecorationPlan Decoration;
			TArray<FPHGenerationIssue> Issues;
			if (!UPHDecorationPlanLibrary::BuildDecorationPlan(Layout, Plan, { Debris },
				URunSeedFunctionLibrary::DeriveSeed(Layout.Seed, FName(TEXT("Decoration")), 0),
				Decoration, Issues))
			{
				AddError(FString::Printf(TEXT("Seed %d: decoration planning failed."), Seed));
				return false;
			}

			TArray<FIntPoint> Tiles;
			for (const FPHPiecePlacement& Prop : Decoration.Placements)
			{
				Tiles.Add(ToTile(Prop.Transform.GetLocation()));
			}

			Touching += FMath::RoundToInt32(TouchingShare(Tiles) * Tiles.Num());
			OutCount += Tiles.Num();
		}

		OutShare = (OutCount > 0)
			? static_cast<float>(Touching) / static_cast<float>(OutCount) : 0.0f;
		return OutCount > 0;
	};

	float EvenShare = 0.0f;
	float ClusteredShare = 0.0f;
	int32 EvenCount = 0;
	int32 ClusteredCount = 0;

	if (!Measure(0, EvenShare, EvenCount) || !Measure(2, ClusteredShare, ClusteredCount))
	{
		AddError(TEXT("No props were placed, so clustering cannot be measured."));
		return false;
	}

	AddInfo(FString::Printf(
		TEXT("Props with a neighbour: even scatter %.1f%% of %d, clustered %.1f%% of %d."),
		EvenShare * 100.0f, EvenCount, ClusteredShare * 100.0f, ClusteredCount));

	TestTrue(FString::Printf(TEXT("Clustering makes props touch far more often (%.1f%% vs %.1f%%)"),
		ClusteredShare * 100.0f, EvenShare * 100.0f), ClusteredShare > EvenShare * 1.4f);

	// Clustering decides where props go, not how many. Left unnormalised, concentrating weight into
	// a few tiles thins everywhere else and empties the floor, which was measured at a 4-to-1 drop
	// before the weights were normalised back to the authored chance.
	const float CountRatio = static_cast<float>(ClusteredCount) / static_cast<float>(EvenCount);
	AddInfo(FString::Printf(TEXT("Clustered floors carry %.2f times the props of even ones."),
		CountRatio));
	TestTrue(FString::Printf(TEXT("Clustering preserves prop density (ratio %.2f)"), CountRatio),
		CountRatio > 0.75f && CountRatio < 1.25f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHDecorationSpacingTest,
	"ProjectHunter.Generation.Decoration.HonoursMinimumSpacing", PHDecorationPlanTests::TestFlags)

bool FPHDecorationSpacingTest::RunTest(const FString&)
{
	using namespace PHDecorationPlanTests;

	// Spacing is counted against props of every kind, not just this rule's, because a table
	// wedged against a barrel looks wrong regardless of which rule placed the barrel.
	const int32 Spacing = 2;

	for (int32 Seed = 1; Seed <= 40; ++Seed)
	{
		FPHGeneratedLayout Layout;
		FPHBlockoutPlan Plan;
		if (!BuildFor(Seed, Layout, Plan))
		{
			AddError(FString::Printf(TEXT("Seed %d produced no plan."), Seed));
			return false;
		}

		TArray<FPHPropRule> Rules = MakeRules();
		for (FPHPropRule& Rule : Rules)
		{
			Rule.MinSpacingTiles = Spacing;
			Rule.MaxTotal = 0;
		}

		FPHDecorationPlan Decoration;
		TArray<FPHGenerationIssue> Issues;
		if (!UPHDecorationPlanLibrary::BuildDecorationPlan(Layout, Plan, Rules,
			URunSeedFunctionLibrary::DeriveSeed(Layout.Seed, FName(TEXT("Decoration")), 0),
			Decoration, Issues))
		{
			AddError(FString::Printf(TEXT("Seed %d: decoration planning failed."), Seed));
			return false;
		}

		TArray<FIntPoint> Tiles;
		for (const FPHPiecePlacement& Prop : Decoration.Placements)
		{
			Tiles.Add(ToTile(Prop.Transform.GetLocation()));
		}

		for (int32 Index = 0; Index < Tiles.Num(); ++Index)
		{
			for (int32 Other = Index + 1; Other < Tiles.Num(); ++Other)
			{
				const int32 Distance = FMath::Max(
					FMath::Abs(Tiles[Index].X - Tiles[Other].X),
					FMath::Abs(Tiles[Index].Y - Tiles[Other].Y));
				if (Distance < Spacing)
				{
					AddError(FString::Printf(
						TEXT("Seed %d: props at %s and %s are %d tiles apart, under the %d asked for."),
						Seed, *Tiles[Index].ToString(), *Tiles[Other].ToString(), Distance, Spacing));
					return false;
				}
			}
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHDecorationMixedSpacingTest,
	"ProjectHunter.Generation.Decoration.PreservesEarlierPropClearance", PHDecorationPlanTests::TestFlags)

bool FPHDecorationMixedSpacingTest::RunTest(const FString&)
{
	using namespace PHDecorationPlanTests;

	FPHGeneratedLayout Layout;
	Layout.Bounds = FBox(FVector::ZeroVector, FVector(2000.0, 2000.0, 400.0));
	FPHGeneratedRegion& Region = Layout.Regions.AddDefaulted_GetRef();
	Region.RegionID = 0;
	Region.Bounds = Layout.Bounds;

	FPHBlockoutPlan Blockout;
	Blockout.TileSize = TileSize;
	for (int32 X = 0; X < 5; ++X)
	{
		for (int32 Y = 0; Y < 5; ++Y)
		{
			Blockout.FloorTiles.Add(FIntPoint(X, Y));
		}
	}
	Blockout.PlayerStart = FTransform(FVector(200.0, 200.0, 0.0));
	Blockout.Exit = FTransform(FVector(1800.0, 1800.0, 0.0));

	FPHPropRule Furniture;
	Furniture.PropTag = PHGenerationTags::Prop_Furniture.GetTag();
	Furniture.Placement = EPHPropPlacement::Anywhere;
	Furniture.ChancePerTile = 1.0f;
	Furniture.MaxTotal = 1;
	Furniture.MinSpacingTiles = 2;
	Furniture.ClustersPerRegion = 0;
	Furniture.CornerBias = 0.0f;
	Furniture.RegionDensityJitter = 0.0f;
	Furniture.YawJitter = 0.0f;

	FPHPropRule Debris = Furniture;
	Debris.PropTag = PHGenerationTags::Prop_Debris.GetTag();
	Debris.MaxTotal = 0;
	Debris.MinSpacingTiles = 0;

	FPHDecorationPlan Plan;
	TArray<FPHGenerationIssue> Issues;
	if (!TestTrue(TEXT("Mixed-spacing rules produce a plan"),
		UPHDecorationPlanLibrary::BuildDecorationPlan(Layout, Blockout, { Furniture, Debris }, 1, Plan, Issues)))
	{
		return false;
	}

	const FPHPiecePlacement* PlacedFurniture = Plan.Placements.FindByPredicate(
		[&Furniture](const FPHPiecePlacement& Placement) { return Placement.PieceTag == Furniture.PropTag; });
	if (!TestNotNull(TEXT("The fixture actually places furniture"), PlacedFurniture))
	{
		return false;
	}

	const FIntPoint FurnitureTile = ToTile(PlacedFurniture->Transform.GetLocation());
	TestEqual(TEXT("Furniture retains its requesting rule"), PlacedFurniture->SourceRuleIndex, 0);
	int32 DebrisCount = 0;
	for (const FPHPiecePlacement& Placement : Plan.Placements)
	{
		if (Placement.PieceTag != Debris.PropTag)
		{
			continue;
		}
		++DebrisCount;
		TestEqual(TEXT("Debris retains its requesting rule"), Placement.SourceRuleIndex, 1);
		const FIntPoint Tile = ToTile(Placement.Transform.GetLocation());
		const int32 Distance = FMath::Max(FMath::Abs(Tile.X - FurnitureTile.X), FMath::Abs(Tile.Y - FurnitureTile.Y));
		TestTrue(FString::Printf(TEXT("Debris at %s respects furniture clearance at %s"),
			*Tile.ToString(), *FurnitureTile.ToString()), Distance > Furniture.MinSpacingTiles);
	}
	TestTrue(TEXT("Clearance still allows debris elsewhere in the room"), DebrisCount > 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHDecorationScalarValidationTest,
	"ProjectHunter.Generation.Decoration.RejectsInvalidScalarAuthoring", PHDecorationPlanTests::TestFlags)

bool FPHDecorationScalarValidationTest::RunTest(const FString&)
{
	using namespace PHDecorationPlanTests;
	FPHGeneratedLayout Layout;
	FPHBlockoutPlan Blockout;
	if (!TestTrue(TEXT("A normal floor is available"), BuildFor(1, Layout, Blockout)))
	{
		return false;
	}
	FPHBlockoutPlan EmptyBlockout = Blockout;
	EmptyBlockout.FloorTiles.Reset();

	auto ExpectRejected = [&](const TCHAR* Label, const TFunction<void(FPHPropRule&)>& Mutate)
	{
		FPHPropRule Rule = MakeRules()[0];
		Mutate(Rule);
		FPHDecorationPlan Plan;
		Plan.Placements.AddDefaulted();
		TArray<FPHGenerationIssue> Issues;
		// Validate authoring even on an empty plan, without sending invalid values into transforms.
		TestFalse(Label, UPHDecorationPlanLibrary::BuildDecorationPlan(Layout, EmptyBlockout, { Rule }, 7, Plan, Issues));
		TestTrue(FString::Printf(TEXT("%s reports InvalidRequest"), Label), Issues.ContainsByPredicate(
			[](const FPHGenerationIssue& Issue) { return Issue.Code == EPHGenerationIssueCode::InvalidRequest; }));
		TestEqual(FString::Printf(TEXT("%s leaves no partial placements"), Label), Plan.Placements.Num(), 0);
	};

	ExpectRejected(TEXT("NaN chance"), [](FPHPropRule& Rule) { Rule.ChancePerTile = std::numeric_limits<float>::quiet_NaN(); });
	ExpectRejected(TEXT("Chance over one"), [](FPHPropRule& Rule) { Rule.ChancePerTile = 1.01f; });
	ExpectRejected(TEXT("Negative chance"), [](FPHPropRule& Rule) { Rule.ChancePerTile = -0.01f; });
	ExpectRejected(TEXT("Infinite yaw"), [](FPHPropRule& Rule) { Rule.YawJitter = std::numeric_limits<float>::infinity(); });
	ExpectRejected(TEXT("Negative yaw"), [](FPHPropRule& Rule) { Rule.YawJitter = -1.0f; });
	ExpectRejected(TEXT("Yaw over the authored range"), [](FPHPropRule& Rule) { Rule.YawJitter = 181.0f; });
	ExpectRejected(TEXT("Negative cluster count"), [](FPHPropRule& Rule) { Rule.ClustersPerRegion = -1; });
	ExpectRejected(TEXT("Zero cluster radius"), [](FPHPropRule& Rule) { Rule.ClusterRadiusTiles = 0; });
	ExpectRejected(TEXT("Negative spacing"), [](FPHPropRule& Rule) { Rule.MinSpacingTiles = -1; });
	ExpectRejected(TEXT("NaN corner bias"), [](FPHPropRule& Rule) { Rule.CornerBias = std::numeric_limits<float>::quiet_NaN(); });
	ExpectRejected(TEXT("Negative corner bias"), [](FPHPropRule& Rule) { Rule.CornerBias = -0.1f; });
	ExpectRejected(TEXT("Corner bias over the authored range"), [](FPHPropRule& Rule) { Rule.CornerBias = 4.1f; });
	ExpectRejected(TEXT("Infinite density jitter"), [](FPHPropRule& Rule) { Rule.RegionDensityJitter = std::numeric_limits<float>::infinity(); });
	ExpectRejected(TEXT("Density jitter over one"), [](FPHPropRule& Rule) { Rule.RegionDensityJitter = 1.1f; });
	ExpectRejected(TEXT("Negative density jitter"), [](FPHPropRule& Rule) { Rule.RegionDensityJitter = -0.1f; });
	ExpectRejected(TEXT("Unknown placement mode"), [](FPHPropRule& Rule) { Rule.Placement = static_cast<EPHPropPlacement>(255); });

	// An empty rule list still cannot turn an unusable construction scale into a valid plan.
	for (const double InvalidTileSize : { std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::infinity() })
	{
		FPHBlockoutPlan InvalidBlockout = Blockout;
		InvalidBlockout.TileSize = InvalidTileSize;
		FPHDecorationPlan Plan;
		TArray<FPHGenerationIssue> Issues;
		TestFalse(TEXT("Nonfinite tile size is refused before tile arithmetic"),
			UPHDecorationPlanLibrary::BuildDecorationPlan(Layout, InvalidBlockout, {}, 7, Plan, Issues));
		TestTrue(TEXT("Invalid tile size reports an issue"), !Issues.IsEmpty());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHDecorationLargeSpacingTest,
	"ProjectHunter.Generation.Decoration.HandlesLargeSpacingAndSparseEnvelopes", PHDecorationPlanTests::TestFlags)

bool FPHDecorationLargeSpacingTest::RunTest(const FString&)
{
	using namespace PHDecorationPlanTests;
	FPHGeneratedLayout Layout;
	FPHBlockoutPlan Blockout;
	if (!TestTrue(TEXT("A normal floor is available"), BuildFor(1, Layout, Blockout)))
	{
		return false;
	}

	// The supplied floor remains small even when a region has a very large envelope.
	Layout.Regions.SetNum(1);
	Layout.Regions[0].Bounds = FBox(FVector::ZeroVector, FVector(4000000.0, 4000000.0, 400.0));
	Layout.Bounds = Layout.Regions[0].Bounds;
	FPHPropRule Rule = MakeRules()[0];
	Rule.Placement = EPHPropPlacement::Anywhere;
	Rule.ChancePerTile = 1.0f;
	Rule.ClustersPerRegion = 1;
	Rule.ClusterRadiusTiles = MAX_int32;
	Rule.MinSpacingTiles = MAX_int32;
	Rule.CornerBias = 0.0f;
	Rule.RegionDensityJitter = 0.0f;
	Rule.YawJitter = 0.0f;

	FPHDecorationPlan Plan;
	TArray<FPHGenerationIssue> Issues;
	TestTrue(TEXT("Large radius and sparse bounds do not require rasterizing empty coordinates"),
		UPHDecorationPlanLibrary::BuildDecorationPlan(Layout, Blockout, { Rule }, 1, Plan, Issues));
	TestEqual(TEXT("The first prop reserves its large clearance without allocating that area"), Plan.PropCount, 1);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
