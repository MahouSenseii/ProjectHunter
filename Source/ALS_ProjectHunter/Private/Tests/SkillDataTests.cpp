#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "UObject/StrongObjectPtr.h"

#include "AbilitySystem/HunterAttributeSet.h"
#include "AbilitySystem/Library/FunctionLibraries/PHSkillFunctionLibrary.h"
#include "Stats/Library/FunctionLibraries/StatsModifierMath.h"
#include "Tags/PHGameplayTags.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace PHSkillTests
{
	constexpr float Tolerance = 0.01f;

	constexpr EAutomationTestFlags TestFlags =
		EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter;

	TStrongObjectPtr<UHunterAttributeSet> MakeAttributes()
	{
		return TStrongObjectPtr<UHunterAttributeSet>(
			NewObject<UHunterAttributeSet>(GetTransientPackage()));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPHSkillDataDefaultsTest,
	"ProjectHunter.AbilitySystem.SkillData.Defaults",
	PHSkillTests::TestFlags)

bool FPHSkillDataDefaultsTest::RunTest(const FString&)
{
	const FPHSkillData SkillData;
	const FGameplayTagContainer SkillTags;
	const FPHResolvedSkillData Resolved =
		FPHSkillDataResolver::Resolve(SkillData, SkillTags, nullptr);

	TestEqual(TEXT("Default use rate is neutral"), Resolved.UseRate, 1.f, PHSkillTests::Tolerance);
	TestEqual(TEXT("Default use interval is one second"), Resolved.UseIntervalSeconds, 1.f, PHSkillTests::Tolerance);
	TestEqual(TEXT("Default cooldown is zero"), Resolved.CooldownSeconds, 0.f, PHSkillTests::Tolerance);
	TestEqual(TEXT("Default aura effect multiplier is neutral"), Resolved.Aura.EffectMultiplier, 1.f, PHSkillTests::Tolerance);
	TestEqual(TEXT("Default projectile count is zero"), Resolved.Projectile.Count, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPHSkillAffixTypesAreNotScalarStatsTest,
	"ProjectHunter.AbilitySystem.SkillData.SpecialAffixTypesAreNotScalars",
	PHSkillTests::TestFlags)

bool FPHSkillAffixTypesAreNotScalarStatsTest::RunTest(const FString&)
{
	FResolvedStatModifier Modifier;
	TestFalse(
		TEXT("Grant Skill cannot become a numeric attribute modifier"),
		FStatsModifierMath::ResolveGameplayModifier(EModifyType::MT_GrantSkill, 1.f, Modifier));
	TestFalse(
		TEXT("Set Rank cannot become a numeric attribute override"),
		FStatsModifierMath::ResolveGameplayModifier(EModifyType::MT_SetRank, 5.f, Modifier));
	TestTrue(
		TEXT("Explicit Override remains a valid numeric modifier"),
		FStatsModifierMath::ResolveGameplayModifier(EModifyType::MT_Override, 5.f, Modifier));
	TestEqual(TEXT("Explicit Override keeps its operation"), Modifier.ModOp, EGameplayModOp::Override);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPHSkillDataAttackWeaponTest,
	"ProjectHunter.AbilitySystem.SkillData.AttackWeaponSnapshot",
	PHSkillTests::TestFlags)

bool FPHSkillDataAttackWeaponTest::RunTest(const FString&)
{
	using namespace PHSkillTests;

	FPHSkillData SkillData;
	SkillData.SkillId = TEXT("BasicAttack");
	SkillData.BaseUseRate = 1.25f;
	SkillData.BaseCooldownSeconds = 8.f;
	SkillData.BaseRange = 5.f;
	SkillData.DamageInfo.WeaponSource = ECombatWeaponSource::MainHand;

	FGameplayTagContainer SkillTags;
	SkillTags.AddTag(FPHGameplayTags::Get().Skill_Attack);
	SkillTags.AddTag(FPHGameplayTags::Get().Skill_Melee);

	const TStrongObjectPtr<UHunterAttributeSet> Attributes = MakeAttributes();
	Attributes->InitAttackSpeed(25.f);
	Attributes->InitCastSpeed(500.f);
	Attributes->InitAttackRange(3.f);
	Attributes->InitCooldown(100.f);

	FResolvedWeaponStats WeaponStats;
	WeaponStats.bIsValid = true;
	WeaponStats.Values.AttackSpeed = 2.f;
	WeaponStats.Values.Range = 6.f;

	const FPHResolvedSkillData Resolved =
		FPHSkillDataResolver::Resolve(SkillData, SkillTags, Attributes.Get(), &WeaponStats);

	TestEqual(TEXT("Attack uses the selected weapon's local attack rate"), Resolved.UseRate, 2.5f, Tolerance);
	TestEqual(TEXT("Attack interval is derived from use rate"), Resolved.UseIntervalSeconds, 0.4f, Tolerance);
	TestEqual(TEXT("Cooldown recovery divides base cooldown"), Resolved.CooldownSeconds, 4.f, Tolerance);
	TestEqual(TEXT("Flat attack range adds to selected weapon range"), Resolved.Range, 9.f, Tolerance);
	TestTrue(TEXT("Melee asset tag reaches combat metadata"), Resolved.DamageInfo.Tags.bIsMelee);
	TestEqual(TEXT("Damage input is preserved"), Resolved.DamageInfo.WeaponSource, ECombatWeaponSource::MainHand);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPHSkillDataSpellUtilityTest,
	"ProjectHunter.AbilitySystem.SkillData.SpellProjectileAura",
	PHSkillTests::TestFlags)

bool FPHSkillDataSpellUtilityTest::RunTest(const FString&)
{
	using namespace PHSkillTests;

	FPHSkillData SkillData;
	SkillData.SkillId = TEXT("TestSpell");
	SkillData.BaseUseRate = 2.f;
	SkillData.BaseCooldownSeconds = 6.f;
	SkillData.BaseRange = 200.f;
	SkillData.BaseAreaRadius = 50.f;
	SkillData.Costs.Mana = 20.f;
	SkillData.Costs.Health = 10.f;
	SkillData.Costs.Stamina = 5.f;
	SkillData.Projectile.Count = 1;
	SkillData.Projectile.Speed = 1000.f;
	SkillData.Projectile.ChainCount = 1;
	SkillData.Aura.EffectMultiplier = 1.5f;
	SkillData.Aura.Radius = 300.f;

	FGameplayTagContainer SkillTags;
	const FPHGameplayTags& Tags = FPHGameplayTags::Get();
	SkillTags.AddTag(Tags.Skill_Spell);
	SkillTags.AddTag(Tags.Skill_Projectile);
	SkillTags.AddTag(Tags.Skill_AoE);
	SkillTags.AddTag(Tags.Skill_Chain);
	SkillTags.AddTag(Tags.Skill_Fork);
	SkillTags.AddTag(Tags.Skill_Aura);

	const TStrongObjectPtr<UHunterAttributeSet> Attributes = MakeAttributes();
	Attributes->InitAttackSpeed(500.f);
	Attributes->InitCastSpeed(50.f);
	Attributes->InitCooldown(50.f);
	Attributes->InitAreaOfEffect(100.f);
	Attributes->InitManaCostChanges(-100.f);
	Attributes->InitHealthCostChanges(50.f);
	Attributes->InitStaminaCostChanges(-20.f);
	Attributes->InitProjectileCount(2.f);
	Attributes->InitProjectileSpeed(20.f);
	Attributes->InitChainCount(3.f);
	Attributes->InitForkCount(1.f);
	Attributes->InitAuraEffect(100.f);
	Attributes->InitAuraRadius(50.f);

	const FPHResolvedSkillData Resolved =
		FPHSkillDataResolver::Resolve(SkillData, SkillTags, Attributes.Get());

	TestEqual(TEXT("Spell uses cast speed instead of attack speed"), Resolved.UseRate, 3.f, Tolerance);
	TestEqual(TEXT("Spell interval is derived from cast rate"), Resolved.UseIntervalSeconds, 1.f / 3.f, Tolerance);
	TestEqual(TEXT("Cooldown recovery applies as a rate"), Resolved.CooldownSeconds, 4.f, Tolerance);
	TestEqual(TEXT("Area of effect scales authored area radius"), Resolved.AreaRadius, 100.f, Tolerance);
	TestEqual(TEXT("Negative 100 percent can make mana cost free"), Resolved.Costs.Mana, 0.f, Tolerance);
	TestEqual(TEXT("Increased health cost applies"), Resolved.Costs.Health, 15.f, Tolerance);
	TestEqual(TEXT("Reduced stamina cost applies"), Resolved.Costs.Stamina, 4.f, Tolerance);
	TestEqual(TEXT("Additional projectile count is additive"), Resolved.Projectile.Count, 3);
	TestEqual(TEXT("Projectile speed applies as a percentage"), Resolved.Projectile.Speed, 1200.f, Tolerance);
	TestEqual(TEXT("Additional chain count is additive"), Resolved.Projectile.ChainCount, 4);
	TestEqual(TEXT("Fork count is gated by Skill.Fork"), Resolved.Projectile.ForkCount, 1);
	TestEqual(TEXT("Aura effect scales the authored multiplier"), Resolved.Aura.EffectMultiplier, 3.f, Tolerance);
	TestEqual(TEXT("Aura radius bonus is flat"), Resolved.Aura.Radius, 350.f, Tolerance);
	TestTrue(TEXT("Spell tag reaches combat metadata"), Resolved.DamageInfo.Tags.bIsSpell);
	TestTrue(TEXT("Projectile tag reaches ranged combat metadata"), Resolved.DamageInfo.Tags.bIsRanged);
	TestTrue(TEXT("Area tag reaches combat metadata"), Resolved.DamageInfo.Tags.bIsArea);
	TestFalse(TEXT("Skill.Chain capability does not mark the first hit as a chain bounce"), Resolved.DamageInfo.Tags.bIsChainHit);
	return true;
}

#endif
