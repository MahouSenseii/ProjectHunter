#include "CoreMinimal.h"
#include "Generation/Generators/PHDungeonGenerator.h"
#include "Generation/Library/FunctionLibraries/PHGenerationValidationLibrary.h"
#include "Generation/PHGenerationTags.h"
#include "Misc/AutomationTest.h"
#include "Tower/Library/FunctionLibraries/RunSeedFunctionLibrary.h"
#include "UObject/StrongObjectPtr.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace PHDungeonGeneratorTests
{
	constexpr EAutomationTestFlags TestFlags =
		EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter;

	/** Seeds swept by the bulk tests. Generation is deterministic, so a pass is reproducible. */
	constexpr int32 SweepSeeds = 500;

	FPHLayoutRequest MakeRequest(const int32 Seed)
	{
		FPHLayoutRequest Request;
		Request.Seed = Seed;
		return Request;
	}

	/** A real tag that is deliberately not under Anchor, for rejection cases. */
	UE_DEFINE_GAMEPLAY_TAG_STATIC(NonAnchorTag, "Test.Generation.DungeonRuleNotAnAnchor");

	/** Rooms and links only, so anchor changes can be compared against fixed geometry. */
	FString GeometryFingerprint(const FPHGeneratedLayout& Layout)
	{
		FString Text = FString::Printf(TEXT("B%s"), *Layout.Bounds.ToString());
		for (const FPHGeneratedRegion& Region : Layout.Regions)
		{
			Text += FString::Printf(TEXT("|R%d %s"), Region.RegionID, *Region.Bounds.ToString());
		}
		for (const FPHGeneratedConnection& Connection : Layout.Connections)
		{
			Text += FString::Printf(TEXT("|C%d %d>%d"),
				Connection.ConnectionID, Connection.FromRegionID, Connection.ToRegionID);
		}
		return Text;
	}

	/** Order-sensitive fingerprint of everything a caller can observe. */
	FString Fingerprint(const FPHGeneratedLayout& Layout)
	{
		FString Text = FString::Printf(TEXT("S%d V%d B%s"),
			Layout.Seed, Layout.GenerationVersion, *Layout.Bounds.ToString());
		for (const FPHGeneratedRegion& Region : Layout.Regions)
		{
			Text += FString::Printf(TEXT("|R%d %s"), Region.RegionID, *Region.Bounds.ToString());
		}
		for (const FPHGeneratedConnection& Connection : Layout.Connections)
		{
			Text += FString::Printf(TEXT("|C%d %d>%d"),
				Connection.ConnectionID, Connection.FromRegionID, Connection.ToRegionID);
		}
		for (const FPHGeneratedAnchor& Anchor : Layout.Anchors)
		{
			Text += FString::Printf(TEXT("|A%d R%d %s %s"), Anchor.AnchorID, Anchor.RegionID,
				*Anchor.Transform.GetLocation().ToString(), *Anchor.SemanticTag.ToString());
		}
		return Text;
	}

	UPHDungeonGenerator* MakeGenerator(TStrongObjectPtr<UPHDungeonGenerator>& Keeper)
	{
		Keeper.Reset(NewObject<UPHDungeonGenerator>());
		return Keeper.Get();
	}

	bool HasCode(const TArray<FPHGenerationIssue>& Issues, const EPHGenerationIssueCode Code)
	{
		return Issues.ContainsByPredicate(
			[Code](const FPHGenerationIssue& Issue) { return Issue.Code == Code; });
	}

	/** Anchors are addressed by ID, never by array position. */
	const FPHGeneratedAnchor* FindAnchor(const FPHGeneratedLayout& Layout, const int32 AnchorID)
	{
		return Layout.Anchors.FindByPredicate(
			[AnchorID](const FPHGeneratedAnchor& Anchor) { return Anchor.AnchorID == AnchorID; });
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHDungeonValidAcrossSeedsTest,
	"ProjectHunter.Generation.Dungeon.ValidAcrossSeeds", PHDungeonGeneratorTests::TestFlags)

bool FPHDungeonValidAcrossSeedsTest::RunTest(const FString&)
{
	using namespace PHDungeonGeneratorTests;
	TStrongObjectPtr<UPHDungeonGenerator> Keeper;
	UPHDungeonGenerator* Generator = MakeGenerator(Keeper);

	for (int32 Seed = 1; Seed <= SweepSeeds; ++Seed)
	{
		FPHGeneratedLayout Layout;
		TArray<FPHGenerationIssue> Issues;
		if (!Generator->GenerateLayout(MakeRequest(Seed), Layout, Issues))
		{
			const FString Reason = Issues.IsEmpty() ? TEXT("no issue reported") : Issues[0].Message;
			AddError(FString::Printf(TEXT("Seed %d produced no layout: %s"), Seed, *Reason));
			return false;
		}

		// The base class already gated on this; re-running proves the returned copy is the gated one.
		TArray<FPHGenerationIssue> Recheck;
		if (!UPHGenerationValidationLibrary::ValidateLayout(Layout, Recheck))
		{
			AddError(FString::Printf(TEXT("Seed %d returned a layout that fails validation: %s"),
				Seed, Recheck.IsEmpty() ? TEXT("unknown") : *Recheck[0].Message));
			return false;
		}

		if (Layout.Seed != Seed || Layout.GenerationVersion != Generator->GetGenerationVersion())
		{
			AddError(FString::Printf(TEXT("Seed %d lost its provenance stamp."), Seed));
			return false;
		}

		const FPHGeneratedAnchor* Start = FindAnchor(Layout, Layout.PlayerStartAnchorID);
		const FPHGeneratedAnchor* Exit = FindAnchor(Layout, Layout.ExitAnchorID);
		if (!Start || !Exit
			|| Start->SemanticTag != PHGenerationTags::Anchor_PlayerStart.GetTag()
			|| Exit->SemanticTag != PHGenerationTags::Anchor_Exit.GetTag())
		{
			AddError(FString::Printf(TEXT("Seed %d produced the wrong endpoint anchors."), Seed));
			return false;
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHDungeonDeterminismTest,
	"ProjectHunter.Generation.Dungeon.SameSeedSameLayout", PHDungeonGeneratorTests::TestFlags)

bool FPHDungeonDeterminismTest::RunTest(const FString&)
{
	using namespace PHDungeonGeneratorTests;
	TStrongObjectPtr<UPHDungeonGenerator> KeeperA;
	TStrongObjectPtr<UPHDungeonGenerator> KeeperB;
	UPHDungeonGenerator* First = MakeGenerator(KeeperA);
	UPHDungeonGenerator* Second = MakeGenerator(KeeperB);

	TSet<FString> DistinctLayouts;
	for (int32 Seed = 1; Seed <= SweepSeeds; ++Seed)
	{
		FPHGeneratedLayout LayoutA;
		FPHGeneratedLayout LayoutB;
		TArray<FPHGenerationIssue> Issues;
		if (!First->GenerateLayout(MakeRequest(Seed), LayoutA, Issues))
		{
			AddError(FString::Printf(TEXT("Seed %d produced no layout."), Seed));
			return false;
		}

		// A second instance proves the result depends on the seed, not on generator state.
		if (!Second->GenerateLayout(MakeRequest(Seed), LayoutB, Issues))
		{
			AddError(FString::Printf(TEXT("Seed %d failed on the second generator instance."), Seed));
			return false;
		}

		const FString SignatureA = Fingerprint(LayoutA);
		if (SignatureA != Fingerprint(LayoutB))
		{
			AddError(FString::Printf(TEXT("Seed %d produced two different layouts."), Seed));
			return false;
		}
		// The provenance contains the seed itself, which would make even constant geometry
		// appear distinct. Diversity must compare rooms and links independently of that stamp.
		DistinctLayouts.Add(GeometryFingerprint(LayoutA));
	}

	// Determinism must not collapse into one layout for every seed.
	if (DistinctLayouts.Num() < SweepSeeds / 2)
	{
		AddError(FString::Printf(TEXT("Only %d distinct layouts across %d seeds."),
			DistinctLayouts.Num(), SweepSeeds));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHDungeonSettingsReplayTest,
	"ProjectHunter.Generation.Dungeon.SettingsReplayIdentically", PHDungeonGeneratorTests::TestFlags)

bool FPHDungeonSettingsReplayTest::RunTest(const FString&)
{
	using namespace PHDungeonGeneratorTests;
	TStrongObjectPtr<UPHDungeonGenerator> Keeper;
	UPHDungeonGenerator* Generator = MakeGenerator(Keeper);

	// Settings, not only the seed, must take part in the replay guarantee.
	FPHLayoutRequest Tight = MakeRequest(99);
	Tight.AreaSize = FVector2D(12000.0, 7000.0);
	Tight.MinRegionCount = 4;
	Tight.MaxRegionCount = 7;
	Tight.MinRegionSize = FVector2D(500.0, 900.0);
	Tight.MaxRegionSize = FVector2D(1100.0, 1300.0);
	Tight.RegionHeight = 650.0;
	Tight.RegionSpacing = 350.0;
	Tight.LoopChance = 0.4f;

	FPHGeneratedLayout FirstPass;
	FPHGeneratedLayout SecondPass;
	TArray<FPHGenerationIssue> Issues;
	if (!Generator->GenerateLayout(Tight, FirstPass, Issues)
		|| !Generator->GenerateLayout(Tight, SecondPass, Issues))
	{
		AddError(TEXT("A buildable non-default request should produce a layout."));
		return false;
	}

	TestEqual(TEXT("Identical settings replay identically"),
		Fingerprint(FirstPass), Fingerprint(SecondPass));

	// A setting that feeds the stream must change the output, or it is not being consumed.
	// Spacing deliberately is not such a setting: it only rejects candidates, so a small
	// change to it leaves a sparse layout untouched.
	FPHLayoutRequest Altered = Tight;
	Altered.MaxRegionSize = FVector2D(1400.0, 1600.0);
	FPHGeneratedLayout AlteredLayout;
	if (!Generator->GenerateLayout(Altered, AlteredLayout, Issues))
	{
		AddError(TEXT("The altered request should still be buildable."));
		return false;
	}
	TestNotEqual(TEXT("A different size range yields a different layout"),
		Fingerprint(AlteredLayout), Fingerprint(FirstPass));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHDungeonRespectsRequestTest,
	"ProjectHunter.Generation.Dungeon.RespectsRequest", PHDungeonGeneratorTests::TestFlags)

bool FPHDungeonRespectsRequestTest::RunTest(const FString&)
{
	using namespace PHDungeonGeneratorTests;
	TStrongObjectPtr<UPHDungeonGenerator> Keeper;
	UPHDungeonGenerator* Generator = MakeGenerator(Keeper);

	for (int32 Seed = 1; Seed <= SweepSeeds; ++Seed)
	{
		const FPHLayoutRequest Request = MakeRequest(Seed);
		FPHGeneratedLayout Layout;
		TArray<FPHGenerationIssue> Issues;
		if (!Generator->GenerateLayout(Request, Layout, Issues))
		{
			AddError(FString::Printf(TEXT("Seed %d produced no layout."), Seed));
			return false;
		}

		// Honouring the count is now unconditional: too dense a request is refused, never downgraded.
		if (Layout.Regions.Num() < Request.MinRegionCount || Layout.Regions.Num() > Request.MaxRegionCount)
		{
			AddError(FString::Printf(TEXT("Seed %d produced %d regions, outside the requested %d-%d."),
				Seed, Layout.Regions.Num(), Request.MinRegionCount, Request.MaxRegionCount));
			return false;
		}

		const FBox Area(FVector::ZeroVector,
			FVector(Request.AreaSize.X, Request.AreaSize.Y, Request.RegionHeight));
		for (int32 Index = 0; Index < Layout.Regions.Num(); ++Index)
		{
			if (!Area.IsInsideOrOn(Layout.Regions[Index].Bounds))
			{
				AddError(FString::Printf(TEXT("Seed %d placed region %d outside the requested area."),
					Seed, Layout.Regions[Index].RegionID));
				return false;
			}

			for (int32 Other = Index + 1; Other < Layout.Regions.Num(); ++Other)
			{
				if (Layout.Regions[Index].Bounds.Intersect(Layout.Regions[Other].Bounds))
				{
					AddError(FString::Printf(TEXT("Seed %d overlapped regions %d and %d."),
						Seed, Layout.Regions[Index].RegionID, Layout.Regions[Other].RegionID));
					return false;
				}
			}
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHDungeonRefusesImpossibleRequestTest,
	"ProjectHunter.Generation.Dungeon.RefusesImpossibleRequests", PHDungeonGeneratorTests::TestFlags)

bool FPHDungeonRefusesImpossibleRequestTest::RunTest(const FString&)
{
	using namespace PHDungeonGeneratorTests;
	TStrongObjectPtr<UPHDungeonGenerator> Keeper;
	UPHDungeonGenerator* Generator = MakeGenerator(Keeper);

	// Each entry breaks exactly one field, so a refusal cannot be credited to the wrong cause.
	TArray<TPair<FString, TFunction<void(FPHLayoutRequest&)>>> Impossible;
	Impossible.Emplace(TEXT("zero area"),
		[](FPHLayoutRequest& R) { R.AreaSize = FVector2D(0.0, 5000.0); });
	Impossible.Emplace(TEXT("negative area"),
		[](FPHLayoutRequest& R) { R.AreaSize = FVector2D(5000.0, -1.0); });
	Impossible.Emplace(TEXT("non-positive region height"),
		[](FPHLayoutRequest& R) { R.RegionHeight = 0.0; });
	Impossible.Emplace(TEXT("negative spacing"),
		[](FPHLayoutRequest& R) { R.RegionSpacing = -1.0; });
	Impossible.Emplace(TEXT("zero region count"),
		[](FPHLayoutRequest& R) { R.MinRegionCount = 0; });
	Impossible.Emplace(TEXT("inverted count range"),
		[](FPHLayoutRequest& R) { R.MinRegionCount = 9; R.MaxRegionCount = 2; });
	Impossible.Emplace(TEXT("non-positive region size"),
		[](FPHLayoutRequest& R) { R.MinRegionSize = FVector2D(0.0, 800.0); });
	Impossible.Emplace(TEXT("inverted size range"),
		[](FPHLayoutRequest& R) { R.MaxRegionSize = FVector2D(10.0, 10.0); });
	Impossible.Emplace(TEXT("region larger than area"),
		[](FPHLayoutRequest& R) { R.AreaSize = FVector2D(500.0, 500.0);
			R.MinRegionSize = FVector2D(900.0, 900.0); R.MaxRegionSize = FVector2D(1200.0, 1200.0); });
	Impossible.Emplace(TEXT("out-of-range loop chance"),
		[](FPHLayoutRequest& R) { R.LoopChance = 5.0f; });
	Impossible.Emplace(TEXT("zero placement attempts"),
		[](FPHLayoutRequest& R) { R.MaxPlacementAttempts = 0; });

	for (const TPair<FString, TFunction<void(FPHLayoutRequest&)>>& Case : Impossible)
	{
		FPHLayoutRequest Request = MakeRequest(7);
		Case.Value(Request);

		FPHGeneratedLayout Layout;
		TArray<FPHGenerationIssue> Issues;
		if (Generator->GenerateLayout(Request, Layout, Issues))
		{
			AddError(FString::Printf(TEXT("Request with %s should have been refused."), *Case.Key));
			continue;
		}

		TestTrue(FString::Printf(TEXT("%s reports InvalidRequest"), *Case.Key),
			HasCode(Issues, EPHGenerationIssueCode::InvalidRequest));
		TestEqual(FString::Printf(TEXT("%s leaves no partial layout"), *Case.Key),
			Layout.Regions.Num(), 0);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHDungeonRefusesOvercrowdedRequestTest,
	"ProjectHunter.Generation.Dungeon.RefusesOvercrowdedRequest", PHDungeonGeneratorTests::TestFlags)

bool FPHDungeonRefusesOvercrowdedRequestTest::RunTest(const FString&)
{
	using namespace PHDungeonGeneratorTests;
	TStrongObjectPtr<UPHDungeonGenerator> Keeper;
	UPHDungeonGenerator* Generator = MakeGenerator(Keeper);

	// Individually legal, collectively unbuildable: twelve near-area rooms cannot coexist.
	FPHLayoutRequest Crowded = MakeRequest(11);
	Crowded.AreaSize = FVector2D(2000.0, 2000.0);
	Crowded.MinRegionCount = 12;
	Crowded.MaxRegionCount = 12;
	Crowded.MinRegionSize = FVector2D(1800.0, 1800.0);
	Crowded.MaxRegionSize = FVector2D(1900.0, 1900.0);

	FPHGeneratedLayout Layout;
	TArray<FPHGenerationIssue> Issues;
	TestFalse(TEXT("An overcrowded request is refused"),
		Generator->GenerateLayout(Crowded, Layout, Issues));
	TestTrue(TEXT("Refusal names the placement failure"),
		HasCode(Issues, EPHGenerationIssueCode::UnplaceableRegions));
	TestEqual(TEXT("No partial layout is returned"), Layout.Regions.Num(), 0);

	// The same shape with room to breathe must still succeed, proving the refusal is
	// about density rather than the settings themselves.
	FPHLayoutRequest Roomy = Crowded;
	Roomy.AreaSize = FVector2D(30000.0, 30000.0);
	TestTrue(TEXT("The same request in a larger area succeeds"),
		Generator->GenerateLayout(Roomy, Layout, Issues));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHDungeonAnchorRuleTest,
	"ProjectHunter.Generation.Dungeon.SeatsAnchorRules", PHDungeonGeneratorTests::TestFlags)

bool FPHDungeonAnchorRuleTest::RunTest(const FString&)
{
	using namespace PHDungeonGeneratorTests;
	TStrongObjectPtr<UPHDungeonGenerator> Keeper;
	UPHDungeonGenerator* Generator = MakeGenerator(Keeper);

	FPHAnchorRule Enemies;
	Enemies.SemanticTag = PHGenerationTags::Anchor_Enemy_Small.GetTag();
	Enemies.MinPerRegion = 1;
	Enemies.MaxPerRegion = 3;
	Enemies.bAllowInStartRegion = false;

	FPHAnchorRule Chests;
	Chests.SemanticTag = PHGenerationTags::Anchor_Chest.GetTag();
	Chests.MinPerRegion = 0;
	Chests.MaxPerRegion = 1;
	Chests.MaxTotal = 2;

	for (int32 Seed = 1; Seed <= 200; ++Seed)
	{
		FPHLayoutRequest Request = MakeRequest(Seed);
		Request.AnchorRules = { Enemies, Chests };

		FPHGeneratedLayout Layout;
		TArray<FPHGenerationIssue> Issues;
		if (!Generator->GenerateLayout(Request, Layout, Issues))
		{
			AddError(FString::Printf(TEXT("Seed %d with anchor rules produced no layout."), Seed));
			return false;
		}

		TSet<int32> SeenIDs;
		TSet<FVector> SeenLocations;
		int32 ChestCount = 0;
		for (const FPHGeneratedAnchor& Anchor : Layout.Anchors)
		{
			if (SeenIDs.Contains(Anchor.AnchorID))
			{
				AddError(FString::Printf(TEXT("Seed %d reused anchor ID %d."), Seed, Anchor.AnchorID));
				return false;
			}
			SeenIDs.Add(Anchor.AnchorID);

			const FPHGeneratedRegion* Region = Layout.Regions.FindByPredicate(
				[&Anchor](const FPHGeneratedRegion& R) { return R.RegionID == Anchor.RegionID; });
			if (!Region || !Region->Bounds.IsInsideOrOn(Anchor.Transform.GetLocation()))
			{
				AddError(FString::Printf(TEXT("Seed %d anchor %d escapes its region."),
					Seed, Anchor.AnchorID));
				return false;
			}

			if (Anchor.SemanticTag == PHGenerationTags::Anchor_Chest.GetTag())
			{
				++ChestCount;
			}

			// Endpoints share a room with rule anchors, so only rule anchors claim unique tiles.
			if (Anchor.AnchorID >= 2 && SeenLocations.Contains(Anchor.Transform.GetLocation()))
			{
				AddError(FString::Printf(TEXT("Seed %d seated two anchors on one tile."), Seed));
				return false;
			}
			SeenLocations.Add(Anchor.Transform.GetLocation());

			// The enemy rule is barred from the start region.
			if (Anchor.SemanticTag == PHGenerationTags::Anchor_Enemy_Small.GetTag()
				&& Anchor.RegionID == 0)
			{
				AddError(FString::Printf(TEXT("Seed %d seated an enemy in the start region."), Seed));
				return false;
			}
		}

		if (ChestCount > Chests.MaxTotal)
		{
			AddError(FString::Printf(TEXT("Seed %d placed %d chests, over the cap of %d."),
				Seed, ChestCount, Chests.MaxTotal));
			return false;
		}

		if (Layout.Anchors.Num() <= 2)
		{
			AddError(FString::Printf(TEXT("Seed %d seated no rule anchors at all."), Seed));
			return false;
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHDungeonAnchorsPreserveGeometryTest,
	"ProjectHunter.Generation.Dungeon.AnchorRulesDoNotMoveGeometry", PHDungeonGeneratorTests::TestFlags)

bool FPHDungeonAnchorsPreserveGeometryTest::RunTest(const FString&)
{
	using namespace PHDungeonGeneratorTests;
	TStrongObjectPtr<UPHDungeonGenerator> Keeper;
	UPHDungeonGenerator* Generator = MakeGenerator(Keeper);

	FPHAnchorRule Traps;
	Traps.SemanticTag = PHGenerationTags::Anchor_Trap.GetTag();
	Traps.MinPerRegion = 1;
	Traps.MaxPerRegion = 2;

	// Anchor draws happen after geometry, so authoring rules must not move a single room.
	// Without this, adding content to a floor would silently reshape it.
	for (int32 Seed = 1; Seed <= 200; ++Seed)
	{
		FPHLayoutRequest Bare = MakeRequest(Seed);
		FPHLayoutRequest Ruled = MakeRequest(Seed);
		Ruled.AnchorRules = { Traps };

		FPHGeneratedLayout BareLayout;
		FPHGeneratedLayout RuledLayout;
		TArray<FPHGenerationIssue> Issues;
		Generator->GenerateLayout(Bare, BareLayout, Issues);
		Generator->GenerateLayout(Ruled, RuledLayout, Issues);

		if (GeometryFingerprint(BareLayout) != GeometryFingerprint(RuledLayout))
		{
			AddError(FString::Printf(TEXT("Seed %d changed geometry when anchor rules were added."),
				Seed));
			return false;
		}

		if (RuledLayout.Anchors.Num() <= BareLayout.Anchors.Num())
		{
			AddError(FString::Printf(TEXT("Seed %d gained no anchors from its rule."), Seed));
			return false;
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHDungeonRejectsBadAnchorRuleTest,
	"ProjectHunter.Generation.Dungeon.RefusesInvalidAnchorRules", PHDungeonGeneratorTests::TestFlags)

bool FPHDungeonRejectsBadAnchorRuleTest::RunTest(const FString&)
{
	using namespace PHDungeonGeneratorTests;
	TStrongObjectPtr<UPHDungeonGenerator> Keeper;
	UPHDungeonGenerator* Generator = MakeGenerator(Keeper);

	auto ExpectRefused = [this, Generator](const TCHAR* Label, const FPHAnchorRule& Rule)
	{
		FPHLayoutRequest Request = MakeRequest(5);
		Request.AnchorRules = { Rule };
		FPHGeneratedLayout Layout;
		TArray<FPHGenerationIssue> Issues;
		TestFalse(FString::Printf(TEXT("%s is refused"), Label),
			Generator->GenerateLayout(Request, Layout, Issues));
		TestTrue(FString::Printf(TEXT("%s reports InvalidRequest"), Label),
			HasCode(Issues, EPHGenerationIssueCode::InvalidRequest));
	};

	FPHAnchorRule EmptyTag;
	ExpectRefused(TEXT("Rule with no tag"), EmptyTag);

	FPHAnchorRule RootTag;
	RootTag.SemanticTag = PHGenerationTags::Anchor.GetTag();
	ExpectRefused(TEXT("Rule using the Anchor root"), RootTag);

	FPHAnchorRule NonAnchor;
	NonAnchor.SemanticTag = NonAnchorTag;
	ExpectRefused(TEXT("Rule with a tag outside the Anchor hierarchy"), NonAnchor);

	FPHAnchorRule Inverted;
	Inverted.SemanticTag = PHGenerationTags::Anchor_Chest.GetTag();
	Inverted.MinPerRegion = 4;
	Inverted.MaxPerRegion = 1;
	ExpectRefused(TEXT("Rule with an inverted count range"), Inverted);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHDungeonModuleGridTest,
	"ProjectHunter.Generation.Dungeon.RoomsLandOnModuleGrid", PHDungeonGeneratorTests::TestFlags)

bool FPHDungeonModuleGridTest::RunTest(const FString&)
{
	using namespace PHDungeonGeneratorTests;
	TStrongObjectPtr<UPHDungeonGenerator> Keeper;
	UPHDungeonGenerator* Generator = MakeGenerator(Keeper);

	// 100 matches the BlockingStarterPack architecture module; 200 and 400 prove the rule is
	// driven by the request rather than by a constant baked into the strategy.
	const TArray<double> Grids = { 100.0, 200.0, 400.0 };
	for (const double Grid : Grids)
	{
		for (int32 Seed = 1; Seed <= 150; ++Seed)
		{
			FPHLayoutRequest Request = MakeRequest(Seed);
			Request.GridSize = Grid;

			FPHGeneratedLayout Layout;
			TArray<FPHGenerationIssue> Issues;
			if (!Generator->GenerateLayout(Request, Layout, Issues))
			{
				AddError(FString::Printf(TEXT("Grid %f seed %d produced no layout."), Grid, Seed));
				return false;
			}

			for (const FPHGeneratedRegion& Region : Layout.Regions)
			{
				const FVector Corners[] = { Region.Bounds.Min, Region.Bounds.Max };
				for (const FVector& Corner : Corners)
				{
					const bool bAligned =
						FMath::IsNearlyZero(FMath::Fmod(Corner.X, Grid), UE_KINDA_SMALL_NUMBER)
						&& FMath::IsNearlyZero(FMath::Fmod(Corner.Y, Grid), UE_KINDA_SMALL_NUMBER)
						&& FMath::IsNearlyZero(FMath::Fmod(Corner.Z, Grid), UE_KINDA_SMALL_NUMBER);
					if (!bAligned)
					{
						AddError(FString::Printf(
							TEXT("Grid %f seed %d: region %d corner %s is off the module grid."),
							Grid, Seed, Region.RegionID, *Corner.ToString()));
						return false;
					}
				}

				// A room must also be a whole number of modules across, not merely start on one.
				const FVector Size = Region.Bounds.GetSize();
				if (!FMath::IsNearlyZero(FMath::Fmod(Size.X, Grid), UE_KINDA_SMALL_NUMBER)
					|| !FMath::IsNearlyZero(FMath::Fmod(Size.Y, Grid), UE_KINDA_SMALL_NUMBER))
				{
					AddError(FString::Printf(TEXT("Grid %f seed %d: region %d spans %s."),
						Grid, Seed, Region.RegionID, *Size.ToString()));
					return false;
				}
			}
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHDungeonGridRefusalTest,
	"ProjectHunter.Generation.Dungeon.RefusesUnbuildableGrid", PHDungeonGeneratorTests::TestFlags)

bool FPHDungeonGridRefusalTest::RunTest(const FString&)
{
	using namespace PHDungeonGeneratorTests;
	TStrongObjectPtr<UPHDungeonGenerator> Keeper;
	UPHDungeonGenerator* Generator = MakeGenerator(Keeper);

	auto ExpectRefused = [this, Generator](const TCHAR* Label, const FPHLayoutRequest& Request)
	{
		FPHGeneratedLayout Layout;
		TArray<FPHGenerationIssue> Issues;
		TestFalse(FString::Printf(TEXT("%s is refused"), Label),
			Generator->GenerateLayout(Request, Layout, Issues));
		TestTrue(FString::Printf(TEXT("%s reports InvalidRequest"), Label),
			HasCode(Issues, EPHGenerationIssueCode::InvalidRequest));
	};

	FPHLayoutRequest NoGrid = MakeRequest(3);
	NoGrid.GridSize = 0.0;
	ExpectRefused(TEXT("Zero grid size"), NoGrid);

	// 850..899 is a positive, correctly ordered range in units that still contains no whole
	// 100-unit module count, so it is unbuildable despite looking valid.
	FPHLayoutRequest NoWholeModule = MakeRequest(3);
	NoWholeModule.MinRegionSize = FVector2D(850.0, 800.0);
	NoWholeModule.MaxRegionSize = FVector2D(899.0, 1200.0);
	ExpectRefused(TEXT("Size range spanning no whole module"), NoWholeModule);

	FPHLayoutRequest ShortRoom = MakeRequest(3);
	ShortRoom.RegionHeight = 40.0;
	ExpectRefused(TEXT("Region shorter than one module"), ShortRoom);

	// A grid coarser than the area leaves no module to place.
	FPHLayoutRequest CoarseGrid = MakeRequest(3);
	CoarseGrid.GridSize = 20000.0;
	ExpectRefused(TEXT("Grid coarser than the area"), CoarseGrid);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHDungeonLayoutSeedStreamTest,
	"ProjectHunter.Generation.Dungeon.LayoutStreamIsIndependent", PHDungeonGeneratorTests::TestFlags)

bool FPHDungeonLayoutSeedStreamTest::RunTest(const FString&)
{
	const int32 FloorSeed = URunSeedFunctionLibrary::DeriveFloorSeed(12345, 3);
	const int32 LayoutSeed = URunSeedFunctionLibrary::DeriveLayoutSeed(FloorSeed);

	TestEqual(TEXT("Layout seed is stable for a floor seed"),
		URunSeedFunctionLibrary::DeriveLayoutSeed(FloorSeed), LayoutSeed);

	// Decoration or encounter changes must not shift layout draws, so the streams differ.
	TestNotEqual(TEXT("Layout seed differs from the encounter stream"),
		LayoutSeed, URunSeedFunctionLibrary::DeriveEncounterSeed(FloorSeed, 0));
	TestNotEqual(TEXT("Layout seed differs from the reward stream"),
		LayoutSeed, URunSeedFunctionLibrary::DeriveRewardSeed(FloorSeed));
	TestNotEqual(TEXT("Layout seed differs from its parent floor seed"), LayoutSeed, FloorSeed);
	TestTrue(TEXT("Layout seed stays in the positive range"), LayoutSeed > 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHDungeonRegionCountVariesTest,
	"ProjectHunter.Generation.Dungeon.RegionCountVariesWithinRange", PHDungeonGeneratorTests::TestFlags)

bool FPHDungeonRegionCountVariesTest::RunTest(const FString&)
{
	using namespace PHDungeonGeneratorTests;
	TStrongObjectPtr<UPHDungeonGenerator> Keeper;
	UPHDungeonGenerator* Generator = MakeGenerator(Keeper);

	// A count drawn from a range is only worth having if the range is actually explored. Placement
	// can quietly collapse it: a strategy that drops every region it cannot seat lands on the same
	// achievable number every seed while still reporting a range.
	FPHLayoutRequest Request = MakeRequest(1);
	Request.MinRegionCount = 6;
	Request.MaxRegionCount = 14;

	TSet<int32> ObservedCounts;
	for (int32 Seed = 1; Seed <= 200; ++Seed)
	{
		Request.Seed = Seed;
		FPHGeneratedLayout Layout;
		TArray<FPHGenerationIssue> Issues;
		if (!Generator->GenerateLayout(Request, Layout, Issues))
		{
			AddError(FString::Printf(TEXT("Seed %d produced no layout."), Seed));
			return false;
		}

		if (Layout.Regions.Num() < Request.MinRegionCount
			|| Layout.Regions.Num() > Request.MaxRegionCount)
		{
			AddError(FString::Printf(TEXT("Seed %d produced %d regions, outside the requested %d-%d."),
				Seed, Layout.Regions.Num(), Request.MinRegionCount, Request.MaxRegionCount));
			return false;
		}

		ObservedCounts.Add(Layout.Regions.Num());
	}

	AddInfo(FString::Printf(TEXT("Observed %d distinct region counts across 200 seeds."),
		ObservedCounts.Num()));

	// Over 200 seeds a nine-wide range should show most of its values; anything under half means
	// the draw is being flattened somewhere between the request and the placed regions.
	TestTrue(TEXT("The region count range is actually explored"), ObservedCounts.Num() >= 5);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHDungeonGrowthFrontageTest,
	"ProjectHunter.Generation.Dungeon.GrowthLinksRegionsThatShareFrontage",
	PHDungeonGeneratorTests::TestFlags)

bool FPHDungeonGrowthFrontageTest::RunTest(const FString&)
{
	using namespace PHDungeonGeneratorTests;
	TStrongObjectPtr<UPHDungeonGenerator> Keeper;
	UPHDungeonGenerator* Generator = MakeGenerator(Keeper);

	// The point of growth placement: connected regions overlap on an axis, so the blockout can
	// join them with a short straight corridor instead of a dog-leg across the floor. Scatter has
	// no such relationship, which is what makes its floors read as a cloud of rooms.
	auto SharesFrontage = [](const FBox& A, const FBox& B)
	{
		return (FMath::Min(A.Max.Y, B.Max.Y) > FMath::Max(A.Min.Y, B.Min.Y))
			|| (FMath::Min(A.Max.X, B.Max.X) > FMath::Max(A.Min.X, B.Min.X));
	};

	auto MeasureFrontage = [&](const EPHRegionPlacement Placement)
	{
		int32 Shared = 0;
		int32 Total = 0;
		for (int32 Seed = 1; Seed <= 120; ++Seed)
		{
			FPHLayoutRequest Request = MakeRequest(Seed);
			Request.RegionPlacement = Placement;
			// Loops are excluded: they deliberately join regions that growth never related, so
			// counting them would measure the loop rule rather than the placement rule.
			Request.LoopChance = 0.0f;

			FPHGeneratedLayout Layout;
			TArray<FPHGenerationIssue> Issues;
			if (!Generator->GenerateLayout(Request, Layout, Issues))
			{
				AddError(FString::Printf(TEXT("Seed %d produced no layout."), Seed));
				return 0.0f;
			}

			for (const FPHGeneratedConnection& Connection : Layout.Connections)
			{
				++Total;
				if (SharesFrontage(Layout.Regions[Connection.FromRegionID].Bounds,
					Layout.Regions[Connection.ToRegionID].Bounds))
				{
					++Shared;
				}
			}
		}
		return (Total > 0) ? static_cast<float>(Shared) / static_cast<float>(Total) : 0.0f;
	};

	const float GrowthShare = MeasureFrontage(EPHRegionPlacement::Growth);
	const float ScatterShare = MeasureFrontage(EPHRegionPlacement::Scatter);

	AddInfo(FString::Printf(
		TEXT("Connections joining regions that share frontage: growth %.3f, scatter %.3f."),
		GrowthShare, ScatterShare));

	// Not all of them: a region that finds no growth slot falls back to a free draw and is linked
	// to its nearest neighbour after the fact, which need not share frontage.
	TestTrue(FString::Printf(TEXT("Growth relates most connected regions (got %.3f)"), GrowthShare),
		GrowthShare > 0.75f);
	TestTrue(TEXT("Growth relates more of them than scatter does"), GrowthShare > ScatterShare);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
