#include "Stats/Library/FunctionLibraries/ContextualStatModifierEvaluator.h"

#include "AbilitySystem/HunterAttributeSet.h"
#include "Item/ItemInstance.h"
#include "Stats/Components/StatsManager.h"
#include "Stats/Library/FunctionLibraries/StatsModifierMath.h"
#include "Tags/PHGameplayTags.h"

void FContextualAttributeModifier::Accumulate(
	const EModifyType ModifyType,
	const float Value)
{
	switch (ModifyType)
	{
	case EModifyType::MT_Add:
	case EModifyType::MT_Increased:
	case EModifyType::MT_AddRange:
		Additive += Value;
		break;
	case EModifyType::MT_Reduced:
		Additive -= FMath::Abs(Value);
		break;
	case EModifyType::MT_Multiply:
	case EModifyType::MT_More:
	case EModifyType::MT_MultiplyRange:
		Product *= FStatsModifierMath::PercentToMultiplier(Value);
		break;
	case EModifyType::MT_Less:
		Product *= FStatsModifierMath::PercentToMultiplier(-FMath::Abs(Value));
		break;
	case EModifyType::MT_Override:
		Override = Value;
		bHasOverride = true;
		break;
	default:
		break;
	}
}

float FContextualAttributeModifier::Resolve(const float BaseValue) const
{
	return bHasOverride ? Override : (BaseValue + Additive) * Product;
}

void FContextualStatModifierSnapshot::Accumulate(
	const FGameplayAttribute& Attribute,
	const EModifyType ModifyType,
	const float Value)
{
	if (Attribute.IsValid())
	{
		ByAttribute.FindOrAdd(Attribute).Accumulate(ModifyType, Value);
	}
}

float FContextualStatModifierSnapshot::Resolve(
	const FGameplayAttribute& Attribute,
	const float BaseValue) const
{
	const FContextualAttributeModifier* Modifier = ByAttribute.Find(Attribute);
	return Modifier ? Modifier->Resolve(BaseValue) : BaseValue;
}

namespace ContextualStatModifierEvaluatorPrivate
{
	bool TryToCombatDamageType(
		const EDamageType DamageType,
		EHunterDamageType& OutDamageType)
	{
		switch (DamageType)
		{
		case EDamageType::DT_Physical:   OutDamageType = EHunterDamageType::Physical; return true;
		case EDamageType::DT_Fire:       OutDamageType = EHunterDamageType::Fire; return true;
		case EDamageType::DT_Ice:        OutDamageType = EHunterDamageType::Ice; return true;
		case EDamageType::DT_Lightning:  OutDamageType = EHunterDamageType::Lightning; return true;
		case EDamageType::DT_Light:      OutDamageType = EHunterDamageType::Light; return true;
		case EDamageType::DT_Corruption: OutDamageType = EHunterDamageType::Corruption; return true;
		default:                         return false;
		}
	}

	bool HasRequiredAndNoBlockedTags(
		const FGameplayTagContainer& ActualTags,
		const FGameplayTagContainer& RequiredTags,
		const FGameplayTagContainer& BlockedTags)
	{
		return ActualTags.HasAll(RequiredTags)
			&& !ActualTags.HasAny(BlockedTags);
	}

	bool MatchesLegacyCondition(
		const EAffixCondition Condition,
		const FStatModifierEvaluationContext& Context)
	{
		const FPHGameplayTags& Tags = FPHGameplayTags::Get();
		switch (Condition)
		{
		case EAffixCondition::AC_None:
			return true;
		case EAffixCondition::AC_WhileDualWielding:
			return Context.bIsDualWielding;
		case EAffixCondition::AC_WhileUnarmed:
			return Context.bIsUnarmed;
		case EAffixCondition::AC_WhileShieldEquipped:
			return Context.bHasShield;
		case EAffixCondition::AC_OnFullLife:
			return Context.SourceHealthPercent >= 0.999f;
		case EAffixCondition::AC_OnLowLife:
			return Context.SourceHealthPercent <= 0.35f;
		case EAffixCondition::AC_RecentlyHit:
			return Context.SourceTags.HasTagExact(Tags.Condition_RecentlyHit)
				|| Context.SourceTags.HasTagExact(Tags.Condition_HitTakenRecently);
		case EAffixCondition::AC_RecentlyKilled:
			return Context.SourceTags.HasTagExact(Tags.Condition_KilledRecently)
				|| Context.SourceTags.HasTagExact(Tags.Condition_EnemyKilledRecently);
		case EAffixCondition::AC_AgainstBoss:
			return Context.TargetTags.HasTagExact(Tags.Condition_TargetIsBoss);
		case EAffixCondition::AC_AgainstElite:
			return Context.TargetTags.HasTagExact(Tags.Condition_Target_IsElite);
		case EAffixCondition::AC_WhileMoving:
			return Context.bIsMoving
				|| Context.SourceTags.HasTagExact(Tags.Condition_WhileMoving);
		case EAffixCondition::AC_WhileStationary:
			return !Context.bIsMoving
				|| Context.SourceTags.HasTagExact(Tags.Condition_WhileStationary);
		case EAffixCondition::AC_AgainstCorrupted:
			return Context.TargetTags.HasTagExact(Tags.Condition_Target_Corrupted);
		case EAffixCondition::AC_DuringFlaskEffect:
			return Context.SourceTags.HasTagExact(Tags.Condition_DuringFlaskEffect);
		case EAffixCondition::AC_InDungeon:
			return Context.SourceTags.HasTagExact(Tags.Condition_InDungeon);
		default:
			return false;
		}
	}

