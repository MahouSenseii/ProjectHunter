// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/WorldSettings.h"
#include "Generation/Actors/PHGeneratedFloorActor.h"
#include "Generation/Generators/PHDungeonGenerator.h"
#include "Generation/Library/FunctionLibraries/PHEncounterPlanLibrary.h"
#include "Generation/PHGenerationTags.h"
#include "Interactable/Actors/Portal/PortalActor.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Tests/PHGenerationReviewTestActors.h"
#include "Tower/Library/FunctionLibraries/RunSeedFunctionLibrary.h"
#include "Tower/Subsystems/PortalSubsystem.h"
#include "Tower/Subsystems/RunSubsystem.h"
#include "UObject/StrongObjectPtr.h"

namespace PHGenerationRuntimeReviewTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	struct FFixture
	{
		FTestWorldWrapper TestWorld;
		TStrongObjectPtr<UPHBiomeModuleSet> Modules;
		APHGeneratedFloorActor* Floor = nullptr;

		bool Initialize(FAutomationTestBase& Test, const bool bBeginPlay = false)
		{
			if (!TestWorld.CreateTestWorld(bBeginPlay ? EWorldType::Game : EWorldType::GamePreview))
			{
				TestWorld.ForwardErrorMessages(&Test);
				return false;
			}
			if (bBeginPlay)
			{
				// A native empty game mode avoids the project's player, HUD and save startup paths.
				TestWorld.GetTestWorld()->GetWorldSettings()->DefaultGameMode = AGameModeBase::StaticClass();
				if (!TestWorld.BeginPlayInTestWorld())
				{
					TestWorld.ForwardErrorMessages(&Test);
					return false;
				}
			}

			Modules.Reset(NewObject<UPHBiomeModuleSet>());
			Modules->GridSize = 100.0;
			FPHModuleEntry Entry;
			Entry.Footprint = FVector2D(400.0, 400.0);
			Entry.Mesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(
				TEXT("/Game/BlockingStarterPack/Meshes/Architecture/Floors/SM_Floor_400x400.SM_Floor_400x400")));
			Modules->Modules.Add(PHGenerationTags::Piece_Floor, Entry);
			Entry.Height = 400.0;
			Entry.Mesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(
				TEXT("/Game/BlockingStarterPack/Meshes/Architecture/Walls/SM_Wall_400x400.SM_Wall_400x400")));
			Modules->Modules.Add(PHGenerationTags::Piece_Wall, Entry);
			return true;
		}

		void SpawnFloor(const bool bEncounters = false, const bool bPortal = false,
			const FTransform& Transform = FTransform::Identity)
		{
			FActorSpawnParameters Params;
			Params.bDeferConstruction = true;
			Params.ObjectFlags = RF_Transient;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			Floor = TestWorld.GetTestWorld()->SpawnActor<APHGeneratedFloorActor>(
				APHGeneratedFloorActor::StaticClass(), Transform, Params);
			Floor->ModuleSet = Modules.Get();
			Floor->bBuildCeiling = false;
			Floor->bBuildLighting = false;
			Floor->bBuildNavigation = false;
			Floor->bDecorate = false;
			Floor->bMarkEndpoints = false;
			Floor->bPlacePlayerStart = false;
			Floor->Request.MinRegionCount = 1;
			Floor->Request.MaxRegionCount = 1;
			Floor->Request.MinRegionSize = FVector2D(2000.0, 2000.0);
			Floor->Request.MaxRegionSize = Floor->Request.MinRegionSize;
			Floor->Request.MaxHeightStacks = 1;
			if (bEncounters)
			{
				Floor->EncounterManagerClass = APHGenerationReviewTestManager::StaticClass();
				Floor->EncounterMobClasses.Add(APHGenerationReviewTestCharacter::StaticClass());
				Floor->bSkipStartRegion = false;
				FPHAnchorRule& Rule = Floor->Request.AnchorRules.AddDefaulted_GetRef();
				Rule.SemanticTag = PHGenerationTags::Anchor_Enemy_Small;
				Rule.MinPerRegion = Rule.MaxPerRegion = 2;
				Rule.bAllowInStartRegion = Rule.bAllowInExitRegion = true;
			}
			if (bPortal)
			{
				Floor->ExitPortalClass = APortalActor::StaticClass();
			}
			Floor->FinishSpawning(Transform);
		}

		APHGenerationReviewTestManager* FindManager() const
		{
			for (TActorIterator<APHGenerationReviewTestManager> It(TestWorld.GetTestWorld()); It; ++It)
			{
				return *It;
			}
			return nullptr;
		}

		int32 CountInstances() const
		{
			TArray<UInstancedStaticMeshComponent*> Components;
			Floor->GetComponents(Components);
			int32 Count = 0;
			for (const UInstancedStaticMeshComponent* Component : Components)
			{
				Count += Component->GetInstanceCount();
			}
			return Count;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHGenerationFailedRebuildTest,
	"ProjectHunter.Generation.Runtime.FailedRebuildPreservesFloor", PHGenerationRuntimeReviewTests::Flags)
