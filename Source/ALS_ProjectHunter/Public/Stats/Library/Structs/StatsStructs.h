#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"

struct ALS_PROJECTHUNTER_API FResolvedStatModifier
{
	EGameplayModOp::Type ModOp = EGameplayModOp::Additive;
	float Magnitude = 0.0f;
	bool bCreatesGameplayModifier = false;
};
