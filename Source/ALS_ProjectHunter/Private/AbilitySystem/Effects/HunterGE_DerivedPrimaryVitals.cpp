#include "AbilitySystem/Effects/HunterGE_DerivedPrimaryVitals.h"

#include "AbilitySystem/HunterAttributeSet.h"
#include "AbilitySystem/ModMagnitude/HunterMMC_EffectiveResource.h"
#include "AbilitySystem/ModMagnitude/HunterMMC_EnduranceMaxStamina.h"
#include "AbilitySystem/ModMagnitude/HunterMMC_EnduranceStaminaDegen.h"
#include "AbilitySystem/ModMagnitude/HunterMMC_IntelligenceManaRegen.h"
#include "AbilitySystem/ModMagnitude/HunterMMC_ReservedResource.h"
#include "AbilitySystem/ModMagnitude/HunterMMC_StrengthMaxHealth.h"

namespace HunterDerivedPrimaryVitalsPrivate
{
	static FGameplayModifierInfo MakeCustomModifier(
		const FGameplayAttribute& Attribute,
		EGameplayModOp::Type ModifierOp,
		const TSubclassOf<UGameplayModMagnitudeCalculation>& CalculationClass,
		float Coefficient = 1.0f)
	{
		FGameplayModifierInfo ModifierInfo;
		ModifierInfo.Attribute = Attribute;
		ModifierInfo.ModifierOp = ModifierOp;

		FCustomCalculationBasedFloat CustomMagnitude;
		CustomMagnitude.CalculationClassMagnitude = CalculationClass;
		CustomMagnitude.Coefficient = FScalableFloat(Coefficient);
		ModifierInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(CustomMagnitude);

		return ModifierInfo;
	}
}

UHunterGE_DerivedPrimaryVitals::UHunterGE_DerivedPrimaryVitals()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;

	Modifiers.Reserve(12);
	Modifiers.Add(HunterDerivedPrimaryVitalsPrivate::MakeCustomModifier(
		UHunterAttributeSet::GetMaxHealthAttribute(),
		EGameplayModOp::Override,
		UHunterMMC_StrengthMaxHealth::StaticClass()));
	Modifiers.Add(HunterDerivedPrimaryVitalsPrivate::MakeCustomModifier(
		UHunterAttributeSet::GetMaxStaminaAttribute(),
		EGameplayModOp::Override,
		UHunterMMC_EnduranceMaxStamina::StaticClass()));
	// BUG FIX: ManaRegenRate must NOT also scale with Intelligence.
	// The regen tick formula is Rate * Amount * dt.  If BOTH Rate and Amount add
	// +Intelligence, the result is Intelligence^2 * dt — quadratic scaling.
	// Mirror the StaminaDegen fix: keep Rate at a stable 1.0 via Override so only
	// ManaRegenAmount scales linearly with Intelligence.
	{
		FGameplayModifierInfo RateModifier;
		RateModifier.Attribute   = UHunterAttributeSet::GetManaRegenRateAttribute();
		RateModifier.ModifierOp  = EGameplayModOp::Override;
		RateModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(1.0f));
		Modifiers.Add(RateModifier);
	}
	Modifiers.Add(HunterDerivedPrimaryVitalsPrivate::MakeCustomModifier(
		UHunterAttributeSet::GetManaRegenAmountAttribute(),
		EGameplayModOp::Additive,
		UHunterMMC_IntelligenceManaRegen::StaticClass()));
	// StaminaDegenRate is a fixed multiplier (1.0) — it must not use the Endurance MMC
	// alongside StaminaDegenAmount, because the drain formula is Rate * Amount * dt,
	// giving Endurance^2 * dt (quadratic) if both attributes scale with Endurance.
	// Using Override here ensures the rate stays at a stable 1.0 regardless of stats,
	// while only StaminaDegenAmount scales linearly with Endurance.
	{
		FGameplayModifierInfo RateModifier;
		RateModifier.Attribute   = UHunterAttributeSet::GetStaminaDegenRateAttribute();
		RateModifier.ModifierOp  = EGameplayModOp::Override;
		RateModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(1.0f));
		Modifiers.Add(RateModifier);
	}
	Modifiers.Add(HunterDerivedPrimaryVitalsPrivate::MakeCustomModifier(
		UHunterAttributeSet::GetStaminaDegenAmountAttribute(),
		EGameplayModOp::Additive,
		UHunterMMC_EnduranceStaminaDegen::StaticClass()));
	Modifiers.Add(HunterDerivedPrimaryVitalsPrivate::MakeCustomModifier(
		UHunterAttributeSet::GetReservedHealthAttribute(),
		EGameplayModOp::Override,
		UHunterMMC_ReservedHealth::StaticClass()));
	Modifiers.Add(HunterDerivedPrimaryVitalsPrivate::MakeCustomModifier(
		UHunterAttributeSet::GetReservedManaAttribute(),
		EGameplayModOp::Override,
		UHunterMMC_ReservedMana::StaticClass()));
	Modifiers.Add(HunterDerivedPrimaryVitalsPrivate::MakeCustomModifier(
		UHunterAttributeSet::GetReservedStaminaAttribute(),
		EGameplayModOp::Override,
		UHunterMMC_ReservedStamina::StaticClass()));
	Modifiers.Add(HunterDerivedPrimaryVitalsPrivate::MakeCustomModifier(
		UHunterAttributeSet::GetMaxEffectiveHealthAttribute(),
		EGameplayModOp::Override,
		UHunterMMC_EffectiveHealth::StaticClass()));
	Modifiers.Add(HunterDerivedPrimaryVitalsPrivate::MakeCustomModifier(
		UHunterAttributeSet::GetMaxEffectiveManaAttribute(),
		EGameplayModOp::Override,
		UHunterMMC_EffectiveMana::StaticClass()));
	Modifiers.Add(HunterDerivedPrimaryVitalsPrivate::MakeCustomModifier(
		UHunterAttributeSet::GetMaxEffectiveStaminaAttribute(),
		EGameplayModOp::Override,
		UHunterMMC_EffectiveStamina::StaticClass()));
}
