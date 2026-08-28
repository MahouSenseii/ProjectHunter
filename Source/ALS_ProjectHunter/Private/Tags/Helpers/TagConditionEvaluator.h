#pragma once

#include "Tags/Library/Structs/TagStructs.h"

struct FTagResourceConditionState
{
	bool bLow = false;
	bool bFull = false;
};

/** Stateless threshold and hysteresis rules used by TagManager's derived conditions. */
class FTagConditionEvaluator
{
public:
	static FTagResourceConditionState EvaluateResource(
		float CurrentValue,
		float MaxValue,
		bool bWasLow,
		bool bWasFull,
		const FTagConditionThresholds& Thresholds);

	static bool EvaluateMovement(
		float SpeedSquared2D,
		bool bHasPreviousState,
		bool bWasMoving,
		const FTagConditionThresholds& Thresholds);
};
