#include "Tower/Subsystems/RunSubsystem.h"
#include "Tower/Library/Structs/RunStructs.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Framework/GameModes/PHGameState.h"

DEFINE_LOG_CATEGORY(LogRunSubsystem);

void URunSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG(LogRunSubsystem, Log, TEXT("URunSubsystem initialized"));
}

void URunSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void URunSubsystem::StartRun(const int32 RunSeed, const int32 Difficulty)
{
	if (!HasServerAuthority())
	{
		UE_LOG(LogRunSubsystem, Warning, TEXT("StartRun rejected: run mutations are server-authoritative."));
		return;
	}

	if (RunState == ERunState::Active)
	{
		UE_LOG(LogRunSubsystem, Warning,
			TEXT("StartRun called while a run is already active - ignored."));
		return;
	}

	ResetState();
	RunState = ERunState::Active;
	SessionData.RunID = FGuid::NewGuid();
	SessionData.RunSeed = RunSeed != 0 ? RunSeed : static_cast<int32>(FPlatformTime::Cycles());
	SessionData.Difficulty = FMath::Max(1, Difficulty);
	SessionData.CurrentFloor = 1;

	// Real time, not world time: this is a GameInstance subsystem, but
	// World->GetTimeSeconds() resets to ~0 on every OpenLevel - a run that
	// crosses map loads would report nonsense elapsed time. FPlatformTime is
	// monotonic for the process. (Includes pause time, which is the standard
	// roguelite run-clock behavior.)
	RunStartTimeSeconds = FPlatformTime::Seconds();

	SyncToGameState();
	UE_LOG(LogRunSubsystem, Log, TEXT("Run started - ID=%s Seed=%d Difficulty=%d"),
		*SessionData.RunID.ToString(), SessionData.RunSeed, SessionData.Difficulty);
	OnRunStarted.Broadcast();
}

void URunSubsystem::AdvanceFloor()
{
	if (!HasServerAuthority() || RunState != ERunState::Active)
	{
		UE_LOG(LogRunSubsystem, Warning,
			TEXT("AdvanceFloor called with no active run - ignored."));
		return;
	}

	++SessionData.CurrentFloor;
	++SessionData.FloorsCleared;

	SessionData.TimeElapsed = GetElapsedTime();
	SyncToGameState();
	UE_LOG(LogRunSubsystem, Log, TEXT("Floor advanced - now on Floor %d"), SessionData.CurrentFloor);
	OnFloorAdvanced.Broadcast(SessionData.CurrentFloor);
}

void URunSubsystem::EndRun(const ERunEndReason EndReason)
{
	if (!HasServerAuthority() || RunState != ERunState::Active)
	{
		return;
	}

	SessionData.TimeElapsed = GetElapsedTime();
	SessionData.EndReason = EndReason;
	RunState = ERunState::Ended;
	SyncToGameState();

	UE_LOG(LogRunSubsystem, Log,
		TEXT("Run ended - Floors cleared: %d  |  Time: %.1fs"),
		SessionData.FloorsCleared,
		SessionData.TimeElapsed);

	OnRunEnded.Broadcast(SessionData);

}

void URunSubsystem::RegisterKill()
{
	if (!HasServerAuthority() || RunState != ERunState::Active)
	{
		return;
	}

	++SessionData.TotalKills;
	SessionData.TimeElapsed = GetElapsedTime();
	SyncToGameState();
}

float URunSubsystem::GetElapsedTime() const
{
	if (const UWorld* World = GetWorld())
	{
		if (const APHGameState* GameState = World->GetGameState<APHGameState>();
			GameState && !HasServerAuthority())
		{
			return GameState->GetRunElapsedTime();
		}
	}

	if (RunState != ERunState::Active)
	{
		return SessionData.TimeElapsed;
	}

	// Matches the FPlatformTime base captured in StartRun - survives OpenLevel.
	// Subtract in double, then narrow: the difference is small even when the
	// absolute timestamps are large.
	return static_cast<float>(FPlatformTime::Seconds() - RunStartTimeSeconds);
}

void URunSubsystem::ResetState()
{
	SessionData = FRunSessionData();
}

bool URunSubsystem::HasServerAuthority() const
{
	const UWorld* World = GetWorld();
	return World && World->GetNetMode() != NM_Client;
}

ERunState URunSubsystem::GetRunState() const
{
	if (const UWorld* World = GetWorld())
	{
		if (const APHGameState* GameState = World->GetGameState<APHGameState>())
		{
			return GameState->RunState;
		}
	}
	return RunState;
}

bool URunSubsystem::IsRunActive() const
{
	return GetRunState() == ERunState::Active;
}

int32 URunSubsystem::GetCurrentFloor() const
{
	return GetSessionData().CurrentFloor;
}

int32 URunSubsystem::GetTotalKills() const
{
	return GetSessionData().TotalKills;
}

FRunSessionData URunSubsystem::GetSessionData() const
{
	if (const UWorld* World = GetWorld())
	{
		if (const APHGameState* GameState = World->GetGameState<APHGameState>())
		{
			FRunSessionData Result = GameState->RunSession;
			Result.TimeElapsed = GameState->GetRunElapsedTime();
			return Result;
		}
	}

	FRunSessionData Result = SessionData;
	Result.TimeElapsed = GetElapsedTime();
	return Result;
}

void URunSubsystem::SyncToGameState()
{
	if (!HasServerAuthority())
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		if (APHGameState* GameState = World->GetGameState<APHGameState>())
		{
			FRunSessionData Snapshot = SessionData;
			Snapshot.TimeElapsed = RunState == ERunState::Active ? GetElapsedTime() : SessionData.TimeElapsed;
			GameState->ApplyRunSnapshot(RunState, Snapshot);
		}
	}
}
