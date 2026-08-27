#include "Tower/Subsystems/RunSubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Framework/GameModes/PHGameState.h"
#include "Framework/Player/PHPlayerState.h"
#include "TimerManager.h"
#include "Tower/Library/FunctionLibraries/RunSeedFunctionLibrary.h"
#include "Tower/Library/Structs/RunStructs.h"

DEFINE_LOG_CATEGORY(LogRunSubsystem);

void URunSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG(LogRunSubsystem, Log, TEXT("URunSubsystem initialized"));
}

void URunSubsystem::Deinitialize()
{
	if (const UWorld* World = GetWorld(); World && SnapshotFlushTimer.IsValid())
	{
		World->GetTimerManager().ClearTimer(SnapshotFlushTimer);
	}
	Super::Deinitialize();
}

// ---------------------------------------------------------------------------
// Run lifecycle
// ---------------------------------------------------------------------------

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

	// Real time, not world time: this is a GameInstance subsystem, but
	// World->GetTimeSeconds() resets to ~0 on every OpenLevel - a run that
	// crosses map loads would report nonsense elapsed time. FPlatformTime is
	// monotonic for the process. (Includes pause time, which is the standard
	// roguelite run-clock behavior.)
	RunStartTimeSeconds = FPlatformTime::Seconds();

	UE_LOG(LogRunSubsystem, Log, TEXT("Run started - ID=%s Seed=%d Difficulty=%d"),
		*SessionData.RunID.ToString(), SessionData.RunSeed, SessionData.Difficulty);

	OnRunStarted.Broadcast();
	BeginFloor(1);
}

void URunSubsystem::AdvanceFloor()
{
	if (!HasServerAuthority() || RunState != ERunState::Active)
	{
		UE_LOG(LogRunSubsystem, Warning,
			TEXT("AdvanceFloor called with no active run - ignored."));
		return;
	}

	// Advancing is gated on the floor actually being finished. Without this any
	// stray caller could skip a floor's objective and reward step.
	if (!CanAdvanceFloor())
	{
		UE_LOG(LogRunSubsystem, Warning,
			TEXT("AdvanceFloor rejected: floor %d is in phase %s, not RewardReady."),
			SessionData.Floor.FloorNumber,
			*UEnum::GetDisplayValueAsText(SessionData.Floor.Phase).ToString());
		return;
	}

	SetFloorPhase(EFloorPhase::Transitioning);

	++SessionData.FloorsCleared;
	SessionData.TimeElapsed = GetElapsedTime();

	const int32 NextFloor = SessionData.Floor.FloorNumber + 1;

	// A floor transition is the natural recovery point: anyone who went down on
	// the cleared floor rejoins for the next one.
	ReviveDownedPlayersForNewFloor();

	if (FloorPlan.FloorsToCompleteRun > 0 && SessionData.FloorsCleared >= FloorPlan.FloorsToCompleteRun)
	{
		UE_LOG(LogRunSubsystem, Log, TEXT("Final floor cleared - run complete."));
		EndRun(ERunEndReason::Completed);
		return;
	}

	BeginFloor(NextFloor);
	OnFloorAdvanced.Broadcast(SessionData.Floor.FloorNumber);
}

void URunSubsystem::EndRun(const ERunEndReason EndReason)
{
	if (!HasServerAuthority() || RunState != ERunState::Active)
	{
		return;
	}

	SessionData.TimeElapsed = GetElapsedTime();
	SessionData.EndReason = EndReason;
	SessionData.Floor.Phase = EFloorPhase::None;
	RunState = ERunState::Ended;

	if (const UWorld* World = GetWorld(); World && SnapshotFlushTimer.IsValid())
	{
		World->GetTimerManager().ClearTimer(SnapshotFlushTimer);
	}

	PublishSnapshot();

	UE_LOG(LogRunSubsystem, Log,
		TEXT("Run ended (%s) - Floors cleared: %d  |  Kills: %d  |  Time: %.1fs"),
		*UEnum::GetDisplayValueAsText(EndReason).ToString(),
		SessionData.FloorsCleared,
		SessionData.TotalKills,
		SessionData.TimeElapsed);

	OnRunEnded.Broadcast(SessionData);
}

