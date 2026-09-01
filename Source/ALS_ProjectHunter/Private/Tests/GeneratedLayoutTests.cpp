#include "CoreMinimal.h"
#include "Generation/Library/FunctionLibraries/PHGenerationValidationLibrary.h"
#include "Generation/PHGenerationTags.h"
#include "GameplayTagsManager.h"
#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"

#include <limits>

#if WITH_DEV_AUTOMATION_TESTS

namespace PHGeneratedLayoutTests
{
	constexpr EAutomationTestFlags TestFlags =
		EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter;

	UE_DEFINE_GAMEPLAY_TAG_STATIC(PlayerStartChild, "Anchor.PlayerStart.AutomationFixture");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(ExitChild, "Anchor.Exit.AutomationFixture");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(OptionalAnchor, "Anchor.AutomationFixture");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(NonAnchor, "Test.Generation.NotAnAnchor");

	FPHGeneratedRegion MakeRegion(const int32 ID, const FVector& Min, const FVector& Max)
	{
		FPHGeneratedRegion Region;
		Region.RegionID = ID;
		Region.Bounds = FBox(Min, Max);
		return Region;
	}

	FPHGeneratedConnection MakeConnection(const int32 ID, const int32 From, const int32 To)
	{
		FPHGeneratedConnection Connection;
		Connection.ConnectionID = ID;
		Connection.FromRegionID = From;
		Connection.ToRegionID = To;
		return Connection;
	}

	FPHGeneratedAnchor MakeAnchor(const int32 ID, const int32 RegionID,
		const FVector& Location, const FGameplayTag Tag)
	{
		FPHGeneratedAnchor Anchor;
		Anchor.AnchorID = ID;
		Anchor.RegionID = RegionID;
		Anchor.Transform = FTransform(Location);
		Anchor.SemanticTag = Tag;
		return Anchor;
	}

	FPHGeneratedLayout MakeSingleRegionLayout()
	{
		FPHGeneratedLayout Layout;
		Layout.Seed = 73;
		Layout.Bounds = FBox(FVector::ZeroVector, FVector(100.0, 100.0, 0.0));
		Layout.Regions.Add(MakeRegion(10, Layout.Bounds.Min, Layout.Bounds.Max));
		Layout.Anchors.Add(MakeAnchor(10, 10, Layout.Bounds.Min, PHGenerationTags::Anchor_PlayerStart));
		Layout.Anchors.Add(MakeAnchor(99, 10, Layout.Bounds.Max, PHGenerationTags::Anchor_Exit));
		Layout.PlayerStartAnchorID = 10;
		Layout.ExitAnchorID = 99;
		return Layout;
	}

	FPHGeneratedLayout MakeChainLayout()
	{
		FPHGeneratedLayout Layout = MakeSingleRegionLayout();
		Layout.Bounds.Max.X = 300.0;
		Layout.Regions.Add(MakeRegion(100, FVector(100.0, 0.0, 0.0), FVector(200.0, 100.0, 0.0)));
		Layout.Regions.Add(MakeRegion(1000, FVector(200.0, 0.0, 0.0), Layout.Bounds.Max));
		Layout.Connections.Add(MakeConnection(10, 10, 100));
		Layout.Connections.Add(MakeConnection(20, 100, 1000));
		Layout.Anchors[1].RegionID = 1000;
		Layout.Anchors[1].Transform.SetTranslation(Layout.Bounds.Max);
		return Layout;
	}

	bool HasIssue(const TArray<FPHGenerationIssue>& Issues, const EPHGenerationIssueCode Code)
	{
		return Issues.ContainsByPredicate([Code](const FPHGenerationIssue& Issue)
		{
			return Issue.Code == Code;
		});
	}

	void ExpectValid(FAutomationTestBase& Test, const TCHAR* Description, const FPHGeneratedLayout& Layout)
	{
		TArray<FPHGenerationIssue> Issues;
		Test.TestTrue(Description, UPHGenerationValidationLibrary::ValidateLayout(Layout, Issues));
		Test.TestTrue(FString::Printf(TEXT("%s: no issues"), Description), Issues.IsEmpty());
	}

