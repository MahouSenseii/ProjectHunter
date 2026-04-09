// PHGameMode.h
// Game mode for Project Hunter.
//
// Inherits from AGameMode (not AGameModeBase) to get match-state management,
// spectating, seamless travel support, and proper player spawn rules — all
// required foundations for the eventual multiplayer transition.
//
// Currently single-player, but the multiplayer hooks are in place so the
// migration is as painless as possible when the time comes.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "PHGameMode.generated.h"

class APHBaseCharacter;
class APHPlayerState;
class APHGameState;

DECLARE_LOG_CATEGORY_EXTERN(LogPHGameMode, Log, All);

UCLASS()
class ALS_PROJECTHUNTER_API APHGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	APHGameMode();

	virtual void BeginPlay() override;

	// ── Player spawn ─────────────────────────────────────────────────────────

	/**
	 * Choose the best PlayerStart for a restarting player.
	 * Override point for spawn-point selection logic (e.g. respawn at checkpoint,
	 * nearest safe zone, etc.).  Default just calls Super.
	 */
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

	/**
	 * Called after a player pawn is spawned and possessed.
	 * Good place to apply startup GEs, grant default abilities, set team ID, etc.
	 */
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;

	// ── Match lifecycle (multiplayer foundation) ─────────────────────────────

	/**
	 * Can the match start?  In single-player this always returns true.
	 * Override in a dedicated-server subclass to wait for min player count.
	 */
	virtual bool ReadyToStartMatch_Implementation() override;

	// ── Utility ──────────────────────────────────────────────────────────────

	/**
	 * Called when a player character dies.
	 * Handles respawn timer, spectating, or game-over logic.
	 * Call this from PHBaseCharacter::OnDeath or from wherever you handle
	 * player death to centralize the response.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Game")
	virtual void OnPlayerDied(AController* DeadPlayer, AController* Killer);

	// ── Configuration ────────────────────────────────────────────────────────

	/** Seconds before a dead player is automatically respawned. 0 = instant. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Game|Respawn",
		meta = (ClampMin = 0.0f))
	float RespawnDelay = 3.0f;

	/** Whether players should respawn automatically or wait for input. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Game|Respawn")
	bool bAutoRespawn = true;

protected:
	/** Internal: perform the actual respawn for a controller. */
	void RespawnPlayer(AController* Controller);
};
