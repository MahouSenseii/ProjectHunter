// Outgoing damage math. Plain C++, no UCLASS -- UCombatManager (the owner)
// resolves attacker attributes and directs the overall hit flow; this
// calculator only computes the outgoing FCombatDamagePacket from the inputs
// it's given. It holds no state, calls into no other helper, and has zero
// Blueprint/serialization surface.
#pragma once

#include "CoreMinimal.h"
#include "Combat/Library/Structs/CombatStructs.h"

class UHunterAttributeSet;
struct FResolvedWeaponStats;
struct FContextualStatModifierSnapshot;

/**
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
 * UCombatManager owns everything downstream of the resulting packet
 * (mitigation, application through GAS); this calculator never touches GAS.
 */
class ALS_PROJECTHUNTER_API FCombatOutgoingDamageCalculator
{
public:
	static float RollDamageRange(float MinDamage, float MaxDamage, FRandomStream& RandomStream);

	static FCombatDamagePacket BuildOutgoingDamagePacket(
		const UHunterAttributeSet* AttackerAttributes,
		const FAnimationDamageInfo& DamageInfo,
		FRandomStream& RandomStream,
		const FResolvedWeaponStats* WeaponStats = nullptr,
		const FContextualStatModifierSnapshot* ContextualModifiers = nullptr);

	// Weapon roll + flat added damage for one type, before any scaling.
	static float CalculateBaseDamageForType(
		EHunterDamageType DamageType,
		const UHunterAttributeSet* AttackerAttributes,
		const FAnimationDamageInfo& DamageInfo,
		FRandomStream& RandomStream,
		const FResolvedWeaponStats* WeaponStats = nullptr,
		const FContextualStatModifierSnapshot* ContextualModifiers = nullptr);

	/** One non-chaining conversion stage. Conversion is normalized at 100%; gain-as-extra is not. */
	static FCombatDamagePacket ApplyDamageConversionRules(
		const FCombatDamagePacket& InPacket,
		const TArray<FCombatDamageConversionRule>& Rules);

	// One-pass attribute conversion. Runs on unscaled base damage so converted
	// damage scales only with modifiers of its final type.
	static FCombatDamagePacket ApplyDamageConversion(
		const FCombatDamagePacket& InPacket,
		const UHunterAttributeSet* AttackerAttributes,
		const FContextualStatModifierSnapshot* ContextualModifiers = nullptr);

	// Additive increased pool for one type: global + type + elemental +
	// tag-conditional attribute buckets + animation BaseMulti + HP-state bonuses.
	static float GetIncreasedDamagePercent(
		EHunterDamageType DamageType,
		const UHunterAttributeSet* AttackerAttributes,
		const FAnimationDamageInfo& DamageInfo,
		const FContextualStatModifierSnapshot* ContextualModifiers = nullptr);

	// Multiplicative more multipliers: global x elemental x type.
	static float GetMoreDamageMultiplier(
		EHunterDamageType DamageType,
		const UHunterAttributeSet* AttackerAttributes,
		const FContextualStatModifierSnapshot* ContextualModifiers = nullptr);

	// One crit roll for the whole hit. Damage-over-time hits never crit.
	static void ResolveCriticalStrike(
		FCombatDamagePacket& Packet,
		const UHunterAttributeSet* AttackerAttributes,
		const FAnimationDamageInfo& DamageInfo,
		FRandomStream& RandomStream,
		const FResolvedWeaponStats* WeaponStats = nullptr,
		const FContextualStatModifierSnapshot* ContextualModifiers = nullptr);

	// Debug/log formatting shared with UCombatManager's own ApplyHit logging.
	static FString FormatPacket(const FCombatDamagePacket& Packet);
};
