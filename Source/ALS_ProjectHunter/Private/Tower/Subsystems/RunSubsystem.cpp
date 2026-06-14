#include "Tower/Subsystems/RunSubsystem.h"
#include "Tower/Library/RunStructs.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

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

	// Real time, not world time: this is a GameInstance subsystem, but
	// World->GetTimeSeconds() resets to ~0 on every OpenLevel — a run that
	// crosses map loads would report nonsense elapsed time. FPlatformTime is
	// monotonic for the process. (Includes pause time, which is the standard
	// roguelite run-clock behavior.)
	RunStartTimeSeconds = FPlatformTime::Seconds();

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

float URunSubsystem::GetElapsedTime() const
{
	if (!bRunActive)
	{
		return SessionData.TimeElapsed;
	}

	// Matches the FPlatformTime base captured in StartRun — survives OpenLevel.
	// Subtract in double, then narrow: the difference is small even when the
	// absolute timestamps are large.
	return static_cast<float>(FPlatformTime::Seconds() - RunStartTimeSeconds);
}

void URunSubsystem::ResetState()
{
	SessionData = FRunSessionData();
}
