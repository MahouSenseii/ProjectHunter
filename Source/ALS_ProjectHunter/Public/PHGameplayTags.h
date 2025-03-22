// Copyright@2024 Quentin Davis 

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "NativeGameplayTags.h"

/**
 * Singleton containing all native gameplay tags.
 */
struct FPHGameplayTags
{
public:
	/** Retrieves the singleton instance of gameplay tags. */
	static const FPHGameplayTags& Get() { return GameplayTags; }

	/** Initializes all native gameplay tags. */
	static void InitializeNativeGameplayTags();

	/* ========================== */
	/* === Primary Attributes === */
	/* ========================== */
	FGameplayTag Attributes_Primary_Strength;
	FGameplayTag Attributes_Primary_Intelligence;
	FGameplayTag Attributes_Primary_Dexterity;
	FGameplayTag Attributes_Primary_Endurance;
	FGameplayTag Attributes_Primary_Affliction;
	FGameplayTag Attributes_Primary_Luck;
	FGameplayTag Attributes_Primary_Covenant;

	/* =========================== */
	/* === Secondary Attributes === */
	/* =========================== */

	/** Defenses */
	FGameplayTag Attributes_Secondary_Defenses_Armor;

	/** Health & Regen */
	FGameplayTag Attributes_Secondary_Vital_MaxHealth;
	FGameplayTag Attributes_Secondary_Vital_HealthRegenRate;
	FGameplayTag Attributes_Secondary_Vital_HealthRegenAmount;
	FGameplayTag Attributes_Secondary_Vital_MaxHealthRegenAmount;
	FGameplayTag Attributes_Secondary_Vital_MaxHealthRegenRate;
	FGameplayTag Attributes_Secondary_Vital_HealthReservedAmount;
	FGameplayTag Attributes_Secondary_Vital_MaxHealthReservedAmount;
	FGameplayTag Attributes_Secondary_Vital_HealthFlatReservedAmount;
	FGameplayTag Attributes_Secondary_Vital_HealthPercentageReserved;
	FGameplayTag Attributes_Secondary_Vital_MaxEffectiveHealth;

	/** Mana & Regen */
	FGameplayTag Attributes_Secondary_Vital_MaxMana;
	FGameplayTag Attributes_Secondary_Vital_ManaRegenRate;
	FGameplayTag Attributes_Secondary_Vital_ManaRegenAmount;
	FGameplayTag Attributes_Secondary_Vital_MaxManaRegenRate;
	FGameplayTag Attributes_Secondary_Vital_MaxManaRegenAmount;
	FGameplayTag Attributes_Secondary_Vital_ManaReservedAmount;
	FGameplayTag Attributes_Secondary_Vital_MaxManaRRegenAmount;
	FGameplayTag Attributes_Secondary_Vital_MaxManaReservedAmount;
	FGameplayTag Attributes_Secondary_Vital_ManaFlatReservedAmount;
	FGameplayTag Attributes_Secondary_Vital_ManaPercentageReserved;
	FGameplayTag Attributes_Secondary_Vital_MaxEffectiveMana;

	/** Stamina & Regen */
	FGameplayTag Attributes_Secondary_Vital_MaxStamina;
	FGameplayTag Attributes_Secondary_Vital_MaxEffectiveStamina;
	FGameplayTag Attributes_Secondary_Vital_StaminaRegenRate;
	FGameplayTag Attributes_Secondary_Vital_StaminaRegenAmount;
	FGameplayTag Attributes_Secondary_Vital_MaxStaminaRegenRate;
	FGameplayTag Attributes_Secondary_Vital_StaminaFlatReservedAmount;
	FGameplayTag Attributes_Secondary_Vital_StaminaPercentageReserved;
	FGameplayTag Attributes_Secondary_Vital_MaxStaminaRegenAmount;
	FGameplayTag Attributes_Secondary_Vital_StaminaReservedAmount;
	FGameplayTag Attributes_Secondary_Vital_MaxStaminaReservedAmount;

	/** Arcane Shield */
	FGameplayTag Attributes_Secondary_Vital_ArcaneShield;
	FGameplayTag Attributes_Secondary_Vital_ArcaneShieldRegenRate;
	FGameplayTag Attributes_Secondary_Vital_MaxArcaneShieldRegenRate;
	FGameplayTag Attributes_Secondary_Vital_MaxArcaneShieldRegenAmount;
	FGameplayTag Attributes_Secondary_Vital_MaxArcaneShieldReservedAmount;
	FGameplayTag Attributes_Secondary_Vital_ArcaneShieldReservedAmount;
	FGameplayTag Attributes_Secondary_Vital_ArcaneShieldRegenAmount;
	FGameplayTag Attributes_Secondary_Vital_ArcaneShieldFlatReservedAmount;
	FGameplayTag Attributes_Secondary_Vital_ArcaneShieldPercentageReserved;

	/* =================== */
	/* === Damage Tags === */
	/* =================== */

	/** Min Damage */
	FGameplayTag Attributes_Secondary_Damages_MinPhysicalDamage;
	FGameplayTag Attributes_Secondary_Damages_MinFireDamage;
	FGameplayTag Attributes_Secondary_Damages_MinIceDamage;
	FGameplayTag Attributes_Secondary_Damages_MinLightDamage;
	FGameplayTag Attributes_Secondary_Damages_MinLightningDamage;
	FGameplayTag Attributes_Secondary_Damages_MinCorruptionDamage;

