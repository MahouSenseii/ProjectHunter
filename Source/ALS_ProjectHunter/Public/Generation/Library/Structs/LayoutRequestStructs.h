// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LayoutRequestStructs.generated.h"

/** How a strategy decides where the next region goes. */
UENUM(BlueprintType)
enum class EPHRegionPlacement : uint8
{
	/**
	 * Every region is drawn independently anywhere in the area and kept if it does not collide.
	 * Cheap and unbiased, but the result reads as a cloud of rooms: nothing relates a room to its
	 * neighbours, so the corridors joining them run long and cross the floor at random.
	 */
	Scatter,

	/**
	 * Each region is grown off one already placed: a side is chosen, a gap is drawn, and the new
	 * region is slid along that side until it shares at least one module of frontage with its
	 * parent. Two regions that share frontage can be joined by a short straight corridor, so the
	 * floor comes out as a plan of connected spaces rather than a scatter joined by long dog-legs.
	 *
	 * A region that finds no growth slot falls back to a free draw so the requested count is still
	 * honoured; those regions are linked to their nearest neighbour instead.
	 */
	Growth
};

/**
 * How many anchors of one semantic kind a region may receive. Content is authored as rules;
 * the placement algorithm stays in C++, so a new anchor kind needs data rather than code.
 *
 * A rule is a budget, not a guarantee: a region with too little interior space to seat the
 * drawn count places what fits.
 */
USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FPHAnchorRule
{
	GENERATED_BODY()

	/** Must be a registered descendant of Anchor, and not Anchor itself. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|Anchors", meta = (Categories = "Anchor"))
	FGameplayTag SemanticTag;

	/** Inclusive per-region count range, drawn from the layout stream. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|Anchors", meta = (ClampMin = "0"))
	int32 MinPerRegion = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|Anchors", meta = (ClampMin = "0"))
	int32 MaxPerRegion = 1;

	/** Cap across the whole layout. Zero means no cap. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|Anchors", meta = (ClampMin = "0"))
	int32 MaxTotal = 0;

	/** Keeps hostile content out of the entry room, or reserves the exit room for it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|Anchors")
	bool bAllowInStartRegion = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|Anchors")
	bool bAllowInExitRegion = true;
};

/**
 * Generator-neutral input. Describes the size and shape of the wanted layout without
 * naming a strategy, an asset pack, or a world. Sizes are Unreal units; a strategy
 * that cannot honour a field ignores it rather than failing.
 */
USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FPHLayoutRequest
{
	GENERATED_BODY()

	/**
	 * Literal layout-stream seed; callers derive it with URunSeedFunctionLibrary::DeriveLayoutSeed.
	 *
	 * Derive it, do not count. FRandomStream advances linearly from its seed, so nearby literal
	 * seeds produce correlated first draws: 1, 2, 3 walk the room count up a step at a time rather
	 * than sampling the range. Anything reached through DeriveLayoutSeed is already hashed and
	 * scatters properly.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|Request")
	int32 Seed = 0;

	/**
	 * Module size every region bound is snapped to, in Unreal units. Modular blockout meshes
	 * only tile a room whose edges land on their own grid, so this is a property of logical
	 * generation rather than something world construction can correct later.
	 *
	 * The default matches the BlockingStarterPack architecture set, whose pieces are authored
	 * at 100/200/300/400 and therefore share a 100-unit module.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|Request", meta = (ClampMin = "1.0"))
	double GridSize = 100.0;

	/**
	 * Planar area the layout must fit inside, measured from the layout-local origin.
	 * The default leaves the default room count around a third of the area, which
	 * rejection sampling fills comfortably.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|Request", meta = (ClampMin = "1.0"))
	FVector2D AreaSize = FVector2D(10000.0, 10000.0);

	/** Inclusive region-count range. A request too dense to hold the minimum is refused. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|Request", meta = (ClampMin = "1"))
	int32 MinRegionCount = 6;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|Request", meta = (ClampMin = "1"))
	int32 MaxRegionCount = 10;

	/** Inclusive per-region footprint range. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|Request", meta = (ClampMin = "1.0"))
	FVector2D MinRegionSize = FVector2D(800.0, 800.0);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|Request", meta = (ClampMin = "1.0"))
	FVector2D MaxRegionSize = FVector2D(1600.0, 1600.0);

	/**
	 * Vertical extent of the shortest region. Taller regions are whole multiples of this, so a
	 * modular wall kit stacks cleanly.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|Request", meta = (ClampMin = "1.0"))
	double RegionHeight = 400.0;

	/**
	 * Largest number of RegionHeight steps a region may be. 1 makes every region the same height;
	 * 3 gives rooms of one, two or three stacked walls, which is what stops a floor reading as one
	 * flat slab. Drawn per region from the layout stream.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|Request", meta = (ClampMin = "1"))
	int32 MaxHeightStacks = 1;

	/** Gap kept between regions, leaving room for later corridor and wall construction. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|Request", meta = (ClampMin = "0.0"))
	double RegionSpacing = 200.0;

	/** Whether regions are drawn independently or grown off one another. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|Layout")
	EPHRegionPlacement RegionPlacement = EPHRegionPlacement::Growth;

	/**
	 * Modules of corridor a grown region may sit beyond RegionSpacing, drawn per region. Zero
	 * puts every region exactly one spacing away from its parent, which makes the floor read as a
	 * uniform lattice; a few modules of slack is what gives short and long passages on one floor.
	 * Ignored by Scatter.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|Layout", meta = (ClampMin = "0"))
	int32 ExtraCorridorModules = 4;

	/**
	 * How many of the most recently placed regions growth may branch from. 1 grows a single
	 * corridor-like chain; large values let the floor branch back off anything already placed,
	 * which clusters it around the entrance. Ignored by Scatter.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|Layout", meta = (ClampMin = "1"))
	int32 GrowthFrontier = 4;

	/**
	 * Longest centre-to-centre distance a loop connection may span. A loop between distant regions
	 * builds as a corridor cutting the whole floor, which erases the room structure it was meant to
	 * enrich. Zero removes the limit.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|Layout", meta = (ClampMin = "0.0"))
	double MaxLoopDistance = 6000.0;

	/** Chance per candidate pair of adding a connection beyond the spanning tree, creating loops. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|Request", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LoopChance = 0.15f;

	/** Placement retries per region before the generator gives up on that region. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|Request", meta = (ClampMin = "1"))
	int32 MaxPlacementAttempts = 32;

	/**
	 * Optional semantic anchors to seat beyond the player start and exit. Empty means a layout
	 * of rooms and links only, and consumes no randomness, so a seed replays identically either
	 * way until rules are authored.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|Anchors")
	TArray<FPHAnchorRule> AnchorRules;

	/** Copied onto the produced layout, letting callers tag a floor's biome or role. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation|Request")
	FGameplayTagContainer Tags;
};
