// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Generation/Library/Structs/EncounterPlanStructs.h"
#include "Generation/Library/Structs/GeneratedLayoutStructs.h"
#include "Generation/Library/Structs/GenerationValidationStructs.h"
#include "PHEncounterPlanLibrary.generated.h"

/**
 * Turns enemy anchors into per-region encounter requests.
 *
 * Stateless and world-free. It spawns nothing and knows nothing about mobs, AI or pooling: it only
 * reports which regions want enemies, how many, and inside what volume. The encounter owner decides
 * exact positions, because it owns navmesh projection, ground and collision checks, and pooling
 * (GAME_DESIGN §29).
 */
UCLASS()
class ALS_PROJECTHUNTER_API UPHEncounterPlanLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Replaces OutPlan and OutIssues. Groups Anchor.Enemy, Anchor.Elite and Anchor.Boss anchors by
	 * region and reports one request per region that has any.
	 *
	 * Inset shrinks each request volume away from the region's walls, so the encounter owner is not
	 * handed candidate positions that sit inside geometry. A region too small to inset is reported
	 * with its full bounds rather than dropped.
	 */
	UFUNCTION(BlueprintCallable, Category = "Generation|Encounter")
	static bool BuildEncounterPlan(const FPHGeneratedLayout& Layout, double Inset,
		FPHEncounterPlan& OutPlan, TArray<FPHGenerationIssue>& OutIssues);
};
