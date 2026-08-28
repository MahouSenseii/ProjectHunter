#include "Combat/Resolvers/CombatAilmentResolver.h"

#include "AbilitySystem/HunterAttributeSet.h"
#include "Stats/Library/FunctionLibraries/PrimaryAttributeRules.h"

float FCombatAilmentResolver::ResolveThreshold(const UHunterAttributeSet* DefenderAttributes)
{
	if (!DefenderAttributes)
	{
		return 1.f;
	}

	if (DefenderAttributes->GetAilmentThreshold() > 0.f)
	{
		return DefenderAttributes->GetAilmentThreshold();
	}

	const float EffectiveHealth = DefenderAttributes->GetMaxEffectiveHealth();
	return FMath::Max(
		EffectiveHealth > 0.f ? EffectiveHealth : DefenderAttributes->GetMaxHealth(),
		1.f);
}

float FCombatAilmentResolver::ResolveChancePercent(const FCombatAilmentRollInput& Input)
{
	float ChancePercent = FMath::Max(0.f, Input.BaseChancePercent)
		+ FMath::Max(0.f, Input.PrimaryChanceBonusPercent);

	if (Input.bAddDamageBasedChance && Input.HitDamage > 0.f)
	{
		const float SafeThreshold = FMath::Max(Input.AilmentThreshold, 1.f);
		ChancePercent += FMath::Clamp(Input.HitDamage / SafeThreshold, 0.f, 1.f) * 100.f;
	}

	const float AvoidanceMultiplier = 1.f - FMath::Clamp(Input.AvoidancePercent, 0.f, 100.f) / 100.f;
	return FMath::Clamp(ChancePercent * AvoidanceMultiplier, 0.f, 100.f);
}

bool FCombatAilmentResolver::Roll(
	const FCombatAilmentRollInput& Input,
	FRandomStream& RandomStream)
{
	const float ChancePercent = ResolveChancePercent(Input);
	return ChancePercent > 0.f && RandomStream.FRandRange(0.f, 100.f) < ChancePercent;
}

float FCombatAilmentResolver::ResolveDuration(
	const float AuthoredDuration,
	const float DefaultDuration,
	const UHunterAttributeSet* AttackerAttributes)
{
	const float BaseDuration = AuthoredDuration > 0.f ? AuthoredDuration : DefaultDuration;
	const float PrimaryBonus = FPrimaryAttributeRules::Resolve(AttackerAttributes).AilmentDurationBonusSeconds;
	return FMath::Max(0.f, BaseDuration + PrimaryBonus);
}

float FCombatAilmentResolver::ResolveDamagePerTick(
	const float HitDamage,
	const float DamagePerTickFraction,
	const UHunterAttributeSet* AttackerAttributes)
{
	const float AttributeDamageOverTime = AttackerAttributes
		? AttackerAttributes->GetDamageOverTime()
		: 0.f;
	const float PrimaryDamageOverTime = FPrimaryAttributeRules::Resolve(AttackerAttributes).DamageOverTimePercent;
	const float DamageOverTimeMultiplier = FMath::Max(
		0.f,
		1.f + (AttributeDamageOverTime + PrimaryDamageOverTime) / 100.f);
	return FMath::Max(0.f, HitDamage)
		* FMath::Max(0.f, DamagePerTickFraction)
		* DamageOverTimeMultiplier;
}