	void ExpectIssue(FAutomationTestBase& Test, const TCHAR* Description, const FPHGeneratedLayout& Layout,
		const EPHGenerationIssueCode Code, const int32 ElementIndex = INDEX_NONE)
	{
		TArray<FPHGenerationIssue> Issues;
		Test.TestFalse(Description, UPHGenerationValidationLibrary::ValidateLayout(Layout, Issues));
		Test.TestTrue(FString::Printf(TEXT("%s: expected issue and location"), Description),
			Issues.ContainsByPredicate([Code, ElementIndex](const FPHGenerationIssue& Issue)
			{
				return Issue.Code == Code && Issue.ElementIndex == ElementIndex;
			}));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHLayoutValidSmallLayoutsTest,
	"ProjectHunter.Generation.Layout.ValidSmallLayouts", PHGeneratedLayoutTests::TestFlags)

bool FPHLayoutValidSmallLayoutsTest::RunTest(const FString&)
{
	using namespace PHGeneratedLayoutTests;
	FPHGeneratedLayout Layout = MakeSingleRegionLayout();
	ExpectValid(*this, TEXT("Planar bounds include start and exit on their edges"), Layout);

	Layout.Anchors[1].Transform.SetTranslation(Layout.Anchors[0].Transform.GetTranslation());
	ExpectValid(*this, TEXT("Distinct endpoint roles may occupy one position in one region"), Layout);

	Layout = MakeSingleRegionLayout();
	Layout.Bounds = FBox(FVector(-200.0, -100.0, -50.0), FVector(100.0, 300.0, 700.0));
	Layout.Regions[0].Bounds = Layout.Bounds;
	Layout.Anchors[0].Transform.SetTranslation(Layout.Bounds.Min);
	Layout.Anchors[1].Transform.SetTranslation(Layout.Bounds.Max);
	Layout.Anchors[1].Transform.SetRotation(FRotator(15.0, 45.0, 10.0).Quaternion());
	ExpectValid(*this, TEXT("Volumetric layouts accept normalized oriented anchors"), Layout);

	Layout.Bounds = FBox(FVector(5.0, 10.0, -50.0), FVector(5.0, 10.0, 700.0));
	Layout.Regions[0].Bounds = Layout.Bounds;
	Layout.Anchors[0].Transform.SetTranslation(Layout.Bounds.Min);
	Layout.Anchors[1].Transform.SetTranslation(Layout.Bounds.Max);
	ExpectValid(*this, TEXT("A linear envelope is structurally valid"), Layout);

	Layout = MakeChainLayout();
	Layout.Regions[1].Bounds = Layout.Bounds;
	ExpectValid(*this, TEXT("Overlapping logical envelopes are permitted"), Layout);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHLayoutStableIDsTest,
	"ProjectHunter.Generation.Layout.IDsIndependentOfArrayOrder", PHGeneratedLayoutTests::TestFlags)

bool FPHLayoutStableIDsTest::RunTest(const FString&)
{
	using namespace PHGeneratedLayoutTests;
	FPHGeneratedLayout Layout = MakeChainLayout();
	ExpectValid(*this, TEXT("Sparse IDs may be reused across different collections"), Layout);
	Layout.Regions.Swap(0, 2);
	Layout.Connections.Swap(0, 1);
	Layout.Anchors.Swap(0, 1);
	ExpectValid(*this, TEXT("Reordering records does not change their references"), Layout);

	Layout.Anchors.Add(MakeAnchor(MAX_int32, 100, FVector(150.0, 50.0, 0.0), OptionalAnchor));
	ExpectValid(*this, TEXT("IDs need not be small or sequential"), Layout);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHLayoutSeedVersionTest,
	"ProjectHunter.Generation.Layout.LiteralSeedsAndVersions", PHGeneratedLayoutTests::TestFlags)

bool FPHLayoutSeedVersionTest::RunTest(const FString&)
{
	using namespace PHGeneratedLayoutTests;
	for (const int32 Seed : {MIN_int32, -1, 0, 1, MAX_int32})
	{
		FPHGeneratedLayout Layout = MakeSingleRegionLayout();
		Layout.Seed = Seed;
		Layout.GenerationVersion = MAX_int32;
		ExpectValid(*this, TEXT("Every signed seed and positive producer version is literal data"), Layout);
		TestEqual(TEXT("Validation preserves the literal seed"), Layout.Seed, Seed);
		TestEqual(TEXT("Validation preserves the producer version"), Layout.GenerationVersion, MAX_int32);
	}

	FPHGeneratedLayout Layout = MakeSingleRegionLayout();
	Layout.GenerationVersion = 0;
	ExpectIssue(*this, TEXT("A zero generation version is invalid"), Layout,
		EPHGenerationIssueCode::InvalidGenerationVersion);
	Layout.GenerationVersion = -7;
	ExpectIssue(*this, TEXT("A negative generation version is invalid"), Layout,
		EPHGenerationIssueCode::InvalidGenerationVersion);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHLayoutRegionConnectionIDsTest,
	"ProjectHunter.Generation.Layout.RegionAndConnectionIDs", PHGeneratedLayoutTests::TestFlags)

bool FPHLayoutRegionConnectionIDsTest::RunTest(const FString&)
{
	using namespace PHGeneratedLayoutTests;
	FPHGeneratedLayout Layout = MakeChainLayout();
	Layout.Regions[1].RegionID = INDEX_NONE;
	ExpectIssue(*this, TEXT("An unset region ID is invalid"), Layout,
		EPHGenerationIssueCode::InvalidRegionID, 1);
	Layout.Regions[1].RegionID = -17;
	ExpectIssue(*this, TEXT("Other negative region IDs are also invalid"), Layout,
		EPHGenerationIssueCode::InvalidRegionID, 1);
	Layout = MakeChainLayout();
	Layout.Regions[2].RegionID = Layout.Regions[0].RegionID;
	ExpectIssue(*this, TEXT("Region IDs must be unique"), Layout,
		EPHGenerationIssueCode::DuplicateRegionID, 2);
	Layout.Regions.Empty();
	ExpectIssue(*this, TEXT("A layout requires a region"), Layout, EPHGenerationIssueCode::EmptyRegions);

	Layout = MakeChainLayout();
	Layout.Connections[0].ConnectionID = -4;
	ExpectIssue(*this, TEXT("Connection IDs must be nonnegative"), Layout,
		EPHGenerationIssueCode::InvalidConnectionID, 0);
	Layout = MakeChainLayout();
	Layout.Connections[1].ConnectionID = Layout.Connections[0].ConnectionID;
	ExpectIssue(*this, TEXT("Connection IDs must be unique"), Layout,
		EPHGenerationIssueCode::DuplicateConnectionID, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHLayoutBrokenLinksTest,
	"ProjectHunter.Generation.Layout.BrokenRegionLinks", PHGeneratedLayoutTests::TestFlags)

bool FPHLayoutBrokenLinksTest::RunTest(const FString&)
{
	using namespace PHGeneratedLayoutTests;
	FPHGeneratedLayout Layout = MakeChainLayout();
	Layout.Connections[0].FromRegionID = 222;
	ExpectIssue(*this, TEXT("The source region must exist"), Layout,
		EPHGenerationIssueCode::MissingConnectionRegion, 0);
	Layout = MakeChainLayout();
	Layout.Connections[1].ToRegionID = INDEX_NONE;
	ExpectIssue(*this, TEXT("The destination region must exist"), Layout,
		EPHGenerationIssueCode::MissingConnectionRegion, 1);
	Layout = MakeChainLayout();
	Layout.Connections[0].ToRegionID = Layout.Connections[0].FromRegionID;
	ExpectIssue(*this, TEXT("A connection cannot target its own region"), Layout,
		EPHGenerationIssueCode::SelfConnection, 0);
	Layout = MakeChainLayout();
	Layout.Anchors[1].RegionID = 222;
	ExpectIssue(*this, TEXT("An anchor's region must exist"), Layout,
		EPHGenerationIssueCode::MissingAnchorRegion, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHLayoutAnchorReferencesTest,
	"ProjectHunter.Generation.Layout.AnchorIDsAndEndpointReferences", PHGeneratedLayoutTests::TestFlags)

bool FPHLayoutAnchorReferencesTest::RunTest(const FString&)
{
	using namespace PHGeneratedLayoutTests;
	FPHGeneratedLayout Layout = MakeSingleRegionLayout();
	Layout.Anchors[1].AnchorID = -3;
	ExpectIssue(*this, TEXT("Anchor IDs must be nonnegative"), Layout,
		EPHGenerationIssueCode::InvalidAnchorID, 1);
	Layout = MakeSingleRegionLayout();
	Layout.Anchors[1].AnchorID = Layout.Anchors[0].AnchorID;
	ExpectIssue(*this, TEXT("Anchor IDs must be unique"), Layout,
		EPHGenerationIssueCode::DuplicateAnchorID, 1);

	Layout = MakeSingleRegionLayout();
	Layout.PlayerStartAnchorID = INDEX_NONE;
	ExpectIssue(*this, TEXT("A start tag alone does not replace the explicit start reference"), Layout,
		EPHGenerationIssueCode::MissingPlayerStart);
	Layout.PlayerStartAnchorID = 456;
	ExpectIssue(*this, TEXT("A nonnegative start reference must identify an anchor"), Layout,
		EPHGenerationIssueCode::MissingPlayerStart);
	Layout = MakeSingleRegionLayout();
	Layout.ExitAnchorID = INDEX_NONE;
	ExpectIssue(*this, TEXT("An exit tag alone does not replace the explicit exit reference"), Layout,
		EPHGenerationIssueCode::MissingExit);
	Layout.ExitAnchorID = 456;
	ExpectIssue(*this, TEXT("A nonnegative exit reference must identify an anchor"), Layout,
		EPHGenerationIssueCode::MissingExit);

	Layout = MakeSingleRegionLayout();
	Layout.Anchors.Add(MakeAnchor(25, 10, FVector(50.0, 50.0, 0.0), PHGenerationTags::Anchor_PlayerStart));
	Layout.PlayerStartAnchorID = 25;
	ExpectValid(*this, TEXT("The reference selects one of multiple eligible start anchors"), Layout);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHLayoutBoundsTest,
	"ProjectHunter.Generation.Layout.BoundsValidation", PHGeneratedLayoutTests::TestFlags)

bool FPHLayoutBoundsTest::RunTest(const FString&)
{
	using namespace PHGeneratedLayoutTests;
	FPHGeneratedLayout Layout = MakeSingleRegionLayout();
	Layout.Bounds = FBox(ForceInit);
	ExpectIssue(*this, TEXT("An uninitialized layout box is invalid"), Layout,
		EPHGenerationIssueCode::InvalidLayoutBounds);
	Layout.Bounds = FBox(FVector(100.0, 0.0, 0.0), FVector(0.0, 100.0, 0.0));
	ExpectIssue(*this, TEXT("IsValid does not make reversed bounds valid"), Layout,
		EPHGenerationIssueCode::InvalidLayoutBounds);
	Layout.Bounds = FBox(FVector::ZeroVector, FVector::ZeroVector);
	ExpectIssue(*this, TEXT("Point layout bounds are invalid"), Layout,
		EPHGenerationIssueCode::InvalidLayoutBounds);
	Layout = MakeSingleRegionLayout();
	Layout.Bounds.Min.X = std::numeric_limits<double>::quiet_NaN();
	ExpectIssue(*this, TEXT("NaN layout coordinates are invalid"), Layout,
		EPHGenerationIssueCode::InvalidLayoutBounds);
	Layout = MakeSingleRegionLayout();
	Layout.Bounds.Max.Z = std::numeric_limits<double>::infinity();
	ExpectIssue(*this, TEXT("Infinite layout coordinates are invalid"), Layout,
		EPHGenerationIssueCode::InvalidLayoutBounds);

	Layout = MakeSingleRegionLayout();
	Layout.Regions[0].Bounds = FBox(ForceInit);
	ExpectIssue(*this, TEXT("An uninitialized region box is invalid"), Layout,
		EPHGenerationIssueCode::InvalidRegionBounds, 0);
	Layout.Regions[0].Bounds = FBox(FVector(0.0, 100.0, 0.0), FVector(100.0, 0.0, 0.0));
	ExpectIssue(*this, TEXT("Region bounds must be ordered on every axis"), Layout,
		EPHGenerationIssueCode::InvalidRegionBounds, 0);
	Layout.Regions[0].Bounds = FBox(FVector::ZeroVector, FVector::ZeroVector);
	ExpectIssue(*this, TEXT("Point region bounds are invalid"), Layout,
		EPHGenerationIssueCode::InvalidRegionBounds, 0);
	Layout = MakeSingleRegionLayout();
	Layout.Regions[0].Bounds.Max.Y = std::numeric_limits<double>::infinity();
	ExpectIssue(*this, TEXT("Region coordinates must be finite"), Layout,
		EPHGenerationIssueCode::InvalidRegionBounds, 0);
	Layout = MakeSingleRegionLayout();
	Layout.Regions[0].Bounds.Max.X += 1.0;
	ExpectIssue(*this, TEXT("Regions must remain inside the layout envelope"), Layout,
		EPHGenerationIssueCode::RegionOutsideLayout, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHLayoutAnchorPoseTest,
	"ProjectHunter.Generation.Layout.AnchorPoseValidation", PHGeneratedLayoutTests::TestFlags)

bool FPHLayoutAnchorPoseTest::RunTest(const FString&)
{
	using namespace PHGeneratedLayoutTests;
	FPHGeneratedLayout Layout = MakeChainLayout();
	Layout.Anchors[0].Transform.SetTranslation(FVector(150.0, 50.0, 0.0));
	ExpectIssue(*this, TEXT("Being inside the layout is insufficient when outside the anchor's region"), Layout,
		EPHGenerationIssueCode::AnchorOutsideRegion, 0);
	Layout = MakeSingleRegionLayout();
	Layout.Anchors[0].Transform.SetTranslation(FVector(50.0, 50.0, 1.0));
	ExpectIssue(*this, TEXT("Planar containment still checks the collapsed axis"), Layout,
		EPHGenerationIssueCode::AnchorOutsideRegion, 0);

	for (const FVector Scale : {FVector::ZeroVector, FVector(2.0, 1.0, 1.0), FVector(-1.0, 1.0, 1.0)})
	{
		Layout = MakeSingleRegionLayout();
		Layout.Anchors[0].Transform.SetScale3D(Scale);
		ExpectIssue(*this, TEXT("Anchor scale belongs to content construction and must remain one"), Layout,
			EPHGenerationIssueCode::InvalidAnchorTransform, 0);
	}
	Layout = MakeSingleRegionLayout();
	Layout.Anchors[1].Transform.SetRotation(FQuat(0.0, 0.0, 0.0, 2.0));
	ExpectIssue(*this, TEXT("Finite but unnormalized anchor rotation is invalid"), Layout,
		EPHGenerationIssueCode::InvalidAnchorTransform, 1);

#if !ENABLE_NAN_DIAGNOSTIC
	// Engine NaN diagnostics sanitize setters before the validator can inspect the fixture.
	Layout = MakeSingleRegionLayout();
	Layout.Anchors[0].Transform.SetTranslation(FVector(std::numeric_limits<double>::infinity(), 0.0, 0.0));
	ExpectIssue(*this, TEXT("Non-finite anchor translation is invalid"), Layout,
		EPHGenerationIssueCode::InvalidAnchorTransform, 0);
	Layout = MakeSingleRegionLayout();
	Layout.Anchors[1].Transform.SetScale3D(FVector(1.0, std::numeric_limits<double>::quiet_NaN(), 1.0));
	ExpectIssue(*this, TEXT("Non-finite anchor scale is invalid"), Layout,
		EPHGenerationIssueCode::InvalidAnchorTransform, 1);
#endif
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHLayoutSemanticEndpointsTest,
	"ProjectHunter.Generation.Layout.SemanticEndpoints", PHGeneratedLayoutTests::TestFlags)

bool FPHLayoutSemanticEndpointsTest::RunTest(const FString&)
{
	using namespace PHGeneratedLayoutTests;
	FPHGeneratedLayout Layout = MakeSingleRegionLayout();
	Layout.Anchors[0].SemanticTag = PlayerStartChild;
	Layout.Anchors[1].SemanticTag = ExitChild;
	Layout.Anchors.Add(MakeAnchor(50, 10, FVector(50.0, 50.0, 0.0), OptionalAnchor));
	ExpectValid(*this, TEXT("Endpoint descendants and other anchor roles are supported"), Layout);

	for (const FGameplayTag Tag : {FGameplayTag(), PHGenerationTags::Anchor.GetTag(), NonAnchor.GetTag()})
	{
		Layout.Anchors[2].SemanticTag = Tag;
		ExpectIssue(*this, TEXT("Every anchor needs a concrete semantic tag under Anchor"), Layout,
			EPHGenerationIssueCode::InvalidAnchorTag, 2);
	}
	Layout = MakeSingleRegionLayout();
	Layout.PlayerStartAnchorID = Layout.ExitAnchorID;
	ExpectIssue(*this, TEXT("The designated start must have the start role"), Layout,
		EPHGenerationIssueCode::InvalidPlayerStartTag, 1);
	Layout = MakeSingleRegionLayout();
	Layout.ExitAnchorID = Layout.PlayerStartAnchorID;
	ExpectIssue(*this, TEXT("The designated exit must have the exit role"), Layout,
		EPHGenerationIssueCode::InvalidExitTag, 0);

	// A retained serialized tag may name a removed registration.
	const FNameProperty* TagNameProperty = FindFProperty<FNameProperty>(FGameplayTag::StaticStruct(), TEXT("TagName"));
	if (!TestNotNull(TEXT("The malformed-tag fixture uses the reflected name field"), TagNameProperty))
	{
		return false;
	}
	FGameplayTag StaleTag;
	TagNameProperty->SetPropertyValue_InContainer(&StaleTag, FName(TEXT("Anchor.AutomationFixture.NeverRegistered")));
	TestTrue(TEXT("A stale tag can be nonempty without registration"), StaleTag.IsValid());
	TestFalse(TEXT("The stale-tag fixture is not registered"), UGameplayTagsManager::Get().FindTagNode(StaleTag).IsValid());

	Layout = MakeSingleRegionLayout();
	Layout.Anchors.Add(MakeAnchor(50, 10, FVector(50.0, 50.0, 0.0), StaleTag));
	ExpectIssue(*this, TEXT("An unregistered ordinary anchor tag is rejected without an engine ensure"), Layout,
		EPHGenerationIssueCode::InvalidAnchorTag, 2);
	Layout = MakeSingleRegionLayout();
	Layout.Anchors[0].SemanticTag = StaleTag;
	ExpectIssue(*this, TEXT("An unregistered start tag is rejected without an engine ensure"), Layout,
		EPHGenerationIssueCode::InvalidAnchorTag, 0);
	Layout = MakeSingleRegionLayout();
	Layout.Anchors[1].SemanticTag = StaleTag;
	ExpectIssue(*this, TEXT("An unregistered exit tag is rejected without an engine ensure"), Layout,
		EPHGenerationIssueCode::InvalidAnchorTag, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHLayoutDirectedReachabilityTest,
	"ProjectHunter.Generation.Layout.DirectedReachability", PHGeneratedLayoutTests::TestFlags)

bool FPHLayoutDirectedReachabilityTest::RunTest(const FString&)
{
	using namespace PHGeneratedLayoutTests;
	FPHGeneratedLayout Layout = MakeChainLayout();
	for (FPHGeneratedConnection& Connection : Layout.Connections)
	{
		Connection.bBidirectional = false;
	}
	ExpectValid(*this, TEXT("Directed links permit a forward route through every region"), Layout);
	Swap(Layout.Connections[1].FromRegionID, Layout.Connections[1].ToRegionID);
	TArray<FPHGenerationIssue> Issues;
	TestFalse(TEXT("A reversed one-way link cannot reach the exit"),
		UPHGenerationValidationLibrary::ValidateLayout(Layout, Issues));
	TestTrue(TEXT("The disconnected region is reported"), HasIssue(Issues, EPHGenerationIssueCode::UnreachableRegion));
	TestTrue(TEXT("The disconnected exit is reported explicitly"), HasIssue(Issues, EPHGenerationIssueCode::UnreachableExit));
	Layout.Connections[1].bBidirectional = true;
	ExpectValid(*this, TEXT("A bidirectional link restores traversal in either orientation"), Layout);

	Layout = MakeChainLayout();
	Layout.Connections = {MakeConnection(1, 10, 1000)};
	TestFalse(TEXT("A reachable exit does not hide an isolated middle region"),
		UPHGenerationValidationLibrary::ValidateLayout(Layout, Issues));
	TestTrue(TEXT("The isolated region is reported"), HasIssue(Issues, EPHGenerationIssueCode::UnreachableRegion));
	TestFalse(TEXT("A reachable exit is not mislabeled as unreachable"), HasIssue(Issues, EPHGenerationIssueCode::UnreachableExit));

	Layout = MakeChainLayout();
	Layout.Connections.Add(MakeConnection(30, 1000, 10));
	Layout.Connections.Add(MakeConnection(40, 10, 100));
	ExpectValid(*this, TEXT("Cycles and parallel connections with distinct IDs are valid"), Layout);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHLayoutIssueOutputTest,
	"ProjectHunter.Generation.Layout.LocalErrorsAndOutputReuse", PHGeneratedLayoutTests::TestFlags)

bool FPHLayoutIssueOutputTest::RunTest(const FString&)
{
	using namespace PHGeneratedLayoutTests;
	TArray<FPHGenerationIssue> Issues;
	TestFalse(TEXT("The default layout is not ready for use"),
		UPHGenerationValidationLibrary::ValidateLayout(FPHGeneratedLayout(), Issues));
	for (const EPHGenerationIssueCode Code : {EPHGenerationIssueCode::InvalidLayoutBounds,
		EPHGenerationIssueCode::EmptyRegions, EPHGenerationIssueCode::MissingPlayerStart, EPHGenerationIssueCode::MissingExit})
	{
		TestTrue(TEXT("Default-layout structural errors are collected together"), HasIssue(Issues, Code));
	}
	TestFalse(TEXT("No graph error is inferred from the incomplete default layout"),
		HasIssue(Issues, EPHGenerationIssueCode::UnreachableRegion) || HasIssue(Issues, EPHGenerationIssueCode::UnreachableExit));

	FPHGeneratedLayout Layout = MakeChainLayout();
	Layout.GenerationVersion = 0;
	Layout.Connections.Empty();
	TestFalse(TEXT("A local error takes precedence over graph diagnostics"),
		UPHGenerationValidationLibrary::ValidateLayout(Layout, Issues));
	TestTrue(TEXT("The current local error replaces prior output"), HasIssue(Issues, EPHGenerationIssueCode::InvalidGenerationVersion));
	TestFalse(TEXT("Old missing-endpoint errors are discarded"), HasIssue(Issues, EPHGenerationIssueCode::MissingPlayerStart));
	TestFalse(TEXT("Connectivity waits until local errors are repaired"),
		HasIssue(Issues, EPHGenerationIssueCode::UnreachableRegion) || HasIssue(Issues, EPHGenerationIssueCode::UnreachableExit));

	Layout.GenerationVersion = 1;
	TestFalse(TEXT("Graph validation runs once local data is valid"),
		UPHGenerationValidationLibrary::ValidateLayout(Layout, Issues));
	TestTrue(TEXT("Disconnected graph is now reported"), HasIssue(Issues, EPHGenerationIssueCode::UnreachableExit));
	TestFalse(TEXT("The repaired version is no longer reported"), HasIssue(Issues, EPHGenerationIssueCode::InvalidGenerationVersion));
	TestTrue(TEXT("A later valid layout succeeds using the same output array"),
		UPHGenerationValidationLibrary::ValidateLayout(MakeChainLayout(), Issues));
	TestTrue(TEXT("Success clears all old issues"), Issues.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHLayoutScaleFixturesTest,
	"ProjectHunter.Generation.Layout.VariedScaleFixtures", PHGeneratedLayoutTests::TestFlags)

bool FPHLayoutScaleFixturesTest::RunTest(const FString&)
{
	using namespace PHGeneratedLayoutTests;
	const double Scales[] = {0.01, 1.0, 100.0, 100000.0};
	for (int32 FixtureIndex = 0; FixtureIndex < 256; ++FixtureIndex)
	{
		// These are transformed test fixtures, not outputs from a map generator.
		FPHGeneratedLayout Layout = MakeChainLayout();
		Layout.Seed = FixtureIndex - 128;
		Layout.GenerationVersion = 1 + FixtureIndex % 4;
		const double Scale = Scales[FixtureIndex % UE_ARRAY_COUNT(Scales)];
		const FVector Offset(100000000.0, -100000000.0, static_cast<double>(FixtureIndex * 100));
		Layout.Bounds = FBox(Layout.Bounds.Min * Scale + Offset, Layout.Bounds.Max * Scale + Offset);
		for (FPHGeneratedRegion& Region : Layout.Regions)
		{
			Region.Bounds = FBox(Region.Bounds.Min * Scale + Offset, Region.Bounds.Max * Scale + Offset);
		}
		for (FPHGeneratedAnchor& Anchor : Layout.Anchors)
		{
			Anchor.Transform.SetTranslation(Anchor.Transform.GetTranslation() * Scale + Offset);
		}
		ExpectValid(*this, TEXT("Logical fixtures support different scales, seeds, versions, and elevations"), Layout);
		Layout.Regions.Swap(0, FixtureIndex % Layout.Regions.Num());
		Layout.Anchors.Swap(0, 1);
		ExpectValid(*this, TEXT("Reordered scale fixtures retain their logical references"), Layout);
	}
	return true;
}

#endif
