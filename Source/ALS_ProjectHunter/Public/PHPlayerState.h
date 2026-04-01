// PHPlayerState.h
//
// N-01 FIX: Player-persistent state that survives pawn destruction/respawn.
// In multiplayer, PlayerState lives on the server and replicates to all clients.
// Holds identity, progression stats, and team data that should NOT reset on death.
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

	/** Team identifier — persists across respawns. 0 = hostile/monster default. */
	UPROPERTY(Replicated, BlueprintReadWrite, Category = "Team")
	uint8 TeamID = 1; // Players default to team 1

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Team")
	void SetTeamID(uint8 NewTeamID);

	UFUNCTION(BlueprintPure, Category = "Team")
	uint8 GetTeamID() const { return TeamID; }

	// ═══════════════════════════════════════════════
	// KILL / DEATH TRACKING
	// ═══════════════════════════════════════════════

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Stats")
	int32 KillCount = 0;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Stats")
	int32 DeathCount = 0;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Stats")
	int64 TotalXPEarned = 0;

	/** Server-only: record a kill */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Stats")
	void RecordKill();

	/** Server-only: record a death */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Stats")
	void RecordDeath();

	/** Server-only: add XP earned (tracking, not granting) */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Stats")
	void AddXPEarned(int64 Amount);

	// ═══════════════════════════════════════════════
	// CHARACTER SLOT (save system link)
	// ═══════════════════════════════════════════════

	/** Name of the character save slot this player loaded. Used to key stash, progression, etc. */
	UPROPERTY(Replicated, BlueprintReadWrite, Category = "Save")
	FString CharacterSlotName;
};
