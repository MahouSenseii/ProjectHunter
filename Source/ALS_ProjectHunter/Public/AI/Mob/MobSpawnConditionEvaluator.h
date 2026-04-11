// AI/Mob/MobSpawnConditionEvaluator.h
// Stateless helper that evaluates FMobSpecialSpawnRule rows for AMobManagerActor.
//
// Lives in its own class (rather than inline on the manager) because:
//   1. MobManagerActor.cpp is already ~1.4k lines — avoid adding more bloat.
//   2. Keeps trigger-evaluation logic unit-testable in isolation.
//   3. Matches the "split helpers" pattern from the project coding standards.
#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "AI/Mob/MobSpawnRules.h"
#include "MobSpawnConditionEvaluator.generated.h"

class AMobManagerActor;

/** Per-file log category for the rule evaluator. */
DECLARE_LOG_CATEGORY_EXTERN(LogMobSpawnRules, Log, All);

/**
 * Stateless evaluator — just static helpers. Does not own any state itself;
 * all runtime state (bHasFired, LastFireTime, kill counts) lives on the rule row
 * or the owning manager.
 */
UCLASS()
class ALS_PROJECTHUNTER_API UMobSpawnConditionEvaluator : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Returns true if the rule is enabled, its lifetime allows firing right now,
	 * and its trigger condition is currently satisfied.
	 *
	 * @param Rule           Rule row to evaluate (reads runtime state).
	 * @param Manager        Owning manager — used to query kill counts and key-item presence.
	 * @param CurrentTime    GetWorld()->GetTimeSeconds() at the call site.
	 */
	static bool IsRuleReady(const FMobSpecialSpawnRule& Rule,
	                        const AMobManagerActor* Manager,
	                        float CurrentTime);

	/**
	 * Stamps bHasFired / LastFireTime on the rule. Call this right after the
	 * manager successfully performs the rule's action.
	 */
	static void MarkRuleFired(FMobSpecialSpawnRule& Rule, float CurrentTime);

private:
	/** Evaluates just the trigger condition (ignoring lifetime / cooldown state). */
	static bool EvaluateTrigger(const FMobSpecialSpawnRule& Rule,
	                            const AMobManagerActor* Manager);
};
