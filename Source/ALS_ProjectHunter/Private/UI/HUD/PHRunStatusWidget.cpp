// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#include "UI/HUD/PHRunStatusWidget.h"

#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"

UPanelWidget* UPHRunStatusWidget::GetMissionContainer() const
{
	return MissionEntries.Get();
}

void UPHRunStatusWidget::ApplyRunSnapshot(const ERunState State, const FRunSessionData& Session)
{
	const FRunFloorData& Floor = Session.Floor;
	const bool bActive = State == ERunState::Active && Floor.FloorNumber > 0;
	if (FloorLabelText)
	{
		FloorLabelText->SetText(bActive ? FText::Format(NSLOCTEXT("HunterHUD", "CurrentFloor", "FLOOR {0}"),
			FText::AsNumber(Floor.FloorNumber)) : FText::GetEmpty());
		FloorLabelText->SetVisibility(bActive ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	const bool bEnemyObjective = Floor.Objective == EFloorObjective::ClearAllEnemies || Floor.Objective == EFloorObjective::KillBoss;
	if (RemainingEnemiesText)
	{
		RemainingEnemiesText->SetVisibility(bActive && bEnemyObjective ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (bActive && bEnemyObjective)
		{
			const FText Count = Floor.Phase == EFloorPhase::Generating || Floor.Phase == EFloorPhase::Transitioning
				? NSLOCTEXT("HunterHUD", "PendingEnemyCount", "--")
				: FText::AsNumber(FMath::Max(0, FMath::Max(0, Floor.ObjectiveTarget) - FMath::Max(0, Floor.ObjectiveProgress)));
			RemainingEnemiesText->SetText(FText::Format(Floor.Objective == EFloorObjective::KillBoss
				? NSLOCTEXT("HunterHUD", "RemainingBosses", "Bosses remaining: {0}")
				: NSLOCTEXT("HunterHUD", "RemainingEnemies", "Enemies remaining: {0}"), Count));
		}
		else
		{
			RemainingEnemiesText->SetText(FText::GetEmpty());
		}
	}
	if (!FloorMissionText)
	{
		return;
	}
	FText Mission = NSLOCTEXT("HunterHUD", "NoFloorMission", "No active floor objective");
	if (bActive)
	{
		switch (Floor.Phase)
		{
		case EFloorPhase::Generating: Mission = NSLOCTEXT("HunterHUD", "FloorPreparing", "Preparing the floor..."); break;
		case EFloorPhase::Transitioning: Mission = NSLOCTEXT("HunterHUD", "FloorTransition", "Entering the next floor..."); break;
		case EFloorPhase::ObjectiveComplete: Mission = NSLOCTEXT("HunterHUD", "FloorComplete", "Floor objective complete"); break;
		case EFloorPhase::RewardReady: Mission = NSLOCTEXT("HunterHUD", "FloorExitOpen", "Exit open - reach the portal"); break;
		case EFloorPhase::InProgress:
			switch (Floor.Objective)
			{
			case EFloorObjective::ClearAllEnemies: Mission = NSLOCTEXT("HunterHUD", "ClearFloor", "Clear the floor"); break;
			case EFloorObjective::KillBoss: Mission = NSLOCTEXT("HunterHUD", "KillFloorBoss", "Defeat the floor boss"); break;
			case EFloorObjective::ReachExit: Mission = NSLOCTEXT("HunterHUD", "ReachFloorExit", "Reach the exit"); break;
			case EFloorObjective::SurviveDuration: Mission = NSLOCTEXT("HunterHUD", "SurviveFloor", "Survive the encounter"); break;
			case EFloorObjective::CollectObjective: Mission = NSLOCTEXT("HunterHUD", "CollectFloorObjective", "Collect the objective"); break;
			}
			break;
		default: break;
		}
	}
	FloorMissionText->SetText(Mission);
}
