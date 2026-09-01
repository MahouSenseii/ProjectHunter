// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "BlockoutPlanStructs.generated.h"

/** One piece to place, named logically. Nothing here knows an asset exists. */
USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FPHPiecePlacement
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Generation|Blockout", meta = (Categories = "Piece"))
	FGameplayTag PieceTag;

	/** Layout-local pose. The construction stage adds the actor's own transform. */
	UPROPERTY(BlueprintReadOnly, Category = "Generation|Blockout")
	FTransform Transform = FTransform::Identity;

	/**
	 * Index of the prop rule that asked for this placement, or INDEX_NONE for
	 * structural pieces. Construction reads it to apply that rule's own scale and
	 * rotation, so one prop tag can be placed differently by different rules.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Generation|Blockout")
	int32 SourceRuleIndex = INDEX_NONE;
};

/**
 * A buildable floor derived from a validated layout: what to place and where, still expressed in
 * logical pieces. Computing this without a world is what lets construction be tested in automation.
 */
USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FPHBlockoutPlan
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Generation|Blockout")
	TArray<FPHPiecePlacement> Placements;

	UPROPERTY(BlueprintReadOnly, Category = "Generation|Blockout")
	FTransform PlayerStart = FTransform::Identity;

	UPROPERTY(BlueprintReadOnly, Category = "Generation|Blockout")
	FTransform Exit = FTransform::Identity;

	/**
	 * Tile coordinates of the two endpoints. Both poses above are the centres of these tiles, so a
	 * spawned actor stands on floor rather than on a room edge that a modular kit leaves as a seam.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Generation|Blockout")
	FIntPoint PlayerStartTile = FIntPoint::ZeroValue;

	UPROPERTY(BlueprintReadOnly, Category = "Generation|Blockout")
	FIntPoint ExitTile = FIntPoint::ZeroValue;

	/**
	 * Where light should go. A sealed floor is lit only from inside, and lights cannot be
	 * instanced, so the planner decides positions and the construction stage spawns actors.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Generation|Blockout")
	TArray<FTransform> LightPoses;


	UPROPERTY(BlueprintReadOnly, Category = "Generation|Blockout")
	int32 FloorTileCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Generation|Blockout")
	int32 WallCount = 0;

	/** Tiles added to join regions the layout says are connected. */
	UPROPERTY(BlueprintReadOnly, Category = "Generation|Blockout")
	int32 CorridorTileCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Generation|Blockout")
	int32 CeilingCount = 0;

	/** Sorted floor tiles in tile coordinates, so later stages need not re-derive them. */
	UPROPERTY(BlueprintReadOnly, Category = "Generation|Blockout")
	TArray<FIntPoint> FloorTiles;

	/** The subset of FloorTiles that exists only to join regions. */
	UPROPERTY(BlueprintReadOnly, Category = "Generation|Blockout")
	TArray<FIntPoint> CorridorTiles;

	/** Units per tile, carried so decoration works in the same space without re-reading the kit. */
	UPROPERTY(BlueprintReadOnly, Category = "Generation|Blockout")
	double TileSize = 0.0;
};

/** Where a prop is allowed to sit relative to the floor it decorates. */
UENUM(BlueprintType)
enum class EPHPropPlacement : uint8
{
	/** Tiles touching at least one wall; barrels, crates, pipes. */
	AgainstWall,
	/** Tiles with floor on all four sides; campfires, tables, large debris. */
	OpenFloor,
	/** Any floor tile. */
	Anywhere
};

/**
 * One authored decoration rule. Content is data; the scattering algorithm stays in C++, so a new
 * prop kind needs a rule rather than code (GAME_DESIGN §38).
 */
USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FPHPropRule
{
	GENERATED_BODY()

	/** Must be a registered descendant of Prop. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|Decoration")
	FGameplayTag PropTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|Decoration")
	EPHPropPlacement Placement = EPHPropPlacement::AgainstWall;

	/** Chance each eligible tile receives this prop. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|Decoration",
		meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ChancePerTile = 0.1f;

	/** Cap across the floor. Zero means no cap. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|Decoration", meta = (ClampMin = "0"))
	int32 MaxTotal = 0;

	/** Keeps clutter off tiles holding a gameplay anchor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|Decoration")
	bool bAvoidAnchors = true;

	/**
	 * Keeps clutter out of corridors, so dressing never narrows the required path.
	 * Corridor tiles are the ones that exist only to join regions.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|Decoration")
	bool bAvoidCorridors = true;

	/** Random yaw spread in degrees, so repeated meshes do not read as a grid. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|Decoration",
		meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float YawJitter = 180.0f;

	/**
	 * Clumps this prop into this many piles per region instead of sprinkling it evenly. An even
	 * per-tile chance is what makes a floor read as randomly generated: real clutter accumulates
	 * where something happened and leaves the rest of the room bare. 0 restores the even scatter.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|Decoration", meta = (ClampMin = "0"))
	int32 ClustersPerRegion = 2;

	/**
	 * Tile radius a clump reaches. Beyond it the prop still appears, but rarely. Keep it small
	 * against room size: a clump wider than half the room is not a clump, and the planner caps the
	 * clump count for that reason.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|Decoration", meta = (ClampMin = "1"))
	int32 ClusterRadiusTiles = 1;

	/**
	 * Tiles kept clear around each placed prop, counted against props of every kind. 0 lets props
	 * sit on adjacent tiles, which is right for clutter and wrong for furniture.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|Decoration", meta = (ClampMin = "0"))
	int32 MinSpacingTiles = 0;


	/**
	 * Pushes this prop toward room corners and edges. 0 scatters evenly; higher
	 * values weight the perimeter, which is where real clutter accumulates.
	 * Validation rejects anything outside 0-4 or non-finite.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|Decoration", meta = (ClampMin = "0.0", ClampMax = "4.0"))
	float CornerBias = 1.0f;

	/**
	 * How much this prop's density varies between regions, 0-1. 0 gives every room
	 * the same amount, which reads as evenly machine-placed.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|Decoration", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float RegionDensityJitter = 0.25f;

};

/** Decoration output, kept separate from the blockout so the two can be compared and tested apart. */
USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FPHDecorationPlan
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Generation|Decoration")
	TArray<FPHPiecePlacement> Placements;

	UPROPERTY(BlueprintReadOnly, Category = "Generation|Decoration")
	int32 PropCount = 0;
};
