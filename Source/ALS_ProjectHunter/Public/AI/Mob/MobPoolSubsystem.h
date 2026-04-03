// AI/Mob/MobPoolSubsystem.h
// World subsystem that manages per-class pools of mob actors.
//
// ─── WHY ────────────────────────────────────────────────────────────────────
//   SpawnActor + Destroy is expensive: constructing UObjects, initializing
//   components, running BeginPlay, and then tearing it all down every death
//   creates GC pressure and frame spikes, especially with dozens of mobs.
//
//   The pool keeps "dead" actors hidden and inert.  When the manager needs a
//   new mob, we teleport + reinitialize an existing one instead of spawning
//   from scratch — roughly 10× cheaper on a warm pool.
//
// ─── USAGE ──────────────────────────────────────────────────────────────────
//   // Acquire (creates if pool is empty)
//   APHBaseCharacter* Mob = GetWorld()->GetSubsystem<UMobPoolSubsystem>()
//       ->Acquire(MobClass, SpawnLoc, SpawnRot);
//
//   // Return when dead / no longer needed
//   GetWorld()->GetSubsystem<UMobPoolSubsystem>()->Release(Mob);
//
// ─── NOTES ──────────────────────────────────────────────────────────────────
//   • The subsystem does NOT own the actors — they live in the world like any
//     other AActor.  The pool just keeps weak references.
//   • Release() hides the actor, disables collision/AI, and strips GAS state.
//   • Acquire() re-enables everything and calls a virtual reinit hook.
//   • MaxPoolSizePerClass prevents unbounded memory use from idle actors.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MobPoolSubsystem.generated.h"

class APHBaseCharacter;
class AAIController;
class UAbilitySystemComponent;

DECLARE_LOG_CATEGORY_EXTERN(LogMobPool, Log, All);

UCLASS()
class ALS_PROJECTHUNTER_API UMobPoolSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// ── Configuration ────────────────────────────────────────────────────────

	/**
	 * Maximum number of inactive actors kept per class.
	 * Extras are destroyed immediately on Release().
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pool")
	int32 MaxPoolSizePerClass = 20;

	// ── Public API ───────────────────────────────────────────────────────────

	/**
	 * Get a mob from the pool, or spawn a fresh one if the pool is empty.
	 * The returned actor is hidden with collision/AI disabled — the caller
	 * is responsible for calling FinalizeSpawn / making it visible.
	 *
	 * Returns nullptr on failure (bad class, world shutting down, etc.).
	 */
	UFUNCTION(BlueprintCallable, Category = "Mob Pool")
	APHBaseCharacter* Acquire(TSubclassOf<APHBaseCharacter> MobClass,
		const FVector& Location, const FRotator& Rotation);

	/**
	 * Return a mob to the pool.  Hides it, strips GAS state, disables AI.
	 * If the pool for that class is full, the actor is destroyed instead.
	 */
	UFUNCTION(BlueprintCallable, Category = "Mob Pool")
	void Release(APHBaseCharacter* Mob);

	/**
	 * Destroy all pooled (inactive) actors.  Does NOT touch active mobs.
	 * Call this on level transitions or when you want to reclaim memory.
	 */
	UFUNCTION(BlueprintCallable, Category = "Mob Pool")
	void DrainAllPools();

	/** Number of inactive actors sitting in the pool for a given class. */
	UFUNCTION(BlueprintPure, Category = "Mob Pool")
	int32 GetPooledCount(TSubclassOf<APHBaseCharacter> MobClass) const;

	/** Total number of inactive actors across all classes. */
	UFUNCTION(BlueprintPure, Category = "Mob Pool")
	int32 GetTotalPooledCount() const;

	// ── UWorldSubsystem interface ────────────────────────────────────────────
	virtual void Deinitialize() override;

protected:
	/**
	 * Reset a mob's runtime state so it behaves like a freshly spawned actor.
	 * Clears GAS effects/tags, resets attributes to defaults, removes modifiers,
	 * resets death state, etc.
	 */
	void ResetMobState(APHBaseCharacter* Mob) const;

	/**
	 * Deactivate a mob: hide, disable collision, pause AI, stop movement.
	 * Does NOT reset state — call ResetMobState separately if needed.
	 */
	void DeactivateMob(APHBaseCharacter* Mob) const;

	/**
	 * Teleport a pooled mob to a new location and prepare it for reuse.
	 * The mob is still hidden after this — caller must finalize.
	 */
	void PrepareMobForReuse(APHBaseCharacter* Mob,
		const FVector& Location, const FRotator& Rotation) const;

private:
	/** Per-class pool of inactive actors */
	TMap<UClass*, TArray<TWeakObjectPtr<APHBaseCharacter>>> Pool;
};