// ---------------------------------------------------------------------------
// Floor lifecycle
// ---------------------------------------------------------------------------

void URunSubsystem::BeginFloor(const int32 FloorNumber)
{
	SessionData.Floor = BuildFloorData(FloorNumber);
	SessionData.CurrentFloor = FloorNumber;

	MirrorFloorToPlayerStates();
	PublishSnapshot();

	UE_LOG(LogRunSubsystem, Log,
		TEXT("Floor %d begun - Type=%s Seed=%d Difficulty=%d Modifiers=%d"),
		SessionData.Floor.FloorNumber,
		*UEnum::GetDisplayValueAsText(SessionData.Floor.FloorType).ToString(),
		SessionData.Floor.FloorSeed,
		SessionData.Floor.Difficulty,
		SessionData.Floor.Modifiers.Num());

	// Blueprint listens here and drives Dungeon Architect with FloorSeed, then
	// calls NotifyFloorGenerated once the layout and spawners are in place.
	OnFloorGenerationRequested.Broadcast(SessionData.Floor);
}

FRunFloorData URunSubsystem::BuildFloorData(const int32 FloorNumber) const
{
	FRunFloorData Data;
	Data.FloorNumber = FloorNumber;
	Data.FloorSeed = URunSeedFunctionLibrary::DeriveFloorSeed(SessionData.RunSeed, FloorNumber);
	Data.Phase = EFloorPhase::Generating;
	Data.Difficulty = SessionData.Difficulty + (FMath::Max(0, FloorNumber - 1) * FloorPlan.DifficultyPerFloor);

	FRandomStream Stream(Data.FloorSeed);

	// Floor type. Boss outranks elite, and both outrank the treasure roll, so
	// the milestone cadence stays predictable while the filler varies.
	const bool bIsBossFloor = FloorPlan.BossFloorInterval > 0
		&& (FloorNumber % FloorPlan.BossFloorInterval) == 0;
	const bool bIsEliteFloor = !bIsBossFloor
		&& FloorPlan.EliteFloorInterval > 0
		&& (FloorNumber % FloorPlan.EliteFloorInterval) == 0;

	if (bIsBossFloor)
	{
		Data.FloorType = EFloorType::Boss;
		Data.Objective = EFloorObjective::KillBoss;
	}
	else if (bIsEliteFloor)
	{
		Data.FloorType = EFloorType::Elite;
		Data.Objective = EFloorObjective::ClearAllEnemies;
	}
	else if (FloorPlan.TreasureFloorChance > 0.f && Stream.FRand() < FloorPlan.TreasureFloorChance)
	{
		Data.FloorType = EFloorType::Treasure;
		Data.Objective = EFloorObjective::ReachExit;
	}
	else
	{
		Data.FloorType = EFloorType::Combat;
		Data.Objective = EFloorObjective::ClearAllEnemies;
	}

	// Floor modifiers. Drawn without replacement from the configured pool so a
	// floor never rolls the same rule twice.
	int32 ModifierCount = FloorPlan.ModifiersPerFloor;
	if (bIsBossFloor || bIsEliteFloor)
	{
		ModifierCount += FloorPlan.BonusModifiersOnHardFloors;
	}

	if (ModifierCount > 0 && FloorPlan.ModifierPool.Num() > 0)
	{
		TArray<FGameplayTag> Available;
		FloorPlan.ModifierPool.GetGameplayTagArray(Available);

		const int32 Draws = FMath::Min(ModifierCount, Available.Num());
		for (int32 Draw = 0; Draw < Draws; ++Draw)
		{
			const int32 PickIndex = Stream.RandRange(0, Available.Num() - 1);
			Data.Modifiers.AddTag(Available[PickIndex]);
			Available.RemoveAtSwap(PickIndex, EAllowShrinking::No);
		}
	}

	return Data;
}

