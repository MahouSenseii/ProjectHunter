#pragma once

#include "CoreMinimal.h"
#include "Stats/Library/Structs/StatsStructs.h"

class UItemInstance;
class UStatsManager;
struct FPHAttributeData;

/** Stateless tag/condition gate and modifier fold shared by every affix source. */
class ALS_PROJECTHUNTER_API FContextualStatModifierEvaluator
{
public:
	static FContextualStatModifierSnapshot BuildFromItems(
		const TArray<UItemInstance*>& Items,
		const UStatsManager* StatsManager,
		const FStatModifierEvaluationContext& Context);

	static void AccumulateModifiers(
		const TArray<FPHAttributeData>& Modifiers,
		const UStatsManager* StatsManager,
		const FStatModifierEvaluationContext& Context,
		FContextualStatModifierSnapshot& InOutSnapshot);

	static bool MatchesContext(
		const FPHAttributeData& Modifier,
		const FStatModifierEvaluationContext& Context);
};
