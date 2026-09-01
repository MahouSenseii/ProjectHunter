#include "CoreMinimal.h"
#include "Generation/Generators/PHDungeonGenerator.h"
#include "Generation/Generators/PHLayoutGenerator.h"
#include "Generation/PHGenerationTags.h"
#include "Misc/AutomationTest.h"
#include "UObject/StrongObjectPtr.h"

#include <limits>

#if WITH_DEV_AUTOMATION_TESTS

namespace PHGenerationRequestValidationTests
{
	constexpr EAutomationTestFlags TestFlags =
		EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter;

	bool HasInvalidRequest(const TArray<FPHGenerationIssue>& Issues)
	{
		return Issues.ContainsByPredicate([](const FPHGenerationIssue& Issue)
		{
			return Issue.Code == EPHGenerationIssueCode::InvalidRequest;
		});
	}

	bool ExpectRequestRefused(FAutomationTestBase& Test, const FString& Label,
		const FPHLayoutRequest& Request)
	{
		TArray<FPHGenerationIssue> Issues;
		if (!Test.TestFalse(Label + TEXT(" is rejected at the request gate"),
			UPHLayoutGenerator::ValidateRequest(Request, Issues)))
		{
			return false;
		}
		return Test.TestTrue(Label + TEXT(" reports InvalidRequest"), HasInvalidRequest(Issues));
	}

	bool ExpectDungeonRefused(FAutomationTestBase& Test, UPHDungeonGenerator& Generator,
		const FString& Label, const FPHLayoutRequest& Request, const TCHAR* Reason = TEXT(""))
	{
		FPHGeneratedLayout Layout;
		Layout.Regions.AddDefaulted();
		Layout.Connections.AddDefaulted();
		Layout.Anchors.AddDefaulted();
		TArray<FPHGenerationIssue> Issues;
		bool bPassed = Test.TestFalse(Label + TEXT(" is refused without publishing a layout"),
			Generator.GenerateLayout(Request, Layout, Issues));
		bPassed &= Test.TestTrue(Label + TEXT(" reports InvalidRequest"), HasInvalidRequest(Issues));
		bPassed &= Test.TestTrue(Label + TEXT(" clears previous output"),
			Layout.Regions.IsEmpty() && Layout.Connections.IsEmpty() && Layout.Anchors.IsEmpty());
		if (Reason[0] != '\0')
		{
			bPassed &= Test.TestTrue(Label + TEXT(" names the violated constraint"),
				Issues.ContainsByPredicate([Reason](const FPHGenerationIssue& Issue)
				{
					return Issue.Code == EPHGenerationIssueCode::InvalidRequest && Issue.Message.Contains(Reason);
				}));
		}
		return bPassed;
	}

