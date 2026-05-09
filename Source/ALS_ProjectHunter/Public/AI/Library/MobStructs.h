// AI/Library/MobStructs.h
// All structs for the Mob Manager / Spawner / Modifier system.
//
// Dependency chain:
//   MobEnumLibrary.h  →  MobStructs.h  →  (system headers)
//
// Include this alone when you only need to pass or receive mob / spawn data
// (e.g. debug visualisation, analytics) without pulling in full manager headers.

#pragma once

#include "CoreMinimal.h"
#include "AI/Library/MobEnumLibrary.h"
#include "AttributeSet.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "MobStructs.generated.h"

class APHBaseCharacter;
class APlayerController;
class UGameplayEffect;
class UGameplayAbility;

// ─────────────────────────────────────────────────────────────────────────────
// FMonsterStatVariation — per-instance random multipliers rolled once at spawn
//
// Makes two Normal Goblins mechanically different. Every mob rolls one of these
// so nothing is ever perfectly identical. Tier-based mods apply on top.
// ─────────────────────────────────────────────────────────────────────────────
USTRUCT(BlueprintType)
struct FMonsterStatVariation
{
	GENERATED_BODY()

	/** 1.0 = no change. Rolled in [1 - HPVariancePct, 1 + HPVariancePct]. */
	UPROPERTY(BlueprintReadOnly, Category = "Variation")
	float HPMultiplier = 1.0f;

	/** 1.0 = no change. Rolled in [1 - DamageVariancePct, 1 + DamageVariancePct]. */
	UPROPERTY(BlueprintReadOnly, Category = "Variation")
	float DamageMultiplier = 1.0f;

	/** 1.0 = no change. Applied directly to MaxWalkSpeed. */
	UPROPERTY(BlueprintReadOnly, Category = "Variation")
	float MoveSpeedMultiplier = 1.0f;

	/** Additive bonus to all resistances in percent. E.g. +3.0 = +3% to all resists. */
	UPROPERTY(BlueprintReadOnly, Category = "Variation")
	float ResistBonusPct = 0.0f;

	/** True if this struct has been rolled (so we don't double-roll on reroll). */
	UPROPERTY(BlueprintReadOnly, Category = "Variation")
	bool bRolled = false;
};

// ─────────────────────────────────────────────────────────────────────────────
// FMonsterStatOverride — one stat override entry within a modifier
// ─────────────────────────────────────────────────────────────────────────────
USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FMonsterStatOverride
{
	GENERATED_BODY()

	/** GAS attribute to modify on the monster */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	FGameplayAttribute Attribute;

	/** Additive flat bonus */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float FlatBonus = 0.0f;

	/** Multiplicative percent bonus (0.25 = +25%) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float PercentBonus = 0.0f;
};

// ─────────────────────────────────────────────────────────────────────────────
// FMonsterModRow — DataTable row defining one named monster modifier
// ─────────────────────────────────────────────────────────────────────────────
USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FMonsterModRow : public FTableRowBase
{
	GENERATED_BODY()

	// ── Identity ─────────────────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity")
	FName ModID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity")
	FText DisplayLabel;

	/** Is this a prefix ("Berserker X") or suffix ("X the Unyielding")? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity")
	EMonsterModType ModType = EMonsterModType::MMT_Prefix;

	// ── Eligibility ───────────────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eligibility",
		meta = (ClampMin = 1, ClampMax = 100))
	int32 MinAreaLevel = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eligibility")
	EMonsterTier MinTier = EMonsterTier::MT_Magic;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eligibility",
		meta = (ClampMin = 1))
	int32 Weight = 100;

	// ── Stat Modifications ────────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats",
		meta = (ClampMin = 0.1f))
	float HPMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats",
		meta = (ClampMin = 0.1f))
	float DamageMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	TArray<FMonsterStatOverride> StatOverrides;

	// ── GAS Integration ───────────────────────────────────────────────────────

	/** Optional Gameplay Effect applied to the monster at spawn (Infinite duration). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAS")
	TSubclassOf<UGameplayEffect> OnSpawnGE;

	/** Optional aura GE applied to players within AuraRadius. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAS")
	TSubclassOf<UGameplayEffect> AuraGE;

	/** Radius (cm) for the aura — 0 = no aura */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAS",
		meta = (ClampMin = 0.0f))
	float AuraRadius = 0.0f;

	/** Optional ability granted to the monster at spawn. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAS")
	TSubclassOf<UGameplayAbility> GrantedAbilityClass;

	// ── AI Tuning ─────────────────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI",
		meta = (ClampMin = 0.1f))
	float AggroRangeMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI",
		meta = (ClampMin = 0.1f))
	float MoveSpeedMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	FGameplayTagContainer GrantedTags;
};

// ─────────────────────────────────────────────────────────────────────────────
// FMobTypeEntry — one slot in the manager's MobTypes array
// ─────────────────────────────────────────────────────────────────────────────
USTRUCT(BlueprintType)
struct FMobTypeEntry
{
	GENERATED_BODY()

	/** Enemy class to spawn — must be a subclass of APHBaseCharacter. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mob Type")
	TSubclassOf<APHBaseCharacter> MobClass;

	/** Relative spawn weight (0 = skip). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mob Type",
		meta = (ClampMin = 0, UIMin = 1))
	int32 SpawnWeight = 100;

	/** Per-type cooldown in seconds (0 = no cooldown). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mob Type",
		meta = (ClampMin = 0.0f))
	float SpawnCooldown = 0.0f;

	/** World time of last successful spawn. -1 = never. Runtime only. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Mob Type|Runtime")
	float LastSpawnTime = -1.0f;

	/**
	 * Returns true if this entry may be used for a spawn at CurrentTime.
	 * Defined in MobStructs.cpp — requires the full APHBaseCharacter definition
	 * that the header cannot provide via forward declaration alone.
	 */
	bool IsEligible(float CurrentTime) const;
};