bool FPHGenerationFailedRebuildTest::RunTest(const FString&)
{
	using namespace PHGenerationRuntimeReviewTests;
	FFixture Fixture;
	if (!Fixture.Initialize(*this)) { return false; }
	Fixture.SpawnFloor();
	Fixture.Floor->Generate();
	const int32 Before = Fixture.CountInstances();
	const FTransform StartBefore = Fixture.Floor->LastPlayerStart;
	TestTrue(TEXT("The initial floor was actually built"), Before > 0);
	Fixture.Floor->Request.MinRegionCount = 3;
	Fixture.Floor->Request.MaxRegionCount = 1;
	Fixture.Floor->GenerateForSeed(42);
	TestEqual(TEXT("A rejected replacement retains the existing geometry"), Fixture.CountInstances(), Before);
	TestTrue(TEXT("A rejected replacement retains its valid start"),
		Fixture.Floor->LastPlayerStart.Equals(StartBefore));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHGenerationMissingPieceTest,
	"ProjectHunter.Generation.Runtime.MissingRequiredPiecePreservesFloor", PHGenerationRuntimeReviewTests::Flags)
bool FPHGenerationMissingPieceTest::RunTest(const FString&)
{
	using namespace PHGenerationRuntimeReviewTests;
	FFixture Fixture;
	if (!Fixture.Initialize(*this)) { return false; }
	Fixture.SpawnFloor();
	Fixture.Floor->Generate();
	const int32 Before = Fixture.CountInstances();
	TestTrue(TEXT("The initial complete floor was built"), Before > 0);
	Fixture.Modules->Modules.Remove(PHGenerationTags::Piece_Wall);
	Fixture.Floor->Generate();
	TestEqual(TEXT("A missing required wall must not replace the floor with partial geometry"),
		Fixture.CountInstances(), Before);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHGenerationRuntimeEncounterSetupTest,
	"ProjectHunter.Generation.Runtime.EncountersConfiguredBeforeBeginPlay", PHGenerationRuntimeReviewTests::Flags)
bool FPHGenerationRuntimeEncounterSetupTest::RunTest(const FString&)
{
	using namespace PHGenerationRuntimeReviewTests;
	FFixture Fixture;
	if (!Fixture.Initialize(*this, true)) { return false; }
	Fixture.SpawnFloor(true);
	Fixture.Floor->Generate();
	APHGenerationReviewTestManager* Manager = Fixture.FindManager();
	if (!TestNotNull(TEXT("The generated encounter owner exists"), Manager)) { return false; }
	TestTrue(TEXT("The runtime encounter really began play"), Manager->HasActorBegunPlay());
	TestEqual(TEXT("The requested budget is visible during BeginPlay"), Manager->BudgetAtBeginPlay, 2);
	TestEqual(TEXT("The configured classes are visible during BeginPlay"), Manager->MobTypesAtBeginPlay, 1);
	TestTrue(TEXT("The authored volume is visible during BeginPlay"),
		Manager->ExtentAtBeginPlay.Equals(FVector(800.0, 800.0, 200.0)));
	TestTrue(TEXT("The generation owner supplies a deterministic encounter seed before activation"),
		Manager->SeedAtBeginPlay != 0);
	TestEqual(TEXT("Configured runtime encounter auto-activation is retained"),
		Manager->ManagerState, EMobManagerState::Active);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHGenerationEncounterTransformTest,
	"ProjectHunter.Generation.Runtime.EncounterVolumesFollowFloorTransform", PHGenerationRuntimeReviewTests::Flags)
