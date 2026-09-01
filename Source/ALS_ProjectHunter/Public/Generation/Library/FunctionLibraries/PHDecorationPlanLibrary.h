// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Generation/Library/Structs/BlockoutPlanStructs.h"
#include "Generation/Library/Structs/GeneratedLayoutStructs.h"
#include "Generation/Library/Structs/GenerationValidationStructs.h"
#include "PHDecorationPlanLibrary.generated.h"

/**
 * Scatters props over a built floor so it reads as a place rather than a test level.
 *
 * Stateless and world-free, like the blockout planner. It draws only from a decoration stream
 * derived separately from the layout stream, which is what guarantees that adding, removing, or
 * reweighting clutter can never move a room, an anchor, an encounter, or a loot roll
 * (GAME_DESIGN §12 and §38).
 */
UCLASS()
class ALS_PROJECTHUNTER_API UPHDecorationPlanLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Replaces OutPlan and OutIssues. Rules are applied in array order; each tile takes at most one
	 * prop, so an earlier rule wins a contested tile. Player start, exit, and anchor tiles are
	 * protected, and corridors can be excluded so dressing never narrows the required path.
	 * Each prop's MinSpacingTiles also constrains later rules with smaller spacing. Scalar settings
	 * must be finite and within their authored ranges. Work is bounded for this synchronous planner;
	 * oversized input is refused without partial output.
	 *
	 * DecorationSeed should come from URunSeedFunctionLibrary::DeriveSeed on the floor seed, never
	 * from the layout seed.
	 */
	UFUNCTION(BlueprintCallable, Category = "Generation|Decoration")
	static bool BuildDecorationPlan(const FPHGeneratedLayout& Layout, const FPHBlockoutPlan& Blockout,
		const TArray<FPHPropRule>& Rules, int32 DecorationSeed,
		FPHDecorationPlan& OutPlan, TArray<FPHGenerationIssue>& OutIssues);
};
