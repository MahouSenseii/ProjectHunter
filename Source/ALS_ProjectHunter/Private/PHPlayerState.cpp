// PHPlayerState.cpp
#include "PHPlayerState.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY(LogPHPlayerState);

APHPlayerState::APHPlayerState()
{
}

void APHPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APHPlayerState, TeamID);
	DOREPLIFETIME(APHPlayerState, KillCount);
	DOREPLIFETIME(APHPlayerState, DeathCount);
	DOREPLIFETIME(APHPlayerState, TotalXPEarned);
	DOREPLIFETIME(APHPlayerState, CharacterSlotName);
}

// ─────────────────────────────────────────────────────────────────────────────
// Team
// ─────────────────────────────────────────────────────────────────────────────

void APHPlayerState::SetTeamID(uint8 NewTeamID)
{
	TeamID = NewTeamID;
	UE_LOG(LogPHPlayerState, Log, TEXT("%s: TeamID set to %d"), *GetName(), TeamID);
}

// ─────────────────────────────────────────────────────────────────────────────
// Stats tracking
// ─────────────────────────────────────────────────────────────────────────────

void APHPlayerState::RecordKill()
{
	++KillCount;
}

void APHPlayerState::RecordDeath()
{
	++DeathCount;
}

void APHPlayerState::AddXPEarned(int64 Amount)
{
	if (Amount > 0)
	{
		TotalXPEarned += Amount;
	}
}
