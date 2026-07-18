// Incoming mitigation math: armour, resistances, block, stagger, hit-response
// gating. Plain C++, no UCLASS -- UCombatManager (the owner) directs the
// overall hit flow and applies the final result through GAS; this resolver
// only computes the defender's side of the pipeline. Fully self-contained
// (does not depend on UCombatManager or FCombatOutgoingDamageCalculator) so dependency
// direction stays Manager -> Resolver, never Resolver -> Manager.
#pragma once

#include "CoreMinimal.h"
#include "Combat/Library/Structs/CombatStructs.h"

class AActor;
class UAbilitySystemComponent;
class UHunterAttributeSet;

/**
 * Incoming pipeline (defender attributes):
 *   6. Physical mitigates through armour (piercing-reduced), others through
 *      resistances (piercing-reduced, clamped to the per-type max cap).
 *   7. Block (angle, strength, flat, chip damage, stamina cost, guard break).
 *   8. Damage-taken multipliers, then stagger and hit-response gating.
 *
 * UCombatManager owns applying the resulting FCombatResolveResult through
 * GAS (damage, recovery, ailments, reflect); this resolver never mutates
 * gameplay state and never calls into GAS itself.
 */
class ALS_PROJECTHUNTER_API FCombatIncomingDamageResolver
{
public:
	static FCombatResolveResult MitigateDamagePacket(
		const FCombatDamagePacket& InPacket,
		AActor* AttackerActor,
		AActor* DefenderActor,
		const UHunterAttributeSet* AttackerAttributes,
		const UHunterAttributeSet* DefenderAttributes,
		const FAnimationDamageInfo& DamageInfo);

	static float GetResistanceValue(EHunterDamageType DamageType, const UHunterAttributeSet* DefenderAttributes);
	static float GetResistanceCap(EHunterDamageType DamageType, const UHunterAttributeSet* DefenderAttributes);

	// Attacker attribute piercing + animation piercing, clamped 0-100.
	static float GetResistancePierceValue(
		EHunterDamageType DamageType,
		const UHunterAttributeSet* AttackerAttributes,
		const FAnimationDamageInfo& DamageInfo);

	static float GetDamageTakenMultiplier(EHunterDamageType DamageType, const UHunterAttributeSet* DefenderAttributes);

	static bool IsActorBlocking(AActor* Actor);
	static bool CanBlockHit(AActor* AttackerActor, AActor* DefenderActor, const UHunterAttributeSet* DefenderAttributes);
	static float GetBlockTypeMultiplier(EHunterDamageType DamageType, const UHunterAttributeSet* DefenderAttributes);
	static void ApplyBlockingToMitigatedResult(
		AActor* AttackerActor,
		AActor* DefenderActor,
		const UHunterAttributeSet* DefenderAttributes,
		FCombatResolveResult& InOutResult);
	static void ApplyStaminaBlockCost(const UHunterAttributeSet* DefenderAttributes, FCombatResolveResult& InOutResult);

	// Sets bShouldStagger when the hit depletes stamina and the defender is not
	// protected by the State_Self_ExecutingSkill gameplay tag.
	static void EvaluateStagger(
		AActor* DefenderActor,
		const UHunterAttributeSet* DefenderAttributes,
		FCombatResolveResult& InOutResult);

	// Parry zeroes routed damage but keeps per-type taken values so ailments
	// still roll with real magnitudes. Invincible zeroes everything.
	static void ApplyHitResponse(EHitResponse HitResponse, bool bCanApplyAilments, FCombatResolveResult& InOutResult);

	// Debug/log formatting shared with UCombatManager's own ApplyHit logging.
	static FString FormatResult(const FCombatResolveResult& Result);

private:
	// Self-contained ASC lookup for gameplay-tag checks (blocking, hyper-armor).
	// Deliberately not shared with UCombatManager's own copy -- keeps this
	// resolver fully independent instead of depending on its owner.
	static UAbilitySystemComponent* GetAbilitySystemComponentFromActor(const AActor* Actor);
};
