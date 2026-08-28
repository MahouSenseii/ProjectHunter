#include "Stats/Library/FunctionLibraries/PrimaryAttributeRules.h"

#include "AbilitySystem/HunterAttributeSet.h"

namespace PrimaryAttributeRulesPrivate
{
	constexpr float StrengthPhysicalDamagePerPoint = 2.f;
	constexpr float IntelligenceElementalDamagePerPoint = 1.3f;
	constexpr float DexteritySpeedPerPoint = 0.5f;
	constexpr float DexterityCriticalDamagePerPoint = 0.5f;
	constexpr float EnduranceResistancePerPoint = 0.01f;
	constexpr float EnduranceDegenReductionPerPoint = 0.01f;
	constexpr float MaximumEnduranceDegenReduction = 0.75f;
	constexpr float AfflictionDamageOverTimePerPoint = 1.f;
	constexpr float AfflictionDurationPerPoint = 0.05f;
	constexpr float LuckAilmentChancePerPoint = 0.01f;
	constexpr float CovenantMinionDamagePerPoint = 2.f;
	constexpr float CovenantMinionHealthPerPoint = 1.f;

	float SanitizePrimary(const float Value)
	{
		return FMath::Max(0.f, Value);
	}
}

FPHPrimaryAttributeBonuses FPrimaryAttributeRules::Resolve(
	const float Strength,
	const float Intelligence,
	const float Dexterity,
	const float Endurance,
	const float Affliction,
	const float Luck,
	const float Covenant)
{
	using namespace PrimaryAttributeRulesPrivate;

	const float SafeStrength = SanitizePrimary(Strength);
	const float SafeIntelligence = SanitizePrimary(Intelligence);
	const float SafeDexterity = SanitizePrimary(Dexterity);
	const float SafeEndurance = SanitizePrimary(Endurance);
	const float SafeAffliction = SanitizePrimary(Affliction);
	const float SafeLuck = SanitizePrimary(Luck);
	const float SafeCovenant = SanitizePrimary(Covenant);

	FPHPrimaryAttributeBonuses Result;
	Result.PhysicalDamagePercent = SafeStrength * StrengthPhysicalDamagePerPoint;
	Result.ElementalDamagePercent = SafeIntelligence * IntelligenceElementalDamagePerPoint;
	Result.AttackCastSpeedPercent = SafeDexterity * DexteritySpeedPerPoint;
	Result.CriticalDamageBonusPercent = SafeDexterity * DexterityCriticalDamagePerPoint;
	Result.AllResistancePoints = SafeEndurance * EnduranceResistancePerPoint;
	Result.StaminaDegenMultiplier = 1.f - FMath::Min(
		SafeEndurance * EnduranceDegenReductionPerPoint,
		MaximumEnduranceDegenReduction);
	Result.DamageOverTimePercent = SafeAffliction * AfflictionDamageOverTimePerPoint;
	Result.AilmentDurationBonusSeconds = SafeAffliction * AfflictionDurationPerPoint;
	Result.AilmentChanceBonusPercent = SafeLuck * LuckAilmentChancePerPoint;
	Result.MinionDamagePercent = SafeCovenant * CovenantMinionDamagePerPoint;
	Result.MinionHealthPercent = SafeCovenant * CovenantMinionHealthPerPoint;
	return Result;
}

FPHPrimaryAttributeBonuses FPrimaryAttributeRules::Resolve(const UHunterAttributeSet* AttributeSet)
{
	return AttributeSet
		? Resolve(
			AttributeSet->GetStrength(),
			AttributeSet->GetIntelligence(),
			AttributeSet->GetDexterity(),
			AttributeSet->GetEndurance(),
			AttributeSet->GetAffliction(),
			AttributeSet->GetLuck(),
			AttributeSet->GetCovenant())
		: FPHPrimaryAttributeBonuses{};
}

FPHPrimaryAttributeBonuses UPHPrimaryAttributeFunctionLibrary::ResolvePrimaryAttributeBonuses(
	const UHunterAttributeSet* AttributeSet)
{
	return FPrimaryAttributeRules::Resolve(AttributeSet);
}