void URunSubsystem::NotifyFloorGenerated(const int32 ObjectiveTarget)
{
	if (!HasServerAuthority() || RunState != ERunState::Active)
	{
		return;
	}

	if (SessionData.Floor.Phase != EFloorPhase::Generating)
	{
		UE_LOG(LogRunSubsystem, Warning,
			TEXT("NotifyFloorGenerated ignored: floor %d is in phase %s, not Generating."),
			SessionData.Floor.FloorNumber,
			*UEnum::GetDisplayValueAsText(SessionData.Floor.Phase).ToString());
		return;
	}

	SessionData.Floor.ObjectiveTarget = FMath::Max(0, ObjectiveTarget);
	SessionData.Floor.ObjectiveProgress = 0;
	SetFloorPhase(EFloorPhase::InProgress);

	UE_LOG(LogRunSubsystem, Log, TEXT("Floor %d generated - objective target %d."),
		SessionData.Floor.FloorNumber, SessionData.Floor.ObjectiveTarget);

	// A floor with nothing to do is already finished (e.g. a treasure floor
	// whose only requirement is reaching the exit).
	if (SessionData.Floor.ObjectiveTarget == 0)
	{
		CompleteFloorObjective();
	}
}

void URunSubsystem::AddObjectiveProgress(const int32 Delta)
{
	if (!HasServerAuthority() || RunState != ERunState::Active)
	{
		return;
	}

	if (SessionData.Floor.Phase != EFloorPhase::InProgress || Delta == 0)
	{
		return;
	}

	SessionData.Floor.ObjectiveProgress =
		FMath::Clamp(SessionData.Floor.ObjectiveProgress + Delta, 0, SessionData.Floor.ObjectiveTarget);

	if (SessionData.Floor.IsObjectiveComplete())
	{
		CompleteFloorObjective();
		return;
	}

	// Objective progress is a counter, not a milestone: let it ride the throttle.
	MarkSnapshotDirty();
}

void URunSubsystem::CompleteFloorObjective()
{
	if (!HasServerAuthority() || RunState != ERunState::Active)
	{
		return;
	}

	if (SessionData.Floor.Phase != EFloorPhase::InProgress)
	{
		return;
	}

	SessionData.Floor.ObjectiveProgress = SessionData.Floor.ObjectiveTarget;
	SetFloorPhase(EFloorPhase::ObjectiveComplete);

	UE_LOG(LogRunSubsystem, Log, TEXT("Floor %d objective complete."), SessionData.Floor.FloorNumber);
	OnFloorObjectiveComplete.Broadcast(SessionData.Floor);
}

void URunSubsystem::NotifyRewardGranted()
{
	if (!HasServerAuthority() || RunState != ERunState::Active)
	{
		return;
	}

	if (SessionData.Floor.Phase != EFloorPhase::ObjectiveComplete)
	{
		UE_LOG(LogRunSubsystem, Warning,
			TEXT("NotifyRewardGranted ignored: floor %d is in phase %s, not ObjectiveComplete."),
			SessionData.Floor.FloorNumber,
			*UEnum::GetDisplayValueAsText(SessionData.Floor.Phase).ToString());
		return;
	}

	SetFloorPhase(EFloorPhase::RewardReady);

	UE_LOG(LogRunSubsystem, Log, TEXT("Floor %d rewards granted - exit may open."),
		SessionData.Floor.FloorNumber);
	OnFloorRewardReady.Broadcast(SessionData.Floor);
}

void URunSubsystem::SetFloorPhase(const EFloorPhase NewPhase)
{
	if (SessionData.Floor.Phase == NewPhase)
	{
		return;
	}

	SessionData.Floor.Phase = NewPhase;
	PublishSnapshot();
}

FRunFloorData URunSubsystem::GetCurrentFloorData() const
{
	return GetSessionData().Floor;
}

