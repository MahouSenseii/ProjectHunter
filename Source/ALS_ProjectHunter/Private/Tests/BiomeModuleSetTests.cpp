#include "CoreMinimal.h"
#include "Engine/StaticMesh.h"
#include "GameplayTagsManager.h"
#include "Generation/Data/PHBiomeModuleSet.h"
#include "Generation/PHGenerationTags.h"
#include "Misc/AutomationTest.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"

#include <limits>

#if WITH_DEV_AUTOMATION_TESTS

namespace PHBiomeModuleSetTests
{
	constexpr EAutomationTestFlags TestFlags =
		EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter;

	UE_DEFINE_GAMEPLAY_TAG_STATIC(NonPieceTag, "Test.Generation.NotAPiece");

	/**
	 * A soft reference to a path that is never loaded. Validation must judge authoring without
	 * pulling meshes in, so a fake path is the correct fixture here.
	 */
	FPHModuleEntry MakeEntry(const FVector2D Footprint = FVector2D(100.0, 100.0))
	{
		FPHModuleEntry Entry;
		Entry.Mesh = TSoftObjectPtr<UStaticMesh>(
			FSoftObjectPath(TEXT("/Game/BlockingStarterPack/Meshes/Architecture/Floors/SM_Floor_100x100.SM_Floor_100x100")));
		Entry.Footprint = Footprint;
		return Entry;
	}

