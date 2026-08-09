#pragma once

#include "CoreMinimal.h"

class UCharacterProgressionManager;

class ALS_PROJECTHUNTER_API FProgressionStatPointHelper
{
public:
	static bool ApplyStatPointToAttribute(UCharacterProgressionManager& Manager, FName AttributeName);
	static void RemoveStatPointFromAttribute(UCharacterProgressionManager& Manager, FName AttributeName, int32 PointsToRemove);
	static void RebuildSpentStatPointsCache(UCharacterProgressionManager& Manager);
};
