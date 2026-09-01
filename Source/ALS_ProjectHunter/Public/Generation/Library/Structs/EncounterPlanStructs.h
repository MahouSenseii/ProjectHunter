// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "EncounterPlanStructs.generated.h"

/**
 * One region's request for enemies, expressed as a volume and a count.
 *
 * Deliberately not a spawn instruction: generation says "this space should hold about this many
 * enemies of these kinds", and the encounter owner decides exact positions, because it is the thing
 * that knows about navmesh projection, ground checks, collision and pooling.
 */
USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FPHEncounterPlacement
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Generation|Encounter")
	int32 RegionID = INDEX_NONE;

	/** Volume the encounter owner should populate, inset from the region's walls. */
	UPROPERTY(BlueprintReadOnly, Category = "Generation|Encounter")
	FBox SpawnBounds = FBox(ForceInit);

	/** How many enemy anchors this region carried. A budget, not a guarantee. */
	UPROPERTY(BlueprintReadOnly, Category = "Generation|Encounter")
	int32 EnemyCount = 0;

	/** Which enemy anchor kinds were present, so the owner can pick suitable classes. */
	UPROPERTY(BlueprintReadOnly, Category = "Generation|Encounter")
	FGameplayTagContainer EnemyKinds;

	/** True when this region holds the player start, so callers can keep it quiet if they wish. */
	UPROPERTY(BlueprintReadOnly, Category = "Generation|Encounter")
	bool bIsStartRegion = false;
};

/** All encounter requests for one generated layout. */
USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FPHEncounterPlan
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Generation|Encounter")
	TArray<FPHEncounterPlacement> Placements;

	/** Total enemy anchors across every region. */
	UPROPERTY(BlueprintReadOnly, Category = "Generation|Encounter")
	int32 TotalEnemyCount = 0;
};
