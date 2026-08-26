#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tower/Library/Structs/RunStructs.h"
#include "RunSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogRunSubsystem, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRunStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFloorAdvanced, int32, NewFloor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRunEnded, FRunSessionData, SessionData);

UCLASS()
class ALS_PROJECTHUNTER_API URunSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "Run")
	void StartRun(int32 RunSeed = 0, int32 Difficulty = 1);

	UFUNCTION(BlueprintCallable, Category = "Run")
	void AdvanceFloor();

	UFUNCTION(BlueprintCallable, Category = "Run")
	void EndRun(ERunEndReason EndReason = ERunEndReason::Quit);

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

	UPROPERTY(BlueprintAssignable, Category = "Run|Events")
	FOnRunStarted OnRunStarted;

	UPROPERTY(BlueprintAssignable, Category = "Run|Events")
	FOnFloorAdvanced OnFloorAdvanced;

	UPROPERTY(BlueprintAssignable, Category = "Run|Events")
	FOnRunEnded OnRunEnded;

private:
	UPROPERTY()
	ERunState RunState = ERunState::Inactive;

	// Process time is used because OpenLevel resets world time during a run.
	double RunStartTimeSeconds = 0.0;

	UPROPERTY()
	FRunSessionData SessionData;

	void ResetState();
	bool HasServerAuthority() const;
};
