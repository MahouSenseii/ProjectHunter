// Progression/Library/ProgressionStructs.h
// Shared structs for the character progression (XP / leveling / stat points) system.
//
// Dependency rule: no project includes — only CoreMinimal.
// Include this alone when you need to pass or receive progression data
// (e.g. respec screens, analytics) without pulling in UCharacterProgressionManager.

#pragma once

#include "CoreMinimal.h"
#include "ProgressionStructs.generated.h"

// ─────────────────────────────────────────────────────────────────────────────
// FStatPointSpending — replicated record of how many points were spent on one attribute
// ─────────────────────────────────────────────────────────────────────────────
USTRUCT(BlueprintType)
struct FStatPointSpending
{
	GENERATED_BODY()

	/** Attribute name (Strength, Dexterity, etc.) */
	UPROPERTY(BlueprintReadOnly)
	FName AttributeName;

	/** Number of stat points spent on this attribute. */
	UPROPERTY(BlueprintReadOnly)
	int32 PointsSpent = 0;

	FStatPointSpending()
		: AttributeName(NAME_None)
		, PointsSpent(0)
	{}

	FStatPointSpending(FName InAttributeName, int32 InPointsSpent)
		: AttributeName(InAttributeName)
		, PointsSpent(InPointsSpent)
	{}
};
