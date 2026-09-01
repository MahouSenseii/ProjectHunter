// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Generation/Library/Structs/GeneratedLayoutStructs.h"
#include "Generation/Library/Structs/GenerationValidationStructs.h"
#include "Generation/Library/Structs/LayoutRequestStructs.h"
#include "PHLayoutGenerator.generated.h"

/**
 * Shared contract for every layout strategy: one request in, one validated
 * FPHGeneratedLayout out. Strategies override BuildLayout only; this base owns the
 * deterministic stream, the provenance stamp, and both validation gates, so no caller
 * can receive a layout that would fail validation.
 *
 * An impossible request is refused with a typed reason rather than quietly reshaped,
 * so a mis-authored Data Asset surfaces at the point of authoring instead of silently
 * producing a floor nobody asked for.
 *
 * Logical only. Generators never spawn actors, touch a world, or read the run owner.
 */
UCLASS(Abstract, BlueprintType)
class ALS_PROJECTHUNTER_API UPHLayoutGenerator : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Replaces OutLayout and OutIssues. Returns false, leaving OutLayout empty, when the
	 * request is impossible, the strategy cannot build it, or the result fails validation.
	 */
	UFUNCTION(BlueprintCallable, Category = "Generation")
	bool GenerateLayout(const FPHLayoutRequest& Request, FPHGeneratedLayout& OutLayout,
		TArray<FPHGenerationIssue>& OutIssues);

	/**
	 * Strategy-independent checks for finite extents, representable positive module counts,
	 * ordered ranges, and a footprint that fits the area. Individual strategies apply their
	 * own capacity limits. Exposed so content can be validated before a run needs it.
	 */
	UFUNCTION(BlueprintCallable, Category = "Generation")
	static bool ValidateRequest(const FPHLayoutRequest& Request, TArray<FPHGenerationIssue>& OutIssues);

	/** Bumped by a strategy whenever the same seed would stop producing the same layout. */
	UFUNCTION(BlueprintPure, Category = "Generation")
	int32 GetGenerationVersion() const { return GenerationVersion; }

	/**
	 * Whole modules spanned by Size, rounded down and up respectively. Strategies place in
	 * module counts rather than units so every bound lands exactly on the grid; the tolerance
	 * keeps an exact multiple from falling to the wrong side through float error.
	 * Returns INDEX_NONE for a non-finite input, a non-positive grid, or a count outside int32.
	 */
	static int32 ModulesDown(double Size, double GridSize);
	static int32 ModulesUp(double Size, double GridSize);

protected:
	/**
	 * Strategy implementation. The request has already passed ValidateRequest, so ranges are
	 * ordered and positive. All randomness must come from Stream so a seed reproduces its
	 * layout across processes. Seed, GenerationVersion, and request Tags are stamped by the caller.
	 */
	virtual bool BuildLayout(const FPHLayoutRequest& Request, FRandomStream& Stream,
		FPHGeneratedLayout& OutLayout, TArray<FPHGenerationIssue>& OutIssues)
		PURE_VIRTUAL(UPHLayoutGenerator::BuildLayout, return false;);

	/**
	 * Geometry checks the shared structural validator deliberately does not make, such as
	 * room overlap or containment. Structural validity is not proof of a walkable floor,
	 * so each strategy proves its own constraints instead of trusting ValidateLayout.
	 */
	virtual bool ValidateStrategyConstraints(const FPHLayoutRequest& Request,
		const FPHGeneratedLayout& Layout, TArray<FPHGenerationIssue>& OutIssues) const;

	/** Appends one issue; a helper because every refusal path needs the same shape. */
	static void AddIssue(TArray<FPHGenerationIssue>& Issues, EPHGenerationIssueCode Code,
		int32 ElementIndex, FString Message);

	int32 GenerationVersion = 1;
};
