// AI/Mob/MobSpawnRules.h
// Data types for the special / event-driven spawn rules system used by AMobManagerActor.
//
// Designers add entries to AMobManagerActor::SpecialSpawnRules. Each tick the manager
// asks UMobSpawnConditionEvaluator whether any rule is ready to fire; if so, the
// manager performs the associated action (force-spawn a specific mob, or boost the
// tier of the next normal spawn).
//
// Kept in a dedicated header so the evaluator helper and the manager can share the
// same types without a circular include.
#pragma once

#include "CoreMinimal.h"
#include "Character/PHBaseCharacter.h"
#include "Data/MonsterModifierData.h"
#include "MobSpawnRules.generated.h"

// ─────────────────────────────────────────────────────────────────────────────
// Trigger source — what gameplay condition fires the rule
// ─────────────────────────────────────────────────────────────────────────────
UENUM(BlueprintType)
enum class EMobSpawnTriggerType : uint8
{
	/** Fires when any tracked player is carrying a specific key item. */
	PlayerHasKeyItem    UMETA(DisplayName = "Player Has Key Item"),

	/** Fires when the manager's kill count for a mob class reaches the threshold. */
	KillCountReached    UMETA(DisplayName = "Kill Count Reached"),
};

// ─────────────────────────────────────────────────────────────────────────────
// Action — what the manager does once the rule fires
// ─────────────────────────────────────────────────────────────────────────────
UENUM(BlueprintType)
enum class EMobSpawnRuleAction : uint8
{
	/** Immediately spawns SpecialMobClass via the normal spawn pipeline. */
	ForceSpawnSpecialMob    UMETA(DisplayName = "Force Spawn Special Mob"),

	/** Boosts the tier of the next normal spawn to ForcedTier. */
	BoostNextSpawnTier      UMETA(DisplayName = "Boost Next Spawn Tier"),
};

// ─────────────────────────────────────────────────────────────────────────────
// Lifetime — does the rule run once, or can it re-arm?
// ─────────────────────────────────────────────────────────────────────────────
UENUM(BlueprintType)
enum class EMobSpawnRuleLifetime : uint8
{
	/** Fires exactly once per manager instance, then disables itself. */
	OneShot                   UMETA(DisplayName = "One Shot"),

	/** Fires, cools down for CooldownSeconds, then can fire again. */
	RepeatableWithCooldown    UMETA(DisplayName = "Repeatable With Cooldown"),
};

// ─────────────────────────────────────────────────────────────────────────────
// FMobSpecialSpawnRule — one designer-authored rule row
// ─────────────────────────────────────────────────────────────────────────────
USTRUCT(BlueprintType)
struct FMobSpecialSpawnRule
{
	GENERATED_BODY()

	// ── Identity ─────────────────────────────────────────────────────────────

	/** Unique ID for this rule — used in logs and by designers to track fires. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity")
	FName RuleId;

	/** Master toggle — set false to disable without deleting the row. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity")
	bool bEnabled = true;

	// ── Trigger ───────────────────────────────────────────────────────────────

	/** How this rule decides it's ready to fire. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trigger")
	EMobSpawnTriggerType TriggerType = EMobSpawnTriggerType::PlayerHasKeyItem;

	/**
	 * Used when TriggerType == PlayerHasKeyItem.
	 * Item ID that at least one tracked player must be carrying.
	 * The inventory check is done via AMobManagerActor::DoesAnyPlayerHaveKeyItem
	 * (BlueprintNativeEvent — designers override in the child BP).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trigger",
		meta = (EditCondition = "TriggerType == EMobSpawnTriggerType::PlayerHasKeyItem"))
	FName RequiredKeyItemId;

	/**
	 * Used when TriggerType == KillCountReached.
	 * Only kills of this class (or subclasses) count toward the threshold.
	 * Leave null to count every kill the manager tracks.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trigger",
		meta = (EditCondition = "TriggerType == EMobSpawnTriggerType::KillCountReached"))
	TSubclassOf<APHBaseCharacter> KillCountMobClass;

	/** Used when TriggerType == KillCountReached. Threshold to reach. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trigger",
		meta = (EditCondition = "TriggerType == EMobSpawnTriggerType::KillCountReached",
			ClampMin = 1))
	int32 RequiredKillCount = 10;

	// ── Action ────────────────────────────────────────────────────────────────

	/** What the manager does when this rule fires. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action")
	EMobSpawnRuleAction Action = EMobSpawnRuleAction::ForceSpawnSpecialMob;

	/**
	 * Used when Action == ForceSpawnSpecialMob.
	 * Mob class to spawn — goes through the normal spawn pipeline (nav, collision,
	 * ground, distance checks) and gets modifiers applied by the manager.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action",
		meta = (EditCondition = "Action == EMobSpawnRuleAction::ForceSpawnSpecialMob"))
	TSubclassOf<APHBaseCharacter> SpecialMobClass;

	/**
	 * Used when Action == BoostNextSpawnTier (or as the starting tier of a forced spawn).
	 * Applied via AMobManagerActor::PendingForcedTier and consumed on the next spawn.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action")
	EMonsterTier ForcedTier = EMonsterTier::MT_Rare;

	// ── Lifetime ──────────────────────────────────────────────────────────────

	/** One shot, or repeatable with cooldown. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lifetime")
	EMobSpawnRuleLifetime Lifetime = EMobSpawnRuleLifetime::OneShot;

	/**
	 * Used when Lifetime == RepeatableWithCooldown.
	 * Minimum seconds between successive fires of this rule.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lifetime",
		meta = (EditCondition = "Lifetime == EMobSpawnRuleLifetime::RepeatableWithCooldown",
			ClampMin = 0.0f))
	float CooldownSeconds = 60.0f;

	// ── Runtime state (not editable, reset on manager restart) ───────────────

	/** True once this rule has fired at least once. OneShot rules disable themselves after this flips. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Runtime")
	bool bHasFired = false;

	/** World time of the last fire. -1 = never. Used for cooldown math. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Runtime")
	float LastFireTime = -1.0f;
};
