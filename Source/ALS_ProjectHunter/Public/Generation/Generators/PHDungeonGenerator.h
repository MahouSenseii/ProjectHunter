// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#pragma once

#include "CoreMinimal.h"
#include "Generation/Generators/PHLayoutGenerator.h"
#include "PHDungeonGenerator.generated.h"

/**
 * Rooms-and-links dungeon strategy: scatter non-overlapping rooms inside the requested
 * area, join them with a nearest-neighbour spanning tree, then add optional loops.
 *
 * The spanning tree is what guarantees every region is reachable, so the layout always
 * satisfies structural validation without a retry loop. The exit is placed in the room
 * furthest from the start by connection count, not by distance, so a sprawling floor
 * still ends somewhere that takes several rooms to reach.
 *
 * A request too dense to hold its minimum room count is refused rather than quietly
 * downgraded, so an unbuildable floor plan is visible instead of shipping half a dungeon.
 * Strategy capacities also bound room count, placement retries and anchor-slot work.
 */
UCLASS(BlueprintType)
class ALS_PROJECTHUNTER_API UPHDungeonGenerator : public UPHLayoutGenerator
{
	GENERATED_BODY()

public:
	UPHDungeonGenerator();

protected:
	virtual bool BuildLayout(const FPHLayoutRequest& Request, FRandomStream& Stream,
		FPHGeneratedLayout& OutLayout, TArray<FPHGenerationIssue>& OutIssues) override;

	/** Rooms must stay inside the requested area, sit on the module grid, and not overlap. */
	virtual bool ValidateStrategyConstraints(const FPHLayoutRequest& Request,
		const FPHGeneratedLayout& Layout, TArray<FPHGenerationIssue>& OutIssues) const override;

private:
	/**
	 * Seats the request's optional anchor rules on interior floor tiles, one module in from
	 * every wall. Each region shuffles its own slots so no two anchors share a tile.
	 * Refuses excessive slot work before allocating or consuming any anchor draws.
	 */
	bool PlaceRuleAnchors(const FPHLayoutRequest& Request, FRandomStream& Stream,
		const TArray<FBox>& Rooms, int32 ExitRoom, FPHGeneratedLayout& OutLayout,
		TArray<FPHGenerationIssue>& OutIssues) const;
};