	/** Max Damage */
	FGameplayTag Attributes_Secondary_Damages_MaxPhysicalDamage;
	FGameplayTag Attributes_Secondary_Damages_MaxFireDamage;
	FGameplayTag Attributes_Secondary_Damages_MaxIceDamage;
	FGameplayTag Attributes_Secondary_Damages_MaxLightDamage;
	FGameplayTag Attributes_Secondary_Damages_MaxLightningDamage;
	FGameplayTag Attributes_Secondary_Damages_MaxCorruptionDamage;

	/** Damage Bonuses */
	FGameplayTag Attributes_Secondary_BonusDamage_GlobalDamages;
	FGameplayTag Attributes_Secondary_BonusDamage_PhysicalPercentBonus;
	FGameplayTag Attributes_Secondary_BonusDamage_PhysicalFlatBonus;
	FGameplayTag Attributes_Secondary_BonusDamage_FirePercentBonus;
	FGameplayTag Attributes_Secondary_BonusDamage_FireFlatBonus;
	FGameplayTag Attributes_Secondary_BonusDamage_IcePercentBonus;
	FGameplayTag Attributes_Secondary_BonusDamage_IceFlatBonus;
	FGameplayTag Attributes_Secondary_BonusDamage_LightPercentBonus;
	FGameplayTag Attributes_Secondary_BonusDamage_LightFlatBonus;
	FGameplayTag Attributes_Secondary_BonusDamage_LightningPercentBonus;
	FGameplayTag Attributes_Secondary_BonusDamage_LightningFlatBonus;
	FGameplayTag Attributes_Secondary_BonusDamage_CorruptionPercentBonus;
	FGameplayTag Attributes_Secondary_BonusDamage_CorruptionFlatBonus;

	/* ======================= */
	/* === Resistance Tags === */
	/* ======================= */
	FGameplayTag Attributes_Secondary_Resistances_GlobalDefenses;
	FGameplayTag Attributes_Secondary_Resistances_Armour;
	FGameplayTag Attributes_Secondary_Resistances_ArmourFlatBonus;
	FGameplayTag Attributes_Secondary_Resistances_ArmourPercentBonus;

	FGameplayTag Attributes_Secondary_Resistances_FireResistanceFlat;
	FGameplayTag Attributes_Secondary_Resistances_IceResistanceFlat;
	FGameplayTag Attributes_Secondary_Resistances_LightResistanceFlat;
	FGameplayTag Attributes_Secondary_Resistances_LightningResistanceFlat;
	FGameplayTag Attributes_Secondary_Resistances_CorruptionResistanceFlat;

	FGameplayTag Attributes_Secondary_Resistances_FireResistancePercentage;
	FGameplayTag Attributes_Secondary_Resistances_IceResistancePercentage;
	FGameplayTag Attributes_Secondary_Resistances_LightResistancePercentage;
	FGameplayTag Attributes_Secondary_Resistances_LightningResistancePercentage;
	FGameplayTag Attributes_Secondary_Resistances_CorruptionResistancePercentage;

	FGameplayTag Attributes_Secondary_Resistances_MaxFireResistance;
	FGameplayTag Attributes_Secondary_Resistances_MaxIceResistance;
	FGameplayTag Attributes_Secondary_Resistances_MaxLightResistance;
	FGameplayTag Attributes_Secondary_Resistances_MaxLightningResistance;
	FGameplayTag Attributes_Secondary_Resistances_MaxCorruptionResistance;

	FGameplayTag Attributes_Secondary_Resistances_BlockStrength;

	/* ============================= */
	/* === Chance & Duration Tags === */
	/* ============================= */
	FGameplayTag Attributes_Secondary_ChanceToApply_ChanceToBleed;
	FGameplayTag Attributes_Secondary_ChanceToApply_ChanceToIgnite;
	FGameplayTag Attributes_Secondary_ChanceToApply_ChanceToFreeze;
	FGameplayTag Attributes_Secondary_ChanceToApply_ChanceToShock;
	FGameplayTag Attributes_Secondary_ChanceToApply_ChanceToStun;
	FGameplayTag Attributes_Secondary_ChanceToApply_ChanceToKnockBack;

	FGameplayTag Attributes_Secondary_Duration_BurnDuration;
	FGameplayTag Attributes_Secondary_Duration_BleedDuration;
	FGameplayTag Attributes_Secondary_Duration_FreezeDuration;
	FGameplayTag Attributes_Secondary_Duration_CorruptionDuration;
	FGameplayTag Attributes_Secondary_Duration_ShockDuration;

	/* ==================== */
	/* === Miscellaneous === */
	/* ==================== */
	FGameplayTag Attributes_Secondary_Misc_Poise;
	FGameplayTag Attributes_Secondary_Misc_StunRecovery;
	FGameplayTag Attributes_Secondary_Misc_ManaCostChanges;
	FGameplayTag Attributes_Secondary_Misc_CoolDown;
	FGameplayTag Attributes_Secondary_Misc_LifeLeech;
	FGameplayTag Attributes_Secondary_Misc_ManaLeech;
	FGameplayTag Attributes_Secondary_Misc_MovementSpeed;
	FGameplayTag Attributes_Secondary_Misc_LifeOnHit;
	FGameplayTag Attributes_Secondary_Misc_ManaOnHit;
	FGameplayTag Attributes_Secondary_Misc_StaminaOnHit;
	FGameplayTag Attributes_Secondary_Misc_StaminaCostChanges;
	FGameplayTag Attributes_Secondary_Money_Gems;
	FGameplayTag Attributes_Secondary_Misc_CombatAlignment;

	/* ==================== */
	/* === Vitals === */
	/* ==================== */

	FGameplayTag Attributes_Vital_Health;
	FGameplayTag Attributes_Vital_Stamina;
	FGameplayTag Attributes_Vital_Mana;

protected:

private:
	/** Singleton instance */
	static FPHGameplayTags GameplayTags;
};
