#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ProgressionFunctionLibrary.generated.h"

UCLASS()
class ALS_PROJECTHUNTER_API UProgressionFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Progression|XP")
	static int64 GetXPForLevel(int32 TargetLevel, float BaseXPPerLevel, float XPScalingExponent);

	UFUNCTION(BlueprintPure, Category = "Progression|XP")
	static float CalculateLevelPenalty(int32 LevelDifference);

	UFUNCTION(BlueprintPure, Category = "Progression|XP")
	static float CalculateXPMultiplier(float GlobalXP, float LocalXP, float MoreXP, float Penalty, float LevelPenalty = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Progression|XP")
	static int64 CalculateFinalXP(int64 BaseXP, float FinalMultiplier);

	UFUNCTION(BlueprintPure, Category = "Progression|XP")
	static float GetXPProgressPercent(int64 CurrentXP, int64 XPToNextLevel);
};