	UPHBiomeModuleSet* MakeSet(TStrongObjectPtr<UPHBiomeModuleSet>& Keeper)
	{
		Keeper.Reset(NewObject<UPHBiomeModuleSet>());
		Keeper->GridSize = 100.0;
		Keeper->Modules.Add(PHGenerationTags::Piece_Floor.GetTag(), MakeEntry());
		Keeper->Modules.Add(PHGenerationTags::Piece_Wall_Straight.GetTag(),
			MakeEntry(FVector2D(400.0, 100.0)));
		return Keeper.Get();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHBiomeModuleSetValidTest,
	"ProjectHunter.Generation.Biome.ValidSetPasses", PHBiomeModuleSetTests::TestFlags)

bool FPHBiomeModuleSetValidTest::RunTest(const FString&)
{
	using namespace PHBiomeModuleSetTests;
	TStrongObjectPtr<UPHBiomeModuleSet> Keeper;
	UPHBiomeModuleSet* Set = MakeSet(Keeper);

	TArray<FPHGenerationIssue> Issues;
	TestTrue(TEXT("A well-authored set validates"), Set->ValidateModuleSet(Issues));
	TestEqual(TEXT("No issues reported"), Issues.Num(), 0);

	// A 400x100 wall is four whole modules by one, which is exactly how the kit is authored.
	FPHModuleEntry Wall;
	TestTrue(TEXT("Straight wall resolves"),
		Set->ResolvePiece(PHGenerationTags::Piece_Wall_Straight.GetTag(), Wall));
	TestEqual(TEXT("Wall footprint survives the lookup"), Wall.Footprint, FVector2D(400.0, 100.0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHBiomeModuleSetAuthoringTest,
	"ProjectHunter.Generation.Biome.CatchesAuthoringMistakes", PHBiomeModuleSetTests::TestFlags)

bool FPHBiomeModuleSetAuthoringTest::RunTest(const FString&)
{
	using namespace PHBiomeModuleSetTests;

	auto ExpectRejected = [this](const TCHAR* Label, const TFunction<void(UPHBiomeModuleSet&)>& Break)
	{
		TStrongObjectPtr<UPHBiomeModuleSet> Keeper;
		UPHBiomeModuleSet* Set = MakeSet(Keeper);
		Break(*Set);

		TArray<FPHGenerationIssue> Issues;
		TestFalse(FString::Printf(TEXT("%s is rejected"), Label), Set->ValidateModuleSet(Issues));
		TestTrue(FString::Printf(TEXT("%s reports InvalidModuleSet"), Label),
			Issues.ContainsByPredicate([](const FPHGenerationIssue& Issue)
			{
				return Issue.Code == EPHGenerationIssueCode::InvalidModuleSet;
			}));
	};

	ExpectRejected(TEXT("A piece with no mesh"), [](UPHBiomeModuleSet& Set)
	{
		Set.Modules[PHGenerationTags::Piece_Floor.GetTag()].Mesh = nullptr;
	});

	// 150 is not a whole 100-unit module. This is the real trap in BlockingStarterPack, which
	// ships SM_Floor_200x150 alongside an otherwise 100-clean kit.
	ExpectRejected(TEXT("A footprint off the kit module"), [](UPHBiomeModuleSet& Set)
	{
		Set.Modules[PHGenerationTags::Piece_Floor.GetTag()].Footprint = FVector2D(200.0, 150.0);
	});

	ExpectRejected(TEXT("A non-positive footprint"), [](UPHBiomeModuleSet& Set)
	{
		Set.Modules[PHGenerationTags::Piece_Floor.GetTag()].Footprint = FVector2D(0.0, 100.0);
	});

	ExpectRejected(TEXT("A tag outside the Piece hierarchy"), [](UPHBiomeModuleSet& Set)
	{
		Set.Modules.Add(NonPieceTag, MakeEntry());
	});

	ExpectRejected(TEXT("The Piece root itself"), [](UPHBiomeModuleSet& Set)
	{
		Set.Modules.Add(PHGenerationTags::Piece.GetTag(), MakeEntry());
	});

	ExpectRejected(TEXT("An empty set"), [](UPHBiomeModuleSet& Set)
	{
		Set.Modules.Reset();
	});
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHBiomeModuleSetInheritanceTest,
	"ProjectHunter.Generation.Biome.ResolvesThroughAncestors", PHBiomeModuleSetTests::TestFlags)

bool FPHBiomeModuleSetInheritanceTest::RunTest(const FString&)
{
	using namespace PHBiomeModuleSetTests;
	TStrongObjectPtr<UPHBiomeModuleSet> Keeper;
	UPHBiomeModuleSet* Set = MakeSet(Keeper);

	// Piece.Wall.Corner is not authored, but Piece.Wall is: a kit may map a family once.
	Set->Modules.Add(PHGenerationTags::Piece_Wall.GetTag(), MakeEntry(FVector2D(200.0, 200.0)));

	FPHModuleEntry Corner;
	TestTrue(TEXT("An unauthored variant falls back to its family"),
		Set->ResolvePiece(PHGenerationTags::Piece_Wall_Corner.GetTag(), Corner));
	TestEqual(TEXT("The inherited entry is returned"), Corner.Footprint, FVector2D(200.0, 200.0));

	// An exact entry must still win over the ancestor.
	FPHModuleEntry Straight;
	TestTrue(TEXT("Straight wall still resolves"),
		Set->ResolvePiece(PHGenerationTags::Piece_Wall_Straight.GetTag(), Straight));
	TestEqual(TEXT("Exact authoring beats the ancestor"), Straight.Footprint, FVector2D(400.0, 100.0));

	FPHModuleEntry Missing;
	TestFalse(TEXT("An unrelated family does not resolve"),
		Set->ResolvePiece(PHGenerationTags::Piece_Stair.GetTag(), Missing));
	TestFalse(TEXT("A tag outside Piece does not resolve"),
		Set->ResolvePiece(NonPieceTag, Missing));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHBiomeModuleSetRequirementsTest,
	"ProjectHunter.Generation.Biome.ReportsMissingPieces", PHBiomeModuleSetTests::TestFlags)

bool FPHBiomeModuleSetRequirementsTest::RunTest(const FString&)
{
	using namespace PHBiomeModuleSetTests;
	TStrongObjectPtr<UPHBiomeModuleSet> Keeper;
	UPHBiomeModuleSet* Set = MakeSet(Keeper);

	// The consumer states its own needs; the set does not decide what a generator requires.
	TArray<FGameplayTag> Missing;
	TestTrue(TEXT("A set covering what is asked for reports nothing missing"),
		Set->HasPieces({ PHGenerationTags::Piece_Floor.GetTag(),
			PHGenerationTags::Piece_Wall_Straight.GetTag() }, Missing));

	TestFalse(TEXT("A kit lacking doors and stairs is reported"),
		Set->HasPieces({ PHGenerationTags::Piece_Floor.GetTag(),
			PHGenerationTags::Piece_Door.GetTag(),
			PHGenerationTags::Piece_Stair.GetTag() }, Missing));
	TestEqual(TEXT("Both absent pieces are named"), Missing.Num(), 2);
	TestTrue(TEXT("The door is named"), Missing.Contains(PHGenerationTags::Piece_Door.GetTag()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHBiomeRemovedTagTest,
	"ProjectHunter.Generation.Biome.RejectsRemovedTagsWithoutEnsure", PHBiomeModuleSetTests::TestFlags)

bool FPHBiomeRemovedTagTest::RunTest(const FString&)
{
	using namespace PHBiomeModuleSetTests;
	const FNameProperty* TagNameProperty = FindFProperty<FNameProperty>(FGameplayTag::StaticStruct(), TEXT("TagName"));
	if (!TestNotNull(TEXT("The fixture can write the serialized tag name"), TagNameProperty))
	{
		return false;
	}

	FGameplayTag StaleTag;
	TagNameProperty->SetPropertyValue_InContainer(&StaleTag, FName(TEXT("Piece.AutomationFixture.RemovedModule")));
	TestTrue(TEXT("A retained tag still has a name"), StaleTag.IsValid());
	TestFalse(TEXT("The retained tag is not in the registry"), UGameplayTagsManager::Get().FindTagNode(StaleTag).IsValid());

	TStrongObjectPtr<UPHBiomeModuleSet> Keeper;
	UPHBiomeModuleSet* Set = MakeSet(Keeper);
	Set->Modules.Add(StaleTag, MakeEntry());
	TArray<FPHGenerationIssue> Issues;
	TestFalse(TEXT("A removed tag is refused without an engine ensure"), Set->ValidateModuleSet(Issues));
	TestTrue(TEXT("A removed tag produces an ordinary InvalidModuleSet issue"), Issues.ContainsByPredicate(
		[](const FPHGenerationIssue& Issue) { return Issue.Code == EPHGenerationIssueCode::InvalidModuleSet; }));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHBiomeFiniteAuthoringTest,
	"ProjectHunter.Generation.Biome.RejectsNonfiniteAuthoring", PHBiomeModuleSetTests::TestFlags)

bool FPHBiomeFiniteAuthoringTest::RunTest(const FString&)
{
	using namespace PHBiomeModuleSetTests;
	const FGameplayTag PropTag = PHGenerationTags::Prop_Barrel.GetTag();

	auto MakePropSet = [&]()
	{
		UPHBiomeModuleSet* Set = NewObject<UPHBiomeModuleSet>();
		Set->Modules.Add(PropTag, MakeEntry(FVector2D(60.0, 60.0)));
		return Set;
	};
	TStrongObjectPtr<UPHBiomeModuleSet> Control(MakePropSet());
	TArray<FPHGenerationIssue> ControlIssues;
	TestTrue(TEXT("A prop-only set with a non-grid footprint is valid"), Control->ValidateModuleSet(ControlIssues));

	auto ExpectRejected = [&](const TCHAR* Label, const TFunction<void(UPHBiomeModuleSet&, FPHModuleEntry&)>& Mutate)
	{
		TStrongObjectPtr<UPHBiomeModuleSet> Keeper(MakePropSet());
		Mutate(*Keeper.Get(), Keeper->Modules[PropTag]);
		TArray<FPHGenerationIssue> Issues;
		TestFalse(Label, Keeper->ValidateModuleSet(Issues));
		TestTrue(FString::Printf(TEXT("%s reports InvalidModuleSet"), Label), Issues.ContainsByPredicate(
			[](const FPHGenerationIssue& Issue) { return Issue.Code == EPHGenerationIssueCode::InvalidModuleSet; }));
	};

	ExpectRejected(TEXT("NaN grid size"), [](UPHBiomeModuleSet& Set, FPHModuleEntry&)
	{
		Set.GridSize = std::numeric_limits<double>::quiet_NaN();
	});
	ExpectRejected(TEXT("Infinite grid size"), [](UPHBiomeModuleSet& Set, FPHModuleEntry&)
	{
		Set.GridSize = std::numeric_limits<double>::infinity();
	});
	ExpectRejected(TEXT("NaN prop width"), [](UPHBiomeModuleSet&, FPHModuleEntry& Entry)
	{
		Entry.Footprint.X = std::numeric_limits<double>::quiet_NaN();
	});
	ExpectRejected(TEXT("Infinite prop depth"), [](UPHBiomeModuleSet&, FPHModuleEntry& Entry)
	{
		Entry.Footprint.Y = std::numeric_limits<double>::infinity();
	});
	ExpectRejected(TEXT("NaN module height"), [](UPHBiomeModuleSet&, FPHModuleEntry& Entry)
	{
		Entry.Height = std::numeric_limits<double>::quiet_NaN();
	});
	ExpectRejected(TEXT("Infinite module height"), [](UPHBiomeModuleSet&, FPHModuleEntry& Entry)
	{
		Entry.Height = std::numeric_limits<double>::infinity();
	});
	ExpectRejected(TEXT("NaN module yaw"), [](UPHBiomeModuleSet&, FPHModuleEntry& Entry)
	{
		Entry.YawOffset = std::numeric_limits<double>::quiet_NaN();
	});
	ExpectRejected(TEXT("Infinite module yaw"), [](UPHBiomeModuleSet&, FPHModuleEntry& Entry)
	{
		Entry.YawOffset = std::numeric_limits<double>::infinity();
	});
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
