// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#include "CoreMinimal.h"

#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "AbilitySystem/HunterAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Misc/AutomationTest.h"
#include "Progression/Components/CharacterProgressionManager.h"
#include "Progression/Components/PHPassiveTreeComponent.h"
#include "Progression/Data/PHPassiveTreeDataAsset.h"
#include "Progression/Settings/PHPassiveTreeSettings.h"
#include "Tags/PHGameplayTags.h"
#include "Tests/AutomationCommon.h"

namespace PHPassiveTreeTests
{
	constexpr EAutomationTestFlags TestFlags =
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;
	constexpr float Tolerance = 0.001f;

	FPHPassiveModifier MakeModifier(const FGameplayTag AttributeTag, const float Magnitude)
	{
		FPHPassiveModifier Modifier;
		Modifier.AttributeTag = AttributeTag;
		Modifier.Magnitude = Magnitude;
		return Modifier;
	}

	FPHPassiveNodeDefinition MakeNode(
		const FName ID,
		const FText& Name,
		const FGameplayTag AttributeTag,
		const float Magnitude,
		const int32 Cost = 1,
		const TArray<FName>& Requirements = {})
	{
		FPHPassiveNodeDefinition Node;
		Node.NodeID = ID;
		Node.DisplayName = Name;
		Node.Description = FText::Format(NSLOCTEXT("PHPassiveTreeTests", "Grant", "Grants {0}."), FText::AsNumber(Magnitude));
		Node.PointCost = Cost;
		Node.RequiredNodeIDs = Requirements;
		Node.Modifiers.Add(MakeModifier(AttributeTag, Magnitude));
		return Node;
	}

	UPHPassiveTreeDataAsset* MakeTree()
	{
		UPHPassiveTreeDataAsset* Tree = NewObject<UPHPassiveTreeDataAsset>();
		Tree->TreeID = TEXT("HunterPathsTest");
		Tree->DisplayName = NSLOCTEXT("PHPassiveTreeTests", "Name", "Hunter Paths Test");
		Tree->Nodes.Add(MakeNode(
			TEXT("Awakening"), NSLOCTEXT("PHPassiveTreeTests", "Awakening", "Hunter Awakening"),
			FPHGameplayTags::Attributes_Primary_Strength, 2.0f));
		Tree->Nodes.Add(MakeNode(
			TEXT("Unbroken"), NSLOCTEXT("PHPassiveTreeTests", "Unbroken", "Unbroken Hunter"),
			FPHGameplayTags::Attributes_Primary_Endurance, 5.0f, 2, {TEXT("Awakening")}));
		Tree->Nodes.Last().NodeSize = EPHPassiveNodeSize::Major;
		return Tree;
	}

	struct FFixture
	{
		FTestWorldWrapper TestWorld;
		AActor* Owner = nullptr;
		UAbilitySystemComponent* ASC = nullptr;
		UHunterAttributeSet* Attributes = nullptr;
		UCharacterProgressionManager* Progression = nullptr;
		UPHPassiveTreeComponent* Passives = nullptr;

		bool Initialize(FAutomationTestBase& Test, UPHPassiveTreeDataAsset* Tree)
		{
			if (!TestWorld.CreateTestWorld(EWorldType::GamePreview))
			{
				TestWorld.ForwardErrorMessages(&Test);
				return false;
			}

			Owner = TestWorld.GetTestWorld()->SpawnActor<AActor>();
			if (!Test.TestNotNull(TEXT("The fixture spawned an authority actor"), Owner))
			{
				return false;
			}

			ASC = NewObject<UAbilitySystemComponent>(Owner);
			Owner->AddInstanceComponent(ASC);
			ASC->RegisterComponent();
			Attributes = NewObject<UHunterAttributeSet>(Owner);
			ASC->AddAttributeSetSubobject(Attributes);
			ASC->InitAbilityActorInfo(Owner, Owner);

			Progression = NewObject<UCharacterProgressionManager>(Owner);
			Owner->AddInstanceComponent(Progression);
			Progression->RegisterComponent();

			Passives = NewObject<UPHPassiveTreeComponent>(Owner);
			// Set before registering: BeginPlay may fire from RegisterComponent, and it reads both of
			// these to decide whether to roll an origin.
			Passives->TreeDataOverride = TSoftObjectPtr<UPHPassiveTreeDataAsset>(Tree);
			Passives->bAutoRollRandomStartOnBeginPlay = false;
			Owner->AddInstanceComponent(Passives);
			Passives->RegisterComponent();

			return Test.TestTrue(TEXT("The fixture actor has authority"), Owner->HasAuthority()) &&
				Test.TestTrue(TEXT("The passive owner resolves its authored tree"), Passives->GetTreeData() == Tree);
		}
	};

