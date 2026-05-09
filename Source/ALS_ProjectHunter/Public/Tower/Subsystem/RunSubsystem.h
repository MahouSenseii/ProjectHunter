// Tower/Subsystem/RunSubsystem.h
// Tracks the state of the active roguelite run across floor loads.
//
// WHY GameInstanceSubsystem?
//   Floor number and session metadata must survive level transitions — each floor
//   is a separate load. UWorldSubsystem is torn down on every floor change, so
//   UGameInstanceSubsystem is used instead; it persists for the lifetime of the
//   game instance.
//
// OWNER:    Current floor number, run-active flag, session metadata.
//
// What this subsystem must NOT own:
//   × Character stats       — lives on StatsManager / HunterAttributeSet.
//   × Inventory / equipment — lives on their own managers.
//   × Any direct reference to the player character.
//
// Lifecycle:
//   • StartRun()      — called when the player enters the tower (floor 1).
//   • AdvanceFloor()  — called by PortalActor when a floor is cleared.
//   • EndRun()        — called on player death; broadcasts session data then resets.
//
// Listeners:
//   • FloorManagerActor  → binds OnFloorAdvanced to trigger DA generation.
//   • End-of-run HUD     → binds OnRunEnded for the summary screen.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tower/Library/RunStructs.h"
#include "RunSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogRunSubsystem, Log, All);

// ─────────────────────────────────────────────────────────────────────────────
// Delegates
// ─────────────────────────────────────────────────────────────────────────────

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRunStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFloorAdvanced, int32, NewFloor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRunEnded, FRunSessionData, SessionData);

// ─────────────────────────────────────────────────────────────────────────────

UCLASS()
class ALS_PROJECTHUNTER_API URunSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// ═══════════════════════════════════════════════
	// RUN LIFECYCLE
	// ═══════════════════════════════════════════════

	/**
	 * Begin a new run at Floor 1.
	 * No-op with a warning if a run is already active.
	 */
	UFUNCTION(BlueprintCallable, Category = "Run")
	void StartRun();

	/**
	 * Increment the floor counter and broadcast OnFloorAdvanced.
	 * Called by PortalActor when the player clears a floor.
	 * No-op with a warning if no run is active.
	 */
	UFUNCTION(BlueprintCallable, Category = "Run")
	void AdvanceFloor();

	/**
	 * End the current run, broadcast session data, then reset all state.
	 * Called on player death. Safe to call with no active run.
	 */
	UFUNCTION(BlueprintCallable, Category = "Run")
	void EndRun();

	// ═══════════════════════════════════════════════
	// GETTERS
	// ═══════════════════════════════════════════════

	UFUNCTION(BlueprintPure, Category = "Run")
	int32 GetCurrentFloor() const { return CurrentFloor; }

	UFUNCTION(BlueprintPure, Category = "Run")
	bool IsRunActive() const { return bRunActive; }

	/**
	 * Seconds elapsed since StartRun.
	 * Returns the frozen elapsed time if the run has already ended.
	 */
	UFUNCTION(BlueprintPure, Category = "Run")
	float GetElapsedTime() const;

	/**
	 * Increment the kill counter by one.
	 * Called by the combat system when a monster dies during an active run.
	 * Silent no-op if no run is active.
	 */
	UFUNCTION(BlueprintCallable, Category = "Run")
	void RegisterKill();

	UFUNCTION(BlueprintPure, Category = "Run")
	int32 GetTotalKills() const { return SessionData.TotalKills; }

	// ═══════════════════════════════════════════════
	// EVENTS
	// ═══════════════════════════════════════════════

	/** Broadcast when a new run begins. */
	UPROPERTY(BlueprintAssignable, Category = "Run|Events")
	FOnRunStarted OnRunStarted;

	/**
	 * Broadcast when the floor counter increments.
	 * FloorManagerActor binds here to trigger DA generation for the next floor.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Run|Events")
	FOnFloorAdvanced OnFloorAdvanced;

	/** Broadcast on player death with session data for the end-of-run screen. */
	UPROPERTY(BlueprintAssignable, Category = "Run|Events")
	FOnRunEnded OnRunEnded;

private:

	// ═══════════════════════════════════════════════
	// STATE
	// ═══════════════════════════════════════════════

	UPROPERTY()
	int32 CurrentFloor = 0;

	UPROPERTY()
	bool bRunActive = false;

	/** World time (seconds) captured at StartRun. Used for elapsed time calc. */
	float RunStartTimeSeconds = 0.f;

	/** Accumulated display data — written on AdvanceFloor and frozen on EndRun. */
	UPROPERTY()
	FRunSessionData SessionData;

	// ═══════════════════════════════════════════════
	// INTERNAL
	// ═══════════════════════════════════════════════

	void ResetState();
};
