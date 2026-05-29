// AI/Mob/PlayerLocationCacheSubsystem.h


#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AI/Library/MobStructs.h"
#include "PlayerLocationCacheSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogPlayerLocationCache, Log, All);

/**
 * UPlayerLocationCacheSubsystem
 *
 * One refresh per cadence, many readers. Spawn managers MUST consume this
 * instead of scanning the player controller iterator individually.
 */
UCLASS()
class ALS_PROJECTHUNTER_API UPlayerLocationCacheSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	//~ Begin UWorldSubsystem
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	//~ End UWorldSubsystem

	//~ Begin FTickableGameObject (via UTickableWorldSubsystem)
	virtual void Tick(float DeltaSeconds) override;
	virtual TStatId GetStatId() const override;
	//~ End FTickableGameObject

	/** Read-only access to the most recent snapshot batch. */
	UFUNCTION(BlueprintPure, Category = "ProjectHunter|AI|PlayerLocation")
	const TArray<FPlayerLocationSnapshot>& GetPlayerSnapshots() const { return Snapshots; }

	/** How often the cache refreshes, in seconds. Tunable per project. */
	UPROPERTY(EditDefaultsOnly, Category = "ProjectHunter|AI|PlayerLocation")
	float RefreshIntervalSeconds = 0.25f;

protected:
	void RefreshSnapshots();

private:
	UPROPERTY(Transient)
	TArray<FPlayerLocationSnapshot> Snapshots;

	float TimeSinceLastRefresh = 0.0f;
};
