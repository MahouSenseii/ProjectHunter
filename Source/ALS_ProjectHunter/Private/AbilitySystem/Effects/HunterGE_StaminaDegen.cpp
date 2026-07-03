#include "AbilitySystem/Effects/HunterGE_StaminaDegen.h"

#include "AbilitySystem/HunterAttributeSet.h"
#include "AbilitySystem/ModMagnitude/HunterMMC_StaminaDegen.h"

namespace HunterGEStaminaDegenPrivate
{
	constexpr float DrainPeriodSeconds = 0.1f;
}

UHunterGE_StaminaDegen::UHunterGE_StaminaDegen()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	Period = FScalableFloat(HunterGEStaminaDegenPrivate::DrainPeriodSeconds);
	bExecutePeriodicEffectOnApplication = false;

	FGameplayModifierInfo ModifierInfo;
	ModifierInfo.Attribute = UHunterAttributeSet::GetStaminaAttribute();
	ModifierInfo.ModifierOp = EGameplayModOp::Additive;

	FCustomCalculationBasedFloat CustomMagnitude;
	CustomMagnitude.CalculationClassMagnitude = UHunterMMC_StaminaDegen::StaticClass();
	CustomMagnitude.Coefficient = FScalableFloat(HunterGEStaminaDegenPrivate::DrainPeriodSeconds);
	ModifierInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(CustomMagnitude);
	Modifiers.Add(ModifierInfo);
}