bool FPHGenerationEncounterTransformTest::RunTest(const FString&)
{
	using namespace PHGenerationRuntimeReviewTests;
	FFixture Fixture;
	if (!Fixture.Initialize(*this)) { return false; }
	const FTransform FloorTransform(FRotator(0.0, 90.0, 0.0), FVector(1200.0, -700.0, 80.0));
	Fixture.SpawnFloor(true, false, FloorTransform);
	Fixture.Floor->Generate();
	APHGenerationReviewTestManager* Manager = Fixture.FindManager();
	if (!TestNotNull(TEXT("An encounter owner was placed"), Manager)) { return false; }
	FPHGeneratedLayout Layout;
	FPHEncounterPlan Plan;
	TArray<FPHGenerationIssue> Issues;
	UPHDungeonGenerator* Generator = NewObject<UPHDungeonGenerator>();
	if (!TestTrue(TEXT("The same request replays"), Generator->GenerateLayout(Fixture.Floor->Request, Layout, Issues)) ||
		!TestTrue(TEXT("The encounter plan replays"), UPHEncounterPlanLibrary::BuildEncounterPlan(
			Layout, Fixture.Floor->EncounterInset, Plan, Issues)) ||
		!TestEqual(TEXT("The fixture contains one encounter region"), Plan.Placements.Num(), 1)) { return false; }
	const FVector Expected = FloorTransform.TransformPosition(Plan.Placements[0].SpawnBounds.GetCenter());
	TestTrue(TEXT("Encounter location follows the built floor into world space"),
		Manager->SpawnArea->GetComponentLocation().Equals(Expected, 0.01));
	TestTrue(TEXT("Encounter volume axes rotate with the floor"),
		Manager->SpawnArea->GetComponentQuat().Equals(FloorTransform.GetRotation(), 0.001));
	TestTrue(TEXT("The generated encounter is explicitly owned by this floor"), Manager->GetOwner() == Fixture.Floor);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHGenerationClearLifecycleTest,
	"ProjectHunter.Generation.Runtime.ClearReleasesMobsAndPublishedState", PHGenerationRuntimeReviewTests::Flags)
bool FPHGenerationClearLifecycleTest::RunTest(const FString&)
{
	using namespace PHGenerationRuntimeReviewTests;
	FFixture Fixture;
	if (!Fixture.Initialize(*this)) { return false; }
	Fixture.SpawnFloor(true);
	Fixture.Floor->Generate();
	APHGenerationReviewTestManager* Manager = Fixture.FindManager();
	if (!TestNotNull(TEXT("The encounter owner exists before cleanup"), Manager)) { return false; }
	FActorSpawnParameters Params;
	Params.bDeferConstruction = true;
	Params.ObjectFlags = RF_Transient;
	APHGenerationReviewTestCharacter* Mob = Fixture.TestWorld.GetTestWorld()->SpawnActor<APHGenerationReviewTestCharacter>(
		APHGenerationReviewTestCharacter::StaticClass(), FTransform::Identity, Params);
	if (!TestNotNull(TEXT("The unstarted native test pawn exists"), Mob)) { return false; }
	Manager->ActiveMobs.Add(Mob);
	Fixture.Floor->bLastBuildHadNavigation = true;
	Fixture.Floor->ClearBuilt();
	TestTrue(TEXT("Clearing a floor also clears its encounter's live mobs"), Mob->IsActorBeingDestroyed());
	TestEqual(TEXT("Clearing removes geometry"), Fixture.CountInstances(), 0);
	TestEqual(TEXT("Clearing resets enemy budget"), Fixture.Floor->LastEnemyBudget, 0);
	TestFalse(TEXT("Clearing invalidates navigation status"), Fixture.Floor->bLastBuildHadNavigation);
	TestTrue(TEXT("Clearing invalidates the old player start"), Fixture.Floor->LastPlayerStart.Equals(FTransform::Identity));
	TestTrue(TEXT("Clearing invalidates the old exit"), Fixture.Floor->LastExit.Equals(FTransform::Identity));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHGenerationFailedRunTest,
	"ProjectHunter.Generation.Runtime.FailedRunRequestRemainsGenerating", PHGenerationRuntimeReviewTests::Flags)
