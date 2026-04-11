// AI/Mob/MobManagerActor.h
// Placeable actor that manages a pool of enemies within a defined box area.
//
// ─── HOW TO USE ──────────────────────────────────────────────────────────────
//   1. Drag BP_MobManager (child Blueprint) into the level.
//   2. Set MobTypes array: add each enemy class + weight + optional cooldown.
//   3. Resize the BoxComponent (SpawnArea) to cover the area you want.
//   4. Ensure a NavMeshBoundsVolume covers the same region.
//   5. Play — the manager ticks on SpawnInterval, filling up to MaxNumOfMobs.
//
// ─── SPAWN PIPELINE ──────────────────────────────────────────────────────────
//   SpawnTick → CleanActiveMobs → population check
//     → TrySpawnMob → weighted class selection
//       → loop MaxSpawnAttempts:
//           GetRandomSpawnLocation → NavMesh project → ground trace
//           → HiddenSpawn (actor created hidden, no collision, AI off)
//           → ValidateSpawnPoint (collision, distance, ground checks)
//           → FinalizeSpawn  OR  destroy & retry
//
// ─── AI WANDER INTEGRATION ───────────────────────────────────────────────────
//   Spawned mobs receive HomeLocation + WanderRadius via IMobWanderable.
//   The Blueprint AI Controller reads those values to pick move targets.
//   No C++ AI is required — set it all up in Blueprint.
//
// ─── MONSTER MODIFIER INTEGRATION ───────────────────────────────────────────
//   If the spawned actor has a UMonsterModifierComponent, the manager sets
//   AreaLevel and optionally ForcedTier before calling RollAndApplyMods.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AI/Mob/MobSpawnTypes.h"
#include "AI/Mob/MobSpawnRules.h"
#include "Data/MonsterModifierData.h"
#include "MobManagerActor.generated.h"

class UBoxComponent;
class APHBaseCharacter;
class UMonsterModifierComponent;
class UMobPoolSubsystem;
class UNavigationSystemV1;

// Log category declared here, defined once in the .cpp.
// The old DEFINE_LOG_CATEGORY_STATIC in a header would create a separate
// static category per translation unit — breaking shared verbosity settings
// the moment a second file includes this header.
DECLARE_LOG_CATEGORY_EXTERN(LogMobManager, Log, All);

// ─────────────────────────────────────────────────────────────────────────────
// Delegates
// ─────────────────────────────────────────────────────────────────────────────

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMobSpawned,  APHBaseCharacter*, Mob);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMobDied,     APHBaseCharacter*, Mob);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSpawnFailed, EMobSpawnFailReason, Reason);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnManagerFull);

