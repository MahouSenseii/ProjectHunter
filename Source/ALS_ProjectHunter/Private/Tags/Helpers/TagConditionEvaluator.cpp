#include "Tags/Helpers/TagConditionEvaluator.h"

FTagResourceConditionState FTagConditionEvaluator::EvaluateResource(
	const float CurrentValue,
	const float MaxValue,
	const bool bWasLow,
	const bool bWasFull,
	const FTagConditionThresholds& Thresholds)
{
	FTagResourceConditionState State;
	if (MaxValue <= 0.0f)
	{
		return State;
	}

	const float LowEnter = FMath::Clamp(Thresholds.LowResourceEnterPercent, 0.0f, 1.0f);
	const float LowExit = FMath::Clamp(Thresholds.LowResourceExitPercent, LowEnter, 1.0f);
	const float FullEnter = FMath::Clamp(Thresholds.FullResourceEnterPercent, LowExit, 1.0f);
	const float FullExit = FMath::Clamp(Thresholds.FullResourceExitPercent, LowExit, FullEnter);
	const float CurrentPercent = FMath::Max(CurrentValue, 0.0f) / MaxValue;

	State.bLow = CurrentPercent <= (bWasLow ? LowExit : LowEnter);
	State.bFull = CurrentPercent >= (bWasFull ? FullExit : FullEnter);

	if (State.bLow && State.bFull)
	{
		State.bLow = false;
	}

	return State;
}

bool FTagConditionEvaluator::EvaluateMovement(
	const float SpeedSquared2D,
	const bool bHasPreviousState,
	const bool bWasMoving,
	const FTagConditionThresholds& Thresholds)
{
	const float StartSpeed = FMath::Max(Thresholds.MovementStartSpeed, 0.0f);
	const float StopSpeed = FMath::Clamp(Thresholds.MovementStopSpeed, 0.0f, StartSpeed);
	const float StartSpeedSquared = FMath::Square(StartSpeed);
	const float StopSpeedSquared = FMath::Square(StopSpeed);

	if (!bHasPreviousState)
	{
		return SpeedSquared2D > StartSpeedSquared;
	}

	return bWasMoving
		? SpeedSquared2D > StopSpeedSquared
		: SpeedSquared2D > StartSpeedSquared;
}