	FPHLayoutRequest MakeSingleRoomAnchorRequest(const double Side)
	{
		FPHLayoutRequest Request;
		Request.Seed = 1;
		Request.GridSize = 1.0;
		Request.AreaSize = FVector2D(Side, Side);
		Request.RegionHeight = 1.0;
		Request.MinRegionSize = Request.AreaSize;
		Request.MaxRegionSize = Request.AreaSize;
		Request.MinRegionCount = 1;
		Request.MaxRegionCount = 1;
		FPHAnchorRule Chest;
		Chest.SemanticTag = PHGenerationTags::Anchor_Chest.GetTag();
		Chest.MinPerRegion = 1;
		Chest.MaxPerRegion = 1;
		Request.AnchorRules.Add(Chest);
		return Request;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHGenerationNonFiniteControlsTest,
	"ProjectHunter.Generation.Dungeon.RejectsNonFiniteControls",
	PHGenerationRequestValidationTests::TestFlags)

bool FPHGenerationNonFiniteControlsTest::RunTest(const FString&)
{
	using namespace PHGenerationRequestValidationTests;
	bool bPassed = true;
	const double NonFiniteValues[] =
	{
		std::numeric_limits<double>::quiet_NaN(),
		std::numeric_limits<double>::infinity(),
		-std::numeric_limits<double>::infinity()
	};
	const TCHAR* ValueNames[] = { TEXT("NaN"), TEXT("positive infinity"), TEXT("negative infinity") };

	for (int32 Index = 0; Index < UE_ARRAY_COUNT(NonFiniteValues); ++Index)
	{
		FPHLayoutRequest Request;
		Request.LoopChance = static_cast<float>(NonFiniteValues[Index]);
		bPassed &= ExpectRequestRefused(*this,
			FString::Printf(TEXT("LoopChance %s"), ValueNames[Index]), Request);

		Request = FPHLayoutRequest();
		Request.RegionSpacing = NonFiniteValues[Index];
		bPassed &= ExpectRequestRefused(*this,
			FString::Printf(TEXT("RegionSpacing %s"), ValueNames[Index]), Request);

		Request = FPHLayoutRequest();
		Request.MaxLoopDistance = NonFiniteValues[Index];
		bPassed &= ExpectRequestRefused(*this,
			FString::Printf(TEXT("MaxLoopDistance %s"), ValueNames[Index]), Request);
	}

	// These fields do not reach float-to-module conversion in ValidateRequest. Non-finite
	// dimensions and allocation-sized requests were excluded from the unsafe baseline; the guarded
	// post-fix cases below cover those paths separately.
	FPHLayoutRequest Allowed;
	Allowed.RegionSpacing = 0.0;
	Allowed.MaxLoopDistance = 0.0;
	Allowed.LoopChance = 0.0f;
	TArray<FPHGenerationIssue> Issues;
	bPassed &= TestTrue(TEXT("Finite zero-valued spacing, unlimited loops and disabled loops remain legal"),
		UPHLayoutGenerator::ValidateRequest(Allowed, Issues));
	bPassed &= TestTrue(TEXT("A valid request has no issues"), Issues.IsEmpty());
	return bPassed;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHGenerationInvalidPlacementSettingsTest,
	"ProjectHunter.Generation.Dungeon.RejectsInvalidPlacementSettings",
	PHGenerationRequestValidationTests::TestFlags)

bool FPHGenerationInvalidPlacementSettingsTest::RunTest(const FString&)
{
	using namespace PHGenerationRequestValidationTests;
	bool bPassed = true;
	FPHLayoutRequest Request;
	Request.RegionPlacement = static_cast<EPHRegionPlacement>(255);
	bPassed &= ExpectRequestRefused(*this, TEXT("An unknown placement enum"), Request);

	for (const int32 Stacks : { 0, -1 })
	{
		Request = FPHLayoutRequest();
		Request.MaxHeightStacks = Stacks;
		bPassed &= ExpectRequestRefused(*this,
			FString::Printf(TEXT("MaxHeightStacks %d"), Stacks), Request);
	}

	TArray<FPHGenerationIssue> Issues;
	for (const EPHRegionPlacement Placement : { EPHRegionPlacement::Scatter, EPHRegionPlacement::Growth })
	{
		Request = FPHLayoutRequest();
		Request.RegionPlacement = Placement;
		Request.MaxHeightStacks = 3;
		bPassed &= TestTrue(TEXT("Both supported placement modes permit positive height stacks"),
			UPHLayoutGenerator::ValidateRequest(Request, Issues));
		bPassed &= TestTrue(TEXT("Supported settings produce no validation issues"), Issues.IsEmpty());
	}
	return bPassed;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHDungeonMinimumAboveStrategyCapacityTest,
	"ProjectHunter.Generation.Dungeon.RefusesMinimumAboveStrategyCapacity",
	PHGenerationRequestValidationTests::TestFlags)

bool FPHDungeonMinimumAboveStrategyCapacityTest::RunTest(const FString&)
{
	using namespace PHGenerationRequestValidationTests;
	TStrongObjectPtr<UPHDungeonGenerator> Generator(NewObject<UPHDungeonGenerator>());
	bool bPassed = true;

	for (const int32 Seed : { 1, 7 })
	{
		FPHLayoutRequest Request;
		Request.Seed = Seed;
		Request.RegionPlacement = EPHRegionPlacement::Scatter;
		Request.AreaSize = FVector2D(1000000.0, 1000000.0);
		Request.MinRegionSize = FVector2D(100.0, 100.0);
		Request.MaxRegionSize = Request.MinRegionSize;
		Request.MinRegionCount = 65;
		Request.MaxRegionCount = 65;
		Request.LoopChance = 0.0f;

		// The shared request contract has no 64-region limit. Only the dungeon strategy has
		// that work cap, and it must refuse rather than report success below the authored minimum.
		TArray<FPHGenerationIssue> Issues;
		bPassed &= TestTrue(TEXT("The shared request gate permits a larger future strategy"),
			UPHLayoutGenerator::ValidateRequest(Request, Issues));

		FPHGeneratedLayout Layout;
		Layout.Regions.AddDefaulted();
		bPassed &= TestFalse(FString::Printf(TEXT("Seed %d cannot silently downgrade 65 regions to 64"), Seed),
			Generator->GenerateLayout(Request, Layout, Issues));
		bPassed &= TestTrue(TEXT("Unsupported dungeon minimum reports InvalidRequest"), HasInvalidRequest(Issues));
		bPassed &= TestTrue(TEXT("Refusal clears any previous layout"),
			Layout.Regions.IsEmpty() && Layout.Connections.IsEmpty() && Layout.Anchors.IsEmpty());
	}
	return bPassed;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHGenerationZeroModuleFootprintTest,
	"ProjectHunter.Generation.Dungeon.RefusesZeroModuleFootprint",
	PHGenerationRequestValidationTests::TestFlags)

bool FPHGenerationZeroModuleFootprintTest::RunTest(const FString&)
{
	using namespace PHGenerationRequestValidationTests;
	FPHLayoutRequest Request;
	Request.Seed = 1;
	Request.GridSize = 1000000.0;
	Request.AreaSize = FVector2D(1000000.0, 1000000.0);
	Request.RegionHeight = 1000000.0;
	Request.MinRegionSize = FVector2D(1.0, 1.0);
	Request.MaxRegionSize = Request.MinRegionSize;
	Request.MinRegionCount = 1;
	Request.MaxRegionCount = 1;

	// Both footprint limits are positive, but the ratio falls within the module-rounding
	// tolerance of zero. A dungeon still needs at least one whole module on each planar axis.
	bool bPassed = ExpectRequestRefused(*this, TEXT("A footprint smaller than any whole module"), Request);
	TStrongObjectPtr<UPHDungeonGenerator> Generator(NewObject<UPHDungeonGenerator>());
	FPHGeneratedLayout Layout;
	TArray<FPHGenerationIssue> Issues;
	bPassed &= TestFalse(TEXT("Generation cannot publish a line-shaped dungeon as a successful room"),
		Generator->GenerateLayout(Request, Layout, Issues));
	bPassed &= TestTrue(TEXT("The failed generation leaves no regions"), Layout.Regions.IsEmpty());
	return bPassed;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHGenerationNonFiniteDimensionsTest,
	"ProjectHunter.Generation.Dungeon.RejectsNonFiniteDimensions",
	PHGenerationRequestValidationTests::TestFlags)

bool FPHGenerationNonFiniteDimensionsTest::RunTest(const FString&)
{
	using namespace PHGenerationRequestValidationTests;
	TStrongObjectPtr<UPHDungeonGenerator> Generator(NewObject<UPHDungeonGenerator>());
	const TArray<TPair<FString, TFunction<void(FPHLayoutRequest&, double)>>> Fields =
	{
		{ TEXT("GridSize"), [](FPHLayoutRequest& R, double V) { R.GridSize = V; } },
		{ TEXT("AreaSize.X"), [](FPHLayoutRequest& R, double V) { R.AreaSize.X = V; } },
		{ TEXT("AreaSize.Y"), [](FPHLayoutRequest& R, double V) { R.AreaSize.Y = V; } },
		{ TEXT("MinRegionSize.X"), [](FPHLayoutRequest& R, double V) { R.MinRegionSize.X = V; } },
		{ TEXT("MinRegionSize.Y"), [](FPHLayoutRequest& R, double V) { R.MinRegionSize.Y = V; } },
		{ TEXT("MaxRegionSize.X"), [](FPHLayoutRequest& R, double V) { R.MaxRegionSize.X = V; } },
		{ TEXT("MaxRegionSize.Y"), [](FPHLayoutRequest& R, double V) { R.MaxRegionSize.Y = V; } },
		{ TEXT("RegionHeight"), [](FPHLayoutRequest& R, double V) { R.RegionHeight = V; } }
	};
	const double Values[] =
	{
		std::numeric_limits<double>::quiet_NaN(),
		std::numeric_limits<double>::infinity(),
		-std::numeric_limits<double>::infinity()
	};
	bool bPassed = true;
	for (const auto& Field : Fields)
	{
		for (int32 ValueIndex = 0; ValueIndex < UE_ARRAY_COUNT(Values); ++ValueIndex)
		{
			FPHLayoutRequest Request;
			Field.Value(Request, Values[ValueIndex]);
			const FString Label = FString::Printf(TEXT("%s non-finite case %d"), *Field.Key, ValueIndex);
			const bool bGateRefused = ExpectRequestRefused(*this, Label, Request);
			bPassed &= bGateRefused;
			if (bGateRefused)
			{
				bPassed &= ExpectDungeonRefused(*this, *Generator.Get(), Label, Request, TEXT("finite"));
			}
		}
	}
	return bPassed;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHGenerationModuleConversionBoundsTest,
	"ProjectHunter.Generation.Dungeon.RejectsUnrepresentableModuleCounts",
	PHGenerationRequestValidationTests::TestFlags)

bool FPHGenerationModuleConversionBoundsTest::RunTest(const FString&)
{
	using namespace PHGenerationRequestValidationTests;
	const TArray<TPair<FString, TFunction<void(FPHLayoutRequest&)>>> Cases =
	{
		{ TEXT("Area beyond int32 modules"), [](FPHLayoutRequest& R) { R.AreaSize.X = 3000000000.0; } },
		{ TEXT("Maximum footprint beyond int32 modules"), [](FPHLayoutRequest& R) { R.MaxRegionSize.Y = 3000000000.0; } },
		{ TEXT("Height beyond int32 modules"), [](FPHLayoutRequest& R) { R.RegionHeight = 3000000000.0; } },
		{ TEXT("Spacing beyond int32 modules"), [](FPHLayoutRequest& R) { R.RegionSpacing = 3000000000.0; } },
		{ TEXT("Finite inputs whose quotient overflows"), [](FPHLayoutRequest& R)
			{ R.GridSize = std::numeric_limits<double>::min(); } }
	};
	bool bPassed = true;
	for (const auto& Case : Cases)
	{
		FPHLayoutRequest Request;
		Request.GridSize = 1.0;
		Case.Value(Request);
		bPassed &= ExpectRequestRefused(*this, Case.Key, Request);
	}

	// Public native helpers also fail safely when used before request validation.
	bPassed &= TestEqual(TEXT("ModulesDown rejects an overflowing quotient"),
		UPHLayoutGenerator::ModulesDown(std::numeric_limits<double>::max(), 1.0), INDEX_NONE);
	bPassed &= TestEqual(TEXT("ModulesUp rejects a zero grid"),
		UPHLayoutGenerator::ModulesUp(100.0, 0.0), INDEX_NONE);
	bPassed &= TestEqual(TEXT("ModulesUp preserves exact valid module counts"),
		UPHLayoutGenerator::ModulesUp(1600.0, 100.0), 16);
	return bPassed;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHGenerationExtremeHeightAndCountTest,
	"ProjectHunter.Generation.Dungeon.RejectsHeightAndRandomSpanOverflow",
	PHGenerationRequestValidationTests::TestFlags)

bool FPHGenerationExtremeHeightAndCountTest::RunTest(const FString&)
{
	using namespace PHGenerationRequestValidationTests;
	FPHLayoutRequest Request;
	Request.GridSize = 1.0e300;
	Request.AreaSize = FVector2D(1.0e301, 1.0e301);
	Request.MinRegionSize = FVector2D(2.0e300, 2.0e300);
	Request.MaxRegionSize = Request.MinRegionSize;
	Request.RegionHeight = 1.0e300;
	Request.MaxHeightStacks = MAX_int32;
	bool bPassed = ExpectRequestRefused(*this, TEXT("Finite values with an infinite stacked height"), Request);

	Request = FPHLayoutRequest();
	FPHAnchorRule Rule;
	Rule.SemanticTag = PHGenerationTags::Anchor_Chest.GetTag();
	Rule.MinPerRegion = 0;
	Rule.MaxPerRegion = MAX_int32;
	Request.AnchorRules.Add(Rule);
	bPassed &= ExpectRequestRefused(*this, TEXT("An inclusive anchor count wider than int32"), Request);

	// A large but representable span remains valid; count limits are budgets, not allocation sizes.
	Request.AnchorRules[0].MinPerRegion = 1;
	TArray<FPHGenerationIssue> Issues;
	bPassed &= TestTrue(TEXT("A representable inclusive count span remains legal"),
		UPHLayoutGenerator::ValidateRequest(Request, Issues));
	return bPassed;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHDungeonWorkAndArithmeticLimitsTest,
	"ProjectHunter.Generation.Dungeon.RefusesUnsafeStrategyWork",
	PHGenerationRequestValidationTests::TestFlags)

bool FPHDungeonWorkAndArithmeticLimitsTest::RunTest(const FString&)
{
	using namespace PHGenerationRequestValidationTests;
	TStrongObjectPtr<UPHDungeonGenerator> Generator(NewObject<UPHDungeonGenerator>());
	FPHLayoutRequest Request;
	Request.Seed = 7;
	Request.MaxPlacementAttempts = MAX_int32;
	TArray<FPHGenerationIssue> Issues;
	bool bPassed = TestTrue(TEXT("Retry capacity belongs to the strategy, not the shared request"),
		UPHLayoutGenerator::ValidateRequest(Request, Issues));
	bPassed &= ExpectDungeonRefused(*this, *Generator.Get(), TEXT("An excessive placement retry count"),
		Request, TEXT("retries"));

	Request = FPHLayoutRequest();
	Request.Seed = 7;
	Request.ExtraCorridorModules = MAX_int32;
	bPassed &= TestTrue(TEXT("The shared request permits another strategy to interpret corridor slack"),
		UPHLayoutGenerator::ValidateRequest(Request, Issues));
	bPassed &= ExpectDungeonRefused(*this, *Generator.Get(), TEXT("An overflowing growth gap"),
		Request, TEXT("growth gap"));

	Request.RegionPlacement = EPHRegionPlacement::Scatter;
	FPHGeneratedLayout Layout;
	bPassed &= TestTrue(TEXT("Scatter still ignores growth-only corridor slack"),
		Generator->GenerateLayout(Request, Layout, Issues));

	Request = MakeSingleRoomAnchorRequest(1.0e308);
	Request.AnchorRules.Reset();
	Request.GridSize = 1.0e307;
	Request.RegionHeight = Request.GridSize;
	bPassed &= TestTrue(TEXT("Large finite planar dimensions pass the shared numeric gate"),
		UPHLayoutGenerator::ValidateRequest(Request, Issues));
	bPassed &= ExpectDungeonRefused(*this, *Generator.Get(), TEXT("Bounds whose centre sum overflows"),
		Request, TEXT("centres"));
	return bPassed;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHDungeonAnchorSlotWorkLimitTest,
	"ProjectHunter.Generation.Dungeon.BoundsAnchorSlotAllocation",
	PHGenerationRequestValidationTests::TestFlags)

bool FPHDungeonAnchorSlotWorkLimitTest::RunTest(const FString&)
{
	using namespace PHGenerationRequestValidationTests;
	TStrongObjectPtr<UPHDungeonGenerator> Generator(NewObject<UPHDungeonGenerator>());
	TArray<FPHGenerationIssue> Issues;
	FPHGeneratedLayout Layout;

	// 512 * 512 interior slots is the supported limit: the candidate buffer is exactly 1 MiB.
	const FPHLayoutRequest Boundary = MakeSingleRoomAnchorRequest(514.0);
	bool bPassed = TestTrue(TEXT("The supported anchor work boundary still generates"),
		Generator->GenerateLayout(Boundary, Layout, Issues));
	bPassed &= TestEqual(TEXT("The boundary request seats its one authored chest"), Layout.Anchors.Num(), 3);
	bPassed &= TestEqual(TEXT("Capacity guards preserve generation version 3"), Layout.GenerationVersion, 3);

	// Start with a bounded over-budget case. If the guard disappears this is only a roughly
	// 1 MiB allocation, and the test stops before attempting the overflow-sized fixture.
	if (!ExpectDungeonRefused(*this, *Generator.Get(), TEXT("An anchor grid just over the work limit"),
		MakeSingleRoomAnchorRequest(515.0), TEXT("slot work budget")))
	{
		return false;
	}

	// The verified preflight must reject the 2,500,000,000-slot product before narrowing it
	// to int32 or allocating/shuffling any slots. This case is never run against the unsafe baseline.
	FPHLayoutRequest Overflow = MakeSingleRoomAnchorRequest(50002.0);
	bPassed &= TestTrue(TEXT("The large room itself is a valid generator-neutral request"),
		UPHLayoutGenerator::ValidateRequest(Overflow, Issues));
	bPassed &= ExpectDungeonRefused(*this, *Generator.Get(), TEXT("An overflowing int32 anchor-slot product"),
		Overflow, TEXT("slot work budget"));

	Overflow.AnchorRules.Reset();
	bPassed &= TestTrue(TEXT("A large room without anchor work is not rejected by the slot capacity"),
		Generator->GenerateLayout(Overflow, Layout, Issues));
	bPassed &= TestEqual(TEXT("The unruled large room has only the two endpoint anchors"), Layout.Anchors.Num(), 2);
	return bPassed;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHDungeonLargeGrowthCandidateTest,
	"ProjectHunter.Generation.Dungeon.HandlesLargeGrowthCandidates",
	PHGenerationRequestValidationTests::TestFlags)

bool FPHDungeonLargeGrowthCandidateTest::RunTest(const FString&)
{
	TStrongObjectPtr<UPHDungeonGenerator> Generator(NewObject<UPHDungeonGenerator>());
	bool bPassed = true;
	for (const int32 Seed : { 1, 7, 42 })
	{
		FPHLayoutRequest Request;
		Request.Seed = Seed;
		Request.GridSize = 1.0;
		Request.AreaSize = FVector2D(2000000000.0, 2000000000.0);
		Request.MinRegionSize = FVector2D(1000.0, 1200000000.0);
		Request.MaxRegionSize = Request.MinRegionSize;
		Request.RegionHeight = 1.0;
		Request.RegionSpacing = 1.0;
		Request.MinRegionCount = 2;
		Request.MaxRegionCount = 2;
		Request.LoopChance = 0.0f;

		// Two tall, thin rooms can fit side by side. Rejected growth candidates can exceed int32
		// in their slide span or max corner; they must be rejected safely before scatter fallback.
		// No anchor rules means these large logical bounds never request a large slot allocation.
		FPHGeneratedLayout Layout;
		TArray<FPHGenerationIssue> Issues;
		if (!TestTrue(FString::Printf(TEXT("Seed %d handles wide growth arithmetic"), Seed),
			Generator->GenerateLayout(Request, Layout, Issues)))
		{
			bPassed = false;
			continue;
		}
		bPassed &= TestEqual(TEXT("Both required thin rooms are preserved"), Layout.Regions.Num(), 2);
		for (const FPHGeneratedRegion& Region : Layout.Regions)
		{
			bPassed &= TestTrue(TEXT("No candidate wrapped into an inverted or truncated room"),
				Region.Bounds.GetSize().Equals(FVector(1000.0, 1200000000.0, 1.0)));
		}
	}
	return bPassed;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHDungeonAggregateAnchorSlotWorkTest,
	"ProjectHunter.Generation.Dungeon.BoundsAggregateAnchorSlotWork",
	PHGenerationRequestValidationTests::TestFlags)

bool FPHDungeonAggregateAnchorSlotWorkTest::RunTest(const FString&)
{
	using namespace PHGenerationRequestValidationTests;
	TStrongObjectPtr<UPHDungeonGenerator> Generator(NewObject<UPHDungeonGenerator>());
	FPHLayoutRequest Request = MakeSingleRoomAnchorRequest(402.0);
	Request.AreaSize = FVector2D(10000.0, 10000.0);
	Request.MinRegionCount = 2;
	Request.MaxRegionCount = 2;
	Request.RegionPlacement = EPHRegionPlacement::Scatter;

	// Each room needs 160,000 slots and fits individually, but their total exceeds the work cap.
	return ExpectDungeonRefused(*this, *Generator.Get(), TEXT("Two individually permitted anchor grids"),
		Request, TEXT("slot work budget"));
}

#endif // WITH_DEV_AUTOMATION_TESTS
