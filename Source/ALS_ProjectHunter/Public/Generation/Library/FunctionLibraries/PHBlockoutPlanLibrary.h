// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Generation/Library/Structs/BlockoutPlanStructs.h"
#include "Generation/Library/Structs/GeneratedLayoutStructs.h"
#include "Generation/Library/Structs/GenerationValidationStructs.h"
#include "PHBlockoutPlanLibrary.generated.h"

/**
 * Turns a validated logical layout into a placement plan on a square tile grid.
 *
 * Stateless and world-free: it builds no actors, loads no assets, and names no meshes, so a floor's
 * geometry can be proved in automation before anything is spawned.
 */
UCLASS()
class ALS_PROJECTHUNTER_API UPHBlockoutPlanLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Replaces OutPlan and OutIssues. Refuses unusable settings, off-grid regions, non-coplanar
	 * floors, fractional wall courses and corridors outside the layout envelope. Failure leaves
	 * no partial plan. Corridor width is never silently reduced.
	 *
	 * This synchronous blockout is limited to 65536 floor tiles, 1000000 raster visits/placements
	 * and 16000000 lighting distance checks. Larger construction needs a separate streamed path.
	 *
	 * Regions become floor tiles; every connection is joined by a corridor so a layout that is
	 * logically connected is also physically walkable. A wall is emitted on each tile edge
	 * whose neighbour is not floor, which leaves openings exactly where corridors meet rooms.
	 *
	 * bIncludeCeiling caps every floor tile at region height. It defaults off because a sealed
	 * interior is lit only by what is placed inside it.
	 *
	 * Regions that share frontage are joined by a short straight corridor through that frontage;
	 * only a diagonal pair falls back to a dog-leg between the two centres. Corridor width is drawn
	 * per connection from the range, and the lane within the frontage is drawn too, so routes differ
	 * in breadth and position instead of reading as one repeated passage.
	 *
	 * PlayerStart and Exit are snapped to the centre of a tile the plan actually builds, and the
	 * chosen tiles are published alongside them, so a spawned actor always stands on floor.
	 *
	 * Lights are spread by farthest-point sampling until no floor tile is more than
	 * LightSpacingTiles from one, so a large room is lit to its corners and a corridor is lit along
	 * its length. 0 plans no lights.
	 * The draws come from a Corridor branch off the layout seed, so they are deterministic and
	 * cannot disturb layout, encounter or loot selection.
	 *
	 * Walls are stacked in WallHeight courses up to each region's own height, and a course is also
	 * added where a tall region abuts a shorter one, so a varied-height floor has no gaps to see
	 * through. WallHeight of 0 means "use TileSize", which matches a kit whose wall is as tall as
	 * its floor tile is wide. Negative WallHeight retains this same fallback.
	 */
	UFUNCTION(BlueprintCallable, Category = "Generation|Blockout")
	static bool BuildPlan(const FPHGeneratedLayout& Layout, double TileSize,
		FPHBlockoutPlan& OutPlan, TArray<FPHGenerationIssue>& OutIssues,
		bool bIncludeCeiling = false,
		int32 MinCorridorWidth = 1, int32 MaxCorridorWidth = 1,
		double WallHeight = 0.0, int32 LightSpacingTiles = 3);
};
