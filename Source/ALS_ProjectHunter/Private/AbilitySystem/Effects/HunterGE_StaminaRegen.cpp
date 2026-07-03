#include "AbilitySystem/Effects/HunterGE_StaminaRegen.h"

#include "AbilitySystem/HunterAttributeSet.h"
#include "AbilitySystem/ModMagnitude/HunterMMC_StaminaRegen.h"

namespace HunterGEStaminaRegenPrivate
{
	constexpr float RegenPeriodSeconds = 0.1f;
}

UHunterGE_StaminaRegen::UHunterGE_StaminaRegen()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	Period = FScalableFloat(HunterGEStaminaRegenPrivate::RegenPeriodSeconds);
	bExecutePeriodicEffectOnApplication = false;

	FGameplayModifierInfo ModifierInfo;
	ModifierInfo.Attribute  = UHunterAttributeSet::GetStaminaAttribute();
	ModifierInfo.ModifierOp = EGameplayModOp::Additive;

	FCustomCalculationBasedFloat CustomMagnitude;
	CustomMagnitude.CalculationClassMagnitude = UHunterMMC_StaminaRegen::StaticClass();
	CustomMagnitude.Coefficient = FScalableFloat(HunterGEStaminaRegenPrivate::RegenPeriodSeconds);
	ModifierInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(CustomMagnitude);

	Modifiers.Add(ModifierInfo);
}