EFloorPhase URunSubsystem::GetFloorPhase() const
{
	return GetSessionData().Floor.Phase;
}

bool URunSubsystem::CanAdvanceFloor() const
{
	return GetFloorPhase() == EFloorPhase::RewardReady;
}

int32 URunSubsystem::GetFloorSeed() const
{
	return GetSessionData().Floor.FloorSeed;
}

int32 URunSubsystem::GetEncounterSeed(const int32 EncounterIndex) const
{
	return URunSeedFunctionLibrary::DeriveEncounterSeed(GetFloorSeed(), EncounterIndex);
}

void URunSubsystem::SetFloorPlan(const FRunFloorPlan& NewPlan)
{
	if (!HasServerAuthority())
	{
		return;
	}
	FloorPlan = NewPlan;
}

// ---------------------------------------------------------------------------
// Party / death
// ---------------------------------------------------------------------------

TArray<APHPlayerState*> URunSubsystem::GetPartyPlayerStates() const
{
	TArray<APHPlayerState*> Result;

	const UWorld* World = GetWorld();
	const AGameStateBase* GameState = World ? World->GetGameState() : nullptr;
	if (!GameState)
	{
		return Result;
	}

	Result.Reserve(GameState->PlayerArray.Num());
	for (APlayerState* PlayerState : GameState->PlayerArray)
	{
		if (APHPlayerState* HunterState = Cast<APHPlayerState>(PlayerState))
		{
			Result.Add(HunterState);
		}
	}
	return Result;
}

void URunSubsystem::SetPlayerRunState(APHPlayerState* Player, const ERunPlayerState NewState)
{
	if (!IsValid(Player) || Player->RunPlayerState == NewState)
	{
		return;
	}

	Player->SetRunPlayerStateMirror(NewState);
	OnRunPlayerStateChanged.Broadcast(Player, NewState);
}

void URunSubsystem::NotifyPlayerDowned(APHPlayerState* Player)
{
	if (!HasServerAuthority() || RunState != ERunState::Active || !IsValid(Player))
	{
		return;
	}

	if (Player->RunPlayerState != ERunPlayerState::Alive)
	{
		return;
	}

	SetPlayerRunState(Player, ERunPlayerState::Downed);
	UE_LOG(LogRunSubsystem, Log, TEXT("%s is down."), *Player->GetPlayerName());

	EvaluatePartyState();
}

void URunSubsystem::NotifyPlayerDead(APHPlayerState* Player)
{
	if (!HasServerAuthority() || RunState != ERunState::Active || !IsValid(Player))
	{
		return;
	}

	if (Player->RunPlayerState == ERunPlayerState::Out)
	{
		return;
	}

	// Dead still occupies a party slot and can be brought back by a revive that
	// reaches them; Out is the terminal state that counts toward a wipe.
	SetPlayerRunState(Player, ERunPlayerState::Dead);
	UE_LOG(LogRunSubsystem, Log, TEXT("%s died."), *Player->GetPlayerName());

	EvaluatePartyState();
}

bool URunSubsystem::RevivePlayer(APHPlayerState* Player)
{
	if (!HasServerAuthority() || RunState != ERunState::Active || !IsValid(Player))
	{
		return false;
	}

	if (Player->RunPlayerState != ERunPlayerState::Downed
		&& Player->RunPlayerState != ERunPlayerState::Dead)
	{
		return false;
	}

	SetPlayerRunState(Player, ERunPlayerState::Alive);
	UE_LOG(LogRunSubsystem, Log, TEXT("%s revived."), *Player->GetPlayerName());
	return true;
}

void URunSubsystem::ReviveDownedPlayersForNewFloor()
{
	if (!HasServerAuthority() || RunState != ERunState::Active)
	{
		return;
	}

	for (APHPlayerState* Player : GetPartyPlayerStates())
	{
		if (Player->RunPlayerState == ERunPlayerState::Downed
			|| Player->RunPlayerState == ERunPlayerState::Dead)
		{
			SetPlayerRunState(Player, ERunPlayerState::Alive);
		}
	}
}

