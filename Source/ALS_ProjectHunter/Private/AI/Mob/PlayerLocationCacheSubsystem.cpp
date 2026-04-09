// AI/Mob/PlayerLocationCacheSubsystem.cpp
// PH-8.1 — Player Location Cache Subsystem (skeleton)

#include "AI/Mob/PlayerLocationCacheSubsystem.h"

#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

DEFINE_LOG_CATEGORY(LogPlayerLocationCache);

void UPlayerLocationCacheSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Snapshots.Reset();
	TimeSinceLastRefresh = 0.0f;
}

void UPlayerLocationCacheSubsystem::Deinitialize()
{
	Snapshots.Reset();
	Super::Deinitialize();
}

void UPlayerLocationCacheSubsystem::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	TimeSinceLastRefresh += DeltaSeconds;
	if (TimeSinceLastRefresh < RefreshIntervalSeconds)
	{
		return;
	}
	TimeSinceLastRefresh = 0.0f;

	RefreshSnapshots();
}

TStatId UPlayerLocationCacheSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UPlayerLocationCacheSubsystem, STATGROUP_Tickables);
}

void UPlayerLocationCacheSubsystem::RefreshSnapshots()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	Snapshots.Reset();

	const double Now = World->GetTimeSeconds();
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (!PC)
		{
			continue;
		}

		APawn* Pawn = PC->GetPawn();
		if (!Pawn)
		{
			continue;
		}

		FPlayerLocationSnapshot Snap;
		Snap.Controller = PC;
		Snap.Location = Pawn->GetActorLocation();
		Snap.Velocity = Pawn->GetVelocity();
		Snap.TimestampSeconds = Now;
		Snapshots.Add(MoveTemp(Snap));
	}

	// TODO PH-8.3: handle seamless travel and possession churn explicitly.
}
