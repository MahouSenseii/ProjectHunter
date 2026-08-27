
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AI/Library/Enums/MobEnumLibrary.h"
#include "AI/Library/Structs/MobStructs.h"
#include "AI/Data/MonsterModifierData.h"
#include "GameplayAbilitySpec.h"
#include "GameplayEffectTypes.h"
#include "MonsterModifierComponent.generated.h"

// Log category declared here, defined once in the .cpp.
DECLARE_LOG_CATEGORY_EXTERN(LogMonsterModifier, Log, All);

class UAbilitySystemComponent;
class UMonsterSpawnConfig;
class UDataTable;

// Delegate - broadcast after all mods are applied so UI can refresh the name
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMonsterModsApplied,
	EMonsterTier, Tier, const FText&, FullDisplayName);

/** Broadcast after the per-instance base-stat variation has been rolled. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMonsterBaseStatVariationRolled,
	const FMonsterStatVariation&, Variation);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ALS_PROJECTHUNTER_API UMonsterModifierComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMonsterModifierComponent();

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	/**
	 * Fires on clients when replicated presentation state lands. AssignedTier and
	 * FullDisplayName replicate as independent properties, so this can run twice
	 * for one roll; broadcasting OnMonsterModsApplied is an idempotent UI refresh,
	 * so existing nameplate Blueprints stay correct either way.
	 */
	UFUNCTION()
	void OnRep_MonsterPresentation();

public:


	/**
	 * Reference to the global spawn config.
	 * Assign this in the GameMode Blueprint or in the monster's Blueprint defaults.
	 * If null, the component will attempt to find it via UGameMode.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	TSoftObjectPtr<UMonsterSpawnConfig> SpawnConfig;

	/**
	 * Deterministic seed for this monster's tier, modifier and stat-variation
	 * rolls. Spawners derive it from the run seed via URunSeedFunctionLibrary so
	 * one RunSeed reproduces the same monster composition every time.
	 *
	 * Zero means "unseeded": the component falls back to the global RNG, which is
	 * correct for monsters placed directly in a level or spawned outside a run.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Determinism")
	int32 MonsterSeed = 0;

	/** Server-only. Set before RollAndApplyMods to make this monster's rolls reproducible. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Modifier")
	void SetMonsterSeed(int32 InSeed);

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
	 * Force a specific tier - if set to anything other than MT_Normal, skips the
	 * random roll.  Useful for scripted encounters and bosses.
	 * MT_Normal = use random roll.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	EMonsterTier ForcedTier = EMonsterTier::MT_Normal;

	// Every mob - even Normal-tier - rolls a small random variation on base stats
	// so no two mobs are ever mechanically identical. Set variances to 0 to disable.

	/** Master toggle for per-instance variation. Turn off for cinematic / fixed encounters. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variation")
	bool bEnableBaseStatVariation = true;

	/** Max symmetric HP variance fraction. 0.15 = +/-15% HP. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variation",
		meta = (EditCondition = "bEnableBaseStatVariation", ClampMin = 0.0f, ClampMax = 1.0f))
	float HPVariancePct = 0.15f;

	/** Max symmetric damage variance fraction. 0.10 = +/-10% damage. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variation",
		meta = (EditCondition = "bEnableBaseStatVariation", ClampMin = 0.0f, ClampMax = 1.0f))
	float DamageVariancePct = 0.10f;

	/** Max symmetric move speed variance fraction. 0.08 = +/-8% walk speed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variation",
		meta = (EditCondition = "bEnableBaseStatVariation", ClampMin = 0.0f, ClampMax = 1.0f))
	float MoveSpeedVariancePct = 0.08f;

	/** Max symmetric additive resist bonus in percent. 5.0 = +/-5% flat resist. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variation",
		meta = (EditCondition = "bEnableBaseStatVariation", ClampMin = 0.0f, ClampMax = 25.0f))
	float ResistVariancePct = 5.0f;


	/**
	 * Rolled rarity tier. Replicated: remote clients need it to tint nameplates
	 * and rarity beams, which is the visual language that makes a rare pack
	 * readable mid-fight.
	 */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MonsterPresentation, Category = "Runtime")
	EMonsterTier AssignedTier = EMonsterTier::MT_Normal;

	/**
	 * Rolled modifier rows. Deliberately NOT replicated - these are full DataTable
	 * rows used only for server-side stat math, and FullDisplayName already carries
	 * everything the client renders. Replicate a compact summary struct instead if
	 * per-mod icons are added later.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Runtime")
	TArray<FMonsterModRow> AppliedMods;

	/**
	 * Per-instance base-stat variation rolled at spawn. Folded into combined getters.
	 * Server-only calculation state; nothing client-side reads it.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Runtime")
	FMonsterStatVariation BaseStatVariation;

	/** Full display name built from base name + prefix/suffix labels. Replicated for nameplates. */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MonsterPresentation, Category = "Runtime")
	FText FullDisplayName;


	/**
	 * Roll tier and mods, then apply them to the owning character's ASC.
	 * Idempotent - safe to call once from BeginPlay.
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
	 * (which triggers BeginPlay -> RollAndApplyMods with the wrong defaults).
	 * Safe to call multiple times.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Modifier")
	void RerollMods();

	/**
	 * Roll the per-instance base-stat variation struct. Idempotent - only rolls
	 * the first time it's called; subsequent calls are no-ops unless the struct
	 * has been reset (e.g. via RerollMods).
	 * Called automatically at the top of RollAndApplyMods, so designers usually
	 * don't need to invoke it directly.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Modifier")
	void RollBaseStatVariation();

	/** Seeded overload used by RollAndApplyMods so the whole roll shares one stream. */
	void RollBaseStatVariation(FRandomStream& Stream);

	/** Returns the total HP multiplier from all applied mods combined. */
	UFUNCTION(BlueprintPure, Category = "Modifier")
	float GetCombinedHPMultiplier() const;

	/** Returns the total damage multiplier from all applied mods combined. */
	UFUNCTION(BlueprintPure, Category = "Modifier")
	float GetCombinedDamageMultiplier() const;


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
	/** Builds the roll stream: seeded from MonsterSeed, or global RNG when unseeded. */
	FRandomStream MakeRollStream() const;

	TArray<FMonsterModRow> RollMods(int32 NumMods, EMonsterTier Tier,
		const UDataTable* Table, FRandomStream& Stream) const;

	/** Apply a single mod row to the owning character's ASC. */
	void ApplyMod(const FMonsterModRow& Mod, UAbilitySystemComponent* ASC);
	void ApplyCombinedStatScaling(UAbilitySystemComponent* ASC);

	void ClearAppliedRuntimeMods(UAbilitySystemComponent* ASC);

	/** Build the full display name from the base name + prefix/suffix labels. */
	FText BuildDisplayName(const TArray<FMonsterModRow>& Mods) const;

	/** Handles for GEs applied by mods - kept so we could remove them if needed */
	UPROPERTY(Transient)
	TArray<FActiveGameplayEffectHandle> AppliedGEHandles;

	UPROPERTY(Transient)
	TArray<FGameplayAbilitySpecHandle> GrantedAbilityHandles;

	UPROPERTY(Transient)
	FGameplayTagContainer GrantedLooseTags;

	UPROPERTY(Transient)
	TArray<FGameplayTagContainer> GrantedLooseTagGrants;

	FActiveGameplayEffectHandle AppliedStatScalingHandle;

	float AppliedMoveSpeedMultiplier = 1.0f;
};
