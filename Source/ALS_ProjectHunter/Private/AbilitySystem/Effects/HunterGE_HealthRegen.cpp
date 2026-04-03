#include "AbilitySystem/Effects/HunterGE_HealthRegen.h"

#include "AbilitySystem/HunterAttributeSet.h"

UHunterGE_HealthRegen::UHunterGE_HealthRegen()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo ModifierInfo;
	ModifierInfo.Attribute  = UHunterAttributeSet::GetHealthAttribute();
	ModifierInfo.ModifierOp = EGameplayModOp::Additive;

	// Use DataName (not DataTag) so the lookup is deferred past CDO construction,
	// avoiding the "InterfaceProperty" crash from unregistered tags at startup.
	FSetByCallerFloat SetByCaller;
	SetByCaller.DataName = FName("Data.Recovery.Health");
	ModifierInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);

	Modifiers.Add(ModifierInfo);
}
