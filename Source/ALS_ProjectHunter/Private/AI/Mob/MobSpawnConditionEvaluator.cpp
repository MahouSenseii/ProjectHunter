// AI/Mob/MobSpawnConditionEvaluator.cpp
#include "AI/Mob/MobSpawnConditionEvaluator.h"
#include "AI/Mob/MobManagerActor.h"
#include "Core/Logging/ProjectHunterLogMacros.h"

DEFINE_LOG_CATEGORY(LogMobSpawnRules);

bool UMobSpawnConditionEvaluator::IsRuleReady(const FMobSpecialSpawnRule& Rule,
                                              const AMobManagerActor* Manager,
                                              const float CurrentTime)
{
	if (!Rule.bEnabled)
	{
		return false;
	}

	if (!Manager)
	{
		PH_LOG_WARNING(LogMobSpawnRules,
			TEXT("Rule '%s' skipped — null manager"), *Rule.RuleId.ToString());
		return false;
	}

	// ── Lifetime gate ────────────────────────────────────────────────────────
	switch (Rule.Lifetime)
	{
	case EMobSpawnRuleLifetime::OneShot:
		if (Rule.bHasFired)
		{
			return false;
		}
		break;

	case EMobSpawnRuleLifetime::RepeatableWithCooldown:
		if (Rule.LastFireTime >= 0.0f &&
		    (CurrentTime - Rule.LastFireTime) < Rule.CooldownSeconds)
		{
			return false;
		}
		break;

	default:
		PH_LOG_WARNING(LogMobSpawnRules,
			TEXT("Rule '%s' has unknown lifetime enum value"), *Rule.RuleId.ToString());
		return false;
	}

	return EvaluateTrigger(Rule, Manager);
}

void UMobSpawnConditionEvaluator::MarkRuleFired(FMobSpecialSpawnRule& Rule, const float CurrentTime)
{
	Rule.bHasFired    = true;
	Rule.LastFireTime = CurrentTime;
}

bool UMobSpawnConditionEvaluator::EvaluateTrigger(const FMobSpecialSpawnRule& Rule,
                                                  const AMobManagerActor* Manager)
{
	switch (Rule.TriggerType)
	{
	case EMobSpawnTriggerType::PlayerHasKeyItem:
	{
		if (Rule.RequiredKeyItemId.IsNone())
		{
			PH_LOG_WARNING(LogMobSpawnRules,
				TEXT("Rule '%s' uses PlayerHasKeyItem but has no RequiredKeyItemId set"),
				*Rule.RuleId.ToString());
			return false;
		}
		// AMobManagerActor exposes a BlueprintNativeEvent so designers can wire
		// up whatever inventory system they use without this file depending on it.
		return const_cast<AMobManagerActor*>(Manager)
			->DoesAnyPlayerHaveKeyItem(Rule.RequiredKeyItemId);
	}

	case EMobSpawnTriggerType::KillCountReached:
	{
		const int32 Count = Rule.KillCountMobClass
			? Manager->GetKillCountForClass(Rule.KillCountMobClass)
			: Manager->GetTotalKillCount();
		return Count >= Rule.RequiredKillCount;
	}

	default:
		PH_LOG_WARNING(LogMobSpawnRules,
			TEXT("Rule '%s' has unknown trigger type"), *Rule.RuleId.ToString());
		return false;
	}
}
