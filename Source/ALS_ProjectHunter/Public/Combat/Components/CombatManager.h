// ProjectHunter combat owner: resolves animation-authored hits against
// attacker and defender attributes using the damage pipeline.
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/Library/Structs/CombatStructs.h"
#include "CombatManager.generated.h"

class AActor;
class UAbilitySystemComponent;
class UCombatStatusEffectApplier;
class UGameplayEffect;
class UHunterAttributeSet;

DECLARE_LOG_CATEGORY_EXTERN(LogCombatManager, Log, All);

/**
 * Server-side Blueprint edit point handed out by OnEditIncomingHit before the
 * pipeline runs. Blueprint may adjust the animation info, override the hit
 * response, or reject the hit entirely.
 */
UCLASS(BlueprintType)
class ALS_PROJECTHUNTER_API UCombatIncomingHitEditContext : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Combat|Hit")
	TObjectPtr<AActor> AttackerActor = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Hit")
	TObjectPtr<AActor> DefenderActor = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "Combat|Hit")
	FAnimationDamageInfo DamageInfo;

	UPROPERTY(BlueprintReadWrite, Category = "Combat|Hit")
	EHitResponse HitResponse = EHitResponse::Normal;

	UPROPERTY(BlueprintReadWrite, Category = "Combat|Hit")
	bool bCanApplyAilments = true;

	UPROPERTY(BlueprintReadWrite, Category = "Combat|Hit")
	bool bApplyHit = true;

	UFUNCTION(BlueprintCallable, Category = "Combat|Hit")
	void RejectHit();
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEditIncomingHit, UCombatIncomingHitEditContext*, Context);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCombatDamagePopupRequested, const FCombatDamagePopupData&, PopupData);

/**
 * Owner of hit resolution and damage application.
 *
 * Outgoing pipeline (attacker attributes + FAnimationDamageInfo):
 *   1. Base:       roll weapon Min/Max per type + flat added damage.
 *   2. Convert:    attribute conversion matrix, one pass, no chaining.
 *                  converted damage scales only as its final type.
 *   3. Increased:  one additive pool per type: global + type + elemental +
 *                  tag-conditional buckets + animation BaseMulti + HP-state bonuses.
 *   4. More:       multiplicative: global x elemental x type.
 *   5. Crit:       one roll per hit; spells use spell crit attributes;
 *                  damage-over-time hits never crit.
 *
 * Incoming pipeline (defender attributes):
 *   6. Physical mitigates through armour (piercing-reduced), others through
 *      resistances (piercing-reduced, clamped to the per-type max cap).
 *   7. Block (angle, strength, flat, chip damage, stamina cost, guard break).
 *   8. Damage-taken multipliers, then ArcaneShield-before-Health routing.
 *
 * Application is server-authoritative and flows through the configured
 * GameplayEffects so PreAttributeChange clamping always fires.
 */
