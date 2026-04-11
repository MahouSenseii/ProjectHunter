// Character/Component/MonsterModifierComponent.h
// Applies FMonsterModRow entries to the owning enemy at spawn.
// Add this component to enemy Blueprint classes.
// Call RollAndApplyMods() from BeginPlay (or from the spawner after setting AreaLevel).
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/MonsterModifierData.h"
#include "GameplayEffectTypes.h"
#include "MonsterModifierComponent.generated.h"

// Log category declared here, defined once in the .cpp.
DECLARE_LOG_CATEGORY_EXTERN(LogMonsterModifier, Log, All);

class UAbilitySystemComponent;
class UMonsterSpawnConfig;
class UDataTable;

// ─────────────────────────────────────────────────────────────────────────────
// FMonsterStatVariation — per-instance random multipliers rolled once at spawn.
//
// This is what makes two Normal Goblins mechanically different. Every mob —
// regardless of tier — rolls one of these so nothing is ever perfectly identical.
// Tier-based mods are still applied on top for Magic / Rare / Unique.
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
// Delegate — broadcast after all mods are applied so UI can refresh the name
// ─────────────────────────────────────────────────────────────────────────────
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMonsterModsApplied,
	EMonsterTier, Tier, const FText&, FullDisplayName);

/** Broadcast after the per-instance base-stat variation has been rolled. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMonsterBaseStatVariationRolled,
	const FMonsterStatVariation&, Variation);

// ─────────────────────────────────────────────────────────────────────────────
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ALS_PROJECTHUNTER_API UMonsterModifierComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMonsterModifierComponent();

	virtual void BeginPlay() override;

	// ── Configuration ─────────────────────────────────────────────────────────

	/**
	 * Reference to the global spawn config.
	 * Assign this in the GameMode Blueprint or in the monster's Blueprint defaults.
	 * If null, the component will attempt to find it via UGameMode.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	TSoftObjectPtr<UMonsterSpawnConfig> SpawnConfig;

	/**
	 * Area level at which this monster spawns.
	 * Set by the spawning subsystem before BeginPlay fires, or override in BP.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config",
		meta = (ClampMin = 1, ClampMax = 100))
	int32 AreaLevel = 1;

	/**
	 * MagicFind value of the nearby player (set by the spawner from the player's stats).
	 * Controls how much Magic/Rare chances are boosted.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config",
		meta = (ClampMin = 0.0f))
	float NearbyPlayerMagicFind = 0.0f;

	/**
	 * Force a specific tier — if set to anything other than MT_Normal, skips the
	 * random roll.  Useful for scripted encounters and bosses.
	 * MT_Normal = use random roll.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	EMonsterTier ForcedTier = EMonsterTier::MT_Normal;

	// ── Per-instance base-stat variation ─────────────────────────────────────
	// Every mob — even Normal-tier — rolls a small random variation on base stats
	// so no two mobs are ever mechanically identical. Set variances to 0 to disable.

	/** Master toggle for per-instance variation. Turn off for cinematic / fixed encounters. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variation")
	bool bEnableBaseStatVariation = true;

	/** Max symmetric HP variance fraction. 0.15 = ±15% HP. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variation",
		meta = (EditCondition = "bEnableBaseStatVariation", ClampMin = 0.0f, ClampMax = 1.0f))
	float HPVariancePct = 0.15f;

	/** Max symmetric damage variance fraction. 0.10 = ±10% damage. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variation",
		meta = (EditCondition = "bEnableBaseStatVariation", ClampMin = 0.0f, ClampMax = 1.0f))
	float DamageVariancePct = 0.10f;

	/** Max symmetric move speed variance fraction. 0.08 = ±8% walk speed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variation",
		meta = (EditCondition = "bEnableBaseStatVariation", ClampMin = 0.0f, ClampMax = 1.0f))
	float MoveSpeedVariancePct = 0.08f;

	/** Max symmetric additive resist bonus in percent. 5.0 = ±5% flat resist. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variation",
		meta = (EditCondition = "bEnableBaseStatVariation", ClampMin = 0.0f, ClampMax = 25.0f))
	float ResistVariancePct = 5.0f;

	// ── Runtime state ─────────────────────────────────────────────────────────

	UPROPERTY(BlueprintReadOnly, Category = "Runtime")
	EMonsterTier AssignedTier = EMonsterTier::MT_Normal;

	UPROPERTY(BlueprintReadOnly, Category = "Runtime")
	TArray<FMonsterModRow> AppliedMods;

	/** Per-instance base-stat variation rolled at spawn. Folded into combined getters. */
	UPROPERTY(BlueprintReadOnly, Category = "Runtime")
	FMonsterStatVariation BaseStatVariation;

	/** Full display name built from base name + prefix/suffix labels */
	UPROPERTY(BlueprintReadOnly, Category = "Runtime")
	FText FullDisplayName;

	// ── Public API ────────────────────────────────────────────────────────────

	/**
	 * Roll tier and mods, then apply them to the owning character's ASC.
	 * Idempotent — safe to call once from BeginPlay.
	 * Server-only (authority validated internally).
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Modifier")
	void RollAndApplyMods();

	/**
	 * Manually assign a specific tier without rolling.
	 * Useful for scripted Rare/Unique encounters.
	 * Must be called before RollAndApplyMods.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Modifier")
	void SetTier(EMonsterTier NewTier);

	/**
	 * Remove any previously applied mods and re-roll from scratch.
	 * Used by MobManagerActor to apply its AreaLevel AFTER HiddenSpawn
	 * (which triggers BeginPlay → RollAndApplyMods with the wrong defaults).
	 * Safe to call multiple times.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Modifier")
	void RerollMods();

	/**
	 * Roll the per-instance base-stat variation struct. Idempotent — only rolls
	 * the first time it's called; subsequent calls are no-ops unless the struct
	 * has been reset (e.g. via RerollMods).
	 * Called automatically at the top of RollAndApplyMods, so designers usually
	 * don't need to invoke it directly.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Modifier")
	void RollBaseStatVariation();

	/** Returns the total HP multiplier from all applied mods combined. */
	UFUNCTION(BlueprintPure, Category = "Modifier")
	float GetCombinedHPMultiplier() const;

	/** Returns the total damage multiplier from all applied mods combined. */
	UFUNCTION(BlueprintPure, Category = "Modifier")
	float GetCombinedDamageMultiplier() const;

	// ── Events ────────────────────────────────────────────────────────────────

	UPROPERTY(BlueprintAssignable, Category = "Modifier|Events")
	FOnMonsterModsApplied OnMonsterModsApplied;

	UPROPERTY(BlueprintAssignable, Category = "Modifier|Events")
	FOnMonsterBaseStatVariationRolled OnBaseStatVariationRolled;

protected:
	bool bModsApplied = false;

	/**
	 * Cached resolved pointer for SpawnConfig (avoids repeated LoadSynchronous calls).
	 * Populated on first use.
	 */
	UPROPERTY(Transient)
	TObjectPtr<UMonsterSpawnConfig> CachedSpawnConfig;

	/** Roll N mods from the modifier table, respecting area level and tier. */
	TArray<FMonsterModRow> RollMods(int32 NumMods, EMonsterTier Tier,
		const UDataTable* Table) const;

	/** Apply a single mod row to the owning character's ASC. */
	void ApplyMod(const FMonsterModRow& Mod, UAbilitySystemComponent* ASC);

	/** Build the full display name from the base name + prefix/suffix labels. */
	FText BuildDisplayName(const TArray<FMonsterModRow>& Mods) const;

	/** Handles for GEs applied by mods — kept so we could remove them if needed */
	UPROPERTY(Transient)
	TArray<FActiveGameplayEffectHandle> AppliedGEHandles;
};