int32 URunSubsystem::GetLivingPlayerCount() const
{
	int32 Count = 0;
	for (const APHPlayerState* Player : GetPartyPlayerStates())
	{
		if (Player->RunPlayerState == ERunPlayerState::Alive
			|| Player->RunPlayerState == ERunPlayerState::Downed)
		{
			++Count;
		}
	}
	return Count;
}

bool URunSubsystem::IsPartyWiped() const
{
	const TArray<APHPlayerState*> Party = GetPartyPlayerStates();
	if (Party.Num() == 0)
	{
		// No party to wipe. Never end a run on an empty player array - that
		// state occurs briefly during travel and would abort the run.
		return false;
	}

	return GetLivingPlayerCount() == 0;
}

void URunSubsystem::EvaluatePartyState()
{
	if (!HasServerAuthority() || RunState != ERunState::Active)
	{
		return;
	}

	if (!IsPartyWiped())
	{
		return;
	}

	// Nobody left who is Alive or Downed. Anyone still Dead becomes Out, and the
	// run ends. Solo play reaches this on the first death, which is the intended
	// "run over, restart" rule; the stash keeps whatever was secured.
	for (APHPlayerState* Player : GetPartyPlayerStates())
	{
		SetPlayerRunState(Player, ERunPlayerState::Out);
	}

	UE_LOG(LogRunSubsystem, Log, TEXT("Party wipe - ending run."));
	EndRun(ERunEndReason::PartyWipe);
}

// ---------------------------------------------------------------------------
// Counters and queries
// ---------------------------------------------------------------------------

void URunSubsystem::RegisterKill()
{
	if (!HasServerAuthority() || RunState != ERunState::Active)
	{
		return;
	}

	++SessionData.TotalKills;

	// The cheap live counter replicates on its own through normal property
	// dirtiness. The full run snapshot is NOT forced here - at ten players and a
	// dense floor that would push the whole struct to everyone on every death.
	if (const UWorld* World = GetWorld())
	{
		if (APHGameState* GameState = World->GetGameState<APHGameState>())
		{
			GameState->IncrementMobKills(1);
		}
	}

	// Kills only feed the objective on floors that are counting them.
	if (SessionData.Floor.Phase == EFloorPhase::InProgress
		&& (SessionData.Floor.Objective == EFloorObjective::ClearAllEnemies
			|| SessionData.Floor.Objective == EFloorObjective::KillBoss))
	{
		AddObjectiveProgress(1);
		return;
	}

	MarkSnapshotDirty();
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
	bSnapshotDirty = false;
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

// ---------------------------------------------------------------------------
// Replication
// ---------------------------------------------------------------------------

void URunSubsystem::PublishSnapshot()
{
	bSnapshotDirty = false;
	SyncToGameState();
}

void URunSubsystem::MarkSnapshotDirty()
{
	if (!HasServerAuthority())
	{
		return;
	}

	bSnapshotDirty = true;

	UWorld* World = GetWorld();
	if (!World || World->GetTimerManager().IsTimerActive(SnapshotFlushTimer))
	{
		return;
	}

	World->GetTimerManager().SetTimer(
		SnapshotFlushTimer,
		FTimerDelegate::CreateUObject(this, &URunSubsystem::FlushSnapshotIfDirty),
		SnapshotFlushInterval,
		/*bLoop*/ false);
}

void URunSubsystem::FlushSnapshotIfDirty()
{
	if (bSnapshotDirty)
	{
		PublishSnapshot();
	}
}

void URunSubsystem::MirrorFloorToPlayerStates() const
{
	for (APHPlayerState* Player : GetPartyPlayerStates())
	{
		Player->SetRunFloorMirror(SessionData.Floor.FloorNumber);
	}
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
			Snapshot.Revision = ++SessionData.Revision;
			GameState->ApplyRunSnapshot(RunState, Snapshot);
		}
	}
}
