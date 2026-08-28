#include "AbilitySystem/Effects/HunterGE_DerivedPrimaryVitals.h"

#include "AbilitySystem/HunterAttributeSet.h"
#include "AbilitySystem/ModMagnitude/HunterMMC_EnduranceMaxStamina.h"
#include "AbilitySystem/ModMagnitude/HunterMMC_EnduranceStaminaDegen.h"
#include "AbilitySystem/ModMagnitude/HunterMMC_IntelligenceMaxMana.h"
#include "AbilitySystem/ModMagnitude/HunterMMC_IntelligenceManaRegen.h"
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

/**
 * Couples the primary stats to the vitals they scale, and nothing else.
 *
 * This deliberately does NOT touch ReservedHealth/Mana/Stamina or
 * MaxEffectiveHealth/Mana/Stamina. UHunterAttributeSet::UpdateHealthDerivedAttributes
 * and its siblings already compute those from the Max values through
 * ResolveResourceReservation, and RecalculateAllDerivedVitals runs them
 * whenever anything upstream changes. Modifying them here as well meant an
 * Infinite Override effect and the attribute set writing the same six
 * attributes and overwriting each other.
 *
 * The old ManaRegenRate and StaminaDegenRate modifiers are gone too - they were
 * a literal Override of 1.0 with no calculation behind them, which is data, not
 * an effect. Author those two in the stats data asset instead.
 *
 * Keeping the modifier list this short matters for a second reason:
 * FStatsInitializer skips an entire InitializationEffect when ANY attribute it
 * modifies is authored in the data asset. Every needless modifier here is
 * another way for the whole effect to be silently dropped.
 */
UHunterGE_DerivedPrimaryVitals::UHunterGE_DerivedPrimaryVitals()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;

	Modifiers.Reserve(5);

	// Primary stat -> maximum resource, ADDED to the base the stats data asset
	// authored. Override would discard that base and give every character the
	// same pool, which is what stopped monsters having their own health.
	Modifiers.Add(HunterDerivedPrimaryVitalsPrivate::MakeCustomModifier(
		UHunterAttributeSet::GetMaxHealthAttribute(),
		EGameplayModOp::Additive,
		UHunterMMC_StrengthMaxHealth::StaticClass()));
	Modifiers.Add(HunterDerivedPrimaryVitalsPrivate::MakeCustomModifier(
		UHunterAttributeSet::GetMaxStaminaAttribute(),
		EGameplayModOp::Additive,
		UHunterMMC_EnduranceMaxStamina::StaticClass()));
	Modifiers.Add(HunterDerivedPrimaryVitalsPrivate::MakeCustomModifier(
		UHunterAttributeSet::GetMaxManaAttribute(),
		EGameplayModOp::Additive,
		UHunterMMC_IntelligenceMaxMana::StaticClass()));

	// Primary stat -> regeneration and drain magnitudes. The matching *Rate
	// attributes stay authored data; only the amounts scale with a stat.
	Modifiers.Add(HunterDerivedPrimaryVitalsPrivate::MakeCustomModifier(
		UHunterAttributeSet::GetManaRegenAmountAttribute(),
		EGameplayModOp::Additive,
		UHunterMMC_IntelligenceManaRegen::StaticClass()));
	Modifiers.Add(HunterDerivedPrimaryVitalsPrivate::MakeCustomModifier(
		UHunterAttributeSet::GetStaminaDegenAmountAttribute(),
		EGameplayModOp::Multiplicitive,
		UHunterMMC_EnduranceStaminaDegen::StaticClass()));
}
