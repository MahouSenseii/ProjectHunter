
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "Framework/GameModes/Library/Enums/GameModeEnumLibrary.h"
#include "Tower/Library/Structs/RunStructs.h"
#include "PHGameState.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogPHGameState, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMatchPhaseChanged, EPHMatchPhase, NewPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnReplicatedRunChanged, ERunState, NewState, FRunSessionData, SessionData);

UCLASS()
class ALS_PROJECTHUNTER_API APHGameState : public AGameState
{
	GENERATED_BODY()

public:
	APHGameState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// MATCH STATE

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match")
	EPHMatchPhase MatchPhase = EPHMatchPhase::WaitingToStart;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Match")
	void SetMatchPhase(EPHMatchPhase NewPhase);

	UPROPERTY(BlueprintAssignable, Category = "Match")
	FOnMatchPhaseChanged OnMatchPhaseChanged;

	// TIMING

	/** Server time (seconds) when InProgress phase began. Clients use this to derive elapsed time. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match")
	float MatchStartServerTime = 0.f;

	/** One coherent notification for MatchPhase plus its associated clock data. */
	UPROPERTY(ReplicatedUsing = OnRep_MatchPhase)
	uint32 MatchSnapshotRevision = 0;

	/** Blueprint helper: seconds elapsed since InProgress started */
	UFUNCTION(BlueprintPure, Category = "Match")
	float GetMatchElapsedTime() const;

	// GLOBAL STATS (replicated)

	/** Total monsters killed in this session (all players combined) */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Stats")
	int32 TotalMobKills = 0;

	/** Server-only: increment global mob kill counter */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Stats")
	void IncrementMobKills(int32 Count = 1);

	// DIFFICULTY / WORLD MODIFIERS

	/** World tier / difficulty level. Affects mob scaling, loot quality, etc. */
	UPROPERTY(Replicated, BlueprintReadWrite, Category = "Difficulty")
	int32 WorldTier = 1;

	// TOWER RUN STATE

	/** Server-owned party run state. Clients consume this instead of mutating a local subsystem copy. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Run")
	ERunState RunState = ERunState::Inactive;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Run")
	FRunSessionData RunSession;

	/** Server clock at the last snapshot, allowing clients to display a live run timer. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Run")
	float RunSnapshotServerTime = 0.f;

	/**
	 * Changes once per complete snapshot. RepNotify lives here instead of on
	 * RunState and RunSession individually so Blueprint receives exactly one
	 * coherent callback after the whole snapshot has arrived.
	 */
	UPROPERTY(ReplicatedUsing = OnRep_RunSnapshot)
	uint32 RunSnapshotRevision = 0;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Run")
	void ApplyRunSnapshot(ERunState NewState, const FRunSessionData& NewSession);

	UFUNCTION(BlueprintPure, Category = "Run")
	float GetRunElapsedTime() const;

	UPROPERTY(BlueprintAssignable, Category = "Run")
	FOnReplicatedRunChanged OnReplicatedRunChanged;

protected:
	UFUNCTION()
	void OnRep_MatchPhase();

	UFUNCTION()
	void OnRep_RunSnapshot();
};
