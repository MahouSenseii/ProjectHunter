// PHPlayerState.cpp
#include "Player/PHPlayerState.h"
#include "Net/UnrealNetwork.h"

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

// ─────────────────────────────────────────────────────────────────────────────
// Team
// ─────────────────────────────────────────────────────────────────────────────

void APHPlayerState::SetTeamID(uint8 NewTeamID)
{
	TeamID = NewTeamID;
	UE_LOG(LogPHPlayerState, Log, TEXT("%s: TeamID set to %d"), *GetName(), NewTeamID);
}

// ─────────────────────────────────────────────────────────────────────────────
// Run state
// ─────────────────────────────────────────────────────────────────────────────

void APHPlayerState::AdvanceFloor()
{
	++CurrentFloor;
	UE_LOG(LogPHPlayerState, Log, TEXT("%s: Advanced to floor %d"), *GetName(), CurrentFloor);
}

void APHPlayerState::RecordKill()
{
	++RunKillCount;
}
