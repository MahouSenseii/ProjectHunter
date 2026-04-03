#include "AbilitySystem/Effects/HunterGE_ManaRegen.h"

#include "AbilitySystem/HunterAttributeSet.h"

UHunterGE_ManaRegen::UHunterGE_ManaRegen()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo ModifierInfo;
	ModifierInfo.Attribute  = UHunterAttributeSet::GetManaAttribute();
	ModifierInfo.ModifierOp = EGameplayModOp::Additive;

	// Use DataName (not DataTag) so the lookup is deferred past CDO construction,
	// avoiding the "InterfaceProperty" crash from unregistered tags at startup.
	FSetByCallerFloat SetByCaller;
	SetByCaller.DataName = FName("Data.Recovery.Mana");
	ModifierInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);

	Modifiers.Add(ModifierInfo);
}
