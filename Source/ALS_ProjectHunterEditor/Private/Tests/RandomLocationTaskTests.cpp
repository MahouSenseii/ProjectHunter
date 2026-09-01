// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "AI/ALS_BTTask_GetRandomLocation.h"
#include "AI/NavigationSystemBase.h"
#include "AI/NavigationSystemConfig.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Misc/AutomationTest.h"
#include "Navigation/NavFilter_AIControllerDefault.h"
#include "NavigationData.h"
#include "NavigationSystem.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "NavMesh/RecastNavMesh.h"
#include "Tests/AutomationCommon.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UObjectHash.h"

namespace PHRandomLocationTaskTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;
	const FVector PreviousDestination(12345.0, 23456.0, 34567.0);

	struct FFixture
	{
		FTestWorldWrapper TestWorld;
		TStrongObjectPtr<UBehaviorTree> Tree;
		TStrongObjectPtr<UBlackboardData> BlackboardAsset;
		UALS_BTTask_GetRandomLocation* Task = nullptr;
		AAIController* Controller = nullptr;
		ACharacter* Pawn = nullptr;
		UBehaviorTreeComponent* Behavior = nullptr;
		UBlackboardComponent* Blackboard = nullptr;

		bool Initialize(FAutomationTestBase& Test, const EWorldType::Type WorldType)
		{
			UBehaviorTree* AuthoredTree = LoadObject<UBehaviorTree>(nullptr, TEXT(
				"/ALSV4_CPP/AdvancedLocomotionV4/Blueprints/CharacterLogic/AI/ALS_BT_AICharacter.ALS_BT_AICharacter"));
			if (!Test.TestNotNull(TEXT("Authored ALS behavior tree loads"), AuthoredTree) ||
				!Test.TestNotNull(TEXT("Authored ALS blackboard exists"), AuthoredTree->BlackboardAsset.Get()))
			{
				return false;
			}

			Tree.Reset(DuplicateObject(AuthoredTree, GetTransientPackage(),
				MakeUniqueObjectName(GetTransientPackage(), UBehaviorTree::StaticClass())));
			BlackboardAsset.Reset(DuplicateObject(AuthoredTree->BlackboardAsset.Get(), GetTransientPackage(),
				MakeUniqueObjectName(GetTransientPackage(), UBlackboardData::StaticClass())));
			Tree->BlackboardAsset = BlackboardAsset.Get();
			TArray<UObject*> TreeObjects;
			GetObjectsWithOuter(Tree.Get(), TreeObjects, true);
			for (UObject* Object : TreeObjects)
			{
				if (UALS_BTTask_GetRandomLocation* Candidate = Cast<UALS_BTTask_GetRandomLocation>(Object))
				{
					Task = Candidate;
					break;
				}
			}
			if (!Test.TestNotNull(TEXT("Authored tree contains the real random-location task"), Task)) { return false; }
			Task->InitializeFromAsset(*Tree);
			const FBlackboard::FKey KeyID = BlackboardAsset->GetKeyID(Task->GetSelectedBlackboardKey());
			if (!Test.TestTrue(TEXT("Authored destination is a vector key"),
				BlackboardAsset->GetKeyType(KeyID) == UBlackboardKeyType_Vector::StaticClass())) { return false; }

			if (!TestWorld.CreateTestWorld(WorldType))
			{
				TestWorld.ForwardErrorMessages(&Test);
				return false;
			}
			UWorld* World = TestWorld.GetTestWorld();
			if (!Test.TestNotNull(TEXT("Native AI system initializes the blackboard"), World->CreateAISystem())) { return false; }
			FActorSpawnParameters Spawn;
			Spawn.ObjectFlags = RF_Transient;
			Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			Controller = World->SpawnActor<AAIController>(AAIController::StaticClass(), FTransform::Identity, Spawn);
			Pawn = World->SpawnActor<ACharacter>(ACharacter::StaticClass(),
				FTransform(FVector(0.0, 0.0, 100.0)), Spawn);
			if (!Test.TestNotNull(TEXT("Native controller spawns"), Controller) ||
				!Test.TestNotNull(TEXT("Native pawn spawns"), Pawn)) { return false; }
			Controller->Possess(Pawn);
			Behavior = NewObject<UBehaviorTreeComponent>(Controller);
			Behavior->RegisterComponent();
			return Test.TestTrue(TEXT("Behavior component has the real controller"), Behavior->GetAIOwner() == Controller) &&
				Test.TestTrue(TEXT("Controller possesses the native pawn"), Controller->GetPawn() == Pawn);
		}

		bool BindBlackboard(FAutomationTestBase& Test)
		{
			if (!Test.TestTrue(TEXT("Controller initializes the copied blackboard"),
				Controller->UseBlackboard(BlackboardAsset.Get(), Blackboard))) { return false; }
			Behavior->CacheBlackboardComponent(Blackboard);
			Blackboard->SetValueAsVector(Task->GetSelectedBlackboardKey(), PreviousDestination);
			return true;
		}

		UNavigationSystemV1* AddNavigation(const bool bInitialize)
		{
			UNavigationSystemConfig* Config = NewObject<UNavigationSystemConfig>();
			Config->NavigationSystemClass = FSoftClassPath(UNavigationSystemV1::StaticClass());
			FNavigationSystem::AddNavigationSystemToWorld(*TestWorld.GetTestWorld(),
				FNavigationSystemRunMode::EditorMode, Config, bInitialize);
			return FNavigationSystem::GetCurrent<UNavigationSystemV1>(TestWorld.GetTestWorld());
		}

		UNavigationSystemV1* BuildNavigation(FAutomationTestBase& Test)
		{
			UWorld* World = TestWorld.GetTestWorld();
			AActor* Floor = World->SpawnActor<AActor>();
			UStaticMeshComponent* FloorMesh = NewObject<UStaticMeshComponent>(Floor);
			Floor->SetRootComponent(FloorMesh);
			FloorMesh->SetMobility(EComponentMobility::Static);
			FloorMesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
			if (!Test.TestNotNull(TEXT("Native cube provides real navigation geometry"), FloorMesh->GetStaticMesh().Get())) { return nullptr; }
			FloorMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			FloorMesh->SetCollisionResponseToAllChannels(ECR_Block);
			FloorMesh->SetWorldScale3D(FVector(20.0, 20.0, 1.0));
			FloorMesh->SetWorldLocation(FVector(0.0, 0.0, -50.0));
			FloorMesh->RegisterComponent();

			ANavMeshBoundsVolume* Bounds = World->SpawnActor<ANavMeshBoundsVolume>();
			UBoxComponent* BoundsBox = NewObject<UBoxComponent>(Bounds);
			BoundsBox->SetupAttachment(Bounds->GetRootComponent());
			BoundsBox->SetMobility(EComponentMobility::Static);
			BoundsBox->SetBoxExtent(FVector(1000.0, 1000.0, 500.0));
			BoundsBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			BoundsBox->SetCanEverAffectNavigation(false);
			BoundsBox->RegisterComponent();
			// Navigation bounds use all registered primitive bounds, so no editor BSP brush is needed.
			UNavigationSystemV1* NavSystem = AddNavigation(true);
			if (!Test.TestNotNull(TEXT("Native navigation system exists"), NavSystem)) { return nullptr; }
			NavSystem->Build(); // The engine waits for all navigation build work here.
			return NavSystem;
		}

		FVector Destination() const
		{
			return Blackboard->GetValueAsVector(Task->GetSelectedBlackboardKey());
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHRandomLocationTaskNavigationTest,
	"ProjectHunter.AI.RandomLocation.ControllerMetaFilterAndDefaultNavigation", PHRandomLocationTaskTests::Flags)

bool FPHRandomLocationTaskNavigationTest::RunTest(const FString& Parameters)
{
	using namespace PHRandomLocationTaskTests;
	FFixture Fixture;
	// An isolated editor world supports a static Recast build without modifying runtime generation settings.
	if (!Fixture.Initialize(*this, EWorldType::Editor) || !Fixture.BindBlackboard(*this)) { return false; }
	UNavigationSystemV1* NavSystem = Fixture.BuildNavigation(*this);
	if (!NavSystem) { return false; }
	ANavigationData* NavData = NavSystem->GetDefaultNavDataInstance(FNavigationSystem::DontCreate);
	if (!TestNotNull(TEXT("Real Recast navigation was built"), Cast<ARecastNavMesh>(NavData))) { return false; }
	FNavLocation Start;
	if (!TestTrue(TEXT("Pawn is over the built navigation surface"), NavSystem->ProjectPointToNavigation(
		Fixture.Pawn->GetActorLocation(), Start, FVector(100.0, 100.0, 200.0), NavData))) { return false; }

	Fixture.Task->Filter = UNavFilter_AIControllerDefault::StaticClass();
	TestEqual(TEXT("Controller meta filter executes without an ensure"),
		Fixture.Task->ExecuteTask(*Fixture.Behavior, nullptr), EBTNodeResult::Succeeded);
	TestFalse(TEXT("Task writes the selected blackboard vector"), Fixture.Destination().Equals(PreviousDestination));
	TestFalse(TEXT("Written destination is finite"), Fixture.Destination().ContainsNaN());
	TestTrue(TEXT("Written destination is reachable on the actual navmesh"), NavSystem->TestPathSync(
		FPathFindingQuery(Fixture.Controller, *NavData, Start.Location, Fixture.Destination())));

	Fixture.Blackboard->SetValueAsVector(Fixture.Task->GetSelectedBlackboardKey(), PreviousDestination);
	Fixture.Task->Filter = nullptr;
	TestEqual(TEXT("Unassigned filter retains default navigation behavior"),
		Fixture.Task->ExecuteTask(*Fixture.Behavior, nullptr), EBTNodeResult::Succeeded);
	TestFalse(TEXT("Default navigation also writes a destination"), Fixture.Destination().Equals(PreviousDestination));
	TestTrue(TEXT("Default-filter destination is reachable"), NavSystem->TestPathSync(
		FPathFindingQuery(Fixture.Controller, *NavData, Start.Location, Fixture.Destination())));

	Fixture.Blackboard->SetValueAsVector(Fixture.Task->GetSelectedBlackboardKey(), PreviousDestination);
	Fixture.Pawn->SetActorLocation(FVector(10000.0, 10000.0, 100.0));
	Fixture.Task->Filter = UNavFilter_AIControllerDefault::StaticClass();
	TestEqual(TEXT("A pawn outside navigation fails the query safely"),
		Fixture.Task->ExecuteTask(*Fixture.Behavior, nullptr), EBTNodeResult::Failed);
	TestTrue(TEXT("Failed query preserves the existing destination"), Fixture.Destination().Equals(PreviousDestination));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHRandomLocationTaskPrerequisiteTest,
	"ProjectHunter.AI.RandomLocation.MissingPrerequisitesFailSafely", PHRandomLocationTaskTests::Flags)

bool FPHRandomLocationTaskPrerequisiteTest::RunTest(const FString& Parameters)
{
	using namespace PHRandomLocationTaskTests;
	FFixture Fixture;
	if (!Fixture.Initialize(*this, EWorldType::GamePreview)) { return false; }
	Fixture.Task->Filter = UNavFilter_AIControllerDefault::StaticClass();
	TStrongObjectPtr<UBehaviorTreeComponent> UnownedBehavior(NewObject<UBehaviorTreeComponent>());
	TestEqual(TEXT("Missing AI owner fails without dereferencing it"),
		Fixture.Task->ExecuteTask(*UnownedBehavior, nullptr), EBTNodeResult::Failed);
	TestNull(TEXT("Fixture starts without a blackboard"), Fixture.Behavior->GetBlackboardComponent());
	TestEqual(TEXT("Missing blackboard fails safely"),
		Fixture.Task->ExecuteTask(*Fixture.Behavior, nullptr), EBTNodeResult::Failed);
	if (!Fixture.BindBlackboard(*this)) { return false; }
	TestNull(TEXT("Preview fixture starts without a navigation system"), Fixture.TestWorld.GetTestWorld()->GetNavigationSystem());
	TestEqual(TEXT("Missing navigation system fails safely"),
		Fixture.Task->ExecuteTask(*Fixture.Behavior, nullptr), EBTNodeResult::Failed);
	UNavigationSystemV1* NavSystem = Fixture.AddNavigation(false);
	if (!TestNotNull(TEXT("Navigation system can exist before navigation data"), NavSystem)) { return false; }
	TestNull(TEXT("Navigation data is not yet ready"), NavSystem->GetDefaultNavDataInstance(FNavigationSystem::DontCreate));
	TestEqual(TEXT("Missing navigation data fails safely"),
		Fixture.Task->ExecuteTask(*Fixture.Behavior, nullptr), EBTNodeResult::Failed);
	Fixture.Controller->UnPossess();
	TestEqual(TEXT("Unpossessed controller fails safely"),
		Fixture.Task->ExecuteTask(*Fixture.Behavior, nullptr), EBTNodeResult::Failed);
	TestTrue(TEXT("Prerequisite failures preserve the existing destination"), Fixture.Destination().Equals(PreviousDestination));
	return true;
}

#endif
