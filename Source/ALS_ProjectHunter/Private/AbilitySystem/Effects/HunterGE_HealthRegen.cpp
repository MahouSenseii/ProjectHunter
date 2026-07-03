#include "AbilitySystem/Effects/HunterGE_HealthRegen.h"

#include "AbilitySystem/HunterAttributeSet.h"
#include "AbilitySystem/ModMagnitude/HunterMMC_HealthRegen.h"

namespace HunterGEHealthRegenPrivate
{
	constexpr float RegenPeriodSeconds = 0.1f;
}

UHunterGE_HealthRegen::UHunterGE_HealthRegen()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	Period = FScalableFloat(HunterGEHealthRegenPrivate::RegenPeriodSeconds);
	bExecutePeriodicEffectOnApplication = false;

	FGameplayModifierInfo ModifierInfo;
	ModifierInfo.Attribute  = UHunterAttributeSet::GetHealthAttribute();
	ModifierInfo.ModifierOp = EGameplayModOp::Additive;

	FCustomCalculationBasedFloat CustomMagnitude;
	CustomMagnitude.CalculationClassMagnitude = UHunterMMC_HealthRegen::StaticClass();
	CustomMagnitude.Coefficient = FScalableFloat(HunterGEHealthRegenPrivate::RegenPeriodSeconds);
	ModifierInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(CustomMagnitude);

	Modifiers.Add(ModifierInfo);
}