/** Fired when a special spawn rule triggers and its action is executed. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSpecialSpawnExecuted,
	FName, RuleId, APHBaseCharacter*, SpawnedMob);

// ─────────────────────────────────────────────────────────────────────────────
UCLASS()
class ALS_PROJECTHUNTER_API AMobManagerActor : public AActor
{
	GENERATED_BODY()

public:
	AMobManagerActor();

	virtual void BeginPlay()  override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	// ── Components ────────────────────────────────────────────────────────────

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* SceneRoot;

	/**
	 * Defines the area where mobs can spawn.
	 * Resize this box in the editor to cover your spawn region.
	 * The NavMeshBoundsVolume must overlap this box for spawns to succeed.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBoxComponent* SpawnArea;

	// ─────────────────────────────────────────────────────────────────────────
	// ═══ SPAWN CONFIGURATION ═════════════════════════════════════════════════
	// ─────────────────────────────────────────────────────────────────────────

	/**
	 * Enemy types this manager can spawn.
	 * Each entry has a class, a relative spawn weight, and an optional per-type
	 * cooldown.  Higher weight = spawned more often.
	 *
	 * Example:
	 *   [0] Goblin   weight=100  cooldown=0
	 *   [1] Wolf     weight=50   cooldown=5
	 *   [2] Skeleton weight=25   cooldown=0
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mob Manager|Spawn")
	TArray<FMobTypeEntry> MobTypes;

	/** Hard cap on simultaneously alive mobs for this manager. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mob Manager|Spawn",
		meta = (ClampMin = 1))
	int32 MaxNumOfMobs = 10;

	/** Seconds between spawn-tick evaluations. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mob Manager|Spawn",
		meta = (ClampMin = 0.1f))
	float SpawnInterval = 5.0f;

	/**
	 * Max retries per tick when a candidate location is invalid.
	 * With smart placement enabled, 20 is usually more than enough.
	 * Without it, increase to 30–50 if you see frequent exhaustion.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mob Manager|Spawn",
		meta = (ClampMin = 1, ClampMax = 100))
	int32 MaxSpawnAttempts = 20;

	/**
	 * OPT-BUDGET: Maximum milliseconds the spawn validation loop is allowed
	 * to run per SpawnTick.  If exceeded, the remaining attempts are abandoned
	 * for this tick and retried next interval.  Prevents large spawn areas
	 * with many retries from causing frame spikes.
	 * Set to 0 to disable the budget (old behaviour).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mob Manager|Spawn",
		meta = (ClampMin = 0.0f, ClampMax = 16.0f))
	float SpawnBudgetMs = 2.0f;

	/** Start spawning automatically on BeginPlay. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mob Manager|Spawn")
	bool bAutoActivate = true;

	/**
	 * OPT-POOL: Use the MobPoolSubsystem to recycle dead actors instead of
	 * calling SpawnActor/Destroy each time.  Roughly 10× cheaper on a warm pool.
	 * Disable if you need unique per-spawn construction logic that can't be reset.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mob Manager|Spawn")
	bool bUseActorPooling = true;

	// ─────────────────────────────────────────────────────────────────────────
	// ═══ SPAWN RULES ═════════════════════════════════════════════════════════
	// ─────────────────────────────────────────────────────────────────────────

	/**
	 * Mobs will not spawn within this distance of any player.
	 * Set to 0 to disable the check.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mob Manager|Spawn Rules",
		meta = (ClampMin = 0.0f))
	float MinDistanceFromPlayer = 1000.0f;

	/**
	 * Mobs will not spawn farther than this distance from the nearest player.
	 * Creates a "spawn ring" / donut between Min and Max distance.
	 * Set to 0 to disable the max-distance check (spawns anywhere in the box).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mob Manager|Spawn Rules",
		meta = (ClampMin = 0.0f))
	float MaxDistanceFromPlayer = 0.0f;

	/**
	 * When true, spawn location selection tries to bias candidate points
	 * away from player positions rather than choosing purely random points.
	 * Dramatically reduces DistanceFailed attempts when the player is
	 * inside or near the spawn box.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mob Manager|Spawn Rules")
	bool bUseSmartSpawnPlacement = true;

	/** Project the candidate point onto the NavMesh before spawning. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mob Manager|Spawn Rules")
	bool bUseNavCheck = true;

	/**
	 * Horizontal search radius for the NavMesh projection.
	 * Increase for areas with coarse nav geometry.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mob Manager|Spawn Rules",
		meta = (EditCondition = "bUseNavCheck", ClampMin = 50.0f))
	float NavProjectionExtent = 200.0f;

	/**
	 * Vertical search distance for the NavMesh projection.
	 * Must be large enough to reach the NavMesh from the top of the SpawnArea box.
	 * The NavMesh lives on the ground surface, so if your SpawnArea extends 200+
	 * units above ground, this needs to be at least that tall.
	 * Defaults to 1000 — generous enough for most layouts.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mob Manager|Spawn Rules",
		meta = (EditCondition = "bUseNavCheck", ClampMin = 50.0f))
	float NavProjectionVerticalExtent = 1000.0f;

	/** Run a capsule overlap to ensure the spot is clear of obstacles. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mob Manager|Spawn Rules")
	bool bUseCollisionCheck = true;

	/** Capsule half-height used for the overlap test. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mob Manager|Spawn Rules",
		meta = (EditCondition = "bUseCollisionCheck", ClampMin = 10.0f))
	float CollisionCapsuleHalfHeight = 88.0f;

	/** Capsule radius used for the overlap test. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mob Manager|Spawn Rules",
		meta = (EditCondition = "bUseCollisionCheck", ClampMin = 5.0f))
	float CollisionCapsuleRadius = 34.0f;

	/**
	 * Fire a downward line trace to find the ground.
	 * Adjusts the spawn Z to land exactly on the surface.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mob Manager|Spawn Rules")
	bool bUseGroundCheck = true;

	/** Max distance the ground trace searches downward. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mob Manager|Spawn Rules",
		meta = (EditCondition = "bUseGroundCheck", ClampMin = 50.0f))
	float GroundTraceDistance = 500.0f;

	// ─────────────────────────────────────────────────────────────────────────
	// ═══ AI / WANDER ══════════════════════════════════════════════════════════
	// ─────────────────────────────────────────────────────────────────────────

	/**
	 * Default wander radius passed to each mob via IMobWanderable::SetWanderRadius.
	 * The BP AI Controller uses this to pick points within radius of HomeLocation.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mob Manager|AI",
		meta = (ClampMin = 100.0f))
	float WanderRadius = 1200.0f;

	// ─────────────────────────────────────────────────────────────────────────
	// ═══ MONSTER MODIFIER INTEGRATION ════════════════════════════════════════
	// ─────────────────────────────────────────────────────────────────────────

	/**
	 * Area level passed to UMonsterModifierComponent (if the spawned mob has one).
	 * Controls which modifier tiers are eligible for this zone.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mob Manager|Modifiers",
		meta = (ClampMin = 1))
	int32 AreaLevel = 1;

	/**
	 * Magic Find value contributed by nearby players, forwarded to the modifier
	 * component to influence rare/magic tier chances.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mob Manager|Modifiers",
		meta = (ClampMin = 0.0f))
	float NearbyMagicFind = 0.0f;

	// ─────────────────────────────────────────────────────────────────────────
	// ═══ SPECIAL / EVENT SPAWN RULES ═════════════════════════════════════════
	// ─────────────────────────────────────────────────────────────────────────

	/**
	 * Designer-authored rules that trigger special spawns based on gameplay state
	 * (player holds a key item, total kills of a mob class reached a threshold, …).
	 *
	 * Rules are evaluated at the top of every SpawnTick. When a rule is ready,
	 * its action runs (force-spawn a specific mob, or boost the tier of the next
	 * normal spawn). See UMobSpawnConditionEvaluator for evaluation logic.
	 *
	 * Runtime state on the rule rows (bHasFired, LastFireTime) is Transient and
	 * resets every time this actor is constructed.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mob Manager|Special Spawns")
	TArray<FMobSpecialSpawnRule> SpecialSpawnRules;

	// ─────────────────────────────────────────────────────────────────────────
	// ═══ DEBUG ════════════════════════════════════════════════════════════════
	// ─────────────────────────────────────────────────────────────────────────

	/** Draw spawn-attempt spheres and the SpawnArea box each frame (editor+dev). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mob Manager|Debug")
	bool bShowDebug = false;

	/** How many debug entries to keep in history (avoids unbounded growth). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mob Manager|Debug",
		meta = (ClampMin = 1, ClampMax = 200))
	int32 MaxDebugHistory = 50;

	/** How many seconds a debug sphere stays visible. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mob Manager|Debug",
		meta = (ClampMin = 0.1f))
	float DebugDrawDuration = 8.0f;

	// ─────────────────────────────────────────────────────────────────────────
	// ═══ POOL RECYCLE ═══════════════════════════════════════════════════════
	// ─────────────────────────────────────────────────────────────────────────

	/**
	 * Seconds to wait after a mob dies before returning it to the pool.
	 * Gives death animations, loot drops, and VFX time to finish.
	 * Set higher for bosses with long death sequences.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mob Manager|Spawn",
		meta = (ClampMin = 0.0f, EditCondition = "bUseActorPooling"))
	float PoolRecycleDelay = 5.0f;

	/**
	 * Maximum number of individual mobs to spawn per tick (non-pack mode).
	 *
	 * NON-PACK MODE: hard cap on mobs spawned each SpawnTick. Set to 1 for
	 * the slowest ramp-up, higher to repopulate faster after a mass wipe.
	 *
	 * PACK MODE (bSpawnInPacks = true): this value caps the number of PACKS
	 * spawned per tick, NOT individual mobs.  Each pack can contain up to
	 * PackSizeMax members, so the actual mob budget is roughly:
	 *   MaxSpawnsPerTick * PackSizeMax
	 * Keep this at 1 in pack mode unless you explicitly want multiple packs
	 * to drop in the same tick.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mob Manager|Spawn",
		meta = (ClampMin = 1, ClampMax = 10))
	int32 MaxSpawnsPerTick = 1;

	// ─────────────────────────────────────────────────────────────────────────
	// ═══ BATCH / PACK SPAWNING ═══════════════════════════════════════════════
	// ─────────────────────────────────────────────────────────────────────────

	/**
	 * When true, mobs spawn in packs/batches — a group of N mobs at nearby
	 * locations — instead of one at a time.  The spawner finds one valid
	 * "leader" position, then spawns PackSize mobs in a cluster around it.
	 * Much more natural-looking than one mob popping in every 5 seconds.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mob Manager|Batch Spawn")
	bool bSpawnInPacks = false;

	/** Minimum mobs per pack.  Actual count is randomized [Min, Max]. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mob Manager|Batch Spawn",
		meta = (ClampMin = 1, ClampMax = 20, EditCondition = "bSpawnInPacks"))
	int32 PackSizeMin = 3;

	/** Maximum mobs per pack. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mob Manager|Batch Spawn",
		meta = (ClampMin = 1, ClampMax = 20, EditCondition = "bSpawnInPacks"))
	int32 PackSizeMax = 6;

	/**
	 * How far (in units) pack members can spawn from the leader position.
	 * Smaller = tight cluster, larger = loose group.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mob Manager|Batch Spawn",
		meta = (ClampMin = 50.0f, EditCondition = "bSpawnInPacks"))
	float PackSpreadRadius = 400.0f;

	/**
	 * Maximum attempts to find a valid position for each follower in a pack.
	 * Lower than MaxSpawnAttempts since the leader location is already validated.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mob Manager|Batch Spawn",
		meta = (ClampMin = 1, ClampMax = 20, EditCondition = "bSpawnInPacks"))
	int32 PackFollowerAttempts = 8;

	/**
	 * When true, the manager spawns an initial full burst of mobs on
	 * StartSpawning, filling up to MaxNumOfMobs immediately rather than
	 * waiting for the spawn timer to ramp up one at a time.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mob Manager|Spawn")
	bool bInitialBurst = false;

	// ─────────────────────────────────────────────────────────────────────────
	// ═══ RUNTIME STATE ════════════════════════════════════════════════════════
	// ─────────────────────────────────────────────────────────────────────────

	/** Current manager state. */
	UPROPERTY(BlueprintReadOnly, Category = "Mob Manager|State")
	EMobManagerState ManagerState = EMobManagerState::Disabled;

	/**
	 * Weak pointers to every currently alive mob owned by this manager.
	 * TWeakObjectPtr is not Blueprint-compatible — use GetActiveMobsArray()
	 * for Blueprint access.
	 */
	UPROPERTY()
	TArray<TWeakObjectPtr<APHBaseCharacter>> ActiveMobs;

	// ─────────────────────────────────────────────────────────────────────────
	// ═══ EVENTS ═══════════════════════════════════════════════════════════════
	// ─────────────────────────────────────────────────────────────────────────

	/** Fired when a mob is successfully spawned and made visible. */
	UPROPERTY(BlueprintAssignable, Category = "Mob Manager|Events")
	FOnMobSpawned OnMobSpawned;

	/** Fired when a tracked mob dies / is destroyed. */
	UPROPERTY(BlueprintAssignable, Category = "Mob Manager|Events")
	FOnMobDied OnMobDied;

	/** Fired when a spawn attempt exhausts all retries. */
	UPROPERTY(BlueprintAssignable, Category = "Mob Manager|Events")
	FOnSpawnFailed OnSpawnFailed;

	/** Fired when ActiveMobs hits MaxNumOfMobs (once, resets when a slot opens). */
	UPROPERTY(BlueprintAssignable, Category = "Mob Manager|Events")
	FOnManagerFull OnManagerFull;

	/** Fired whenever a special spawn rule successfully executes its action. */
	UPROPERTY(BlueprintAssignable, Category = "Mob Manager|Events")
	FOnSpecialSpawnExecuted OnSpecialSpawnExecuted;

	// ─────────────────────────────────────────────────────────────────────────
	// ═══ PUBLIC API ═══════════════════════════════════════════════════════════
	// ─────────────────────────────────────────────────────────────────────────

	/** Start or resume the spawn timer. */
	UFUNCTION(BlueprintCallable, Category = "Mob Manager")
	void StartSpawning();

	/** Pause the spawn timer without clearing active mobs. */
	UFUNCTION(BlueprintCallable, Category = "Mob Manager")
	void PauseSpawning();

	/** Stop the spawn timer AND destroy all active mobs. */
	UFUNCTION(BlueprintCallable, Category = "Mob Manager")
	void StopAndClear();

	/** Force one spawn attempt right now, ignoring the timer. */
	UFUNCTION(BlueprintCallable, Category = "Mob Manager")
	void ForceSpawnOne();

	/**
	 * Force a full batch/pack spawn right now, ignoring the timer.
	 * Spawns PackSize mobs in a cluster.  Falls back to single spawn
	 * if bSpawnInPacks is false.
	 */
	UFUNCTION(BlueprintCallable, Category = "Mob Manager")
	void ForceSpawnBatch();

	/** Number of valid (alive) mobs currently tracked. */
	UFUNCTION(BlueprintPure, Category = "Mob Manager")
	int32 GetActiveCount() const;

	/**
	 * Returns a plain array of live mob pointers — safe for Blueprint iteration.
	 * Stale (destroyed) entries are filtered out automatically.
	 */
	UFUNCTION(BlueprintPure, Category = "Mob Manager")
	TArray<APHBaseCharacter*> GetActiveMobsArray() const;

	/** True if the manager is at max population. */
	UFUNCTION(BlueprintPure, Category = "Mob Manager")
	bool IsFull() const { return GetActiveCount() >= MaxNumOfMobs; }

	/** Clear all stale (destroyed/invalid) pointers from ActiveMobs. */
	UFUNCTION(BlueprintCallable, Category = "Mob Manager")
	void CleanActiveMobs();

	// ─────────────────────────────────────────────────────────────────────────
	// ═══ SPECIAL SPAWN / KILL-COUNT API ══════════════════════════════════════
	// ─────────────────────────────────────────────────────────────────────────

	/**
	 * BlueprintNativeEvent — returns true if ANY tracked player currently has
	 * the item identified by KeyItemId in their inventory.
	 *
	 * The C++ default returns false so rule systems can't spuriously trigger
	 * on a manager that doesn't know about an inventory system. Override this
	 * in the child BP to query your InventoryManager / InventoryComponent.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mob Manager|Special Spawns")
	bool DoesAnyPlayerHaveKeyItem(FName KeyItemId);
	virtual bool DoesAnyPlayerHaveKeyItem_Implementation(FName KeyItemId);

	/**
	 * Force-run the action for the rule with the given ID (bypasses trigger checks,
	 * still respects bEnabled and lifetime/cooldown state).
	 * Returns true if the action ran successfully.
	 */
	UFUNCTION(BlueprintCallable, Category = "Mob Manager|Special Spawns")
	bool ForceSpawnSpecial(FName RuleId);

	/**
	 * Number of kills tracked for the given class (or any of its subclasses).
	 * Pass null to get the total across every class.
	 */
	UFUNCTION(BlueprintPure, Category = "Mob Manager|Special Spawns")
	int32 GetKillCountForClass(TSubclassOf<APHBaseCharacter> MobClass) const;

	/** Total kills tracked by this manager, ignoring class. */
	UFUNCTION(BlueprintPure, Category = "Mob Manager|Special Spawns")
	int32 GetTotalKillCount() const;

	/** Reset all tracked kill counts to zero (does not reset rule lifetime state). */
	UFUNCTION(BlueprintCallable, Category = "Mob Manager|Special Spawns")
	void ResetKillCounts();

	/** Read-only access to the debug history (for custom visualisation in BP). */
	UFUNCTION(BlueprintPure, Category = "Mob Manager|Debug")
	const TArray<FSpawnAttemptDebug>& GetDebugHistory() const { return DebugHistory; }

	/** Clear the debug history. */
	UFUNCTION(BlueprintCallable, Category = "Mob Manager|Debug")
	void ClearDebugHistory() { DebugHistory.Empty(); DebugHistoryIndex = 0; }

