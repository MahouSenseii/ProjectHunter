#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Tower/Library/Enums/RunEnumLibrary.h"
#include "RunStructs.generated.h"

/**
 * Everything that describes the floor the party is currently on.
 *
 * A floor is not FloorNumber++. FloorSeed is derived from the run seed by
 * URunSeedFunctionLibrary, so the same run seed reproduces the same layout,
 * encounters and modifiers. Modifiers are gameplay tags rather than a fixed
 * field set so new floor rules can be authored in data without touching this
 * struct or its replicated layout.
 */
USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FRunFloorData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Floor")
	int32 FloorNumber = 0;

	/** Derived from RunSeed. Feed this to Dungeon Architect for layout generation. */
	UPROPERTY(BlueprintReadOnly, Category = "Floor")
	int32 FloorSeed = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Floor")
	EFloorType FloorType = EFloorType::Combat;

	UPROPERTY(BlueprintReadOnly, Category = "Floor")
	EFloorObjective Objective = EFloorObjective::ClearAllEnemies;

	UPROPERTY(BlueprintReadOnly, Category = "Floor")
	EFloorPhase Phase = EFloorPhase::None;

	/** Scales with depth and run difficulty; drives mob area level and reward tier. */
	UPROPERTY(BlueprintReadOnly, Category = "Floor")
	int32 Difficulty = 1;

	/**
	 * Active floor rules, e.g. Floor.Modifier.ExtraElites,
	 * Floor.Modifier.ReducedHealing. Rolled from FloorSeed.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Floor")
	FGameplayTagContainer Modifiers;

	/** Objective progress. Meaning depends on Objective; unused fields stay zero. */
	UPROPERTY(BlueprintReadOnly, Category = "Floor")
	int32 ObjectiveProgress = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Floor")
	int32 ObjectiveTarget = 0;

	/** For SurviveDuration floors. */
	UPROPERTY(BlueprintReadOnly, Category = "Floor")
	float ObjectiveDuration = 0.f;

	bool IsObjectiveComplete() const
	{
		return ObjectiveTarget > 0 && ObjectiveProgress >= ObjectiveTarget;
	}
};

USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FRunSessionData
{
	GENERATED_BODY()

	/** Stable identity for save/reconnect/reward bookkeeping. */
	UPROPERTY(BlueprintReadOnly, Category = "Run")
	FGuid RunID;

	/** Seed used by floor generation, encounter rolls, and reward generation. */
	UPROPERTY(BlueprintReadOnly, Category = "Run")
	int32 RunSeed = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Run")
	int32 Difficulty = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Run")
	int32 CurrentFloor = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Run")
	int32 FloorsCleared = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Run")
	int32 TotalKills = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Run")
	float TimeElapsed = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Run")
	ERunEndReason EndReason = ERunEndReason::None;

	/** The floor the party is currently on. CurrentFloor mirrors Floor.FloorNumber. */
	UPROPERTY(BlueprintReadOnly, Category = "Run")
	FRunFloorData Floor;

	/**
	 * Bumped on every server-side snapshot. RunState, RunSession and the server
	 * timestamp replicate as separate properties and can land in different
	 * packets, so clients compare this to detect a torn read rather than acting
	 * on a half-updated snapshot.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Run")
	int32 Revision = 0;
};
