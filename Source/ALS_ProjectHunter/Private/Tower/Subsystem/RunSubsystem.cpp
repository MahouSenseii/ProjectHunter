// Tower/Subsystem/RunSubsystem.cpp

#include "Tower/Subsystem/RunSubsystem.h"
#include "Tower/Library/RunStructs.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

DEFINE_LOG_CATEGORY(LogRunSubsystem);

// ─────────────────────────────────────────────────────────────────────────────
// LIFECYCLE
// ─────────────────────────────────────────────────────────────────────────────

void URunSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG(LogRunSubsystem, Log, TEXT("URunSubsystem initialized"));
}

void URunSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

// ─────────────────────────────────────────────────────────────────────────────
// RUN LIFECYCLE
// ─────────────────────────────────────────────────────────────────────────────

void URunSubsystem::StartRun()
{
	if (bRunActive)
	{
		UE_LOG(LogRunSubsystem, Warning,
			TEXT("StartRun called while a run is already active — ignored."));
		return;
	}

	CurrentFloor = 1;
	bRunActive   = true;

	UWorld* World = GetGameInstance()->GetWorld();
	RunStartTimeSeconds = World ? World->GetTimeSeconds() : 0.f;

	ResetState();

	UE_LOG(LogRunSubsystem, Log, TEXT("Run started — Floor 1"));
	OnRunStarted.Broadcast();
}

void URunSubsystem::AdvanceFloor()
{
	if (!bRunActive)
	{
		UE_LOG(LogRunSubsystem, Warning,
			TEXT("AdvanceFloor called with no active run — ignored."));
		return;
	}

	++CurrentFloor;
	++SessionData.FloorsCleared;

	UE_LOG(LogRunSubsystem, Log, TEXT("Floor advanced — now on Floor %d"), CurrentFloor);
	OnFloorAdvanced.Broadcast(CurrentFloor);
}

void URunSubsystem::EndRun()
{
	if (!bRunActive)
	{
		return;
	}

	bRunActive             = false;
	SessionData.TimeElapsed = GetElapsedTime();

	UE_LOG(LogRunSubsystem, Log,
		TEXT("Run ended — Floors cleared: %d  |  Time: %.1fs"),
		SessionData.FloorsCleared,
		SessionData.TimeElapsed);

	OnRunEnded.Broadcast(SessionData);

	// Reset floor counter so a fresh StartRun begins cleanly.
	CurrentFloor = 0;
}

void URunSubsystem::RegisterKill()
{
	if (!bRunActive)
	{
		return;
	}

	++SessionData.TotalKills;
}

// ─────────────────────────────────────────────────────────────────────────────
// GETTERS
// ─────────────────────────────────────────────────────────────────────────────

float URunSubsystem::GetElapsedTime() const
{
	if (!bRunActive)
	{
		// Run has ended — return the frozen value written in EndRun.
		return SessionData.TimeElapsed;
	}

	UWorld* World = GetGameInstance()->GetWorld();
	if (!World)
	{
		return 0.f;
	}

	return World->GetTimeSeconds() - RunStartTimeSeconds;
}

// ─────────────────────────────────────────────────────────────────────────────
// INTERNAL
// ─────────────────────────────────────────────────────────────────────────────

void URunSubsystem::ResetState()
{
	SessionData = FRunSessionData();
}
