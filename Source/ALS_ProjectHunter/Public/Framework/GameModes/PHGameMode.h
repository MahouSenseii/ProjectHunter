
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

	// Player spawn
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

	// Match lifecycle (multiplayer foundation)
	/**
	 * Can the match start?  In single-player this always returns true.
	 * Override in a dedicated-server subclass to wait for min player count.
	 */
	virtual bool ReadyToStartMatch_Implementation() override;

	// Utility
	/**
	 * Called when a player character dies.
	 * Handles respawn timer, spectating, or game-over logic.
	 * Call this from PHBaseCharacter::OnDeath or from wherever you handle
	 * player death to centralize the response.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Game")
	virtual void OnPlayerDied(AController* DeadPlayer, AController* Killer);

	// Configuration
	/** Seconds before a dead player is automatically respawned. 0 = instant. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Game|Respawn",
		meta = (ClampMin = 0.0f, EditCondition = "bAutoRespawn"))
	float RespawnDelay = 3.0f;

	/**
	 * Generic arena-style respawn. OFF by default and deliberately so.
	 *
	 * While a tower run is active the run rules own death: the player goes Downed,
	 * a teammate may revive them, and a party wipe ends the run. A three-second
	 * respawn back into the active floor would silently defeat that. OnPlayerDied
	 * routes to RunSubsystem whenever a run is active and only falls back to this
	 * path outside a run (hub, test maps, non-run modes).
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Game|Respawn")
	bool bAutoRespawn = false;

protected:
	/** Internal: perform the actual respawn for a controller. */
	void RespawnPlayer(AController* Controller);
};
