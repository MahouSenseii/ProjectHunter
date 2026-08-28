#include "AbilitySystem/Library/FunctionLibraries/PHSkillFunctionLibrary.h"

#include "AbilitySystem/HunterAttributeSet.h"
#include "Stats/Library/FunctionLibraries/PrimaryAttributeRules.h"
#include "Tags/PHGameplayTags.h"

namespace PHSkillDataResolverPrivate
{
	float PercentToMultiplier(const float Percent)
	{
		return FMath::Max(0.f, 1.f + Percent / 100.f);
	}

	int32 ResolveCount(const int32 BaseCount, const float AddedCount)
	{
		return FMath::Max(0, BaseCount + FMath::RoundToInt(AddedCount));
	}
}

FPHResolvedSkillData FPHSkillDataResolver::Resolve(
	const FPHSkillData& SkillData,
	const FGameplayTagContainer& SkillTags,
	const UHunterAttributeSet* AttributeSet,
	const FResolvedWeaponStats* WeaponStats)
{
	using namespace PHSkillDataResolverPrivate;

	const FPHGameplayTags& Tags = FPHGameplayTags::Get();
	const bool bIsAttack = SkillTags.HasTagExact(Tags.Skill_Attack);
	const bool bIsSpell = SkillTags.HasTagExact(Tags.Skill_Spell);
	const bool bIsProjectile = SkillTags.HasTagExact(Tags.Skill_Projectile);
	const bool bIsArea = SkillTags.HasTagExact(Tags.Skill_AoE);
	const bool bCanChain = SkillTags.HasTagExact(Tags.Skill_Chain);
	const bool bCanFork = SkillTags.HasTagExact(Tags.Skill_Fork);
	const bool bIsAura = SkillTags.HasTagExact(Tags.Skill_Aura);
	const bool bIsSummon = SkillTags.HasTagExact(Tags.Skill_Summon);
	const bool bHasWeapon = WeaponStats && WeaponStats->bIsValid;

	FPHResolvedSkillData Result;
	Result.SkillId = SkillData.SkillId;
	Result.DisplayName = SkillData.DisplayName;
	Result.Description = SkillData.Description;
	Result.Icon = SkillData.Icon;
	Result.SkillTags = SkillTags;
	Result.Costs = SkillData.Costs;
	Result.Projectile = SkillData.Projectile;
	Result.Aura = SkillData.Aura;
	Result.DamageInfo = SkillData.DamageInfo;
	Result.Costs.Mana = FMath::Max(0.f, Result.Costs.Mana);
	Result.Costs.Health = FMath::Max(0.f, Result.Costs.Health);
	Result.Costs.Stamina = FMath::Max(0.f, Result.Costs.Stamina);
	Result.Projectile.Count = FMath::Max(0, Result.Projectile.Count);
	Result.Projectile.Speed = FMath::Max(0.f, Result.Projectile.Speed);
	Result.Projectile.ChainCount = FMath::Max(0, Result.Projectile.ChainCount);
	Result.Projectile.ForkCount = FMath::Max(0, Result.Projectile.ForkCount);
	Result.Aura.EffectMultiplier = FMath::Max(0.f, Result.Aura.EffectMultiplier);
	Result.Aura.Radius = FMath::Max(0.f, Result.Aura.Radius);

	float BaseUseRate = FMath::Max(0.01f, SkillData.BaseUseRate);
	if (bIsAttack && SkillData.bUseWeaponAttackRate && bHasWeapon)
	{
		BaseUseRate = FMath::Max(0.01f, WeaponStats->Values.AttackSpeed);
	}

	float SpeedPercent = 0.f;
	if (AttributeSet)
	{
		const FPHPrimaryAttributeBonuses PrimaryBonuses = FPrimaryAttributeRules::Resolve(AttributeSet);
		SpeedPercent = bIsAttack
			? AttributeSet->GetAttackSpeed()
			: (bIsSpell ? AttributeSet->GetCastSpeed() : 0.f);
		if (bIsAttack || bIsSpell)
		{
			SpeedPercent += PrimaryBonuses.AttackCastSpeedPercent;
		}

		if (bIsSummon)
		{
			Result.MinionDamageMultiplier = PercentToMultiplier(PrimaryBonuses.MinionDamagePercent);
			Result.MinionHealthMultiplier = PercentToMultiplier(PrimaryBonuses.MinionHealthPercent);
		}
	}
	Result.UseRate = FMath::Max(0.01f, BaseUseRate * PercentToMultiplier(SpeedPercent));
	Result.UseIntervalSeconds = 1.f / Result.UseRate;

	const float CooldownRecoveryPercent = AttributeSet ? AttributeSet->GetCooldown() : 0.f;
	const float CooldownRecoveryMultiplier = FMath::Max(0.01f, PercentToMultiplier(CooldownRecoveryPercent));
	Result.CooldownSeconds = FMath::Max(0.f, SkillData.BaseCooldownSeconds) / CooldownRecoveryMultiplier;

	Result.Range = FMath::Max(0.f,
		bIsAttack && SkillData.bUseWeaponRange && bHasWeapon
			? WeaponStats->Values.Range
			: SkillData.BaseRange);
	if (bIsAttack && AttributeSet)
	{
		Result.Range = FMath::Max(0.f, Result.Range + AttributeSet->GetAttackRange());
	}

	Result.AreaRadius = FMath::Max(0.f, SkillData.BaseAreaRadius);
	if (bIsArea && AttributeSet)
	{
		Result.AreaRadius *= PercentToMultiplier(AttributeSet->GetAreaOfEffect());
	}

	if (AttributeSet)
	{
		Result.Costs.Mana *= PercentToMultiplier(AttributeSet->GetManaCostChanges());
		Result.Costs.Health *= PercentToMultiplier(AttributeSet->GetHealthCostChanges());
		Result.Costs.Stamina *= PercentToMultiplier(AttributeSet->GetStaminaCostChanges());
	}

	if (bIsProjectile && AttributeSet)
	{
		Result.Projectile.Count = ResolveCount(Result.Projectile.Count, AttributeSet->GetProjectileCount());
		Result.Projectile.Speed *= PercentToMultiplier(AttributeSet->GetProjectileSpeed());
	}
	if (bCanChain && AttributeSet)
	{
		Result.Projectile.ChainCount = ResolveCount(Result.Projectile.ChainCount, AttributeSet->GetChainCount());
	}
	if (bCanFork && AttributeSet)
	{
		Result.Projectile.ForkCount = ResolveCount(Result.Projectile.ForkCount, AttributeSet->GetForkCount());
	}
	if (bIsAura && AttributeSet)
	{
		Result.Aura.EffectMultiplier *= PercentToMultiplier(AttributeSet->GetAuraEffect());
		Result.Aura.Radius = FMath::Max(0.f, Result.Aura.Radius + AttributeSet->GetAuraRadius());
	}

	MergeSkillTagsIntoDamageInfo(Result.DamageInfo, SkillTags);
	return Result;
}

