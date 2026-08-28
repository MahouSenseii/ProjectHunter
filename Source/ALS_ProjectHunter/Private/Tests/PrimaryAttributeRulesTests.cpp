#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "UObject/StrongObjectPtr.h"

#include "AbilitySystem/HunterAttributeSet.h"
#include "Combat/Resolvers/CombatAilmentResolver.h"
#include "Stats/Library/FunctionLibraries/PrimaryAttributeRules.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace PHPrimaryAttributeTests
{
	constexpr float Tolerance = 0.01f;
	constexpr EAutomationTestFlags TestFlags =
		EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPHPrimaryAttributeRulesTest,
	"ProjectHunter.Stats.PrimaryAttributes.GameplayBonuses",
	PHPrimaryAttributeTests::TestFlags)

bool FPHPrimaryAttributeRulesTest::RunTest(const FString&)
{
	const FPHPrimaryAttributeBonuses Bonuses = FPrimaryAttributeRules::Resolve(
		10.f, 10.f, 10.f, 10.f, 10.f, 10.f, 10.f);

	TestEqual(TEXT("Strength grants physical damage"), Bonuses.PhysicalDamagePercent, 20.f, PHPrimaryAttributeTests::Tolerance);
	TestEqual(TEXT("Intelligence grants elemental damage"), Bonuses.ElementalDamagePercent, 13.f, PHPrimaryAttributeTests::Tolerance);
	TestEqual(TEXT("Dexterity grants attack and cast speed"), Bonuses.AttackCastSpeedPercent, 5.f, PHPrimaryAttributeTests::Tolerance);
	TestEqual(TEXT("Dexterity grants critical damage"), Bonuses.CriticalDamageBonusPercent, 5.f, PHPrimaryAttributeTests::Tolerance);
	TestEqual(TEXT("Endurance grants all resistance points"), Bonuses.AllResistancePoints, 0.1f, PHPrimaryAttributeTests::Tolerance);
	TestEqual(TEXT("Endurance reduces stamina degeneration"), Bonuses.StaminaDegenMultiplier, 0.9f, PHPrimaryAttributeTests::Tolerance);
	TestEqual(TEXT("Affliction grants damage over time"), Bonuses.DamageOverTimePercent, 10.f, PHPrimaryAttributeTests::Tolerance);
	TestEqual(TEXT("Affliction grants ailment duration"), Bonuses.AilmentDurationBonusSeconds, 0.5f, PHPrimaryAttributeTests::Tolerance);
	TestEqual(TEXT("Luck grants ailment chance"), Bonuses.AilmentChanceBonusPercent, 0.1f, PHPrimaryAttributeTests::Tolerance);
	TestEqual(TEXT("Covenant grants minion damage"), Bonuses.MinionDamagePercent, 20.f, PHPrimaryAttributeTests::Tolerance);
	TestEqual(TEXT("Covenant grants minion health"), Bonuses.MinionHealthPercent, 10.f, PHPrimaryAttributeTests::Tolerance);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPHCombatAilmentRulesTest,
	"ProjectHunter.Combat.Ailments.ThresholdAvoidanceAndDuration",
	PHPrimaryAttributeTests::TestFlags)

bool FPHCombatAilmentRulesTest::RunTest(const FString&)
{
	TStrongObjectPtr<UHunterAttributeSet> Attacker(
		NewObject<UHunterAttributeSet>(GetTransientPackage()));
	TStrongObjectPtr<UHunterAttributeSet> Defender(
		NewObject<UHunterAttributeSet>(GetTransientPackage()));
	Attacker->InitAffliction(10.f);
	Attacker->InitDamageOverTime(20.f);
	Defender->InitMaxEffectiveHealth(200.f);

	TestEqual(
		TEXT("Zero authored threshold falls back to effective maximum health"),
		FCombatAilmentResolver::ResolveThreshold(Defender.Get()),
		200.f,
		PHPrimaryAttributeTests::Tolerance);

	FCombatAilmentRollInput Input;
	Input.BaseChancePercent = 20.f;
	Input.HitDamage = 25.f;
	Input.AilmentThreshold = 100.f;
	Input.AvoidancePercent = 20.f;
	Input.PrimaryChanceBonusPercent = 5.f;
	Input.bAddDamageBasedChance = true;
	TestEqual(
		TEXT("Authored, damage-based, and primary chance resolve before avoidance"),
		FCombatAilmentResolver::ResolveChancePercent(Input),
		40.f,
		PHPrimaryAttributeTests::Tolerance);
	TestEqual(
		TEXT("Affliction extends the resolved duration"),
		FCombatAilmentResolver::ResolveDuration(0.f, 2.f, Attacker.Get()),
		2.5f,
		PHPrimaryAttributeTests::Tolerance);
	TestEqual(
		TEXT("Ailment damage uses DamageOverTime and Affliction scaling"),
		FCombatAilmentResolver::ResolveDamagePerTick(100.f, 0.2f, Attacker.Get()),
		26.f,
		PHPrimaryAttributeTests::Tolerance);
	return true;
}

#endif
