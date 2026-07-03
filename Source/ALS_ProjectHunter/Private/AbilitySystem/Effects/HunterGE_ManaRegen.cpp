#include "AbilitySystem/Effects/HunterGE_ManaRegen.h"

#include "AbilitySystem/HunterAttributeSet.h"
#include "AbilitySystem/ModMagnitude/HunterMMC_ManaRegen.h"

namespace HunterGEManaRegenPrivate
{
	constexpr float RegenPeriodSeconds = 0.1f;
}

UHunterGE_ManaRegen::UHunterGE_ManaRegen()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	Period = FScalableFloat(HunterGEManaRegenPrivate::RegenPeriodSeconds);
	bExecutePeriodicEffectOnApplication = false;

	FGameplayModifierInfo ModifierInfo;
	ModifierInfo.Attribute  = UHunterAttributeSet::GetManaAttribute();
	ModifierInfo.ModifierOp = EGameplayModOp::Additive;

	FCustomCalculationBasedFloat CustomMagnitude;
	CustomMagnitude.CalculationClassMagnitude = UHunterMMC_ManaRegen::StaticClass();
	CustomMagnitude.Coefficient = FScalableFloat(HunterGEManaRegenPrivate::RegenPeriodSeconds);
	ModifierInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(CustomMagnitude);

	Modifiers.Add(ModifierInfo);
}
