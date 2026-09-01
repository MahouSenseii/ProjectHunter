// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#pragma once

#include "CoreMinimal.h"
#include "GenerationValidationEnums.generated.h"

UENUM(BlueprintType)
enum class EPHGenerationIssueCode : uint8
{
	None,
	InvalidGenerationVersion,
	InvalidLayoutBounds,
	EmptyRegions,
	InvalidRegionID,
	DuplicateRegionID,
	InvalidRegionBounds,
	RegionOutsideLayout,
	InvalidConnectionID,
	DuplicateConnectionID,
	MissingConnectionRegion,
	SelfConnection,
	InvalidAnchorID,
	DuplicateAnchorID,
	MissingAnchorRegion,
	InvalidAnchorTransform,
	AnchorOutsideRegion,
	InvalidAnchorTag,
	MissingPlayerStart,
	MissingExit,
	InvalidPlayerStartTag,
	InvalidExitTag,
	UnreachableRegion,
	UnreachableExit,

	// Producer-side codes, appended so existing values keep their numbers.
	// ValidateLayout never emits these: they come from a generator refusing a request or
	// failing its own geometry constraints, which structural validation deliberately skips.
	InvalidRequest,
	UnplaceableRegions,
	OverlappingRegions,
	RegionOffGrid,

	// Authoring-side code. Emitted by biome module set validation, not by a generator.
	InvalidModuleSet
};
