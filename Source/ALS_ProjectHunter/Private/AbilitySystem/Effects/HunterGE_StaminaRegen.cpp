#include "AbilitySystem/Effects/HunterGE_StaminaRegen.h"

#include "AbilitySystem/HunterAttributeSet.h"

UHunterGE_StaminaRegen::UHunterGE_StaminaRegen()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo ModifierInfo;
	ModifierInfo.Attribute  = UHunterAttributeSet::GetStaminaAttribute();
	ModifierInfo.ModifierOp = EGameplayModOp::Additive;

	// Use DataName (not DataTag) so the lookup is deferred past CDO construction,
	// avoiding the "InterfaceProperty" crash from unregistered tags at startup.
	FSetByCallerFloat SetByCaller;
	SetByCaller.DataName = FName("Data.Recovery.Stamina");
	ModifierInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);

	Modifiers.Add(ModifierInfo);
}
