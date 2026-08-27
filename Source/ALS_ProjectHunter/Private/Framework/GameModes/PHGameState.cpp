#include "Framework/GameModes/PHGameState.h"
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
	DOREPLIFETIME(APHGameState, MatchSnapshotRevision);
	DOREPLIFETIME(APHGameState, TotalMobKills);
	DOREPLIFETIME(APHGameState, WorldTier);
	DOREPLIFETIME(APHGameState, RunState);
	DOREPLIFETIME(APHGameState, RunSession);
	DOREPLIFETIME(APHGameState, RunSnapshotServerTime);
	DOREPLIFETIME(APHGameState, RunSnapshotRevision);
}

void APHGameState::SetMatchPhase(EPHMatchPhase NewPhase)
{
	if (!HasAuthority())
	{
		return;
	}

	if (MatchPhase == NewPhase)
	{
		return;
	}

	const EPHMatchPhase OldPhase = MatchPhase;
	MatchPhase = NewPhase;

	if (NewPhase == EPHMatchPhase::InProgress)
	{
		MatchStartServerTime = GetServerWorldTimeSeconds();
	}
	++MatchSnapshotRevision;

	UE_LOG(LogPHGameState, Log, TEXT("Match phase: %s -> %s"),
		*UEnum::GetValueAsString(OldPhase),
		*UEnum::GetValueAsString(NewPhase));

	OnMatchPhaseChanged.Broadcast(NewPhase);
	ForceNetUpdate();
}

void APHGameState::OnRep_MatchPhase()
{
	OnMatchPhaseChanged.Broadcast(MatchPhase);
}

float APHGameState::GetMatchElapsedTime() const
{
	if (MatchPhase == EPHMatchPhase::WaitingToStart || MatchStartServerTime <= 0.f)
	{
		return 0.f;
	}

	if (GetWorld())
	{
		return FMath::Max(0.f, GetServerWorldTimeSeconds() - MatchStartServerTime);
	}
	return 0.f;
}

void APHGameState::IncrementMobKills(int32 Count)
{
	TotalMobKills += FMath::Max(0, Count);
}

void APHGameState::ApplyRunSnapshot(const ERunState NewState, const FRunSessionData& NewSession)
{
	if (!HasAuthority())
	{
		return;
	}

	RunState = NewState;
	RunSession = NewSession;
	RunSnapshotServerTime = GetServerWorldTimeSeconds();
	++RunSnapshotRevision;
	OnReplicatedRunChanged.Broadcast(RunState, RunSession);
	ForceNetUpdate();
}

float APHGameState::GetRunElapsedTime() const
{
	float Elapsed = FMath::Max(0.f, RunSession.TimeElapsed);
	if (RunState == ERunState::Active)
	{
		Elapsed += FMath::Max(0.f, GetServerWorldTimeSeconds() - RunSnapshotServerTime);
	}
	return Elapsed;
}

void APHGameState::OnRep_RunSnapshot()
{
	OnReplicatedRunChanged.Broadcast(RunState, RunSession);
}
