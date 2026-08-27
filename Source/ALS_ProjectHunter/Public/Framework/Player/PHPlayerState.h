#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "Tower/Library/Enums/RunEnumLibrary.h"
#include "PHPlayerState.generated.h"

class URunSubsystem;

DECLARE_LOG_CATEGORY_EXTERN(LogPHPlayerState, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerRunFloorChanged, int32, NewFloor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerRunKillsChanged, int32, NewKillCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerRunStatusChanged, ERunPlayerState, NewState);

UCLASS()
class ALS_PROJECTHUNTER_API APHPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	APHPlayerState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(Replicated, BlueprintReadWrite, Category = "Team")
	uint8 TeamID = 1;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Team")
	void SetTeamID(uint8 NewTeamID);

	UFUNCTION(BlueprintPure, Category = "Team")
	uint8 GetTeamID() const { return TeamID; }

	/**
	 * Display mirror of the party floor. RunSubsystem is the single authority
	 * and pushes this value; nothing here may advance it. Prefer
	 * URunSubsystem::GetCurrentFloor() as the source of truth - this exists so
	 * per-player UI can read a floor number without a subsystem lookup.
	 */
	UPROPERTY(ReplicatedUsing = OnRep_CurrentFloor, BlueprintReadOnly, Category = "Run")
	int32 CurrentFloor = 1;

	/** Server-only: called by RunSubsystem when the party floor changes. */
	void SetRunFloorMirror(int32 NewFloor);

	/** This player's kills this run. Per-player stat, not the party total. */
	UPROPERTY(ReplicatedUsing = OnRep_RunKillCount, BlueprintReadOnly, Category = "Run")
	int32 RunKillCount = 0;

	/**
	 * Alive / Downed / Dead / Out. Owned by RunSubsystem, which uses it to
	 * decide whether the run continues after a death.
	 */
	UPROPERTY(ReplicatedUsing = OnRep_RunPlayerState, BlueprintReadOnly, Category = "Run")
	ERunPlayerState RunPlayerState = ERunPlayerState::Alive;

	/** Server-only: called by RunSubsystem when this player's run status changes. */
	void SetRunPlayerStateMirror(ERunPlayerState NewState);

	UFUNCTION(BlueprintPure, Category = "Run")
	bool IsAliveInRun() const { return RunPlayerState == ERunPlayerState::Alive; }

	UFUNCTION(BlueprintPure, Category = "Run")
	bool IsRevivable() const
	{
		return RunPlayerState == ERunPlayerState::Downed || RunPlayerState == ERunPlayerState::Dead;
	}

	/**
	 * Requests a party floor advance. Forwards to RunSubsystem, which is the
	 * only floor authority and rejects the call unless the floor is finished.
	 *
	 * This used to increment a local CurrentFloor when no run was active, which
	 * let a single PlayerState drift out of sync with the party - and, when a
	 * run was active, let any player advance everyone.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Run")
	void RequestAdvanceFloor();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Run")
	void RecordKill();

	UPROPERTY(BlueprintAssignable, Category = "Run|Events")
	FOnPlayerRunFloorChanged OnRunFloorChanged;

	UPROPERTY(BlueprintAssignable, Category = "Run|Events")
	FOnPlayerRunKillsChanged OnRunKillsChanged;

	UPROPERTY(BlueprintAssignable, Category = "Run|Events")
	FOnPlayerRunStatusChanged OnRunStatusChanged;

	UPROPERTY(Replicated, BlueprintReadWrite, Category = "Save")
	FString CharacterSlotName;

protected:
	URunSubsystem* GetRunSubsystem() const;

	UFUNCTION()
	void OnRep_CurrentFloor();

	UFUNCTION()
	void OnRep_RunKillCount();

	UFUNCTION()
	void OnRep_RunPlayerState();
};
