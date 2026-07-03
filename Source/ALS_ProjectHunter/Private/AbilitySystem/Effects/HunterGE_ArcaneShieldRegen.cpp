#include "AbilitySystem/Effects/HunterGE_ArcaneShieldRegen.h"

#include "AbilitySystem/HunterAttributeSet.h"
#include "AbilitySystem/ModMagnitude/HunterMMC_ArcaneShieldRegen.h"

namespace HunterGEArcaneShieldRegenPrivate
{
	constexpr float RegenPeriodSeconds = 0.1f;
}

UHunterGE_ArcaneShieldRegen::UHunterGE_ArcaneShieldRegen()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	Period = FScalableFloat(HunterGEArcaneShieldRegenPrivate::RegenPeriodSeconds);
	bExecutePeriodicEffectOnApplication = false;

	FGameplayModifierInfo ModifierInfo;
	ModifierInfo.Attribute  = UHunterAttributeSet::GetArcaneShieldAttribute();
	ModifierInfo.ModifierOp = EGameplayModOp::Additive;

	FCustomCalculationBasedFloat CustomMagnitude;
	CustomMagnitude.CalculationClassMagnitude = UHunterMMC_ArcaneShieldRegen::StaticClass();
	CustomMagnitude.Coefficient = FScalableFloat(HunterGEArcaneShieldRegenPrivate::RegenPeriodSeconds);
	ModifierInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(CustomMagnitude);

	Modifiers.Add(ModifierInfo);
}
