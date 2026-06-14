#include "Player/PHPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
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
	DOREPLIFETIME(APHPlayerState, CharacterSlotName);
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

void APHPlayerState::AdvanceFloor()
{
	// URunSubsystem owns the run; this PlayerState is the replicated mirror so
	// clients/UI can read the floor. The two counters used to be independent
	// and could silently diverge — now the subsystem advances first and the
	// mirrored value is read back from it.
	if (URunSubsystem* Run = GetRunSubsystem(); Run && Run->IsRunActive())
	{
		Run->AdvanceFloor();
		CurrentFloor = Run->GetCurrentFloor();
	}
	else
	{
		// No active run (or no subsystem): preserve standalone behavior.
		++CurrentFloor;
	}

	UE_LOG(LogPHPlayerState, Log, TEXT("%s: Advanced to floor %d"), *GetName(), CurrentFloor);
}

void APHPlayerState::RecordKill()
{
	// Per-player counter (replicated) AND run-wide counter (subsystem) — these
	// are different stats in co-op, so both increment.
	++RunKillCount;

	if (URunSubsystem* Run = GetRunSubsystem())
	{
		Run->RegisterKill();
	}
}