protected:
	// ─────────────────────────────────────────────────────────────────────────
	// Blueprint events — override in child Blueprint
	// ─────────────────────────────────────────────────────────────────────────

	/** Called after a mob is fully spawned and made visible. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Mob Manager|Events")
	void BP_OnMobSpawned(APHBaseCharacter* Mob);

	/** Called when a tracked mob is destroyed. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Mob Manager|Events")
	void BP_OnMobDied(APHBaseCharacter* Mob);

	/** Called each time TrySpawnMob() fails all retries. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Mob Manager|Events")
	void BP_OnSpawnFailed(EMobSpawnFailReason Reason);

	// ─────────────────────────────────────────────────────────────────────────
	// Core spawn pipeline (virtual so Blueprint-child C++ subclasses can extend)
	// ─────────────────────────────────────────────────────────────────────────

	/** Spawn-timer callback — runs every SpawnInterval seconds. */
	virtual void SpawnTick();

	/**
	 * Single mob spawn attempt.
	 * Returns true on success, false if all MaxSpawnAttempts are exhausted.
	 */
	virtual bool TrySpawnMob();

	/**
	 * Generate a candidate world position inside SpawnArea.
	 * Applies NavMesh projection and ground trace if enabled.
	 * Returns false if no valid position could be found.
	 */
	virtual bool GetRandomSpawnLocation(FVector& OutLocation);

	/**
	 * Spawn a pack of mobs — finds one valid leader location, then spawns
	 * PackSize mobs in a cluster around it.  Returns number of mobs spawned.
	 */
	virtual int32 SpawnBatch();

	/**
	 * Try to spawn a single mob at a specific location (with nearby jitter).
	 * Used by SpawnBatch for pack follower placement.
	 * Returns the spawned actor, or nullptr on failure.
	 */
	APHBaseCharacter* TrySpawnAtLocation(TSubclassOf<APHBaseCharacter> MobClass,
		const FVector& Center, float JitterRadius, int32 Attempts);

	/** Cache per-tick derived values (nav system, box transform, distances). */
	void CacheTickValues();

	// ── Location validation helpers ───────────────────────────────────────────

	/** NavMesh projection. Returns false if the point can't be projected. */
	bool CheckNavMesh(FVector Candidate, FVector& OutProjected) const;

	/**
	 * Capsule overlap — returns false if the capsule at Location overlaps
	 * WorldStatic, WorldDynamic, or Pawn channels.
	 * Optionally pass an actor to ignore (the hidden test actor).
	 */
	bool CheckCollision(FVector Location, AActor* IgnoreActor = nullptr) const;

	/**
	 * Distance check — returns false if too close to ANY player (MinDist)
	 * or too far from ALL players (MaxDist).  In multiplayer, a spawn only
	 * needs to be within MaxDistance of at least one player.
	 * Uses CachedPlayerLocations gathered once per SpawnTick for performance.
	 */
	bool CheckDistanceFromPlayers(FVector Location) const;

	/**
	 * Cache player-controlled pawn positions once per SpawnTick.
	 * Avoids the expensive GetAllActorsOfClass(APawn) call on every attempt.
	 */
	void CachePlayerLocations();

	/**
	 * Generate a spawn candidate that is biased toward the valid distance ring
	 * around players (between MinDistanceFromPlayer and MaxDistanceFromPlayer).
	 * Falls back to pure random if no players are cached or smart placement is off.
	 */
	bool GetSmartSpawnLocation(FVector& OutLocation);

	/**
	 * Downward line trace.  Adjusts OutGroundLocation to the surface hit point.
	 * Returns false if nothing is hit within GroundTraceDistance.
	 */
	bool CheckGround(FVector Candidate, FVector& OutGroundLocation) const;

	// ── Spawn lifecycle ───────────────────────────────────────────────────────

	/**
	 * Spawn the character hidden with no collision and no AI.
	 * Returns nullptr on failure.
	 */
	APHBaseCharacter* HiddenSpawn(TSubclassOf<APHBaseCharacter> Class,
		const FVector& Location, const FRotator& Rotation) const;

	/**
	 * Unhide the actor, restore collision, start AI, notify interfaces,
	 * connect death delegate, add to ActiveMobs.
	 */
	void FinalizeSpawn(APHBaseCharacter* Mob, const FVector& SpawnLocation);

	/**
	 * Configure UMonsterModifierComponent if the mob has one.
	 * Called during FinalizeSpawn.
	 *
	 * Not const — consumes PendingForcedTier if a special rule has queued a
	 * tier boost for the next spawn.
	 */
	void ApplyModifierComponent(APHBaseCharacter* Mob);

	/**
	 * Walks SpecialSpawnRules, firing any whose triggers are satisfied.
	 * Called at the top of SpawnTick, after CacheTickValues but before the
	 * capacity check so force-spawns can slot in even near the cap.
	 */
	virtual void EvaluateSpecialSpawnRules();

	// ── Death tracking ────────────────────────────────────────────────────────

	/**
	 * Bound to APHBaseCharacter::OnDeathEvent for every spawned mob.
	 * Removes the mob from ActiveMobs and fires delegates.
	 */
	UFUNCTION()
	void OnMobDeathEvent(APHBaseCharacter* DeadMob, AActor* Killer);

	// ── Weighted random selection ─────────────────────────────────────────────

	/**
	 * Pick a random eligible mob class using the weight table.
	 * Returns INDEX_NONE if nothing is eligible.
	 */
	int32 GetWeightedRandomMobTypeIndex() const;

	// ── Debug ─────────────────────────────────────────────────────────────────

	void RecordDebugAttempt(const FVector& Location, bool bSuccess,
		EMobSpawnFailReason Reason);

	void DrawDebugVisuals() const;

