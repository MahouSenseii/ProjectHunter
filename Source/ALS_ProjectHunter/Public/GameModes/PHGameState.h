// PHGameState.h
//
// N-02 FIX: Centralized replicated world state for match management.
// Holds data that all clients need: match phase, elapsed time, global events.
// Single-player uses this too — it provides a clean access point for world-level
// queries without coupling to GameMode (which only exists on the server).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "GameModes/Library/GameModeEnumLibrary.h"
#include "PHGameState.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogPHGameState, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMatchPhaseChanged, EPHMatchPhase, NewPhase);

UCLASS()
class ALS_PROJECTHUNTER_API APHGameState : public AGameState
{
	GENERATED_BODY()

public:
	APHGameState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ═══════════════════════════════════════════════
	// MATCH STATE
	// ═══════════════════════════════════════════════

	UPROPERTY(ReplicatedUsing = OnRep_MatchPhase, BlueprintReadOnly, Category = "Match")
	EPHMatchPhase MatchPhase = EPHMatchPhase::WaitingToStart;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Match")
	void SetMatchPhase(EPHMatchPhase NewPhase);

	UPROPERTY(BlueprintAssignable, Category = "Match")
	FOnMatchPhaseChanged OnMatchPhaseChanged;

	// ═══════════════════════════════════════════════
	// TIMING
	// ═══════════════════════════════════════════════

	/** Server time (seconds) when InProgress phase began. Clients use this to derive elapsed time. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match")
	float MatchStartServerTime = 0.f;

	/** Blueprint helper: seconds elapsed since InProgress started */
	UFUNCTION(BlueprintPure, Category = "Match")
	float GetMatchElapsedTime() const;

	// ═══════════════════════════════════════════════
	// GLOBAL STATS (replicated)
	// ═══════════════════════════════════════════════

	/** Total monsters killed in this session (all players combined) */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Stats")
	int32 TotalMobKills = 0;

	/** Server-only: increment global mob kill counter */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Stats")
	void IncrementMobKills(int32 Count = 1);

	// ═══════════════════════════════════════════════
	// DIFFICULTY / WORLD MODIFIERS
	// ═══════════════════════════════════════════════

	/** World tier / difficulty level. Affects mob scaling, loot quality, etc. */
	UPROPERTY(Replicated, BlueprintReadWrite, Category = "Difficulty")
	int32 WorldTier = 1;

protected:
	UFUNCTION()
	void OnRep_MatchPhase();
};
