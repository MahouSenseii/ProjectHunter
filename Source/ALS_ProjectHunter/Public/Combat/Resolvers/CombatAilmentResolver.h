#pragma once

#include "CoreMinimal.h"

class UHunterAttributeSet;

/** Inputs for one ailment application roll. */
struct ALS_PROJECTHUNTER_API FCombatAilmentRollInput
{
	float BaseChancePercent = 0.f;
	float HitDamage = 0.f;
	float AilmentThreshold = 1.f;
	float AvoidancePercent = 0.f;
	float PrimaryChanceBonusPercent = 0.f;
	bool bAddDamageBasedChance = false;
};

/** Stateless POE-style ailment threshold, chance, avoidance, and duration rules. */
class ALS_PROJECTHUNTER_API FCombatAilmentResolver
{
public:
	static float ResolveThreshold(const UHunterAttributeSet* DefenderAttributes);
	static float ResolveChancePercent(const FCombatAilmentRollInput& Input);
	static bool Roll(const FCombatAilmentRollInput& Input, FRandomStream& RandomStream);
	static float ResolveDuration(
		float AuthoredDuration,
		float DefaultDuration,
		const UHunterAttributeSet* AttackerAttributes);
	static float ResolveDamagePerTick(
		float HitDamage,
		float DamagePerTickFraction,
		const UHunterAttributeSet* AttackerAttributes);
};
