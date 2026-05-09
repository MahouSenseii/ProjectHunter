// Character/Component/CombatStatusManager.h
// Convenience layer over GAS for applying Damage-over-Time effects.
//
// SETUP — Required GE assets (create in Content/GAS/Effects/DoT/):
//   GE_DoT_Bleed   — HasDuration, Periodic tick, SetByCaller tag "DoT.Bleed.DamagePerTick"
//   GE_DoT_Ignite  — HasDuration, Periodic tick, SetByCaller tag "DoT.Ignite.DamagePerTick"
//   GE_DoT_Poison  — HasDuration, Periodic tick, SetByCaller tag "DoT.Poison.DamagePerTick"
//                    Stacks: add duration (PoE-style)
//   GE_DoT_Chill   — HasDuration, no tick, applies movement speed reduction
//                    SetByCaller tag "DoT.Chill.Magnitude" (0.0 – 1.0 slow fraction)
//   GE_DoT_Freeze  — HasDuration, removes control (root/stun GE), no tick
//   GE_DoT_Shock   — HasDuration, no tick, amplifies incoming damage
//                    SetByCaller tag "DoT.Shock.Magnitude" (0.0 – 0.5 amp fraction)
//
// STACKING:
//   Bleed  — StackingType = AggregateBySource (each hit = separate stack, PoE1-style)
//   Ignite — StackingType = AggregateBySource, Duration refresh on new apply
//   Poison — StackingType = AggregateByTarget (all poisons share a stack counter)
//   Chill  — StackingType = None (highest magnitude wins; handled by GE)
//   Freeze — StackingType = None (refreshes duration)
//   Shock  — StackingType = None (highest magnitude wins)
//
// CONDITION TAGS:
//   Each GE asset should grant the corresponding Condition_Self_* tag from
//   FPHGameplayTags so the HUD, behaviour tree and code conditions work
//   automatically (e.g., Condition_Self_Bleeding, Condition_Self_Burned).
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayEffectTypes.h"
#include "Combat/Library/CombatStructs.h"
#include "CombatStatusManager.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;

DECLARE_LOG_CATEGORY_EXTERN(LogCombatStatusManager, Log, All);

// ─────────────────────────────────────────────────────────────────────────────
// SetByCaller tag names — match the tag names in your GE assets exactly
// ─────────────────────────────────────────────────────────────────────────────
namespace CombatStatusSetByCallerTags
{
	const FName Bleed_DamagePerTick      = TEXT("DoT.Bleed.DamagePerTick");
	const FName Ignite_DamagePerTick     = TEXT("DoT.Ignite.DamagePerTick");
	const FName Poison_DamagePerTick     = TEXT("DoT.Poison.DamagePerTick");
	const FName Corruption_DamagePerTick = TEXT("DoT.Corruption.DamagePerTick");
	const FName Chill_SlowFraction       = TEXT("DoT.Chill.Magnitude");
	const FName Shock_AmpFraction        = TEXT("DoT.Shock.Magnitude");
}

// FCombatStatusApplyResult is defined in Combat/Library/CombatStructs.h