	/** A Small-only chain plus one Major, so a roll has several legal answers and one illegal one. */
	UPHPassiveTreeDataAsset* MakeChainTree()
	{
		UPHPassiveTreeDataAsset* Tree = NewObject<UPHPassiveTreeDataAsset>();
		Tree->TreeID = TEXT("HunterPathsChain");
		Tree->DisplayName = NSLOCTEXT("PHPassiveTreeTests", "ChainName", "Hunter Paths Chain");
		Tree->Nodes.Add(MakeNode(
			TEXT("Root"), NSLOCTEXT("PHPassiveTreeTests", "Root", "Root"),
			FPHGameplayTags::Attributes_Primary_Strength, 1.0f));
		Tree->Nodes.Add(MakeNode(
			TEXT("Mid"), NSLOCTEXT("PHPassiveTreeTests", "Mid", "Mid"),
			FPHGameplayTags::Attributes_Primary_Dexterity, 1.0f, 1, {TEXT("Root")}));
		Tree->Nodes.Add(MakeNode(
			TEXT("Leaf"), NSLOCTEXT("PHPassiveTreeTests", "Leaf", "Leaf"),
			FPHGameplayTags::Attributes_Primary_Endurance, 1.0f, 1, {TEXT("Mid")}));
		Tree->Nodes.Add(MakeNode(
			TEXT("Milestone"), NSLOCTEXT("PHPassiveTreeTests", "Milestone", "Milestone"),
			FPHGameplayTags::Attributes_Primary_Intelligence, 5.0f, 2, {TEXT("Leaf")}));
		Tree->Nodes.Last().NodeSize = EPHPassiveNodeSize::Major;
		return Tree;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHPassiveTreeDataValidationTest,
	"ProjectHunter.Progression.PassiveTree.DataValidation", PHPassiveTreeTests::TestFlags)

bool FPHPassiveTreeDataValidationTest::RunTest(const FString&)
{
	using namespace PHPassiveTreeTests;
	UPHPassiveTreeDataAsset* Tree = MakeTree();
	TArray<FText> Errors;
	TestTrue(TEXT("A connected small/major Hunter tree validates"), Tree->ValidateTree(Errors));
	TestEqual(TEXT("The valid tree has no diagnostics"), Errors.Num(), 0);
	TestTrue(TEXT("Search finds display names"), Tree->DoesNodeMatchSearch(Tree->Nodes[1], TEXT("unbroken")));
	TestTrue(TEXT("Search finds descriptions"), Tree->DoesNodeMatchSearch(Tree->Nodes[0], TEXT("grants")));
	TestFalse(TEXT("Unrelated search terms do not match"), Tree->DoesNodeMatchSearch(Tree->Nodes[0], TEXT("constellation")));

	// Connections are authored one way but read both ways, which is what lets a node deep in a branch
	// open the one above it.
	const TArray<FName>* RootNeighbours = Tree->FindNeighbours(TEXT("Awakening"));
	TestTrue(TEXT("A connection is visible from the end that did not author it"),
		RootNeighbours && RootNeighbours->Contains(TEXT("Unbroken")));

	// A loop is legal under RequireAny - it is how a cluster gets approached from two directions -
	// so the third node closes one and the graph must still validate.
	Tree->Nodes.Add(MakeNode(
		TEXT("Loop"), NSLOCTEXT("PHPassiveTreeTests", "Loop", "Loop Node"),
		FPHGameplayTags::Attributes_Primary_Dexterity, 1.0f, 1, {TEXT("Awakening"), TEXT("Unbroken")}));
	Tree->InvalidateNodeIndex();
	Errors.Reset();
	TestTrue(TEXT("RequireAny accepts a loop in the graph"), Tree->ValidateTree(Errors));

	// Under RequireAny the authoring mistake that matters is an island: a pair joined only to each
	// other, which no origin - authored or rolled - can ever grow into.
	Tree->Nodes.Add(MakeNode(
		TEXT("IslandA"), NSLOCTEXT("PHPassiveTreeTests", "IslandA", "Island A"),
		FPHGameplayTags::Attributes_Primary_Luck, 1.0f, 1, {TEXT("IslandB")}));
	Tree->Nodes.Add(MakeNode(
		TEXT("IslandB"), NSLOCTEXT("PHPassiveTreeTests", "IslandB", "Island B"),
		FPHGameplayTags::Attributes_Primary_Luck, 1.0f, 1, {TEXT("IslandA")}));
	Tree->InvalidateNodeIndex();
	Errors.Reset();
	TestFalse(TEXT("RequireAny rejects a disconnected island"), Tree->ValidateTree(Errors));
	TestTrue(TEXT("The island produces a useful diagnostic"), Errors.ContainsByPredicate([](const FText& Error)
	{
		return Error.ToString().Contains(TEXT("island"), ESearchCase::IgnoreCase);
	}));
	Tree->Nodes.SetNum(Tree->Nodes.Num() - 2);
	Tree->InvalidateNodeIndex();

	// Same graph under the directed rule: pointing a root back into the loop closes a true cycle.
	Tree->ConnectionRule = EPHPassiveConnectionRule::RequireAll;
	Tree->Nodes[0].RequiredNodeIDs = {TEXT("Loop")};
	Tree->InvalidateNodeIndex();
	Errors.Reset();
	TestFalse(TEXT("RequireAll rejects a prerequisite cycle"), Tree->ValidateTree(Errors));
	TestTrue(TEXT("The cycle produces a useful diagnostic"), Errors.ContainsByPredicate([](const FText& Error)
	{
		return Error.ToString().Contains(TEXT("cycle"), ESearchCase::IgnoreCase);
	}));

	Tree->ConnectionRule = EPHPassiveConnectionRule::RequireAny;
	Tree->Nodes[0].RequiredNodeIDs.Reset();
	Tree->Nodes[1].RequiredNodeIDs = {TEXT("MissingNode")};
	Tree->InvalidateNodeIndex();
	Errors.Reset();
	TestFalse(TEXT("A missing connection is rejected"), Tree->ValidateTree(Errors));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHPassiveTreeRandomStartRollTest,
	"ProjectHunter.Progression.PassiveTree.RandomStartRollsSmallNodesOnly", PHPassiveTreeTests::TestFlags)

bool FPHPassiveTreeRandomStartRollTest::RunTest(const FString&)
{
	using namespace PHPassiveTreeTests;
	UPHPassiveTreeDataAsset* Tree = MakeChainTree();

	TArray<FName> Candidates;
	Tree->GatherRandomStartCandidates(Candidates);
	TestEqual(TEXT("Only the three Small nodes are eligible"), Candidates.Num(), 3);
	TestFalse(TEXT("The Major milestone is never a starting node"), Candidates.Contains(TEXT("Milestone")));

	// Across many seeds the roll must stay inside the eligible set and must actually vary, or it is
	// not producing different builds.
	TSet<FName> Seen;
	for (int32 Seed = 1; Seed <= 200; ++Seed)
	{
		const FName Picked = Tree->PickRandomStart(Seed);
		if (!TestTrue(TEXT("Every roll lands on an eligible Small node"), Candidates.Contains(Picked)))
		{
			return false;
		}
		Seen.Add(Picked);
	}
	TestEqual(TEXT("Every eligible node is reachable by some seed"), Seen.Num(), 3);

	TestTrue(TEXT("One seed always yields the same origin"),
		Tree->PickRandomStart(4242) == Tree->PickRandomStart(4242));

	Tree->RandomStart.ExcludedNodeIDs = {TEXT("Leaf")};
	Tree->GatherRandomStartCandidates(Candidates);
	TestFalse(TEXT("An excluded node is never eligible"), Candidates.Contains(TEXT("Leaf")));

	Tree->RandomStart.ExcludedNodeIDs.Reset();
	Tree->RandomStart.bEnabled = false;
	TestTrue(TEXT("Disabling random starts yields no origin"), Tree->PickRandomStart(7).IsNone());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHPassiveTreeRandomStartGrantTest,
	"ProjectHunter.Progression.PassiveTree.RandomStartGrantsAndOpensBothWays", PHPassiveTreeTests::TestFlags)

bool FPHPassiveTreeRandomStartGrantTest::RunTest(const FString&)
{
	using namespace PHPassiveTreeTests;
	FFixture Fixture;
	if (!Fixture.Initialize(*this, MakeChainTree()))
	{
		return false;
	}

	Fixture.Progression->UnspentPassivePoints = 3;

	// Deliberately the deepest Small node. Its connection points *up* the chain, so this is the case
	// that a directed reading of the graph would strand.
	TestTrue(TEXT("The origin is granted"), Fixture.Passives->SetRandomStart(TEXT("Leaf")));
	TestTrue(TEXT("The origin counts as allocated"), Fixture.Passives->IsNodeAllocated(TEXT("Leaf")));
	TestTrue(TEXT("The origin is flagged as the random start"), Fixture.Passives->IsRandomStartNode(TEXT("Leaf")));
	TestEqual(TEXT("The origin costs no passive points"), Fixture.Progression->UnspentPassivePoints, 3);
	TestEqual(TEXT("The origin applies its attribute"), Fixture.Attributes->GetEndurance(), 1.0f, Tolerance);

	FText Reason;
	TestTrue(TEXT("The node above the origin opens, because connections read both ways"),
		Fixture.Passives->CanAllocateNode(TEXT("Mid"), Reason));
	TestFalse(TEXT("A node two steps away stays locked"),
		Fixture.Passives->CanAllocateNode(TEXT("Root"), Reason));

	// With an origin rolled, the authored root stops being a free entry - otherwise every character
	// would still open in the same place and the roll would not change the build.
	TestTrue(TEXT("The chain walks upward one node at a time"), Fixture.Passives->AllocateNode(TEXT("Mid")));
	TestTrue(TEXT("The authored root is now reachable"),
		Fixture.Passives->CanAllocateNode(TEXT("Root"), Reason));
	TestTrue(TEXT("The Major milestone below the origin also opens"),
		Fixture.Passives->CanAllocateNode(TEXT("Milestone"), Reason));

	TestTrue(TEXT("A second roll is refused once an origin exists"),
		Fixture.Passives->RollRandomStart(99) == FName(TEXT("Leaf")));

	// A reload must not hand the character a different origin.
	Fixture.Passives->RestorePassiveState({TEXT("Leaf"), TEXT("Mid")}, TEXT("Leaf"));
	TestTrue(TEXT("Restore keeps the saved origin"), Fixture.Passives->IsRandomStartNode(TEXT("Leaf")));
	TestEqual(TEXT("Restore keeps both owned nodes"), Fixture.Passives->AllocatedNodeIDs.Num(), 2);
	TestEqual(TEXT("Restore spends no points"), Fixture.Progression->UnspentPassivePoints, 2);
	TestEqual(TEXT("Restored origin attribute is applied once"), Fixture.Attributes->GetEndurance(), 1.0f, Tolerance);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHPassiveTreeAuthoredAssetTest,
	"ProjectHunter.Progression.PassiveTree.AuthoredHunterPaths", PHPassiveTreeTests::TestFlags)

bool FPHPassiveTreeAuthoredAssetTest::RunTest(const FString&)
{
	const UPHPassiveTreeDataAsset* Tree = GetDefault<UPHPassiveTreeSettings>()->DefaultTree.LoadSynchronous();
	if (!TestNotNull(TEXT("DefaultGame resolves the authored Hunter Paths asset"), Tree))
	{
		return false;
	}

	TArray<FText> Errors;
	TestTrue(TEXT("The authored Hunter Paths graph is connected and valid"), Tree->ValidateTree(Errors));
	for (const FText& Error : Errors)
	{
		AddError(Error.ToString());
	}
	TestTrue(TEXT("The authored Hunter Paths graph remains meaningfully expanded"), Tree->Nodes.Num() >= 16);
	int32 MajorCount = 0;
	for (const FPHPassiveNodeDefinition& Node : Tree->Nodes)
	{
		MajorCount += Node.NodeSize == EPHPassiveNodeSize::Major ? 1 : 0;
	}
	TestTrue(TEXT("The expanded graph keeps multiple visible milestones"), MajorCount >= 4);
	TestNotNull(TEXT("The stable starting node exists"), Tree->FindNode(TEXT("HunterAwakening")));
	TestNotNull(TEXT("The Vanguard milestone exists"), Tree->FindNode(TEXT("Gatebreaker")));
	TestNotNull(TEXT("The Tracker milestone exists"), Tree->FindNode(TEXT("ApexTracker")));
	TestNotNull(TEXT("The Gate Scholar milestone exists"), Tree->FindNode(TEXT("WorldReader")));
	TestNotNull(TEXT("The Vanguard terminal milestone exists"), Tree->FindNode(TEXT("LastBastion")));
	TestNotNull(TEXT("The Tracker terminal milestone exists"), Tree->FindNode(TEXT("FinalMark")));
	TestNotNull(TEXT("The Gate Scholar terminal milestone exists"), Tree->FindNode(TEXT("GateboundMind")));
	return Errors.IsEmpty();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHPassiveTreeAllocationTest,
	"ProjectHunter.Progression.PassiveTree.AllocationAppliesRealAttributes", PHPassiveTreeTests::TestFlags)

bool FPHPassiveTreeAllocationTest::RunTest(const FString&)
{
	using namespace PHPassiveTreeTests;
	FFixture Fixture;
	if (!Fixture.Initialize(*this, MakeTree()))
	{
		return false;
	}

	Fixture.Progression->UnspentPassivePoints = 3;
	FText Reason;
	TestFalse(TEXT("A major node stays locked until its path is allocated"),
		Fixture.Passives->CanAllocateNode(TEXT("Unbroken"), Reason));
	TestTrue(TEXT("The locked reason mentions the path"), Reason.ToString().Contains(TEXT("path"), ESearchCase::IgnoreCase));
	TestEqual(TEXT("Strength starts at zero"), Fixture.Attributes->GetStrength(), 0.0f, Tolerance);
	TestTrue(TEXT("The starting node allocates"), Fixture.Passives->AllocateNode(TEXT("Awakening")));
	TestEqual(TEXT("The starting node spends exactly one passive point"), Fixture.Progression->UnspentPassivePoints, 2);
	TestEqual(TEXT("The starting node changes the live GAS attribute"), Fixture.Attributes->GetStrength(), 2.0f, Tolerance);
	TestFalse(TEXT("The same stable node cannot be allocated twice"), Fixture.Passives->AllocateNode(TEXT("Awakening")));
	TestEqual(TEXT("A duplicate request does not spend another point"), Fixture.Progression->UnspentPassivePoints, 2);

	TestTrue(TEXT("The connected major node allocates after its requirement"), Fixture.Passives->AllocateNode(TEXT("Unbroken")));
	TestEqual(TEXT("The major node spends its authored two-point cost"), Fixture.Progression->UnspentPassivePoints, 0);
	TestEqual(TEXT("The major node changes the live Endurance attribute"), Fixture.Attributes->GetEndurance(), 5.0f, Tolerance);
	TestEqual(TEXT("Both stable allocations are owned"), Fixture.Passives->AllocatedNodeIDs.Num(), 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHPassiveTreeRestoreTest,
	"ProjectHunter.Progression.PassiveTree.RestoreIsIdempotent", PHPassiveTreeTests::TestFlags)

bool FPHPassiveTreeRestoreTest::RunTest(const FString&)
{
	using namespace PHPassiveTreeTests;
	FFixture Fixture;
	if (!Fixture.Initialize(*this, MakeTree()))
	{
		return false;
	}

	Fixture.Progression->UnspentPassivePoints = 7;
	AddExpectedError(TEXT("Ignoring saved passive 'Missing'"), EAutomationExpectedErrorFlags::Contains, 1);
	Fixture.Passives->RestoreAllocatedNodes({TEXT("Unbroken"), TEXT("Missing"), TEXT("Awakening")});
	TestEqual(TEXT("Restore ignores an obsolete save ID"), Fixture.Passives->AllocatedNodeIDs.Num(), 2);
	TestEqual(TEXT("Restore does not spend already-paid points"), Fixture.Progression->UnspentPassivePoints, 7);
	TestEqual(TEXT("Restored Strength is applied once"), Fixture.Attributes->GetStrength(), 2.0f, Tolerance);
	TestEqual(TEXT("Restored Endurance is applied once"), Fixture.Attributes->GetEndurance(), 5.0f, Tolerance);

	Fixture.Passives->RefreshPassiveEffects();
	TestEqual(TEXT("Effect refresh does not stack Strength"), Fixture.Attributes->GetStrength(), 2.0f, Tolerance);
	TestEqual(TEXT("Effect refresh does not stack Endurance"), Fixture.Attributes->GetEndurance(), 5.0f, Tolerance);
	return true;
}

#endif