private:
	FTimerHandle SpawnTimerHandle;

	/**
	 * Handle for the one-shot InitialBurst timer so EndPlay() can cancel it
	 * and prevent a dangling-this access if the manager is destroyed quickly
	 * after BeginPlay (e.g. during a fast level teardown in PIE).
	 */
	FTimerHandle InitialBurstTimerHandle;

	/** Whether we already fired OnManagerFull this cycle. */
	bool bFullEventFired = false;

	/** OPT-POOL: Cached reference to the world pool subsystem. */
	UPROPERTY(Transient)
	TObjectPtr<UMobPoolSubsystem> CachedPoolSubsystem;

	/** Rolling debug history (circular buffer). */
	TArray<FSpawnAttemptDebug> DebugHistory;

	/** Write cursor for the circular debug history. */
	int32 DebugHistoryIndex = 0;

	/**
	 * Player positions cached once per SpawnTick to avoid calling
	 * GetAllActorsOfClass(APawn) on every single spawn attempt.
	 */
	TArray<FVector> CachedPlayerLocations;

	/** OPT: Nav system pointer cached once per tick. */
	TWeakObjectPtr<UNavigationSystemV1> CachedNavSystem;

	/** OPT: Box transform + extents cached once per tick. */
	FTransform CachedBoxXform;
	FVector CachedBoxExtent;

	/** Squared min distance, precomputed each tick. */
	float MinDistSq = 0.0f;

	/** Squared max distance, precomputed each tick. 0 = disabled. */
	float MaxDistSq = 0.0f;

	/**
	 * Pending pool-recycle timers for mobs that died but haven't been returned yet.
	 * Tracked so StopAndClear() and EndPlay() can cancel them, preventing
	 * a Release() call on an already-destroyed actor.
	 */
	TArray<FTimerHandle> PendingRecycleTimers;

	/**
	 * Tier queued by a special spawn rule (Action = BoostNextSpawnTier) or set
	 * internally before a force-spawn. Consumed and reset to MT_Normal the next
	 * time ApplyModifierComponent runs.
	 */
	EMonsterTier PendingForcedTier = EMonsterTier::MT_Normal;

	/** Kill counters grouped by exact spawned class. Summed via IsChildOf lookups. */
	TMap<TSubclassOf<APHBaseCharacter>, int32> KillsByClass;

	/** Running total of every kill this manager has recorded. */
	int32 TotalKills = 0;
};
