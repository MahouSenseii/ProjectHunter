// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GeneratedLayoutStructs.generated.h"

/** A traversable logical area. Bounds are an envelope, not collision or navigation geometry. */
USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FPHGeneratedRegion
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation", meta = (ClampMin = "0"))
	int32 RegionID = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	FBox Bounds = FBox(ForceInit);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	FGameplayTagContainer Tags;
};

/** Logical traversal between regions; parallel connections are allowed when their IDs differ. */
USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FPHGeneratedConnection
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation", meta = (ClampMin = "0"))
	int32 ConnectionID = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation", meta = (ClampMin = "0"))
	int32 FromRegionID = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation", meta = (ClampMin = "0"))
	int32 ToRegionID = INDEX_NONE;

	/** False permits traversal only from FromRegionID to ToRegionID. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	bool bBidirectional = true;

	/**
	 * True when the layout would still be fully reachable without this connection - a loop added on
	 * top of the spanning tree rather than part of it.
	 *
	 * Construction may drop an optional connection it cannot physically build, where failing to
	 * build a required one has to refuse the whole floor. Without the distinction a single loop
	 * between two rooms that barely face each other makes an otherwise sound floor unbuildable.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	bool bOptional = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	FGameplayTagContainer Tags;
};

/** A semantic placement candidate, independent of actors, meshes, and asset packs. */
USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FPHGeneratedAnchor
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation", meta = (ClampMin = "0"))
	int32 AnchorID = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation", meta = (ClampMin = "0"))
	int32 RegionID = INDEX_NONE;

	/** Pose in layout-local Unreal units. Scale stays one; content construction owns asset scale. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	FTransform Transform = FTransform::Identity;

	/** A registered descendant of Anchor, such as Anchor.PlayerStart or Anchor.Exit. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation", meta = (Categories = "Anchor"))
	FGameplayTag SemanticTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	FGameplayTagContainer Tags;
};

/**
 * Generator-neutral logical snapshot. IDs are nonnegative, unique within each collection,
 * and independent of array order. All coordinates share one layout-local Unreal-unit space.
 * Every region participates in traversal; hierarchy and unloaded regions are not represented here.
 */
USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FPHGeneratedLayout
{
	GENERATED_BODY()

	/** Literal seed used by the producer. Zero and negative seeds do not request randomization. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	int32 Seed = 0;

	/** Positive algorithm version supplied by the producer, not a save-schema version. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation", meta = (ClampMin = "1"))
	int32 GenerationVersion = 1;

	/** Inclusive envelope. Planar or linear bounds are permitted; a single point is not. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	FBox Bounds = FBox(ForceInit);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	TArray<FPHGeneratedRegion> Regions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	TArray<FPHGeneratedConnection> Connections;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	TArray<FPHGeneratedAnchor> Anchors;

	/** References an Anchor.PlayerStart anchor (or a descendant tag). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation", meta = (ClampMin = "0"))
	int32 PlayerStartAnchorID = INDEX_NONE;

	/** References an Anchor.Exit anchor (or a descendant tag); it may share the start region. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation", meta = (ClampMin = "0"))
	int32 ExitAnchorID = INDEX_NONE;

	/**
	 * Walkable tiles, for a strategy whose floor is not a set of rectangles.
	 *
	 * A cave's region is an envelope for anchors, encounters and navigation only; the shape a player
	 * can stand on is this mask, and construction builds walls on whatever tile edges it leaves
	 * exposed. Empty for a rooms-and-links layout, whose regions describe their own floor.
	 *
	 * Published sorted so the same seed yields a byte-for-byte identical mask - flood fill order is
	 * an implementation detail and must not leak into the layout.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	TArray<FIntPoint> FloorMask;

	/**
	 * Units per mask tile. Meaningless with an empty mask, and it does not have to match the grid a
	 * later stage tiles at - which is why it travels with the mask rather than being assumed.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation", meta = (ClampMin = "0.0"))
	double MaskTileSize = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	FGameplayTagContainer Tags;
};