	bool IsContextual(const FPHAttributeData& Modifier)
	{
		return Modifier.Condition != EAffixCondition::AC_None
			|| Modifier.ModifiedLocation == EAffixScope::AS_Conditional
			|| Modifier.ModifiedLocation == EAffixScope::AS_Skill
			|| !Modifier.RequiredSourceTags.IsEmpty()
			|| !Modifier.BlockedSourceTags.IsEmpty()
			|| !Modifier.RequiredTargetTags.IsEmpty()
			|| !Modifier.BlockedTargetTags.IsEmpty();
	}
}

FContextualStatModifierSnapshot FContextualStatModifierEvaluator::BuildFromItems(
	const TArray<UItemInstance*>& Items,
	const UStatsManager* StatsManager,
	const FStatModifierEvaluationContext& Context)
{
	FContextualStatModifierSnapshot Snapshot;
	for (const UItemInstance* Item : Items)
	{
		if (IsValid(Item))
		{
			AccumulateModifiers(Item->Stats.GetAllStats(), StatsManager, Context, Snapshot);
		}
	}
	return Snapshot;
}

void FContextualStatModifierEvaluator::AccumulateModifiers(
	const TArray<FPHAttributeData>& Modifiers,
	const UStatsManager* StatsManager,
	const FStatModifierEvaluationContext& Context,
	FContextualStatModifierSnapshot& InOutSnapshot)
{
	for (const FPHAttributeData& Modifier : Modifiers)
	{
		const bool bIsConversion = Modifier.ModifyType == EModifyType::MT_ConvertTo;
		const bool bIsContextual = ContextualStatModifierEvaluatorPrivate::IsContextual(Modifier);
		if ((!bIsConversion && !bIsContextual)
			|| Modifier.IsLocal()
			|| Modifier.GameplayEffect
			|| Modifier.ModifyType == EModifyType::MT_GrantSkill
			|| !MatchesContext(Modifier, Context))
		{
			continue;
		}

		if (bIsConversion)
		{
			EHunterDamageType FromType;
			EHunterDamageType ToType;
			if (Modifier.FromDamageType != Modifier.ToDamageType
				&& Modifier.RolledStatValue > 0.f
				&& ContextualStatModifierEvaluatorPrivate::TryToCombatDamageType(
					Modifier.FromDamageType, FromType)
				&& ContextualStatModifierEvaluatorPrivate::TryToCombatDamageType(
					Modifier.ToDamageType, ToType))
			{
				FCombatDamageConversionRule& Rule =
					InOutSnapshot.DamageConversionRules.AddDefaulted_GetRef();
				Rule.From = FromType;
				Rule.To = ToType;
				Rule.Percent = Modifier.RolledStatValue;
				Rule.bGainAsExtra = Modifier.bGainAsExtra;
			}
			continue;
		}

		FGameplayAttribute Attribute = Modifier.ModifiedAttribute;
		if (!Attribute.IsValid() && StatsManager && !Modifier.AttributeName.IsNone())
		{
			StatsManager->ResolveAttributeByName(Modifier.AttributeName, Attribute);
		}
		if (Attribute.IsValid())
		{
			InOutSnapshot.Accumulate(Attribute, Modifier.ModifyType, Modifier.RolledStatValue);
		}
	}
}

bool FContextualStatModifierEvaluator::MatchesContext(
	const FPHAttributeData& Modifier,
	const FStatModifierEvaluationContext& Context)
{
	if (!ContextualStatModifierEvaluatorPrivate::HasRequiredAndNoBlockedTags(
		Context.SourceTags, Modifier.RequiredSourceTags, Modifier.BlockedSourceTags)
		|| !ContextualStatModifierEvaluatorPrivate::HasRequiredAndNoBlockedTags(
			Context.TargetTags, Modifier.RequiredTargetTags, Modifier.BlockedTargetTags))
	{
		return false;
	}

	if (Modifier.ModifiedLocation == EAffixScope::AS_Skill && !Context.bIsSkillHit)
	{
		return false;
	}

	if (Modifier.ModifiedLocation == EAffixScope::AS_Conditional
		&& Modifier.Condition == EAffixCondition::AC_None
		&& Modifier.RequiredSourceTags.IsEmpty()
		&& Modifier.BlockedSourceTags.IsEmpty()
		&& Modifier.RequiredTargetTags.IsEmpty()
		&& Modifier.BlockedTargetTags.IsEmpty())
	{
		return false;
	}

	return ContextualStatModifierEvaluatorPrivate::MatchesLegacyCondition(
		Modifier.Condition, Context);
}
