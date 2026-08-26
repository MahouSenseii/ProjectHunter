#include "Progression/Library/FunctionLibraries/ProgressionFunctionLibrary.h"

int64 UProgressionFunctionLibrary::GetXPForLevel(const int32 TargetLevel, const float BaseXPPerLevel, const float XPScalingExponent)
{
	if (TargetLevel <= 1)
	{
		return 0;
	}

	const float XP = BaseXPPerLevel * FMath::Pow(static_cast<float>(TargetLevel), XPScalingExponent);
	return FMath::RoundToInt64(XP);
}

float UProgressionFunctionLibrary::CalculateLevelPenalty(const int32 LevelDifference)
{
	if (LevelDifference <= 5)
	{
		return 1.0f;
	}
	if (LevelDifference <= 10)
	{
		return 0.8f;
	}
	if (LevelDifference <= 20)
	{
		return 0.5f;
	}
	if (LevelDifference <= 30)
	{
		return 0.25f;
	}

	return 0.05f;
}

float UProgressionFunctionLibrary::CalculateXPMultiplier(
	const float GlobalXP,
	const float LocalXP,
	const float MoreXP,
	const float Penalty,
	const float LevelPenalty)
{
	const float IncreasedMultiplier = 1.0f + (GlobalXP + LocalXP) / 100.0f;
	return FMath::Max(0.f, IncreasedMultiplier)
		* FMath::Max(MoreXP, 0.01f)
		* FMath::Max(Penalty, 0.f)
		* FMath::Max(LevelPenalty, 0.f);
}

int64 UProgressionFunctionLibrary::CalculateFinalXP(const int64 BaseXP, const float FinalMultiplier)
{
	const int64 FinalXP = FMath::RoundToInt64(BaseXP * FinalMultiplier);
	return FMath::Max(FinalXP, 1LL);
}

float UProgressionFunctionLibrary::GetXPProgressPercent(const int64 CurrentXP, const int64 XPToNextLevel)
{
	if (XPToNextLevel <= 0)
	{
		return 1.0f;
	}

	return static_cast<float>(CurrentXP) / static_cast<float>(XPToNextLevel);
}
