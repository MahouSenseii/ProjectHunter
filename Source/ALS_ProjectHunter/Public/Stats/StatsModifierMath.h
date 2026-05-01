#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "Item/Library/AffixEnums.h"

struct ALS_PROJECTHUNTER_API FResolvedStatModifier
{
	EGameplayModOp::Type ModOp = EGameplayModOp::Additive;
	float Magnitude = 0.f;
	bool bCreatesGameplayModifier = false;
};

class ALS_PROJECTHUNTER_API FStatsModifierMath
{
public:
	static bool ResolveGameplayModifier(EModifyType ModifyType, float RolledValue, FResolvedStatModifier& OutModifier);
	static float PercentToMultiplier(float Percent);
	static float ApplyPercentChange(float BaseValue, float PercentChange);
};