bool FPHGenerationFailedRunTest::RunTest(const FString&)
{
	using namespace PHGenerationRuntimeReviewTests;
	FFixture Fixture;
	if (!Fixture.Initialize(*this, true)) { return false; }
	Fixture.SpawnFloor();
	Fixture.Floor->Generate();
	const int32 Before = Fixture.CountInstances();
	URunSubsystem* Run = Fixture.TestWorld.GetTestWorld()->GetGameInstance()->GetSubsystem<URunSubsystem>();
	if (!TestNotNull(TEXT("The real run owner exists in the isolated native game instance"), Run)) { return false; }
	Fixture.Floor->Request.MinRegionCount = 3;
	Fixture.Floor->Request.MaxRegionCount = 1;
	Run->StartRun(20260830, 1);
	TestEqual(TEXT("Failed generation is not acknowledged as a completed empty floor"),
		Run->GetFloorPhase(), EFloorPhase::Generating);
	TestEqual(TEXT("The run request retains previous geometry when its replacement fails"), Fixture.CountInstances(), Before);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHGenerationPendingRunTest,
	"ProjectHunter.Generation.Runtime.PendingRunRequestHandledOnBeginPlay", PHGenerationRuntimeReviewTests::Flags)
bool FPHGenerationPendingRunTest::RunTest(const FString&)
{
	using namespace PHGenerationRuntimeReviewTests;
	FFixture Fixture;
	if (!Fixture.Initialize(*this, true)) { return false; }
	URunSubsystem* Run = Fixture.TestWorld.GetTestWorld()->GetGameInstance()->GetSubsystem<URunSubsystem>();
	if (!TestNotNull(TEXT("The run owner exists"), Run)) { return false; }
	Run->StartRun(20260830, 1);
	TestEqual(TEXT("The run is waiting for a generator"), Run->GetFloorPhase(), EFloorPhase::Generating);
	Fixture.SpawnFloor();
	TestTrue(TEXT("The new floor owner handles a request emitted before it existed"), Fixture.CountInstances() > 0);
	TestEqual(TEXT("The pending floor uses its owned layout seed"), Fixture.Floor->Request.Seed,
		URunSeedFunctionLibrary::DeriveLayoutSeed(Run->GetFloorSeed()));
	TestTrue(TEXT("Only successful construction acknowledges the pending floor"),
		Run->GetFloorPhase() != EFloorPhase::Generating);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHGenerationPortalRegistrationTest,
	"ProjectHunter.Generation.Runtime.GeneratedPortalRegistersDuringBeginPlay", PHGenerationRuntimeReviewTests::Flags)
bool FPHGenerationPortalRegistrationTest::RunTest(const FString&)
{
	using namespace PHGenerationRuntimeReviewTests;
	FFixture Fixture;
	if (!Fixture.Initialize(*this, true)) { return false; }
	Fixture.SpawnFloor(false, true);
	Fixture.Floor->Generate();
	APortalActor* Portal = nullptr;
	for (TActorIterator<APortalActor> It(Fixture.TestWorld.GetTestWorld()); It; ++It) { Portal = *It; break; }
	UPortalSubsystem* Registry = Fixture.TestWorld.GetTestWorld()->GetSubsystem<UPortalSubsystem>();
	if (!TestNotNull(TEXT("The generated portal exists"), Portal) ||
		!TestNotNull(TEXT("The native portal registry exists"), Registry)) { return false; }
	TestTrue(TEXT("The portal began play"), Portal->HasActorBegunPlay());
	TestTrue(TEXT("Its final identity was registered during BeginPlay"), Registry->FindPortal(Portal->PortalID) == Portal);
	Fixture.Floor->ClearBuilt();
	TestTrue(TEXT("Clearing unregisters the generated portal"), Registry->GetAllPortals().IsEmpty());
	return true;
}

#endif
