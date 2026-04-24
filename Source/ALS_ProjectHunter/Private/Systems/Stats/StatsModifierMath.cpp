#include "Systems/Stats/StatsModifierMath.h"

bool FStatsModifierMath::ResolveGameplayModifier(const EModifyType ModifyType, const float RolledValue, FResolvedStatModifier& OutModifier)
{
	OutModifier = FResolvedStatModifier{};

	switch (ModifyType)
	{
	case EModifyType::MT_None:
		return false;

	case EModifyType::MT_Add:
	case EModifyType::MT_Increased:
	case EModifyType::MT_ConvertTo:
	case EModifyType::MT_AddRange:
		OutModifier.ModOp = EGameplayModOp::Additive;
		OutModifier.Magnitude = RolledValue;
		OutModifier.bCreatesGameplayModifier = true;
		return true;

	case EModifyType::MT_Reduced:
		OutModifier.ModOp = EGameplayModOp::Additive;
		OutModifier.Magnitude = -FMath::Abs(RolledValue);
		OutModifier.bCreatesGameplayModifier = true;
		return true;

	case EModifyType::MT_Multiply:
	case EModifyType::MT_More:
	case EModifyType::MT_MultiplyRange:
		OutModifier.ModOp = EGameplayModOp::Multiplicitive;
		OutModifier.Magnitude = PercentToMultiplier(RolledValue);
		OutModifier.bCreatesGameplayModifier = true;
		return true;

	case EModifyType::MT_Less:
		OutModifier.ModOp = EGameplayModOp::Multiplicitive;
		OutModifier.Magnitude = PercentToMultiplier(-FMath::Abs(RolledValue));
		OutModifier.bCreatesGameplayModifier = true;
		return true;

	case EModifyType::MT_Override:
	case EModifyType::MT_SetRank:
		OutModifier.ModOp = EGameplayModOp::Override;
		OutModifier.Magnitude = RolledValue;
		OutModifier.bCreatesGameplayModifier = true;
		return true;

	case EModifyType::MT_GrantSkill:
		// Actual ability grants belong to the moveset/ability owner. If a designer
		// points this stat at a numeric rank attribute, still let it add that value.
		OutModifier.ModOp = EGameplayModOp::Additive;
		OutModifier.Magnitude = FMath::IsNearlyZero(RolledValue) ? 1.f : RolledValue;
		OutModifier.bCreatesGameplayModifier = true;
		return true;

	default:
		return false;
	}
}

float FStatsModifierMath::PercentToMultiplier(const float Percent)
{
	return FMath::Max(0.f, 1.f + (Percent / 100.f));
}

float FStatsModifierMath::ApplyPercentChange(const float BaseValue, const float PercentChange)
{
	return FMath::Max(0.f, BaseValue * PercentToMultiplier(PercentChange));
}
