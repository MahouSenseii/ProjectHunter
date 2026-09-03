// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#pragma once

#include "CoreMinimal.h"
#include "Generation/Generators/PHLayoutGenerator.h"
#include "PHCaveGenerator.generated.h"

/**
 * Cellular-automata caves: an organic, non-rectangular floor on the same tile grid every other
 * strategy uses.
 *
 * The dungeon strategy draws rectangles and joins them with corridors, which is why its floors read
 * as boxes however the walls are dressed - swapping brick for rock changes the material, not the
 * silhouette. This one never draws a rectangle at all. It seeds the grid with noise, then repeatedly
 * asks each tile to become whatever most of its neighbours are, which erodes the noise into rounded
 * chambers joined by irregular passages. The method is the standard one described at
 * roguebasin.com/index.php?title=Cellular_Automata_Method_for_Generating_Random_Cave-Like_Levels.
 *
 * The result is published as FPHGeneratedLayout::FloorMask. Its single region is an envelope for
 * anchors, encounters and navigation only; the walkable shape is the mask, and the blockout planner
 * builds walls on whatever tile edges the mask leaves exposed.
 *
 * Smoothing alone can leave islands the player could never reach, so the largest connected component
 * is kept and everything else is discarded. That is what guarantees a walkable floor, and it is
 * cheaper and more reliable than trying to tunnel between components.
 */
UCLASS(Blueprintable)
class ALS_PROJECTHUNTER_API UPHCaveGenerator : public UPHLayoutGenerator
{
	GENERATED_BODY()

public:
	UPHCaveGenerator();

	/**
	 * Percentage of tiles seeded as rock before smoothing. This is the dial that decides whether a
	 * cave reads as a cave, and it was chosen by measurement rather than by the usual 45.
	 *
	 * ReportCaveShape sweeps it over 30 seeds. With a 5-of-8 majority rule, a seed below the rule's
	 * own equilibrium erodes away, so the classic 45 leaves a cave that fills most of its bounding
	 * box - which is the "still looks like a room" complaint this strategy exists to answer:
	 *
	 *   45%  1139 tiles  fill 0.88  raggedness 1.97x   0 refused
	 *   52%  1027 tiles  fill 0.80  raggedness 2.09x   0 refused
	 *   58%   602 tiles  fill 0.57  raggedness 2.24x   0 refused   <- default
	 *   60%   467 tiles  fill 0.52  raggedness 2.20x   2 of 30 refused
	 *   65%   275 tiles  fill 0.47  raggedness 1.92x  23 of 30 refused
	 *
	 * Past 58 the cave breaks into islands, the largest-component step throws most of the grid
	 * away, and seeds start being refused outright for having no usable chamber left.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cave", meta = (ClampMin = "0", ClampMax = "100"))
	int32 InitialRockPercent = 58;

	/**
	 * Passes that also seed rock into wide open ground, run before the plain smoothing passes.
	 *
	 * Majority smoothing alone has a failure mode worth knowing: on some seeds it erodes into one
	 * large open field that fills most of its bounding box, which reads as a room again rather than
	 * a cave. Measured on the first version here, seeds 6, 8, 16, 21 and 24 came out 90-93% filled.
	 * The second rule of the standard method fixes it - a tile with almost no rock anywhere in its
	 * 5x5 neighbourhood becomes rock, which drops pillars and walls into open ground and breaks it
	 * into chambers. 0 disables it and restores that failure mode.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cave", meta = (ClampMin = "0", ClampMax = "20"))
	int32 OpenFieldPasses = 4;

	/**
	 * Rock tiles in the 5x5 neighbourhood at or below which a tile becomes rock during an
	 * OpenFieldPasses pass. 2 is the classic value; higher fills the cave in aggressively.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cave", meta = (ClampMin = "0", ClampMax = "24"))
	int32 WideOpenThreshold = 2;

	/**
	 * Plain smoothing passes, run after the open-field ones. Each rounds the walls off further;
	 * beyond about 6 the result is so smooth it reads as blobs again.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cave", meta = (ClampMin = "0", ClampMax = "20"))
	int32 SmoothingPasses = 3;

	/**
	 * Neighbouring rock tiles (of 8) needed for a tile to become rock. 5 is the classic value and
	 * is what makes the rule a majority vote; lower carves the cave shut, higher opens it out.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cave", meta = (ClampMin = "1", ClampMax = "8"))
	int32 RockNeighbourThreshold = 5;

	/**
	 * Tiles of solid rock kept around the edge, so a cave never opens onto the void at its
	 * boundary. One tile is enough for a wall to have something to stand on.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cave", meta = (ClampMin = "1"))
	int32 BorderTiles = 2;

	/**
	 * Smallest usable cave, as a percentage of the griddable area. A seed whose largest chamber is
	 * smaller than this is refused rather than shipped as a pocket the run cannot use.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cave", meta = (ClampMin = "1", ClampMax = "100"))
	int32 MinimumFloorPercent = 12;

protected:
	virtual bool BuildLayout(const FPHLayoutRequest& Request, FRandomStream& Stream,
		FPHGeneratedLayout& OutLayout, TArray<FPHGenerationIssue>& OutIssues) override;

	/** The mask is the shape, so the checks are about it rather than about region rectangles. */
	virtual bool ValidateStrategyConstraints(const FPHLayoutRequest& Request,
		const FPHGeneratedLayout& Layout, TArray<FPHGenerationIssue>& OutIssues) const override;
};
