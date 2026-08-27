#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tower/Library/Structs/RunStructs.h"
#include "RunSubsystem.generated.h"

class APHPlayerState;

DECLARE_LOG_CATEGORY_EXTERN(LogRunSubsystem, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRunStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFloorAdvanced, int32, NewFloor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRunEnded, FRunSessionData, SessionData);

/** Fired when a floor's descriptor is ready and Dungeon Architect should build it. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFloorGenerationRequested, FRunFloorData, FloorData);
/** Fired when the floor objective is satisfied and rewards may be granted. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFloorObjectiveComplete, FRunFloorData, FloorData);
/** Fired when the floor's reward step is done and the exit portal may open. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFloorRewardReady, FRunFloorData, FloorData);
/** Fired when a player's run status changes (downed, dead, revived). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRunPlayerStateChanged, APHPlayerState*, Player, ERunPlayerState, NewState);

/**
 * Data-driven shape of a run's floor progression.
 *
 * Defaults produce an all-Combat tower with a boss every fifth floor, which is
 * enough for the first playable run. Designers can replace it wholesale from
 * Blueprint via SetFloorPlan without any code change.
 */
USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FRunFloorPlan
{
	GENERATED_BODY()

	/** Every Nth floor is a Boss floor. 0 disables boss floors. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Floor Plan", meta = (ClampMin = "0"))
	int32 BossFloorInterval = 5;

	/** Every Nth floor is an Elite floor, unless it is already a boss floor. 0 disables. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Floor Plan", meta = (ClampMin = "0"))
	int32 EliteFloorInterval = 3;

	/** Chance (0-1) that an ordinary floor becomes a Treasure floor instead. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Floor Plan", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float TreasureFloorChance = 0.f;

	/** Added to floor difficulty per floor descended. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Floor Plan", meta = (ClampMin = "0"))
	int32 DifficultyPerFloor = 1;

	/** Modifier tags eligible to roll onto a floor. Empty means no floor modifiers. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Floor Plan")
	FGameplayTagContainer ModifierPool;

	/** How many modifiers roll onto an ordinary floor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Floor Plan", meta = (ClampMin = "0"))
	int32 ModifiersPerFloor = 0;

	/** Extra modifiers granted to Elite and Boss floors. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Floor Plan", meta = (ClampMin = "0"))
	int32 BonusModifiersOnHardFloors = 1;

	/** Floor count that completes the run. 0 means the tower is endless. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Floor Plan", meta = (ClampMin = "0"))
	int32 FloorsToCompleteRun = 5;
};

/**
 * Authoritative owner of party run state: run identity, seed, difficulty, the
 * current floor, and the run lifecycle.
 *
 * This is the ONE floor authority. PlayerStates carry per-player run data
 * (kills, run status) and mirror the party floor for display, but never
 * advance it. Clients read state through APHGameState's replicated snapshot;
 * every mutator here is server-only.
 *
 * Lifecycle:
 *   StartRun -> BeginFloor -> (Blueprint builds the floor)
 *            -> NotifyFloorGenerated -> InProgress
 *            -> objective progress -> ObjectiveComplete
 *            -> NotifyRewardGranted -> RewardReady -> portal opens
 *            -> AdvanceFloor -> BeginFloor(next) ... -> EndRun
 */
UCLASS()
class ALS_PROJECTHUNTER_API URunSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// ---- Run lifecycle -------------------------------------------------

	UFUNCTION(BlueprintCallable, Category = "Run")
	void StartRun(int32 RunSeed = 0, int32 Difficulty = 1);

	/**
	 * Ends the current floor and begins the next. Party-wide and server-only:
	 * this is the only path that changes the floor number.
	 */
	UFUNCTION(BlueprintCallable, Category = "Run")
	void AdvanceFloor();

	UFUNCTION(BlueprintCallable, Category = "Run")
	void EndRun(ERunEndReason EndReason = ERunEndReason::Quit);

	// ---- Floor lifecycle -----------------------------------------------

	/**
	 * Call once Dungeon Architect has finished building the floor and spawners
	 * know how much work the objective needs (enemy count, boss count, ...).
	 * Moves the floor from Generating to InProgress.
	 */
	UFUNCTION(BlueprintCallable, Category = "Run|Floor")
	void NotifyFloorGenerated(int32 ObjectiveTarget);

	/** Adds progress toward the current floor objective and completes it when met. */
	UFUNCTION(BlueprintCallable, Category = "Run|Floor")
	void AddObjectiveProgress(int32 Delta = 1);

	/** Forces the current floor objective complete regardless of progress. */
	UFUNCTION(BlueprintCallable, Category = "Run|Floor")
	void CompleteFloorObjective();

	/**
	 * Call after floor rewards have been granted and any secure-to-stash
	 * opportunity is available. Moves the floor to RewardReady so the exit
	 * portal can open.
	 */
	UFUNCTION(BlueprintCallable, Category = "Run|Floor")
	void NotifyRewardGranted();

	UFUNCTION(BlueprintPure, Category = "Run|Floor")
	FRunFloorData GetCurrentFloorData() const;

	UFUNCTION(BlueprintPure, Category = "Run|Floor")
	EFloorPhase GetFloorPhase() const;

	/** True once the floor objective is met and rewards have been granted. */
	UFUNCTION(BlueprintPure, Category = "Run|Floor")
	bool CanAdvanceFloor() const;

	/** Seed for this floor. Hand this to Dungeon Architect. */
	UFUNCTION(BlueprintPure, Category = "Run|Floor")
	int32 GetFloorSeed() const;

	/** Derived seed for one encounter on the current floor. */
	UFUNCTION(BlueprintPure, Category = "Run|Floor")
	int32 GetEncounterSeed(int32 EncounterIndex) const;

	UFUNCTION(BlueprintCallable, Category = "Run|Floor")
	void SetFloorPlan(const FRunFloorPlan& NewPlan);

	UFUNCTION(BlueprintPure, Category = "Run|Floor")
	FRunFloorPlan GetFloorPlan() const { return FloorPlan; }

	// ---- Party / death -------------------------------------------------

	/**
	 * A player went down. Solo, this ends the run immediately. In a party the
	 * run continues while anyone is still Alive or Downed, leaving room for a
	 * revive mechanic to bring them back.
	 */
	UFUNCTION(BlueprintCallable, Category = "Run|Party")
	void NotifyPlayerDowned(APHPlayerState* Player);

	/** A downed player was not saved in time and is now out for this floor. */
	UFUNCTION(BlueprintCallable, Category = "Run|Party")
	void NotifyPlayerDead(APHPlayerState* Player);

	/** Brings a Downed or Dead player back to Alive. Returns false if not revivable. */
	UFUNCTION(BlueprintCallable, Category = "Run|Party")
	bool RevivePlayer(APHPlayerState* Player);

	/** Restores every non-Out player to Alive. Called on floor transition. */
	UFUNCTION(BlueprintCallable, Category = "Run|Party")
	void ReviveDownedPlayersForNewFloor();

	UFUNCTION(BlueprintPure, Category = "Run|Party")
	int32 GetLivingPlayerCount() const;

	/** True when nobody is left who is Alive or Downed. */
	UFUNCTION(BlueprintPure, Category = "Run|Party")
	bool IsPartyWiped() const;

	// ---- Queries -------------------------------------------------------

	UFUNCTION(BlueprintPure, Category = "Run")
	int32 GetCurrentFloor() const;

	UFUNCTION(BlueprintPure, Category = "Run")
	bool IsRunActive() const;

	UFUNCTION(BlueprintPure, Category = "Run")
	ERunState GetRunState() const;

	UFUNCTION(BlueprintPure, Category = "Run")
	FRunSessionData GetSessionData() const;

	UFUNCTION(BlueprintPure, Category = "Run")
	float GetElapsedTime() const;

	UFUNCTION(BlueprintCallable, Category = "Run")
	void RegisterKill();

	UFUNCTION(BlueprintPure, Category = "Run")
	int32 GetTotalKills() const;

	/** Re-publishes persistent GameInstance state after a map transition creates a new GameState. */
	void SyncToGameState();

	// ---- Events --------------------------------------------------------

	UPROPERTY(BlueprintAssignable, Category = "Run|Events")
	FOnRunStarted OnRunStarted;

	UPROPERTY(BlueprintAssignable, Category = "Run|Events")
	FOnFloorAdvanced OnFloorAdvanced;

	UPROPERTY(BlueprintAssignable, Category = "Run|Events")
	FOnRunEnded OnRunEnded;

	UPROPERTY(BlueprintAssignable, Category = "Run|Events")
	FOnFloorGenerationRequested OnFloorGenerationRequested;

	UPROPERTY(BlueprintAssignable, Category = "Run|Events")
	FOnFloorObjectiveComplete OnFloorObjectiveComplete;

	UPROPERTY(BlueprintAssignable, Category = "Run|Events")
	FOnFloorRewardReady OnFloorRewardReady;

	UPROPERTY(BlueprintAssignable, Category = "Run|Events")
	FOnRunPlayerStateChanged OnRunPlayerStateChanged;

private:
	UPROPERTY()
	ERunState RunState = ERunState::Inactive;

	// Process time is used because OpenLevel resets world time during a run.
	double RunStartTimeSeconds = 0.0;

	UPROPERTY()
	FRunSessionData SessionData;

	UPROPERTY()
	FRunFloorPlan FloorPlan;

	/**
	 * Kill counting is high frequency and low value per event, so it does not
	 * force a snapshot. This flag defers the full sync to the next throttled
	 * flush; phase changes still sync immediately.
	 */
	bool bSnapshotDirty = false;
	FTimerHandle SnapshotFlushTimer;

	/** Seconds between throttled snapshot flushes while only counters changed. */
	static constexpr float SnapshotFlushInterval = 1.0f;

	void ResetState();
	bool HasServerAuthority() const;

	/** Builds the descriptor for a floor and requests generation. */
	void BeginFloor(int32 FloorNumber);
	FRunFloorData BuildFloorData(int32 FloorNumber) const;
	void SetFloorPhase(EFloorPhase NewPhase);

	/** Immediate snapshot publish; use for phase and lifecycle changes. */
	void PublishSnapshot();
	/** Defers a publish to the throttle timer; use for counters. */
	void MarkSnapshotDirty();
	void FlushSnapshotIfDirty();

	/** Pushes the authoritative floor number onto every PlayerState for display. */
	void MirrorFloorToPlayerStates() const;

	void SetPlayerRunState(APHPlayerState* Player, ERunPlayerState NewState);
	void EvaluatePartyState();

	TArray<APHPlayerState*> GetPartyPlayerStates() const;
};
