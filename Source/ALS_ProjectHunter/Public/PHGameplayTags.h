// Copyright@2024 Quentin Davis 

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
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

	static void RegisterPrimaryAttributes();
    static void RegisterSecondaryVitals();
    static void RegisterDamageTags();
    static void RegisterResistanceTags();
    static void RegisterMiscAttributes();
    static void RegisterVitals();
    static void RegisterStatusEffectChances();
    static void RegisterStatusEffectDurations();
    static void RegisterConditions();
    static void RegisterConditionTriggers();

	static void RegisterStatusEffectAttributes();
	static void RegisterMinMaxTagMap();
	static void RegisterFlatDamageAttributes();
	static void RegisterPercentDamageAttributes();
	static void RegisterBaseDamageAttributes();
	static void RegisterAllAttributes();
	static void RegisterAllAttribute();

	/* ========================== */
	/* === Primary Attributes === */
	/* ========================== */
	static FGameplayTag Attributes_Primary_Strength;
	static FGameplayTag Attributes_Primary_Intelligence;
	static FGameplayTag Attributes_Primary_Dexterity;
	static FGameplayTag Attributes_Primary_Endurance;
	static FGameplayTag Attributes_Primary_Affliction;
	static FGameplayTag Attributes_Primary_Luck;
	static FGameplayTag Attributes_Primary_Covenant;

	/* =========================== */
	/* === Secondary Attributes === */
	/* =========================== */

	/** Defenses */
	static FGameplayTag Attributes_Secondary_Defenses_Armor;

	/** Health & Regen */
	static FGameplayTag Attributes_Secondary_Vital_MaxHealth;
	static FGameplayTag Attributes_Secondary_Vital_HealthRegenRate;
	static FGameplayTag Attributes_Secondary_Vital_HealthRegenAmount;
	static FGameplayTag Attributes_Secondary_Vital_MaxHealthRegenAmount;
	static FGameplayTag Attributes_Secondary_Vital_MaxHealthRegenRate;
	static FGameplayTag Attributes_Secondary_Vital_HealthReservedAmount;
	static FGameplayTag Attributes_Secondary_Vital_MaxHealthReservedAmount;
	static FGameplayTag Attributes_Secondary_Vital_HealthFlatReservedAmount;
	static FGameplayTag Attributes_Secondary_Vital_HealthPercentageReserved;
	static FGameplayTag Attributes_Secondary_Vital_MaxEffectiveHealth;

	/** Mana & Regen */
	static FGameplayTag Attributes_Secondary_Vital_MaxMana;
	static FGameplayTag Attributes_Secondary_Vital_ManaRegenRate;
	static FGameplayTag Attributes_Secondary_Vital_ManaRegenAmount;
	static FGameplayTag Attributes_Secondary_Vital_MaxManaRegenRate;
	static FGameplayTag Attributes_Secondary_Vital_MaxManaRegenAmount;
	static FGameplayTag Attributes_Secondary_Vital_ManaReservedAmount;
	static FGameplayTag Attributes_Secondary_Vital_MaxManaReservedAmount;
	static FGameplayTag Attributes_Secondary_Vital_ManaFlatReservedAmount;
	static FGameplayTag Attributes_Secondary_Vital_ManaPercentageReserved;
	static FGameplayTag Attributes_Secondary_Vital_MaxEffectiveMana;

	/** Stamina & Regen */
	static FGameplayTag Attributes_Secondary_Vital_MaxStamina;
	static FGameplayTag Attributes_Secondary_Vital_MaxEffectiveStamina;
	static FGameplayTag Attributes_Secondary_Vital_StaminaRegenRate;
	static FGameplayTag Attributes_Secondary_Vital_StaminaRegenAmount;
	static FGameplayTag Attributes_Secondary_Vital_MaxStaminaRegenRate;
	static FGameplayTag Attributes_Secondary_Vital_StaminaFlatReservedAmount;
	static FGameplayTag Attributes_Secondary_Vital_StaminaPercentageReserved;
	static FGameplayTag Attributes_Secondary_Vital_MaxStaminaRegenAmount;
	static FGameplayTag Attributes_Secondary_Vital_StaminaReservedAmount;
	static FGameplayTag Attributes_Secondary_Vital_MaxStaminaReservedAmount;
	static FGameplayTag Attributes_Secondary_Vital_StaminaDegen;

	/** Arcane Shield */
	static FGameplayTag Attributes_Secondary_Vital_ArcaneShield;
	static FGameplayTag Attributes_Secondary_Vital_ArcaneShieldRegenRate;
	static FGameplayTag Attributes_Secondary_Vital_MaxArcaneShieldRegenRate;
	static FGameplayTag Attributes_Secondary_Vital_MaxArcaneShieldRegenAmount;
	static FGameplayTag Attributes_Secondary_Vital_MaxArcaneShieldReservedAmount;
	static FGameplayTag Attributes_Secondary_Vital_ArcaneShieldReservedAmount;
	static FGameplayTag Attributes_Secondary_Vital_ArcaneShieldRegenAmount;
	static FGameplayTag Attributes_Secondary_Vital_ArcaneShieldFlatReservedAmount;
	static FGameplayTag Attributes_Secondary_Vital_ArcaneShieldPercentageReserved;

	/* =================== */
	/* === Damage Tags === */
	/* =================== */

	/** Min Damage */
	static FGameplayTag Attributes_Secondary_Damages_MinPhysicalDamage;
	static FGameplayTag Attributes_Secondary_Damages_MinFireDamage;
	static FGameplayTag Attributes_Secondary_Damages_MinIceDamage;
	static FGameplayTag Attributes_Secondary_Damages_MinLightDamage;
	static FGameplayTag Attributes_Secondary_Damages_MinLightningDamage;
	static FGameplayTag Attributes_Secondary_Damages_MinCorruptionDamage;

	/** Max Damage */
	static FGameplayTag Attributes_Secondary_Damages_MaxPhysicalDamage;
	static FGameplayTag Attributes_Secondary_Damages_MaxFireDamage;
	static FGameplayTag Attributes_Secondary_Damages_MaxIceDamage;
	static FGameplayTag Attributes_Secondary_Damages_MaxLightDamage;
	static FGameplayTag Attributes_Secondary_Damages_MaxLightningDamage;
	static FGameplayTag Attributes_Secondary_Damages_MaxCorruptionDamage;

	/** Damage Bonuses */
	static FGameplayTag Attributes_Secondary_BonusDamage_GlobalDamages;
	static FGameplayTag Attributes_Secondary_BonusDamage_PhysicalPercentBonus;
	static FGameplayTag Attributes_Secondary_BonusDamage_PhysicalFlatBonus;
	static FGameplayTag Attributes_Secondary_BonusDamage_FirePercentBonus;
	static FGameplayTag Attributes_Secondary_BonusDamage_FireFlatBonus;
	static FGameplayTag Attributes_Secondary_BonusDamage_IcePercentBonus;
	static FGameplayTag Attributes_Secondary_BonusDamage_IceFlatBonus;
	static FGameplayTag Attributes_Secondary_BonusDamage_LightPercentBonus;
	static FGameplayTag Attributes_Secondary_BonusDamage_LightFlatBonus;
	static FGameplayTag Attributes_Secondary_BonusDamage_LightningPercentBonus;
	static FGameplayTag Attributes_Secondary_BonusDamage_LightningFlatBonus;
	static FGameplayTag Attributes_Secondary_BonusDamage_CorruptionPercentBonus;
	static FGameplayTag Attributes_Secondary_BonusDamage_CorruptionFlatBonus;

	/* ======================= */
	/* === Resistance Tags === */
	/* ======================= */
	static FGameplayTag Attributes_Secondary_Resistances_GlobalDefenses;
	static FGameplayTag Attributes_Secondary_Resistances_Armour;
	static FGameplayTag Attributes_Secondary_Resistances_ArmourFlatBonus;
	static FGameplayTag Attributes_Secondary_Resistances_ArmourPercentBonus;

	static FGameplayTag Attributes_Secondary_Resistances_FireResistanceFlat;
	static FGameplayTag Attributes_Secondary_Resistances_IceResistanceFlat;
	static FGameplayTag Attributes_Secondary_Resistances_LightResistanceFlat;
	static FGameplayTag Attributes_Secondary_Resistances_LightningResistanceFlat;
	static FGameplayTag Attributes_Secondary_Resistances_CorruptionResistanceFlat;

	static FGameplayTag Attributes_Secondary_Resistances_FireResistancePercentage;
	static FGameplayTag Attributes_Secondary_Resistances_IceResistancePercentage;
	static FGameplayTag Attributes_Secondary_Resistances_LightResistancePercentage;
	static FGameplayTag Attributes_Secondary_Resistances_LightningResistancePercentage;
	static FGameplayTag Attributes_Secondary_Resistances_CorruptionResistancePercentage;

	static FGameplayTag Attributes_Secondary_Resistances_MaxFireResistance;
	static FGameplayTag Attributes_Secondary_Resistances_MaxIceResistance;
	static FGameplayTag Attributes_Secondary_Resistances_MaxLightResistance;
	static FGameplayTag Attributes_Secondary_Resistances_MaxLightningResistance;
	static FGameplayTag Attributes_Secondary_Resistances_MaxCorruptionResistance;

	static FGameplayTag Attributes_Secondary_Resistances_BlockStrength;

	/* ==================== */
	/* === Miscellaneous === */
	/* ==================== */
	static FGameplayTag Attributes_Secondary_Misc_Poise;
	static FGameplayTag Attributes_Secondary_Misc_Weight;
	static FGameplayTag Attributes_Secondary_Misc_StunRecovery;
	static FGameplayTag Attributes_Secondary_Misc_ManaCostChanges;
	static FGameplayTag Attributes_Secondary_Misc_CoolDown;
	static FGameplayTag Attributes_Secondary_Misc_LifeLeech;
	static FGameplayTag Attributes_Secondary_Misc_ManaLeech;
	static FGameplayTag Attributes_Secondary_Misc_MovementSpeed;
	static FGameplayTag Attributes_Secondary_Misc_LifeOnHit;
	static FGameplayTag Attributes_Secondary_Misc_ManaOnHit;
	static FGameplayTag Attributes_Secondary_Misc_StaminaOnHit;
	static FGameplayTag Attributes_Secondary_Misc_StaminaCostChanges;
	static FGameplayTag Attributes_Secondary_Misc_CritChance;
	static FGameplayTag Attributes_Secondary_Misc_CritMultiplier;
	static FGameplayTag Attributes_Secondary_Money_Gems;
	static FGameplayTag Attributes_Secondary_Misc_CombatAlignment;
	static FGameplayTag Relation_HostileToSource;

	/* ==================== */
	/* ====== Vitals ====== */
	/* ==================== */

	static FGameplayTag Attributes_Vital_Health;
	static FGameplayTag Attributes_Vital_Stamina;
	static FGameplayTag Attributes_Vital_Mana;

	/* ============================= */
	/* === Status Effect Chances === */
	/* ============================= */

	static FGameplayTag Attributes_Secondary_ChanceToApply_ChanceToBleed;
	static FGameplayTag Attributes_Secondary_ChanceToApply_ChanceToIgnite;
	static FGameplayTag Attributes_Secondary_ChanceToApply_ChanceToFreeze;
	static FGameplayTag Attributes_Secondary_ChanceToApply_ChanceToShock;
	static FGameplayTag Attributes_Secondary_ChanceToApply_ChanceToStun;
	static FGameplayTag Attributes_Secondary_ChanceToApply_ChanceToKnockBack;
	static FGameplayTag Attributes_Secondary_ChanceToApply_ChanceToPetrify;
	static FGameplayTag Attributes_Secondary_ChanceToApply_ChanceToPurify;
	static FGameplayTag Attributes_Secondary_ChanceToApply_ChanceToCorrupt;

	/* ============================= */
	/* === Status Effect Durations === */
	/* ============================= */

	static FGameplayTag Attributes_Secondary_Duration_BleedDuration;
	static FGameplayTag Attributes_Secondary_Duration_BurnDuration;
	static FGameplayTag Attributes_Secondary_Duration_FreezeDuration;
	static FGameplayTag Attributes_Secondary_Duration_ShockDuration;
	static FGameplayTag Attributes_Secondary_Duration_CorruptionDuration;
	static FGameplayTag Attributes_Secondary_Duration_PetrifyBuildUpDuration;
	static FGameplayTag Attributes_Secondary_Duration_PurifyDuration;

	/* ============================= */
	/* ===      Conditions        === */
	/* ============================= */

	/* === Basic Life/Death States === */
	static FGameplayTag Condition_Alive;
	static FGameplayTag Condition_Dead;
	static FGameplayTag Condition_NearDeathExperience;
	static FGameplayTag Condition_DeathPrevented;

	/* === Health/Mana/Stamina/Shield Thresholds === */
	static FGameplayTag Condition_OnFullHealth;
	static FGameplayTag Condition_OnLowHealth;
	static FGameplayTag Condition_OnFullMana;
	static FGameplayTag Condition_OnLowMana;
	static FGameplayTag Condition_OnFullStamina;
	static FGameplayTag Condition_OnLowStamina;
	static FGameplayTag Condition_OnFullArcaneShield;
	static FGameplayTag Condition_OnLowArcaneShield;

	/* === Combat Interaction States === */
	static FGameplayTag Condition_OnKill;
	static FGameplayTag Condition_OnCrit;
	static FGameplayTag Condition_RecentlyHit;
	static FGameplayTag Condition_RecentlyCrit;
	static FGameplayTag Condition_RecentlyBlocked;
	static FGameplayTag Condition_TakingDamage;
	static FGameplayTag Condition_DealingDamage;
	static FGameplayTag Condition_RecentlyUsedSkill;
	static FGameplayTag Condition_RecentlyAppliedBuff;
	static FGameplayTag Condition_RecentlyDispelled;

	/* === Action States === */
	static FGameplayTag Condition_UsingSkill;
	static FGameplayTag Condition_UsingMelee;
	static FGameplayTag Condition_UsingRanged;
	static FGameplayTag Condition_UsingSpell;
	static FGameplayTag Condition_UsingAura;
	static FGameplayTag Condition_UsingMovementSkill;
	static FGameplayTag Condition_WhileChanneling;
	static FGameplayTag Condition_WhileMoving;
	static FGameplayTag Condition_WhileStationary;
	static FGameplayTag Condition_Sprinting;

	/* === Buff/Debuff & Effect States === */
	static FGameplayTag Condition_BuffDurationBelow50;
	static FGameplayTag Condition_EffectDurationExpired;
	static FGameplayTag Condition_HasBuff;
	static FGameplayTag Condition_HasDebuff;

	/* === Enemy Target States === */
	static FGameplayTag Condition_TargetIsBoss;
	static FGameplayTag Condition_TargetIsMinion;
	static FGameplayTag Condition_TargetHasShield;
	static FGameplayTag Condition_TargetIsCasting;

	/* === Positional / Environmental === */
	static FGameplayTag Condition_NearAllies;
	static FGameplayTag Condition_NearEnemies;
	static FGameplayTag Condition_Alone;
	static FGameplayTag Condition_InLight;
	static FGameplayTag Condition_InDark;
	static FGameplayTag Condition_InDangerZone;

	/* === Ailment & Status Effects (Self) === */
	static FGameplayTag Condition_Self_Bleeding;
	static FGameplayTag Condition_Self_Stunned;
	static FGameplayTag Condition_Self_Frozen;
	static FGameplayTag Condition_Self_Shocked;
	static FGameplayTag Condition_Self_Burned;
	static FGameplayTag Condition_Self_Corrupted;
	static FGameplayTag Condition_Self_Purified;
	static FGameplayTag Condition_Self_Petrified;
	static FGameplayTag Condition_Self_CannotRegenHP;
	static FGameplayTag Condition_Self_CannotRegenStamina;
	static FGameplayTag Condition_Self_CannotRegenMana;
	static FGameplayTag Condition_Self_CannotHealHPAbove50Percent;
	static FGameplayTag Condition_Self_CannotHealStamina50Percent;
	static FGameplayTag Condition_Self_CannotHealMana50Percent;
	static FGameplayTag Condition_Self_ZeroArcaneShield;

	/* === Ailment & Status Effects (Target) === */
	static FGameplayTag Condition_Target_Bleeding;
	static FGameplayTag Condition_Target_Stunned;
	static FGameplayTag Condition_Target_Frozen;
	static FGameplayTag Condition_Target_Shocked;
	static FGameplayTag Condition_Target_Burned;
	static FGameplayTag Condition_Target_Corrupted;
	static FGameplayTag Condition_Target_Purified;
	static FGameplayTag Condition_Target_Petrified;

	/* === Immunity / Restrictions === */

	static FGameplayTag Condition_ImmuneToCC;
	static FGameplayTag Condition_CannotBeFrozen;
	static FGameplayTag Condition_CannotBeCorrupted;
	static FGameplayTag Condition_CannotBeBurned;
	static FGameplayTag Condition_CannotBeSlowed;
	static FGameplayTag Condition_CannotBeInterrupted;
	static FGameplayTag Condition_CannotBeKnockedBack;


	/* === Triggers   === */
	static FGameplayTag Condition_SkillRecentlyUsed;  
	static FGameplayTag Condition_HitTakenRecently;
	static FGameplayTag Condition_CritTakenRecently;
	static FGameplayTag Condition_KilledRecently;
	static FGameplayTag Condition_EnemyKilledRecently;
	static FGameplayTag Condition_HitWithPhysicalDamage;
	static FGameplayTag Condition_HitWithFireDamage;
	static FGameplayTag Condition_HitWithLightningDamage;
	static FGameplayTag Condition_HitWithProjectile;
	static FGameplayTag Condition_HitWithAoE;
	static FGameplayTag Condition_HasMeleeWeaponEquipped;
	static FGameplayTag Condition_HasBowEquipped;
	static FGameplayTag Condition_HasShieldEquipped;
	static FGameplayTag Condition_HasStaffEquipped;
	
	/* ============================= */
	/* === Tag-to-Attribute Mapping === */
	/* ============================= */

	static TMap<FGameplayTag, FGameplayAttribute> StatusEffectTagToAttributeMap;
	static TMap<FGameplayTag, FGameplayTag> TagsMinMax;
	static  TMap<FString, FGameplayAttribute> BaseDamageToAttributesMap;
	static  TMap<FString, FGameplayAttribute> FlatDamageToAttributesMap;
	static TMap<FString, FGameplayAttribute> PercentDamageToAttributesMap;
	static TMap<FString, FGameplayAttribute> AllAttributesMap;
	
	TArray<FGameplayAttribute> GetAttributesByTagPrefix(const FString& Prefix);


private:
	/** Singleton instance */
	static FPHGameplayTags GameplayTags;
};