// ─────────────────────────────────────────────────────────────────────────────
// FSpawnAttemptDebug — one entry in the visual debug history
// ─────────────────────────────────────────────────────────────────────────────
USTRUCT(BlueprintType)
struct FSpawnAttemptDebug
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	bool bSuccess = false;

	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	EMobSpawnFailReason FailReason = EMobSpawnFailReason::None;

	/** GetWorld()->GetTimeSeconds() at the moment of the attempt. */
	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	float Timestamp = 0.0f;
};

// ─────────────────────────────────────────────────────────────────────────────
// FMobSpecialSpawnRule — one designer-authored special-spawn rule
// ─────────────────────────────────────────────────────────────────────────────
USTRUCT(BlueprintType)
struct FMobSpecialSpawnRule
{
	GENERATED_BODY()

	// ── Identity ─────────────────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity")
	FName RuleId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity")
	bool bEnabled = true;

	// ── Trigger ───────────────────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trigger")
	EMobSpawnTriggerType TriggerType = EMobSpawnTriggerType::PlayerHasKeyItem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trigger",
		meta = (EditCondition = "TriggerType == EMobSpawnTriggerType::PlayerHasKeyItem"))
	FName RequiredKeyItemId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trigger",
		meta = (EditCondition = "TriggerType == EMobSpawnTriggerType::KillCountReached"))
	TSubclassOf<APHBaseCharacter> KillCountMobClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trigger",
		meta = (EditCondition = "TriggerType == EMobSpawnTriggerType::KillCountReached",
			ClampMin = 1))
	int32 RequiredKillCount = 10;

	// ── Action ────────────────────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action")
	EMobSpawnRuleAction Action = EMobSpawnRuleAction::ForceSpawnSpecialMob;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action",
		meta = (EditCondition = "Action == EMobSpawnRuleAction::ForceSpawnSpecialMob"))
	TSubclassOf<APHBaseCharacter> SpecialMobClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action")
	EMonsterTier ForcedTier = EMonsterTier::MT_Rare;

	// ── Lifetime ──────────────────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lifetime")
	EMobSpawnRuleLifetime Lifetime = EMobSpawnRuleLifetime::OneShot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lifetime",
		meta = (EditCondition = "Lifetime == EMobSpawnRuleLifetime::RepeatableWithCooldown",
			ClampMin = 0.0f))
	float CooldownSeconds = 60.0f;

	// ── Runtime state ─────────────────────────────────────────────────────────

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Runtime")
	bool bHasFired = false;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Runtime")
	float LastFireTime = -1.0f;
};

// ─────────────────────────────────────────────────────────────────────────────
// FPlayerLocationSnapshot — cached player location used by spawn systems
// ─────────────────────────────────────────────────────────────────────────────
USTRUCT(BlueprintType)
struct FPlayerLocationSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) TWeakObjectPtr<APlayerController> Controller;
	UPROPERTY(BlueprintReadOnly) FVector Location  = FVector::ZeroVector;
	UPROPERTY(BlueprintReadOnly) FVector Velocity  = FVector::ZeroVector;
	UPROPERTY(BlueprintReadOnly) double  TimestampSeconds = 0.0;
};