UCLASS(ClassGroup=(Custom), Blueprintable, meta=(BlueprintSpawnableComponent))
class ALS_PROJECTHUNTER_API UCombatManager : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatManager();

	/**
	 * Instant GE used to apply resolved damage through GAS.
	 * Configure with three Additive SetByCaller modifiers:
	 *   Health -> Data.Damage.Health, ArcaneShield -> Data.Damage.ArcaneShield,
	 *   Stamina -> Data.Damage.Stamina. Magnitudes arrive negative.
	 * If unset the component falls back to SetNumericAttributeBase with a warning.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat",
		meta = (DisplayName = "Damage Application GE"))
	TSubclassOf<UGameplayEffect> DamageApplicationGE;

	/**
	 * Instant GE for on-hit recovery and leech (Health/Mana/Stamina).
	 * Configure with Additive SetByCaller modifiers on Data.Recovery.Health,
	 * Data.Recovery.Mana, Data.Recovery.Stamina. Magnitudes arrive positive.
	 * If unset, on-hit recovery is skipped with a warning.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat",
		meta = (DisplayName = "Recovery Application GE"))
	TSubclassOf<UGameplayEffect> RecoveryApplicationGE;

	/**
	 * Instant GE used to apply reflected damage back to the attacker.
	 * Must be non-recursive; it must NOT trigger ailments or reflect again.
	 * Configure with one Additive SetByCaller modifier:
	 *   Health -> Data.Damage.Health (magnitude arrives negative).
	 * If unset, reflect damage is logged but not applied.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat",
		meta = (DisplayName = "Reflect Application GE"))
	TSubclassOf<UGameplayEffect> ReflectApplicationGE;

	// Server-side Blueprint edit point. Bind to adjust DamageInfo / HitResponse
	// or reject the hit before any damage math runs.
	UPROPERTY(BlueprintAssignable, Category = "Combat|Hit")
	FOnEditIncomingHit OnEditIncomingHit;

	UPROPERTY(BlueprintAssignable, Category = "Combat|Damage Popup")
	FOnCombatDamagePopupRequested OnDamagePopupRequested;

	/**
	 * Owned status helper (Bleed/Ignite/Poison/etc.). The class still derives
	 * from UActorComponent for saved Blueprint compatibility, but CombatManager
	 * owns this instance as an instanced subobject rather than a sibling actor
	 * component.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Instanced, Category = "Combat|Status")
	TObjectPtr<UCombatStatusEffectApplier> CombatStatus;

	UFUNCTION(BlueprintPure, Category = "Combat|Status")
	UCombatStatusEffectApplier* GetCombatStatus() const { return CombatStatus; }

	UFUNCTION(BlueprintPure, Category = "Combat|Status")
	UCombatStatusEffectApplier* GetCombatStatusManager() const { return CombatStatus; }

	/**
	 * Main hit entry point. AnimationDamageInfo is the only Blueprint-authored
	 * input; weapon damage, added damage, conversion, scaling, mitigation, and
	 * every defensive layer come from attacker/defender attributes.
	 *
	 * Runs the full calculation everywhere, but only mutates state (damage,
	 * recovery, ailments, reflect) on the authority. Non-authority calls return
	 * a preview result. Returns false when the hit was invalid or rejected.
	 *
	 * @param HitResponse       Defender-resolved outcome (parry window, i-frames,
	 *                          resource absorb) decided before calling ApplyHit.
	 * @param bCanApplyAilments Master ailment gate for this hit (e.g. false for
	 *                          hazard ticks that should never bleed/ignite).
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat|Hit")
	bool ApplyHit(
		AActor* AttackerActor,
		AActor* DefenderActor,
		const FAnimationDamageInfo& DamageInfo,
		FCombatResolveResult& OutResult,
		EHitResponse HitResponse = EHitResponse::Normal,
		bool bCanApplyAilments = true);

protected:
	//~ Application
	//
	// Hit-resolution math is split across plain, stateless C++ helpers
	// with no Blueprint/serialization surface:
	//   FCombatOutgoingDamageCalculator - base roll, conversion, increased/more, crit.
	//   FCombatIncomingDamageResolver   - armour/resist mitigation, block, stagger, hit-response.
	// CombatManager only validates the hit, directs both helpers in order,
	// and applies the resulting FCombatResolveResult through GAS below.

	static UAbilitySystemComponent* GetAbilitySystemComponentFromActor(const AActor* Actor);
	static const UHunterAttributeSet* GetHunterAttributeSetFromActor(const AActor* Actor);

	void ApplyResolvedDamage(AActor* AttackerActor, AActor* DefenderActor, const FCombatResolveResult& Result) const;

	// LifeOnHit/ManaOnHit/StaminaOnHit plus leech percentages of damage dealt,
	// routed through RecoveryApplicationGE on the attacker.
	void ApplyOnHitRecovery(
		AActor* AttackerActor,
		const FCombatResolveResult& Result,
		UAbilitySystemComponent* AttackerASC,
		const UHunterAttributeSet* AttackerAttributes) const;

	// Rolls attacker ailment chances against per-type mitigated damage and routes successful rolls through CombatStatus.
	void ApplyAilments(AActor* AttackerActor, AActor* DefenderActor, const FCombatResolveResult& Result) const;

	// Defender reflect chance/percent attributes returned to the attacker
	// through ReflectApplicationGE.
	void ApplyReflect(AActor* AttackerActor, AActor* DefenderActor, const FCombatResolveResult& Result) const;

	void BroadcastDamagePopup(AActor* AttackerActor, AActor* DefenderActor, const FCombatResolveResult& Result);
};
