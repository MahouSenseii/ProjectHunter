// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/GameInstance.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Tower/Library/FunctionLibraries/RunSeedFunctionLibrary.h"
#include "Tower/Subsystems/RunSubsystem.h"

namespace PHRunFloorSeedIntegrationTests
{
	constexpr int32 EncounterCount = 3;

	struct FFloorSnapshot
	{
		FRunFloorData Descriptor;
		int32 EncounterSeeds[EncounterCount] = {};
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPHRunFloorSeedLifecycleTest,
	"ProjectHunter.Run.Integration.SeededFloorLifecycle",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FPHRunFloorSeedLifecycleTest::RunTest(const FString&)
{
	using namespace PHRunFloorSeedIntegrationTests;

	// The native game instance leaves the stash unopened. Do not begin play:
	// that would start the configured game mode and its character/save workflow.
	FTestWorldWrapper TestWorld;
	if (!TestWorld.CreateTestWorld(EWorldType::Game))
	{
		TestWorld.ForwardErrorMessages(this);
		return false;
	}

	UGameInstance* GameInstance = TestWorld.GetTestWorld()->GetGameInstance();
	if (!TestNotNull(TEXT("The test world has a native game instance"), GameInstance))
	{
		return false;
	}

	URunSubsystem* Run = GameInstance->GetSubsystem<URunSubsystem>();
	if (!TestNotNull(TEXT("The game instance initializes the real run owner"), Run))
	{
		return false;
	}
	if (!TestTrue(TEXT("The run owner uses the isolated test world"), Run->GetWorld() == TestWorld.GetTestWorld()))
	{
		return false;
	}

	constexpr int32 RunSeed = 20260826;
	constexpr int32 RunDifficulty = 2;
	constexpr int32 FloorCount = 3;
	FRunFloorPlan Plan;
	Plan.BossFloorInterval = 3;
	Plan.EliteFloorInterval = 2;
	Plan.TreasureFloorChance = 0.5f;
	Plan.DifficultyPerFloor = 2;
	Plan.FloorsToCompleteRun = FloorCount;
	Run->SetFloorPlan(Plan);

	TArray<FFloorSnapshot> FirstRunFloors;
	FirstRunFloors.Reserve(FloorCount);
	FGuid FirstRunID;

	for (int32 Replay = 0; Replay < 2; ++Replay)
	{
		Run->StartRun(RunSeed, RunDifficulty);
		if (!TestTrue(TEXT("StartRun activates the run"), Run->IsRunActive()))
		{
			return false;
		}

		const FRunSessionData StartedSession = Run->GetSessionData();
		TestEqual(TEXT("StartRun preserves the supplied seed"), StartedSession.RunSeed, RunSeed);
		TestEqual(TEXT("StartRun preserves the supplied difficulty"), StartedSession.Difficulty, RunDifficulty);
		TestEqual(TEXT("A new run starts on floor one"), Run->GetCurrentFloor(), 1);
		TestEqual(TEXT("A new run resets cleared floors"), StartedSession.FloorsCleared, 0);
		TestEqual(TEXT("A new run resets kills"), Run->GetTotalKills(), 0);
		TestEqual(TEXT("A new run clears the previous end reason"), StartedSession.EndReason, ERunEndReason::None);
		TestTrue(TEXT("A new run has a valid identity"), StartedSession.RunID.IsValid());
		if (Replay == 0)
		{
			FirstRunID = StartedSession.RunID;
		}
		else
		{
			TestTrue(TEXT("Replaying a seed creates a new run identity"), StartedSession.RunID != FirstRunID);
		}

		int32 ExpectedKills = 0;
		for (int32 FloorNumber = 1; FloorNumber <= FloorCount; ++FloorNumber)
		{
			const FString Context = FString::Printf(TEXT("Run %d floor %d"), Replay + 1, FloorNumber);
			FFloorSnapshot Snapshot;
			Snapshot.Descriptor = Run->GetCurrentFloorData();
			const FRunFloorData& Floor = Snapshot.Descriptor;
			TestEqual(Context + TEXT(" starts in generation"), Floor.Phase, EFloorPhase::Generating);
			TestEqual(Context + TEXT(" reports its floor number"), Floor.FloorNumber, FloorNumber);
			TestEqual(Context + TEXT(" keeps the session floor in sync"), Run->GetCurrentFloor(), FloorNumber);
			TestEqual(Context + TEXT(" scales difficulty"), Floor.Difficulty,
				RunDifficulty + (FloorNumber - 1) * Plan.DifficultyPerFloor);
			TestEqual(Context + TEXT(" exposes the descriptor seed"), Run->GetFloorSeed(), Floor.FloorSeed);
			TestEqual(Context + TEXT(" derives its seed from the run and floor number"), Floor.FloorSeed,
				URunSeedFunctionLibrary::DeriveFloorSeed(RunSeed, FloorNumber));
			TestTrue(Context + TEXT(" has a usable floor seed"), Floor.FloorSeed > 0);
			TestEqual(Context + TEXT(" starts without objective progress"), Floor.ObjectiveProgress, 0);
			TestEqual(Context + TEXT(" waits for the generated objective target"), Floor.ObjectiveTarget, 0);
			TestFalse(Context + TEXT(" cannot advance before generation"), Run->CanAdvanceFloor());

			if (FloorNumber == 2)
			{
				TestEqual(Context + TEXT(" follows the elite milestone"), Floor.FloorType, EFloorType::Elite);
				TestEqual(Context + TEXT(" requires clearing the elite encounter"), Floor.Objective,
					EFloorObjective::ClearAllEnemies);
			}
			else if (FloorNumber == 3)
			{
				TestEqual(Context + TEXT(" follows the boss milestone"), Floor.FloorType, EFloorType::Boss);
				TestEqual(Context + TEXT(" requires the boss objective"), Floor.Objective, EFloorObjective::KillBoss);
			}

			for (int32 EncounterIndex = 0; EncounterIndex < EncounterCount; ++EncounterIndex)
			{
				Snapshot.EncounterSeeds[EncounterIndex] = Run->GetEncounterSeed(EncounterIndex);
				TestTrue(Context + TEXT(" has a usable encounter seed"), Snapshot.EncounterSeeds[EncounterIndex] > 0);
				TestEqual(Context + FString::Printf(TEXT(" derives encounter %d from this floor"), EncounterIndex),
					Snapshot.EncounterSeeds[EncounterIndex],
					URunSeedFunctionLibrary::DeriveEncounterSeed(Floor.FloorSeed, EncounterIndex));
			}

			if (Replay == 0)
			{
				FirstRunFloors.Add(Snapshot);
			}
			else
			{
				const FFloorSnapshot& Expected = FirstRunFloors[FloorNumber - 1];
				TestEqual(Context + TEXT(" replays its floor seed"), Floor.FloorSeed, Expected.Descriptor.FloorSeed);
				TestEqual(Context + TEXT(" replays its floor type"), Floor.FloorType, Expected.Descriptor.FloorType);
				TestEqual(Context + TEXT(" replays its objective"), Floor.Objective, Expected.Descriptor.Objective);
				TestEqual(Context + TEXT(" replays its difficulty"), Floor.Difficulty, Expected.Descriptor.Difficulty);
				TestTrue(Context + TEXT(" replays its modifiers"), Floor.Modifiers == Expected.Descriptor.Modifiers);
				TestEqual(Context + TEXT(" replays its objective duration"), Floor.ObjectiveDuration,
					Expected.Descriptor.ObjectiveDuration);
				for (int32 EncounterIndex = 0; EncounterIndex < EncounterCount; ++EncounterIndex)
				{
					TestEqual(Context + FString::Printf(TEXT(" replays encounter %d"), EncounterIndex),
						Snapshot.EncounterSeeds[EncounterIndex], Expected.EncounterSeeds[EncounterIndex]);
				}
			}

			// Supply the callbacks normally produced by generation and gameplay.
			// No geometry, enemies, inventory rewards, or exit actors are created.
			const int32 ObjectiveTarget = Floor.Objective == EFloorObjective::KillBoss ? 1 : 2;
			Run->NotifyFloorGenerated(ObjectiveTarget);
			TestEqual(Context + TEXT(" begins its objective after generation"), Run->GetFloorPhase(), EFloorPhase::InProgress);
			TestEqual(Context + TEXT(" records the generated objective target"),
				Run->GetCurrentFloorData().ObjectiveTarget, ObjectiveTarget);
			TestFalse(Context + TEXT(" cannot advance with an unfinished objective"), Run->CanAdvanceFloor());

			for (int32 Progress = 1; Progress <= ObjectiveTarget; ++Progress)
			{
				if (Floor.Objective == EFloorObjective::ClearAllEnemies || Floor.Objective == EFloorObjective::KillBoss)
				{
					Run->RegisterKill();
					++ExpectedKills;
				}
				else
				{
					Run->AddObjectiveProgress();
				}
				TestEqual(Context + TEXT(" records objective progress"), Run->GetCurrentFloorData().ObjectiveProgress, Progress);
				if (Progress < ObjectiveTarget)
				{
					TestEqual(Context + TEXT(" remains active until its objective is met"),
						Run->GetFloorPhase(), EFloorPhase::InProgress);
				}
			}

			TestEqual(Context + TEXT(" completes its objective"), Run->GetFloorPhase(), EFloorPhase::ObjectiveComplete);
			TestEqual(Context + TEXT(" counts reported kills"), Run->GetTotalKills(), ExpectedKills);
			TestFalse(Context + TEXT(" waits for rewards before allowing exit"), Run->CanAdvanceFloor());
			Run->NotifyRewardGranted();
			TestEqual(Context + TEXT(" reaches the reward-ready phase"), Run->GetFloorPhase(), EFloorPhase::RewardReady);
			TestTrue(Context + TEXT(" allows exit after rewards"), Run->CanAdvanceFloor());

			Run->AdvanceFloor();
			TestEqual(Context + TEXT(" counts exactly one cleared floor"), Run->GetSessionData().FloorsCleared, FloorNumber);
			if (FloorNumber < FloorCount)
			{
				TestTrue(Context + TEXT(" leaves the run active"), Run->IsRunActive());
				TestEqual(Context + TEXT(" advances to the next floor"), Run->GetCurrentFloor(), FloorNumber + 1);
			}
		}

		const FRunSessionData EndedSession = Run->GetSessionData();
		TestEqual(TEXT("The final advance ends the run"), Run->GetRunState(), ERunState::Ended);
		TestEqual(TEXT("The final advance records completion"), EndedSession.EndReason, ERunEndReason::Completed);
		TestEqual(TEXT("Completion preserves the last floor number"), EndedSession.CurrentFloor, FloorCount);
		TestEqual(TEXT("Completion clears the active floor phase"), Run->GetFloorPhase(), EFloorPhase::None);
		TestFalse(TEXT("An ended run cannot advance"), Run->CanAdvanceFloor());
		TestFalse(TEXT("An ended run is inactive"), Run->IsRunActive());
	}

	TestWorld.ForwardErrorMessages(this);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPHRunClimbLoopTest,
	"ProjectHunter.Run.Integration.ClimbLoop",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FPHRunClimbLoopTest::RunTest(const FString&)
{
	// The spine of the playable loop, asserted as a state machine rather than by playing it:
	// arrive on a floor, kill everything, the exit opens, climb, arrive on the next one.
	// Every step here corresponds to something the level now does - the floor actor reports kills,
	// the game mode grants the placeholder reward, the portal calls AdvanceFloor.
	FTestWorldWrapper TestWorld;
	if (!TestWorld.CreateTestWorld(EWorldType::Game))
	{
		TestWorld.ForwardErrorMessages(this);
		return false;
	}

	UGameInstance* GameInstance = TestWorld.GetTestWorld()->GetGameInstance();
	if (!TestNotNull(TEXT("The test world has a native game instance"), GameInstance))
	{
		return false;
	}

	URunSubsystem* Run = GameInstance->GetSubsystem<URunSubsystem>();
	if (!TestNotNull(TEXT("The game instance initializes the real run owner"), Run))
	{
		return false;
	}

	Run->StartRun(4242, 1);
	TestTrue(TEXT("Starting a run makes it active"), Run->IsRunActive());
	TestEqual(TEXT("A run begins on floor one"), Run->GetCurrentFloor(), 1);
	TestEqual(TEXT("A new floor waits to be built"),
		Run->GetFloorPhase(), EFloorPhase::Generating);

	const int32 EnemyBudget = 5;
	int32 ClimbedFloors = 0;

	for (int32 Floor = 1; Floor <= 3; ++Floor)
	{
		TestEqual(FString::Printf(TEXT("Floor %d is the current floor"), Floor),
			Run->GetCurrentFloor(), Floor);

		// What APHGeneratedFloorActor does once its geometry and encounters exist.
		Run->NotifyFloorGenerated(EnemyBudget);
		TestEqual(TEXT("A generated floor is in progress"),
			Run->GetFloorPhase(), EFloorPhase::InProgress);
		TestFalse(TEXT("The exit stays shut on arrival"), Run->CanAdvanceFloor());

		// What the floor actor does for each mob death it hears about.
		for (int32 Kill = 1; Kill < EnemyBudget; ++Kill)
		{
			Run->RegisterKill();
			Run->AddObjectiveProgress(1);
			TestFalse(
				FString::Printf(TEXT("The exit stays shut with %d of %d killed"), Kill, EnemyBudget),
				Run->CanAdvanceFloor());
		}

		// The last kill completes the objective, which the game mode answers with the reward stub.
		Run->RegisterKill();
		Run->AddObjectiveProgress(1);
		TestEqual(TEXT("Killing the budget completes the objective"),
			Run->GetFloorPhase(), EFloorPhase::ObjectiveComplete);

		Run->NotifyRewardGranted();
		TestEqual(TEXT("Granting the reward readies the floor"),
			Run->GetFloorPhase(), EFloorPhase::RewardReady);
		TestTrue(TEXT("Only now may the exit be used"), Run->CanAdvanceFloor());

		const int32 Before = Run->GetCurrentFloor();
		Run->AdvanceFloor();
		if (Run->IsRunActive())
		{
			TestEqual(TEXT("Using the exit climbs exactly one floor"),
				Run->GetCurrentFloor(), Before + 1);
			TestEqual(TEXT("The next floor waits to be built"),
				Run->GetFloorPhase(), EFloorPhase::Generating);
		}
		++ClimbedFloors;
	}

	TestEqual(TEXT("Three floors were climbed"), ClimbedFloors, 3);
	// Kills and objective progress are separate counters, and the floor actor reports both for the
	// same death. Three floors of five enemies is fifteen kills on the run record.
	TestEqual(TEXT("Every kill counted toward the run total"),
		Run->GetTotalKills(), 3 * EnemyBudget);

	TestWorld.ForwardErrorMessages(this);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
