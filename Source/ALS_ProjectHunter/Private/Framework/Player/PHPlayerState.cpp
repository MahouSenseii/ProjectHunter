#include "Framework/Player/PHPlayerState.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "Tower/Subsystems/RunSubsystem.h"

DEFINE_LOG_CATEGORY(LogPHPlayerState);

APHPlayerState::APHPlayerState()
{
}

void APHPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APHPlayerState, TeamID);
	DOREPLIFETIME(APHPlayerState, CurrentFloor);
	DOREPLIFETIME(APHPlayerState, RunKillCount);
	DOREPLIFETIME(APHPlayerState, RunPlayerState);
	DOREPLIFETIME_CONDITION(APHPlayerState, CharacterSlotName, COND_OwnerOnly);
}

void APHPlayerState::SetTeamID(uint8 NewTeamID)
{
	TeamID = NewTeamID;
	UE_LOG(LogPHPlayerState, Log, TEXT("%s: TeamID set to %d"), *GetName(), NewTeamID);
}

URunSubsystem* APHPlayerState::GetRunSubsystem() const
{
	const UWorld* World = GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<URunSubsystem>() : nullptr;
}

void APHPlayerState::SetRunFloorMirror(const int32 NewFloor)
{
	if (!HasAuthority())
	{
		return;
	}

	if (CurrentFloor != NewFloor)
	{
		CurrentFloor = NewFloor;
		OnRunFloorChanged.Broadcast(CurrentFloor);
	}
}

void APHPlayerState::SetRunPlayerStateMirror(const ERunPlayerState NewState)
{
	if (!HasAuthority() || RunPlayerState == NewState)
	{
		return;
	}

	RunPlayerState = NewState;
	OnRunStatusChanged.Broadcast(RunPlayerState);
}

void APHPlayerState::RequestAdvanceFloor()
{
	URunSubsystem* Run = GetRunSubsystem();
	if (!Run || !Run->IsRunActive())
	{
		UE_LOG(LogPHPlayerState, Warning,
			TEXT("%s: RequestAdvanceFloor ignored - no active run."), *GetName());
		return;
	}

	// RunSubsystem owns the party floor. It rejects the request unless the
	// current floor has actually reached RewardReady, and pushes the new floor
	// back to every PlayerState mirror.
	Run->AdvanceFloor();
}

void APHPlayerState::RecordKill()
{
	if (!HasAuthority())
	{
		return;
	}

	++RunKillCount;
	OnRunKillsChanged.Broadcast(RunKillCount);

	if (URunSubsystem* Run = GetRunSubsystem())
	{
		Run->RegisterKill();
	}
}

void APHPlayerState::OnRep_CurrentFloor()
{
	OnRunFloorChanged.Broadcast(CurrentFloor);
}

void APHPlayerState::OnRep_RunKillCount()
{
	OnRunKillsChanged.Broadcast(RunKillCount);
}

void APHPlayerState::OnRep_RunPlayerState()
{
	OnRunStatusChanged.Broadcast(RunPlayerState);
}
