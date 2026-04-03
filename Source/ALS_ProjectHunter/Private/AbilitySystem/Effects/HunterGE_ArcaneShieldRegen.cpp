#include "AbilitySystem/Effects/HunterGE_ArcaneShieldRegen.h"

#include "AbilitySystem/HunterAttributeSet.h"

UHunterGE_ArcaneShieldRegen::UHunterGE_ArcaneShieldRegen()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo ModifierInfo;
	ModifierInfo.Attribute  = UHunterAttributeSet::GetArcaneShieldAttribute();
	ModifierInfo.ModifierOp = EGameplayModOp::Additive;

	// Use DataName (not DataTag) so the lookup is deferred past CDO construction,
	// avoiding the "InterfaceProperty" crash from unregistered tags at startup.
	FSetByCallerFloat SetByCaller;
	SetByCaller.DataName = FName("Data.Recovery.ArcaneShield");
	ModifierInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);

	Modifiers.Add(ModifierInfo);
}
