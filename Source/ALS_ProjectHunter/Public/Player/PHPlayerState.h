// PHPlayerState.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "PHPlayerState.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogPHPlayerState, Log, All);

class URunSubsystem;

UCLASS()
class ALS_PROJECTHUNTER_API APHPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	APHPlayerState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ═══════════════════════════════════════════════
	// TEAM
	// ═══════════════════════════════════════════════

	/** Team identifier. 0 = hostile/monster default. Players default to 1. */
	UPROPERTY(Replicated, BlueprintReadWrite, Category = "Team")
	uint8 TeamID = 1;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Team")
	void SetTeamID(uint8 NewTeamID);

	UFUNCTION(BlueprintPure, Category = "Team")
	uint8 GetTeamID() const { return TeamID; }

	// ═══════════════════════════════════════════════
	// RUN STATE (session only — wiped on death)
	// ═══════════════════════════════════════════════

	/**
	 * Floor the player is currently on this run (replicated mirror of
	 * URunSubsystem::GetCurrentFloor when a run is active). Resets on death.
	 */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Run")
	int32 CurrentFloor = 1;

	/** Kills by THIS player this run (run-wide total lives in URunSubsystem). */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Run")
	int32 RunKillCount = 0;

	/**
	 * Server-only: advance to the next floor.
	 * Delegates to URunSubsystem (single source of truth) when a run is
	 * active, then mirrors its floor value for replication.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Run")
	void AdvanceFloor();

	/** Server-only: record a kill (per-player counter + run-wide subsystem counter). */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Run")
	void RecordKill();

protected:
	/** Resolve the GameInstance run subsystem (null in edge cases like CDO/teardown). */
	URunSubsystem* GetRunSubsystem() const;

public:

	// ═══════════════════════════════════════════════
	// SAVE SYSTEM LINK
	// ═══════════════════════════════════════════════

	/** Save slot key for this player. Used to load stash and write save data on death. */
	UPROPERTY(Replicated, BlueprintReadWrite, Category = "Save")
	FString CharacterSlotName;
};
