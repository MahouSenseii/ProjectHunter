#pragma once

#include "CoreMinimal.h"
#include "Item/Library/Enums/AffixEnums.h"
#include "Stats/Library/Structs/StatsStructs.h"

class ALS_PROJECTHUNTER_API FStatsModifierMath
{
public:
	static bool ResolveGameplayModifier(EModifyType ModifyType, float RolledValue, FResolvedStatModifier& OutModifier);
	static float PercentToMultiplier(float Percent);
	static float ApplyPercentChange(float BaseValue, float PercentChange);
};
