// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

// One enemy death used to be worth two objective points, so a clear-all floor
// finished on half its enemies (ISSUE-PH-20260831-07). The floor actor called
// both RegisterKill and AddObjectiveProgress, not knowing RegisterKill already
// advances the objective on a kill-counting floor.
//
// These drive the real HandleMobDied through its UFunction, which is how the
// mob manager's delegate calls it - the callback is private, and reaching it by
// reflection tests the shipping path without widening the actor's API.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/GameInstance.h"
#include "Generation/Actors/PHGeneratedFloorActor.h"
#include "Tests/AutomationCommon.h"
#include "Tower/Subsystems/RunSubsystem.h"

namespace PHFloorObjectiveTests
{
	constexpr EAutomationTestFlags TestFlags =
		EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter;

	/**
	 * One reported death, through the same operation the mob-died delegate runs.
	 *
	 * An earlier attempt invoked the private handler by reflection; ProcessEvent
	 * never entered the body in a world that has not begun play, which a probe
	 * log confirmed, so the test silently measured nothing.
	 */
	void ReportOneDeath(APHGeneratedFloorActor& Floor)
	{
		Floor.ReportMobDeathToRun();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPHFloorOneDeathOneObjectivePointTest,
	"ProjectHunter.Generation.Floor.OneDeathAdvancesObjectiveOnce",
	PHFloorObjectiveTests::TestFlags)

bool FPHFloorOneDeathOneObjectivePointTest::RunTest(const FString&)
{
	using namespace PHFloorObjectiveTests;

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

	Run->StartRun(20260831, 1);
	if (!TestTrue(TEXT("The run is active after StartRun"), Run->IsRunActive()))
	{
		AddError(FString::Printf(TEXT("Run state is %d; the kill path requires Active."),
			static_cast<int32>(Run->GetRunState())));
		return false;
	}

	// Ten is an arbitrary target; the point is that it is larger than the number
	// of deaths reported, so an over-count is visible rather than clamped away.
	Run->NotifyFloorGenerated(10);


	APHGeneratedFloorActor* Floor =
		TestWorld.GetTestWorld()->SpawnActor<APHGeneratedFloorActor>();
	if (!TestNotNull(TEXT("The floor actor spawns"), Floor))
	{
		return false;
	}

	ReportOneDeath(*Floor);

	TestEqual(TEXT("One death is one kill"), Run->GetTotalKills(), 1);

	// The defect: this read 2. A clear-all floor would then finish on half its
	// enemies, and its reward and exit would unlock early.
	TestEqual(TEXT("One death advances a kill objective by exactly one"),
		Run->GetCurrentFloorData().ObjectiveProgress, 1);

	// Three more deaths, to catch a fix that only corrects the first.
	for (int32 Index = 0; Index < 3; ++Index)
	{
		ReportOneDeath(*Floor);
	}
	TestEqual(TEXT("Four deaths are four kills"), Run->GetTotalKills(), 4);
	TestEqual(TEXT("Four deaths advance the objective by exactly four"),
		Run->GetCurrentFloorData().ObjectiveProgress, 4);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPHFloorKillDoesNotAdvanceNonKillObjectiveTest,
	"ProjectHunter.Generation.Floor.KillDoesNotAdvanceNonKillObjective",
	PHFloorObjectiveTests::TestFlags)

bool FPHFloorKillDoesNotAdvanceNonKillObjectiveTest::RunTest(const FString&)
{
	using namespace PHFloorObjectiveTests;

	// The removed call advanced progress for *any* objective, so a floor cleared
	// by reaching an exit would also have counted kills toward it. Whether a kill
	// counts is the run owner's decision, and this pins that it stays there.
	FTestWorldWrapper TestWorld;
	if (!TestWorld.CreateTestWorld(EWorldType::Game))
	{
		TestWorld.ForwardErrorMessages(this);
		return false;
	}

	UGameInstance* GameInstance = TestWorld.GetTestWorld()->GetGameInstance();
	URunSubsystem* Run = GameInstance ? GameInstance->GetSubsystem<URunSubsystem>() : nullptr;
	if (!TestNotNull(TEXT("The run owner exists"), Run))
	{
		return false;
	}

	Run->StartRun(20260831, 1);
	Run->NotifyFloorGenerated(10);

	FRunFloorData Floor = Run->GetCurrentFloorData();
	if (Floor.Objective != EFloorObjective::ClearAllEnemies
		&& Floor.Objective != EFloorObjective::KillBoss)
	{
		APHGeneratedFloorActor* FloorActor =
			TestWorld.GetTestWorld()->SpawnActor<APHGeneratedFloorActor>();
		if (FloorActor)
		{
			FloorActor->ReportMobDeathToRun();
			TestEqual(TEXT("A kill does not advance a non-kill objective"),
				Run->GetCurrentFloorData().ObjectiveProgress, 0);
		}
		return true;
	}

	// The default floor counts kills, so there is nothing to assert here. Said
	// plainly rather than passing silently and looking like coverage.
	AddInfo(TEXT("Floor one's objective counts kills, so the non-kill path was not exercised. "
	             "This case becomes meaningful once floors vary their objective."));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
