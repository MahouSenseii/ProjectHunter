#pragma once

#include "CoreMinimal.h"
#include "AI/Library/Structs/MobStructs.h"
#include "UObject/NoExportTypes.h"
#include "MobSpawnConditionEvaluator.generated.h"

class AMobManagerActor;

DECLARE_LOG_CATEGORY_EXTERN(LogMobSpawnRules, Log, All);

UCLASS()
class ALS_PROJECTHUNTER_API UMobSpawnConditionEvaluator : public UObject
{
	GENERATED_BODY()

public:
	static bool IsRuleReady(
		const FMobSpecialSpawnRule& Rule,
		const AMobManagerActor* Manager,
		float CurrentTime);

	static void MarkRuleFired(FMobSpecialSpawnRule& Rule, float CurrentTime);

private:
	static bool EvaluateTrigger(
		const FMobSpecialSpawnRule& Rule,
		const AMobManagerActor* Manager);
};
