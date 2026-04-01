// PHGameState.cpp
#include "PHGameState.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY(LogPHGameState);

APHGameState::APHGameState()
{
}

void APHGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APHGameState, MatchPhase);
	DOREPLIFETIME(APHGameState, MatchStartServerTime);
	DOREPLIFETIME(APHGameState, TotalMobKills);
	DOREPLIFETIME(APHGameState, WorldTier);
}

// ─────────────────────────────────────────────────────────────────────────────
// Match phase
// ─────────────────────────────────────────────────────────────────────────────

void APHGameState::SetMatchPhase(EPHMatchPhase NewPhase)
{
	if (MatchPhase == NewPhase)
	{
		return;
	}

	const EPHMatchPhase OldPhase = MatchPhase;
	MatchPhase = NewPhase;

	// Record start time when transitioning to InProgress
	if (NewPhase == EPHMatchPhase::InProgress)
	{
		if (UWorld* World = GetWorld())
		{
			MatchStartServerTime = World->GetTimeSeconds();
		}
	}

	UE_LOG(LogPHGameState, Log, TEXT("Match phase: %s → %s"),
		*UEnum::GetValueAsString(OldPhase),
		*UEnum::GetValueAsString(NewPhase));

	OnMatchPhaseChanged.Broadcast(NewPhase);
}

void APHGameState::OnRep_MatchPhase()
{
	OnMatchPhaseChanged.Broadcast(MatchPhase);
}

// ─────────────────────────────────────────────────────────────────────────────
// Timing
// ─────────────────────────────────────────────────────────────────────────────

float APHGameState::GetMatchElapsedTime() const
{
	if (MatchPhase == EPHMatchPhase::WaitingToStart || MatchStartServerTime <= 0.f)
	{
		return 0.f;
	}

	if (const UWorld* World = GetWorld())
	{
		return FMath::Max(0.f, World->GetTimeSeconds() - MatchStartServerTime);
	}
	return 0.f;
}

// ─────────────────────────────────────────────────────────────────────────────
// Stats
// ─────────────────────────────────────────────────────────────────────────────

void APHGameState::IncrementMobKills(int32 Count)
{
	TotalMobKills += FMath::Max(0, Count);
}
