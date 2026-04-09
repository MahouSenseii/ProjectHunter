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
// Delegate — broadcast after all mods are applied so UI can refresh the name
// ─────────────────────────────────────────────────────────────────────────────
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMonsterModsApplied,
	EMonsterTier, Tier, const FText&, FullDisplayName);

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

	// ── Runtime state ─────────────────────────────────────────────────────────

	UPROPERTY(BlueprintReadOnly, Category = "Runtime")
	EMonsterTier AssignedTier = EMonsterTier::MT_Normal;

	UPROPERTY(BlueprintReadOnly, Category = "Runtime")
	TArray<FMonsterModRow> AppliedMods;

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

	/** Returns the total HP multiplier from all applied mods combined. */
	UFUNCTION(BlueprintPure, Category = "Modifier")
	float GetCombinedHPMultiplier() const;

	/** Returns the total damage multiplier from all applied mods combined. */
	UFUNCTION(BlueprintPure, Category = "Modifier")
	float GetCombinedDamageMultiplier() const;

	// ── Events ────────────────────────────────────────────────────────────────

	UPROPERTY(BlueprintAssignable, Category = "Modifier|Events")
	FOnMonsterModsApplied OnMonsterModsApplied;

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
