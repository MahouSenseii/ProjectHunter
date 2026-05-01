// PHPlayerState.h
//
// Session-scoped player state. Lives on the server and replicates to all clients.
// Holds only data that is relevant for the current session/run.
//
// Permadeath contract:
//   - On death: everything here is discarded. The save file is written first.
//   - Save file owns: HighestFloor, LifetimeKills, Stash.
//   - This class owns: CurrentFloor, RunKills (current run only), CharacterSlotName.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "PHPlayerState.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogPHPlayerState, Log, All);

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

	/** Floor the player is currently on this run. Resets to 1 on death. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Run")
	int32 CurrentFloor = 1;

	/** Kills accumulated this run. Resets to 0 on death. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Run")
	int32 RunKillCount = 0;

	/** Server-only: advance to the next floor. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Run")
	void AdvanceFloor();

	/** Server-only: record a kill this run. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Run")
	void RecordKill();

	// ═══════════════════════════════════════════════
	// SAVE SYSTEM LINK
	// ═══════════════════════════════════════════════

	/** Save slot key for this player. Used to load stash and write save data on death. */
	UPROPERTY(Replicated, BlueprintReadWrite, Category = "Save")
	FString CharacterSlotName;
};
