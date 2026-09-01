// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#pragma once

#include "CoreMinimal.h"
#include "AI/Mob/MobManagerActor.h"
#include "PHMobPlacementProbe.generated.h"

/**
 * Exposes the manager's protected placement and composition draws so a test can
 * compare their sequences directly.
 *
 * It adds no behaviour. Spawning is never started on it, so it creates no mob,
 * runs no timer, and touches no pool.
 */
UCLASS()
class APHMobPlacementProbe : public AMobManagerActor
{
	GENERATED_BODY()

public:
	/** One candidate position, drawn from the placement stream. */
	bool DrawSpawnLocation(FVector& OutLocation)
	{
		return GetRandomSpawnLocation(OutLocation);
	}

	/** One mob-type roll, drawn from the composition stream. */
	int32 DrawMobTypeIndex() const
	{
		return GetWeightedRandomMobTypeIndex();
	}

	/** Refreshes the cached spawn box that the placement draws read. */
	void CacheSpawnBox()
	{
		CacheTickValues();
	}
};
