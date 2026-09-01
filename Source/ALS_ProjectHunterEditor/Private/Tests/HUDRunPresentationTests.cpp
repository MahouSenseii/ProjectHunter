// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Animation/WidgetAnimation.h"
#include "Animation/WidgetAnimationHandle.h"
#include "Animation/WidgetAnimationState.h"
#include "Blueprint/WidgetTree.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Framework/GameModes/PHGameState.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Tower/Subsystems/RunSubsystem.h"
#include "UI/HUD/HunterHUD.h"
#include "UI/HUD/HunterMainHUDWidget.h"
#include "UI/HUD/PHFloorBannerWidget.h"
#include "UI/HUD/PHRunStatusWidget.h"
#include "UObject/UnrealType.h"
#include "WidgetBlueprint.h"

namespace PHHUDRunTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	struct FFixture
	{
		FTestWorldWrapper World;
		TSharedPtr<SWidget> Slate;
		UHunterMainHUDWidget* Main = nullptr;
		UPHRunStatusWidget* Status = nullptr;
		UPHFloorBannerWidget* Banner = nullptr;
		APHGameState* State = nullptr;
		AHunterHUD* HUD = nullptr;
		URunSubsystem* Run = nullptr;

		bool Initialize(FAutomationTestBase& Test)
		{
			if (!World.CreateTestWorld(EWorldType::Game))
			{
				World.ForwardErrorMessages(&Test);
				return false;
			}
			UWorld* TestWorld = World.GetTestWorld();
			if (!Test.TestNull(TEXT("The isolated fixture has no gameplay/save-starting GameMode"), TestWorld->GetAuthGameMode()))
			{
				return false;
			}
			// Actor UFUNCTION delegates require initialized actors. Initialize the empty world
			// without beginning play or creating a game mode, player, or character-save flow.
			TestWorld->InitializeActorsForPlay(FURL());
			Run = TestWorld->GetGameInstance()->GetSubsystem<URunSubsystem>();
			State = TestWorld->SpawnActor<APHGameState>();
			TestWorld->SetGameState(State);
			UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(nullptr,
				TEXT("/Game/ProjectHunter/UI/HUD/WBP_HunterHUD.WBP_HunterHUD"));
			if (!Test.TestNotNull(TEXT("Authored player HUD loads"), Blueprint) || !Blueprint->GeneratedClass)
			{
				return false;
			}
			Main = CreateWidget<UHunterMainHUDWidget>(TestWorld, Blueprint->GeneratedClass.Get());
			if (!Test.TestNotNull(TEXT("Authored player HUD instantiates"), Main))
			{
				return false;
			}
			Slate = Main->TakeWidget();
			Status = Main->GetRunStatusWidget();
			Banner = Main->GetFloorBannerWidget();
			if (!Test.TestNotNull(TEXT("Run status is bound"), Status) ||
				!Test.TestNotNull(TEXT("Floor banner is bound"), Banner) ||
				!Test.TestNotNull(TEXT("Authored opening animation is bound"), Banner->GetEntryAnimation()) ||
				!Test.TestNotNull(TEXT("Run owner exists"), Run) || !Test.TestNotNull(TEXT("Snapshot owner exists"), State))
			{
				return false;
			}
			HUD = TestWorld->SpawnActor<AHunterHUD>();
			FObjectPropertyBase* MainProperty = FindFProperty<FObjectPropertyBase>(AHunterHUD::StaticClass(), TEXT("MainHUDWidget"));
			if (!Test.TestNotNull(TEXT("HUD owner exists"), HUD) || !Test.TestNotNull(TEXT("Existing root reference is injectable in the isolated test"), MainProperty))
			{
				return false;
			}
			// Exercise the real HUD listener without a viewport, player/save startup, or whole-world BeginPlay.
			MainProperty->SetObjectPropertyValue_InContainer(HUD, Main);
			HUD->DispatchBeginPlay();
			return Test.TestTrue(TEXT("The run owner uses this isolated test world"), Run->GetWorld() == TestWorld) &&
				Test.TestTrue(TEXT("The HUD and run owner see the same GameState"), TestWorld->GetGameState() == State) &&
				Test.TestTrue(TEXT("The test snapshot owner has authority"), State->HasAuthority()) &&
				Test.TestTrue(TEXT("The real HUD subscribes to the initial GameState"), State->OnReplicatedRunChanged.IsBound());
		}

		UTextBlock* FindText(const TCHAR* Name) const
		{
			return Cast<UTextBlock>(Status->WidgetTree->FindWidget(Name));
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHHUDRunSnapshotTest,
	"ProjectHunter.HUD.Run.ExistingOwnerToStatusAndFloorAnnouncements", PHHUDRunTests::Flags)

bool FPHHUDRunSnapshotTest::RunTest(const FString&)
{
	PHHUDRunTests::FFixture Fixture;
	if (!Fixture.Initialize(*this)) { return false; }
	UTextBlock* Enemies = Fixture.FindText(TEXT("RemainingEnemiesText"));
	UTextBlock* Mission = Fixture.FindText(TEXT("FloorMissionText"));
	UPanelWidget* Missions = Fixture.Status->GetMissionContainer();
	UWidget* Panel = Fixture.Banner->WidgetTree->FindWidget(TEXT("BannerPanel"));
	if (!TestNotNull(TEXT("Enemy label exists"), Enemies) || !TestNotNull(TEXT("Floor mission label exists"), Mission) ||
		!TestNotNull(TEXT("Additional mission container exists"), Missions) || !TestNotNull(TEXT("Banner panel exists"), Panel))
	{
		return false;
	}
	Fixture.Run->StartRun(20260830, 1);
	TestEqual(TEXT("The existing run publication activates the GameState snapshot"), Fixture.State->RunState, ERunState::Active);
	TestEqual(TEXT("The existing run publication includes the generated floor descriptor"), Fixture.State->RunSession.Floor.FloorNumber, 1);
	TestTrue(TEXT("Starting the run preserves the HUD's GameState source"),
		Fixture.Run->GetWorld() == Fixture.World.GetTestWorld() && Fixture.Run->GetWorld()->GetGameState<APHGameState>() == Fixture.State);
	AddInfo(FString::Printf(TEXT("Run HUD source: world=%s net=%d snapshot=%s floor=%d revision=%d listener=%d"),
		*GetNameSafe(Fixture.Run->GetWorld()), static_cast<int32>(Fixture.Run->GetWorld()->GetNetMode()),
		*UEnum::GetValueAsString(Fixture.State->RunState), Fixture.State->RunSession.Floor.FloorNumber,
		Fixture.State->RunSession.Revision, Fixture.State->OnReplicatedRunChanged.IsBound()));
	TestEqual(TEXT("Generation does not announce a floor before it is ready"), Fixture.Banner->CurrentFloor, 0);
	TestEqual(TEXT("A pending enemy budget is not reported as zero"), Enemies->GetText().ToString(), FString(TEXT("Enemies remaining: --")));
	Fixture.Run->NotifyFloorGenerated(7);
	TestEqual(TEXT("The ready snapshot reaches the existing GameState"), Fixture.State->RunSession.Floor.Phase, EFloorPhase::InProgress);
	TestEqual(TEXT("Floor one is announced through the existing snapshot"), Fixture.Banner->CurrentFloor, 1);
	TestTrue(TEXT("The entry animation starts"), Fixture.Banner->IsAnimationPlaying(Fixture.Banner->GetEntryAnimation()));
	TestEqual(TEXT("Enemy count comes from the actual run owner"), Enemies->GetText().ToString(), FString(TEXT("Enemies remaining: 7")));
	TestEqual(TEXT("Current tracked objective appears as a mission"), Mission->GetText().ToString(), FString(TEXT("Clear the floor")));
	UTextBlock* ExternalMission = NewObject<UTextBlock>(Fixture.Status);
	ExternalMission->SetText(FText::FromString(TEXT("A mission supplied by another owner")));
	Missions->AddChild(ExternalMission);
	Fixture.Banner->SetAnimationCurrentTime(Fixture.Banner->GetEntryAnimation(), 1.0f);
	Fixture.Banner->FlushAnimations();
	Fixture.Run->AddObjectiveProgress(2);
	// Force the owner's existing publication path rather than introducing a UI poll or new counter.
	Fixture.Run->SyncToGameState();
	TestEqual(TEXT("Published progress updates the enemy count"), Enemies->GetText().ToString(), FString(TEXT("Enemies remaining: 5")));
	TestEqual(TEXT("Counter updates do not restart the announcement"), Fixture.Banner->GetAnimationCurrentTime(Fixture.Banner->GetEntryAnimation()), 1.0f);
	TestEqual(TEXT("Snapshot updates preserve external mission widgets"), Missions->GetChildrenCount(), 1);
	Fixture.Run->CompleteFloorObjective();
	Fixture.Run->NotifyRewardGranted();
	TestEqual(TEXT("The existing reward-ready phase drives the mission text"), Mission->GetText().ToString(), FString(TEXT("Exit open - reach the portal")));
	Fixture.Run->AdvanceFloor();
	TestEqual(TEXT("Transition cancels the previous floor announcement"), Panel->GetVisibility(), ESlateVisibility::Collapsed);
	Fixture.Run->NotifyFloorGenerated(4);
	TestEqual(TEXT("The same banner is reused for the next ready floor"), Fixture.Banner->CurrentFloor, 2);
	TestEqual(TEXT("The next floor resets its displayed enemy budget"), Enemies->GetText().ToString(), FString(TEXT("Enemies remaining: 4")));
	Fixture.Run->EndRun(ERunEndReason::Quit);
	TestEqual(TEXT("Run end hides the banner"), Panel->GetVisibility(), ESlateVisibility::Collapsed);
	TestEqual(TEXT("Run end hides the stale enemy count"), Enemies->GetVisibility(), ESlateVisibility::Collapsed);
	TestEqual(TEXT("Run end does not delete mission widgets owned elsewhere"), Missions->GetChildrenCount(), 1);
	Fixture.Run->StartRun(20260830, 1);
	Fixture.Run->NotifyFloorGenerated(3);
	TestEqual(TEXT("Floor one is announced again for a new run identity"), Fixture.Banner->CurrentFloor, 1);

	// A replacement GameState may already have a ready snapshot when the HUD encounters it.
	FRunSessionData Session = Fixture.Run->GetSessionData();
	// GameState actor initialization can register the replacement immediately, so read the
	// authoritative old snapshot before spawning its replacement.
	APHGameState* Replacement = Fixture.World.GetTestWorld()->SpawnActor<APHGameState>();
	Session.Floor.FloorNumber = 8;
	Session.CurrentFloor = 8;
	Session.Floor.ObjectiveTarget = 9;
	Session.Floor.ObjectiveProgress = 3;
	Replacement->ApplyRunSnapshot(ERunState::Active, Session);
	Fixture.World.GetTestWorld()->SetGameState(Replacement);
	TestEqual(TEXT("GameState replacement initializes from its existing snapshot"), Enemies->GetText().ToString(), FString(TEXT("Enemies remaining: 6")));
	TestEqual(TEXT("A ready late snapshot announces its floor"), Fixture.Banner->CurrentFloor, 8);
	Session.Floor.ObjectiveProgress = 8;
	Fixture.State->ApplyRunSnapshot(ERunState::Active, Session);
	TestEqual(TEXT("The old GameState is no longer a UI source"), Enemies->GetText().ToString(), FString(TEXT("Enemies remaining: 6")));
	Fixture.HUD->Destroy();
	Replacement->ApplyRunSnapshot(ERunState::Active, Session);
	TestEqual(TEXT("EndPlay removes the snapshot listener"), Enemies->GetText().ToString(), FString(TEXT("Enemies remaining: 6")));
	TestFalse(TEXT("No gameplay map or character-save startup was run"), Fixture.World.GetTestWorld()->HasBegunPlay());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHHUDRunAnimationTest,
	"ProjectHunter.HUD.Run.AuthoredOpeningAnimationAndObjectiveKinds", PHHUDRunTests::Flags)

bool FPHHUDRunAnimationTest::RunTest(const FString&)
{
	PHHUDRunTests::FFixture Fixture;
	if (!Fixture.Initialize(*this)) { return false; }
	UWidget* Panel = Fixture.Banner->WidgetTree->FindWidget(TEXT("BannerPanel"));
	UWidgetAnimation* Animation = Fixture.Banner->GetEntryAnimation();
	if (!TestNotNull(TEXT("The animated panel exists"), Panel)) { return false; }
	Fixture.Banner->ShowFloor(4);
	const FWidgetAnimationHandle Handle = Fixture.Banner->PlayAnimation(Animation);
	Fixture.Banner->FlushAnimations();
	FWidgetAnimationState* AnimationState = Handle.GetAnimationState();
	if (!TestNotNull(TEXT("The actual UMG animation state exists"), AnimationState)) { return false; }
	// Seeking changes the animation clock; a zero-duration animation frame evaluates it.
	Fixture.Banner->SetAnimationCurrentTime(Animation, 0.12f);
	AnimationState->Tick(0.0f);
	Fixture.Banner->FlushAnimations();
	TestTrue(TEXT("The opening expands the actual UMG panel"), Panel->GetRenderTransform().Scale.X > 0.30f && Panel->GetRenderTransform().Scale.X < 1.0f);
	TestTrue(TEXT("The opening fades in the actual UMG panel"), Panel->GetRenderOpacity() > 0.0f && Panel->GetRenderOpacity() < 1.0f);
	Fixture.Banner->SetAnimationCurrentTime(Animation, 1.0f);
	AnimationState->Tick(0.0f);
	Fixture.Banner->FlushAnimations();
	TestTrue(TEXT("The banner reaches its authored size"), FMath::IsNearlyEqual(Panel->GetRenderTransform().Scale.X, 1.0));
	TestTrue(TEXT("The banner is fully readable during its hold"), FMath::IsNearlyEqual(Panel->GetRenderOpacity(), 1.0f));
	Fixture.Banner->SetAnimationCurrentTime(Animation, 3.0f);
	AnimationState->Tick(0.0f);
	Fixture.Banner->FlushAnimations();
	TestTrue(TEXT("The exit animation fades the panel out"), Panel->GetRenderOpacity() > 0.0f && Panel->GetRenderOpacity() < 1.0f);
	AnimationState->Tick(0.5f);
	Fixture.Banner->FlushAnimations();
	TestEqual(TEXT("The animation completion hides the panel"), Panel->GetVisibility(), ESlateVisibility::Collapsed);
	Fixture.Banner->ShowFloor(5);
	Fixture.Banner->FlushAnimations();
	TestTrue(TEXT("The reusable banner can open again"), Fixture.Banner->IsAnimationPlaying(Animation));
	Fixture.Banner->HideBanner();
	TestEqual(TEXT("An explicit hide cancels presentation"), Panel->GetVisibility(), ESlateVisibility::Collapsed);

	FRunSessionData Session;
	Session.Floor.FloorNumber = 1;
	Session.Floor.Phase = EFloorPhase::InProgress;
	Session.Floor.Objective = EFloorObjective::ReachExit;
	Fixture.Status->ApplyRunSnapshot(ERunState::Active, Session);
	TestEqual(TEXT("A non-enemy objective is not labelled as enemies"), Fixture.FindText(TEXT("RemainingEnemiesText"))->GetVisibility(), ESlateVisibility::Collapsed);
	TestEqual(TEXT("The mission uses the existing objective kind"), Fixture.FindText(TEXT("FloorMissionText"))->GetText().ToString(), FString(TEXT("Reach the exit")));
	Session.Floor.Objective = EFloorObjective::KillBoss;
	Session.Floor.ObjectiveTarget = 1;
	Session.Floor.ObjectiveProgress = 3;
	Fixture.Status->ApplyRunSnapshot(ERunState::Active, Session);
	TestEqual(TEXT("Over-complete progress cannot display a negative count"), Fixture.FindText(TEXT("RemainingEnemiesText"))->GetText().ToString(), FString(TEXT("Bosses remaining: 0")));
	return true;
}

#endif