// ─────────────────────────────────────────────────────────────────────────────
UCLASS(ClassGroup=(ProjectHunter), meta=(BlueprintSpawnableComponent))
class ALS_PROJECTHUNTER_API UCombatStatusManager : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatStatusManager();

	// ── GE class references — assign in Blueprint defaults ────────────────────

	/** GE asset for Bleed (physical DoT). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat Status|Effects")
	TSubclassOf<UGameplayEffect> BleedEffectClass;

	/** GE asset for Ignite (fire DoT). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat Status|Effects")
	TSubclassOf<UGameplayEffect> IgniteEffectClass;

	/** GE asset for Poison (chaos DoT, stacks). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat Status|Effects")
	TSubclassOf<UGameplayEffect> PoisonEffectClass;

	/** GE asset for Corruption (dark DoT). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat Status|Effects")
	TSubclassOf<UGameplayEffect> CorruptionEffectClass;

	/** GE asset for Chill (cold slow, no damage). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat Status|Effects")
	TSubclassOf<UGameplayEffect> ChillEffectClass;

	/** GE asset for Freeze (cold CC, no damage). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat Status|Effects")
	TSubclassOf<UGameplayEffect> FreezeEffectClass;

	/** GE asset for Petrify (light CC, no damage). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat Status|Effects")
	TSubclassOf<UGameplayEffect> PetrifyEffectClass;

	/** GE asset for Shock (lightning amp, no damage). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat Status|Effects")
	TSubclassOf<UGameplayEffect> ShockEffectClass;

	// ── Apply functions — called by hit-processing code ───────────────────────

	/**
	 * Apply a Bleed stack to a target.
	 * @param Target          Actor to apply the bleed to (must have ASC).
	 * @param DamagePerTick   Physical damage dealt each tick.
	 * @param Duration        Duration in seconds.
	 * @param Instigator      Who caused the bleed (used for source-aggregated stacking).
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat Status")
	FCombatStatusApplyResult ApplyBleed(AActor* Target, float DamagePerTick,
		float Duration, AActor* Instigator = nullptr);

	/**
	 * Apply an Ignite stack to a target.
	 * @param DamagePerTick   Fire damage dealt each tick.
	 * @param Duration        Duration in seconds.
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat Status")
	FCombatStatusApplyResult ApplyIgnite(AActor* Target, float DamagePerTick,
		float Duration, AActor* Instigator = nullptr);

	/**
	 * Apply a Poison stack to a target.
	 * Poison stacks by target — each new application adds a stack.
	 * @param DamagePerTick   Chaos damage per tick per stack.
	 * @param Duration        Duration in seconds.
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat Status")
	FCombatStatusApplyResult ApplyPoison(AActor* Target, float DamagePerTick,
		float Duration, AActor* Instigator = nullptr);

	/**
	 * Apply Corruption to a target.
	 * @param DamagePerTick   Corruption damage dealt each tick.
	 * @param Duration        Duration in seconds.
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat Status")
	FCombatStatusApplyResult ApplyCorruption(AActor* Target, float DamagePerTick,
		float Duration, AActor* Instigator = nullptr);

	/**
	 * Apply a Chill to a target (movement slow, no damage).
	 * @param SlowFraction    0.0 = no slow, 1.0 = full stop.  Clamped 0–0.7.
	 * @param Duration        Duration in seconds.
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat Status")
	FCombatStatusApplyResult ApplyChill(AActor* Target, float SlowFraction,
		float Duration, AActor* Instigator = nullptr);

	/**
	 * Apply a Freeze to a target (CC, no damage).
	 * @param Duration   Freeze duration in seconds.
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat Status")
	FCombatStatusApplyResult ApplyFreeze(AActor* Target, float Duration,
		AActor* Instigator = nullptr);

	/**
	 * Apply Petrify to a target (CC, no damage).
	 * @param Duration   Petrify duration in seconds.
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat Status")
	FCombatStatusApplyResult ApplyPetrify(AActor* Target, float Duration,
		AActor* Instigator = nullptr);

	/**
	 * Apply a Shock to a target (amplifies incoming damage, no DoT).
	 * @param AmpFraction   0.0 = no amp, 0.5 = +50% incoming damage.  Clamped 0–0.5.
	 * @param Duration      Duration in seconds.
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat Status")
	FCombatStatusApplyResult ApplyShock(AActor* Target, float AmpFraction,
		float Duration, AActor* Instigator = nullptr);

	// ── Query ─────────────────────────────────────────────────────────────────

	/** Returns true if the target currently has an active Bleed GE. */
	UFUNCTION(BlueprintPure, Category = "Combat Status")
	bool IsBleeding(AActor* Target) const;

	/** Returns true if the target currently has an active Ignite GE. */
	UFUNCTION(BlueprintPure, Category = "Combat Status")
	bool IsIgnited(AActor* Target) const;

	/** Returns the current Poison stack count on the target (0 if none). */
	UFUNCTION(BlueprintPure, Category = "Combat Status")
	int32 GetPoisonStacks(AActor* Target) const;

	/** Returns true if the target currently has an active Corruption GE. */
	UFUNCTION(BlueprintPure, Category = "Combat Status")
	bool IsCorrupted(AActor* Target) const;

	/** Returns true if the target currently has an active Chill GE. */
	UFUNCTION(BlueprintPure, Category = "Combat Status")
	bool IsChilled(AActor* Target) const;

	/** Returns true if the target currently has an active Freeze GE. */
	UFUNCTION(BlueprintPure, Category = "Combat Status")
	bool IsFrozen(AActor* Target) const;

	/** Returns true if the target currently has an active Petrify GE. */
	UFUNCTION(BlueprintPure, Category = "Combat Status")
	bool IsPetrified(AActor* Target) const;

	/** Returns true if the target currently has an active Shock GE. */
	UFUNCTION(BlueprintPure, Category = "Combat Status")
	bool IsShocked(AActor* Target) const;

	// ── Removal ───────────────────────────────────────────────────────────────

	/** Remove all active Bleed stacks from the target. */
	UFUNCTION(BlueprintCallable, Category = "Combat Status")
	void CureBleed(AActor* Target);

	/** Remove all active Ignite stacks from the target. */
	UFUNCTION(BlueprintCallable, Category = "Combat Status")
	void CureIgnite(AActor* Target);

	/** Remove all active Poison stacks from the target. */
	UFUNCTION(BlueprintCallable, Category = "Combat Status")
	void CurePoison(AActor* Target);

	/** Remove Corruption from the target. */
	UFUNCTION(BlueprintCallable, Category = "Combat Status")
	void CureCorruption(AActor* Target);

	/** Remove Chill from the target. */
	UFUNCTION(BlueprintCallable, Category = "Combat Status")
	void RemoveChill(AActor* Target);

	/** Remove Freeze from the target. */
	UFUNCTION(BlueprintCallable, Category = "Combat Status")
	void RemoveFreeze(AActor* Target);

	/** Remove Petrify from the target. */
	UFUNCTION(BlueprintCallable, Category = "Combat Status")
	void RemovePetrify(AActor* Target);

	/** Remove Shock from the target. */
	UFUNCTION(BlueprintCallable, Category = "Combat Status")
	void RemoveShock(AActor* Target);

	/** Remove all DoT and status effects from the target (Cleanse). */
	UFUNCTION(BlueprintCallable, Category = "Combat Status")
	void CleanseAll(AActor* Target);

private:
	// ── Internal helpers ──────────────────────────────────────────────────────

	/**
	 * Build and apply a GE spec with a single SetByCaller magnitude.
	 * Used by Bleed / Ignite / Poison (damage DoTs).
	 */
	FCombatStatusApplyResult ApplyDoTEffect(
		TSubclassOf<UGameplayEffect> EffectClass,
		AActor* Target,
		float SetByCallerValue,
		FName SetByCallerTag,
		float Duration,
		AActor* Instigator) const;

	/**
	 * Remove all active instances of EffectClass from the target.
	 */
	void RemoveEffectByClass(AActor* Target,
		TSubclassOf<UGameplayEffect> EffectClass) const;

	/**
	 * Returns true if at least one active GE of EffectClass is on the target.
	 */
	bool HasActiveEffect(AActor* Target,
		TSubclassOf<UGameplayEffect> EffectClass) const;

	/** Get the target's ASC (or nullptr). */
	static UAbilitySystemComponent* GetTargetASC(AActor* Target);

	/** Get OUR owner's ASC (used as the source/instigator context). */
	UAbilitySystemComponent* GetOwnerASC() const;
};