void FPHSkillDataResolver::MergeSkillTagsIntoDamageInfo(
	FAnimationDamageInfo& DamageInfo,
	const FGameplayTagContainer& SkillTags)
{
	const FPHGameplayTags& Tags = FPHGameplayTags::Get();
	DamageInfo.Tags.bIsMelee |= SkillTags.HasTagExact(Tags.Skill_Melee);
	DamageInfo.Tags.bIsRanged |= SkillTags.HasTagExact(Tags.Skill_Projectile);
	DamageInfo.Tags.bIsSpell |= SkillTags.HasTagExact(Tags.Skill_Spell);
	DamageInfo.Tags.bIsArea |= SkillTags.HasTagExact(Tags.Skill_AoE);
	DamageInfo.Tags.bIsDamageOverTime |= SkillTags.HasTagExact(Tags.Skill_DamageOverTime);

	// Skill.Chain means the skill can chain. The execution owner marks only
	// actual bounce hits as bIsChainHit so the first target gets no chain bonus.
}

FPHResolvedSkillData UPHSkillFunctionLibrary::ResolveSkillData(
	const FPHSkillData& SkillData,
	const FGameplayTagContainer& SkillTags,
	const UHunterAttributeSet* AttributeSet)
{
	return FPHSkillDataResolver::Resolve(SkillData, SkillTags, AttributeSet);
}

FPHResolvedSkillData UPHSkillFunctionLibrary::ResolveSkillDataWithWeapon(
	const FPHSkillData& SkillData,
	const FGameplayTagContainer& SkillTags,
	const UHunterAttributeSet* AttributeSet,
	const FResolvedWeaponStats& WeaponStats)
{
	return FPHSkillDataResolver::Resolve(SkillData, SkillTags, AttributeSet, &WeaponStats);
}
