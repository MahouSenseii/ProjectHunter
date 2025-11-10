// Copyright@2024 Quentin Davis
#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "GameplayTagContainer.h"

/**
 * Singleton containing all native gameplay tags.
 */
struct FPHGameplayTags
{
public:
	/** Singleton accessor */
	static const FPHGameplayTags& Get() { return GameplayTags; }


	/** Initialization entry points */
	static void InitializeNativeGameplayTags();
	static void InitRegister();

	

	/** Registration helpers (public for clarity; keep signatures to avoid breaking .cpp) */
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
	static void RegisterAttributeToTagMappings();
	static void RegisterTagToAttributeMappings();
	static FGameplayAttribute GetAttributeFromTag(const FGameplayTag& Tag);
	
	// Extra registrars used by your map/automation build
	static void RegisterStatusEffectAttributes();
	static void RegisterMinMaxTagMap();
	static void RegisterBaseDamageAttributes();
	static void RegisterFlatDamageAttributes();
	static void RegisterPercentDamageAttributes();
	static void RegisterOffensiveTags();
	static void RegisterPiercingTags();
	static void RegisterReflectionTags();
	static void RegisterDamageConversionTags();
	static void RegisterStatusEffectAliases();
	

	// Keep both to preserve ABI/use-sites (you can deprecate one later)
	static void RegisterAllAttribute();

	/* =========================================================
	 * ================ ATTRIBUTE TAG DECLARATIONS =============
	 * =======================================================*/

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
	/* === Secondary: Defenses  === */
	/* =========================== */
	static FGameplayTag Attributes_Secondary_Defenses_Armor;

	/* ========================= */
	/* === Secondary: Vitals === */
	/* ========================= */
	// Health
	static FGameplayTag Attributes_Secondary_Vital_MaxHealth;
	static FGameplayTag Attributes_Secondary_Vital_MaxEffectiveHealth;
	static FGameplayTag Attributes_Secondary_Vital_HealthRegenRate;
	static FGameplayTag Attributes_Secondary_Vital_HealthRegenAmount;
	static FGameplayTag Attributes_Secondary_Vital_MaxHealthRegenRate;
	static FGameplayTag Attributes_Secondary_Vital_MaxHealthRegenAmount;
	static FGameplayTag Attributes_Secondary_Vital_HealthReservedAmount;
	static FGameplayTag Attributes_Secondary_Vital_MaxHealthReservedAmount;
	static FGameplayTag Attributes_Secondary_Vital_HealthFlatReservedAmount;
	static FGameplayTag Attributes_Secondary_Vital_HealthPercentageReserved;

	// Mana
	static FGameplayTag Attributes_Secondary_Vital_MaxMana;
	static FGameplayTag Attributes_Secondary_Vital_MaxEffectiveMana;
	static FGameplayTag Attributes_Secondary_Vital_ManaRegenRate;
	static FGameplayTag Attributes_Secondary_Vital_ManaRegenAmount;
	static FGameplayTag Attributes_Secondary_Vital_MaxManaRegenRate;
	static FGameplayTag Attributes_Secondary_Vital_MaxManaRegenAmount;
	static FGameplayTag Attributes_Secondary_Vital_ManaReservedAmount;
	static FGameplayTag Attributes_Secondary_Vital_MaxManaReservedAmount;
	static FGameplayTag Attributes_Secondary_Vital_ManaFlatReservedAmount;
	static FGameplayTag Attributes_Secondary_Vital_ManaPercentageReserved;

	// Stamina
	static FGameplayTag Attributes_Secondary_Vital_MaxStamina;
	static FGameplayTag Attributes_Secondary_Vital_MaxEffectiveStamina;
	static FGameplayTag Attributes_Secondary_Vital_StaminaRegenRate;
	static FGameplayTag Attributes_Secondary_Vital_StaminaRegenAmount;
	static FGameplayTag Attributes_Secondary_Vital_MaxStaminaRegenRate;
	static FGameplayTag Attributes_Secondary_Vital_MaxStaminaRegenAmount;
	static FGameplayTag Attributes_Secondary_Vital_StaminaReservedAmount;
	static FGameplayTag Attributes_Secondary_Vital_MaxStaminaReservedAmount;
	static FGameplayTag Attributes_Secondary_Vital_StaminaFlatReservedAmount;
	static FGameplayTag Attributes_Secondary_Vital_StaminaPercentageReserved;
	static FGameplayTag Attributes_Secondary_Vital_StaminaDegenRate;
	static FGameplayTag Attributes_Secondary_Vital_StaminaDegenAmount;

	// Arcane Shield
	static FGameplayTag Attributes_Secondary_Vital_ArcaneShield;
	static FGameplayTag Attributes_Secondary_Vital_MaxArcaneShield;
	static FGameplayTag Attributes_Secondary_Vital_MaxEffectiveArcaneShield;
	static FGameplayTag Attributes_Secondary_Vital_ArcaneShieldRegenRate;
	static FGameplayTag Attributes_Secondary_Vital_ArcaneShieldRegenAmount;
	static FGameplayTag Attributes_Secondary_Vital_MaxArcaneShieldRegenRate;
	static FGameplayTag Attributes_Secondary_Vital_MaxArcaneShieldRegenAmount;
	static FGameplayTag Attributes_Secondary_Vital_ArcaneShieldReservedAmount;
	static FGameplayTag Attributes_Secondary_Vital_MaxArcaneShieldReservedAmount;
	static FGameplayTag Attributes_Secondary_Vital_ArcaneShieldFlatReservedAmount;
	static FGameplayTag Attributes_Secondary_Vital_ArcaneShieldPercentageReserved;

	/* =================== */
	/* === Damage Tags === */
	/* =================== */
	// Min
	static FGameplayTag Attributes_Secondary_Damages_MinPhysicalDamage;
	static FGameplayTag Attributes_Secondary_Damages_MinFireDamage;
	static FGameplayTag Attributes_Secondary_Damages_MinIceDamage;
	static FGameplayTag Attributes_Secondary_Damages_MinLightDamage;
	static FGameplayTag Attributes_Secondary_Damages_MinLightningDamage;
	static FGameplayTag Attributes_Secondary_Damages_MinCorruptionDamage;

	// Max
	static FGameplayTag Attributes_Secondary_Damages_MaxPhysicalDamage;
	static FGameplayTag Attributes_Secondary_Damages_MaxFireDamage;
	static FGameplayTag Attributes_Secondary_Damages_MaxIceDamage;
	static FGameplayTag Attributes_Secondary_Damages_MaxLightDamage;
	static FGameplayTag Attributes_Secondary_Damages_MaxLightningDamage;
	static FGameplayTag Attributes_Secondary_Damages_MaxCorruptionDamage;

	// Bonuses
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
	static FGameplayTag Attributes_Secondary_Resistances_BlockStrength;
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

	/* =============================== */
	/* === Secondary: Offensive     === */
	/* =============================== */
	static FGameplayTag Attributes_Secondary_Offensive_AreaDamage;
	static FGameplayTag Attributes_Secondary_Offensive_AreaOfEffect;
	static FGameplayTag Attributes_Secondary_Offensive_AttackRange;
	static FGameplayTag Attributes_Secondary_Offensive_AttackSpeed;
	static FGameplayTag Attributes_Secondary_Offensive_CastSpeed;
	static FGameplayTag Attributes_Secondary_Offensive_CritChance;
	static FGameplayTag Attributes_Secondary_Offensive_CritMultiplier;
	static FGameplayTag Attributes_Secondary_Offensive_DamageOverTime;
	static FGameplayTag Attributes_Secondary_Offensive_ElementalDamage;
	static FGameplayTag Attributes_Secondary_Offensive_MeleeDamage;
	static FGameplayTag Attributes_Secondary_Offensive_SpellDamage;
	static FGameplayTag Attributes_Secondary_Offensive_ProjectileCount;
	static FGameplayTag Attributes_Secondary_Offensive_ProjectileSpeed;
	static FGameplayTag Attributes_Secondary_Offensive_RangedDamage;
	static FGameplayTag Attributes_Secondary_Offensive_SpellsCritChance;
	static FGameplayTag Attributes_Secondary_Offensive_SpellsCritMultiplier;
	static FGameplayTag Attributes_Secondary_Offensive_ChainCount;
	static FGameplayTag Attributes_Secondary_Offensive_ForkCount;
	static FGameplayTag Attributes_Secondary_Offensive_ChainDamage;
	static FGameplayTag Attributes_Secondary_Offensive_DamageBonusWhileAtFullHP;
	static FGameplayTag Attributes_Secondary_Offensive_DamageBonusWhileAtLowHP;

	/* =============================== */
	/* === Secondary: Piercing      === */
	/* =============================== */
	static FGameplayTag Attributes_Secondary_Piercing_Armour;
	static FGameplayTag Attributes_Secondary_Piercing_Fire;
	static FGameplayTag Attributes_Secondary_Piercing_Ice;
	static FGameplayTag Attributes_Secondary_Piercing_Light;
	static FGameplayTag Attributes_Secondary_Piercing_Lightning;
	static FGameplayTag Attributes_Secondary_Piercing_Corruption;

	/* =============================== */
	/* === Secondary: Reflection    === */
	/* =============================== */
	static FGameplayTag Attributes_Secondary_Reflection_Physical;
	static FGameplayTag Attributes_Secondary_Reflection_Elemental;
	static FGameplayTag Attributes_Secondary_Reflection_ChancePhysical;
	static FGameplayTag Attributes_Secondary_Reflection_ChanceElemental;

	/* ================================= */
	/* === Secondary: Conversions     === */
	/* ================================= */
	// Physical ->
	static FGameplayTag Attributes_Secondary_Conversion_PhysicalToFire;
	static FGameplayTag Attributes_Secondary_Conversion_PhysicalToIce;
	static FGameplayTag Attributes_Secondary_Conversion_PhysicalToLightning;
	static FGameplayTag Attributes_Secondary_Conversion_PhysicalToLight;
	static FGameplayTag Attributes_Secondary_Conversion_PhysicalToCorruption;
	// Fire ->
	static FGameplayTag Attributes_Secondary_Conversion_FireToPhysical;
	static FGameplayTag Attributes_Secondary_Conversion_FireToIce;
	static FGameplayTag Attributes_Secondary_Conversion_FireToLightning;
	static FGameplayTag Attributes_Secondary_Conversion_FireToLight;
	static FGameplayTag Attributes_Secondary_Conversion_FireToCorruption;
	// Ice ->
	static FGameplayTag Attributes_Secondary_Conversion_IceToPhysical;
	static FGameplayTag Attributes_Secondary_Conversion_IceToFire;
	static FGameplayTag Attributes_Secondary_Conversion_IceToLightning;
	static FGameplayTag Attributes_Secondary_Conversion_IceToLight;
	static FGameplayTag Attributes_Secondary_Conversion_IceToCorruption;
	// Lightning ->
	static FGameplayTag Attributes_Secondary_Conversion_LightningToPhysical;
	static FGameplayTag Attributes_Secondary_Conversion_LightningToFire;
	static FGameplayTag Attributes_Secondary_Conversion_LightningToIce;
	static FGameplayTag Attributes_Secondary_Conversion_LightningToLight;
	static FGameplayTag Attributes_Secondary_Conversion_LightningToCorruption;
	// Light ->
	static FGameplayTag Attributes_Secondary_Conversion_LightToPhysical;
	static FGameplayTag Attributes_Secondary_Conversion_LightToFire;
	static FGameplayTag Attributes_Secondary_Conversion_LightToIce;
	static FGameplayTag Attributes_Secondary_Conversion_LightToLightning;
	static FGameplayTag Attributes_Secondary_Conversion_LightToCorruption;
	// Corruption ->
	static FGameplayTag Attributes_Secondary_Conversion_CorruptionToPhysical;
	static FGameplayTag Attributes_Secondary_Conversion_CorruptionToFire;
	static FGameplayTag Attributes_Secondary_Conversion_CorruptionToIce;
	static FGameplayTag Attributes_Secondary_Conversion_CorruptionToLightning;
	static FGameplayTag Attributes_Secondary_Conversion_CorruptionToLight;

	/* ==================== */
	/* === Miscellaneous === */
	/* ==================== */
	static FGameplayTag Attributes_Secondary_Money_Gems;
	static FGameplayTag Attributes_Secondary_Misc_ComboCounter;
	static FGameplayTag Attributes_Secondary_Misc_Poise;
	static FGameplayTag Attributes_Secondary_Misc_PoiseResistance; // if you add later
	static FGameplayTag Attributes_Secondary_Misc_Weight;
	static FGameplayTag Attributes_Secondary_Misc_StunRecovery;
	static FGameplayTag Attributes_Secondary_Misc_MovementSpeed;
	static FGameplayTag Attributes_Secondary_Misc_CoolDown;
	static FGameplayTag Attributes_Secondary_Misc_ManaCostChanges;
	static FGameplayTag Attributes_Secondary_Misc_LifeLeech;
	static FGameplayTag Attributes_Secondary_Misc_ManaLeech;
	static FGameplayTag Attributes_Secondary_Misc_LifeOnHit;
	static FGameplayTag Attributes_Secondary_Misc_ManaOnHit;
	static FGameplayTag Attributes_Secondary_Misc_StaminaOnHit;
	static FGameplayTag Attributes_Secondary_Misc_StaminaCostChanges;
	static FGameplayTag Attributes_Secondary_Misc_CritChance;
	static FGameplayTag Attributes_Secondary_Misc_CritMultiplier;
	static FGameplayTag Attributes_Secondary_Misc_CombatAlignment; // ensure single definition
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
	// Aliases under Attributes.Secondary.* to match FindAttributeByTag usage
	static FGameplayTag Attributes_Secondary_Ailments_ChanceToBleed;
	static FGameplayTag Attributes_Secondary_Ailments_ChanceToIgnite;
	static FGameplayTag Attributes_Secondary_Ailments_ChanceToFreeze;
	static FGameplayTag Attributes_Secondary_Ailments_ChanceToShock;
	static FGameplayTag Attributes_Secondary_Ailments_ChanceToStun;
	static FGameplayTag Attributes_Secondary_Ailments_ChanceToKnockBack;
	static FGameplayTag Attributes_Secondary_Ailments_ChanceToPetrify;
	static FGameplayTag Attributes_Secondary_Ailments_ChanceToPurify;
	static FGameplayTag Attributes_Secondary_Ailments_ChanceToCorrupt;

	/* ============================= */
	/* === Status Effect Durations === */
	/* ============================= */
	// Aliases under Attributes.Secondary.* to match FindAttributeByTag usage
	static FGameplayTag Attributes_Secondary_Duration_Bleed;
	static FGameplayTag Attributes_Secondary_Duration_Burn;
	static FGameplayTag Attributes_Secondary_Duration_Freeze;
	static FGameplayTag Attributes_Secondary_Duration_Shock;
	static FGameplayTag Attributes_Secondary_Duration_Corruption;
	static FGameplayTag Attributes_Secondary_Duration_PetrifyBuildUp;
	static FGameplayTag Attributes_Secondary_Duration_Purify;

	/* ============================= */
	/* ===        Conditions      === */
	/* ============================= */
	// Life/Death
	static FGameplayTag Condition_Alive;
	static FGameplayTag Condition_Dead;
	static FGameplayTag Condition_NearDeathExperience;
	static FGameplayTag Condition_DeathPrevented;

	// Thresholds
	static FGameplayTag Condition_OnFullHealth;
	static FGameplayTag Condition_OnLowHealth;
	static FGameplayTag Condition_OnFullMana;
	static FGameplayTag Condition_OnLowMana;
	static FGameplayTag Condition_OnFullStamina;
	static FGameplayTag Condition_OnLowStamina;
	static FGameplayTag Condition_OnFullArcaneShield;
	static FGameplayTag Condition_OnLowArcaneShield;

	// Combat interaction states
	static FGameplayTag Condition_OnKill;
	static FGameplayTag Condition_OnCrit;
	static FGameplayTag Condition_RecentlyHit;
	static FGameplayTag Condition_RecentlyCrit;
	static FGameplayTag Condition_RecentlyBlocked;
	static FGameplayTag Condition_RecentlyReflected;
	static FGameplayTag Condition_TakingDamage;
	static FGameplayTag Condition_DealingDamage;
	static FGameplayTag Condition_RecentlyUsedSkill;
	static FGameplayTag Condition_RecentlyAppliedBuff;
	static FGameplayTag Condition_RecentlyDispelled;

	// Action states
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

	// Buff/Debuff & effect states
	static FGameplayTag Condition_BuffDurationBelow50;
	static FGameplayTag Condition_EffectDurationExpired;
	static FGameplayTag Condition_HasBuff;
	static FGameplayTag Condition_HasDebuff;

	// Enemy target states
	static FGameplayTag Condition_TargetIsBoss;
	static FGameplayTag Condition_TargetIsMinion;
	static FGameplayTag Condition_TargetHasShield;
	static FGameplayTag Condition_TargetIsCasting;

	// Positional / environmental
	static FGameplayTag Condition_NearAllies;
	static FGameplayTag Condition_NearEnemies;
	static FGameplayTag Condition_Alone;
	static FGameplayTag Condition_InLight;
	static FGameplayTag Condition_InDark;
	static FGameplayTag Condition_InDangerZone;

	// Ailment & status (self)
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
	static FGameplayTag Condition_Self_LowArcaneShield;
	static FGameplayTag Condition_Self_ZeroArcaneShield;
	static FGameplayTag Condition_Self_IsBlocking;

	// Ailment & status (target)
	static FGameplayTag Condition_Target_Bleeding;
	static FGameplayTag Condition_Target_Stunned;
	static FGameplayTag Condition_Target_Frozen;
	static FGameplayTag Condition_Target_Shocked;
	static FGameplayTag Condition_Target_Burned;
	static FGameplayTag Condition_Target_Corrupted;
	static FGameplayTag Condition_Target_Purified;
	static FGameplayTag Condition_Target_Petrified;
	static FGameplayTag Condition_Target_IsBlocking;

	// Immunities / restrictions
	static FGameplayTag Condition_ImmuneToCC;
	static FGameplayTag Condition_CannotBeFrozen;
	static FGameplayTag Condition_CannotBeCorrupted;
	static FGameplayTag Condition_CannotBeBurned;
	static FGameplayTag Condition_CannotBeSlowed;
	static FGameplayTag Condition_CannotBeInterrupted;
	static FGameplayTag Condition_CannotBeKnockedBack;

	/* ===================== */
	/* ===   Triggers    === */
	/* ===================== */
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
	static FGameplayTag Condition_InCombat;
	static FGameplayTag Condition_OutOfCombat;

	/* ===================== */
	/* ===    Effects    === */
	/* ===================== */
	static FGameplayTag Effect_Stamina_RegenActive;
	static FGameplayTag Effect_Stamina_DegenActive;
	static FGameplayTag Effect_ArcaneShield_RegenActive;
	static FGameplayTag Effect_Health_RegenActive;
	static FGameplayTag Effect_Mana_RegenActive;
	static FGameplayTag Effect_Health_DegenActive;
	static FGameplayTag Effect_Mana_DegenActive;

	

	/* ===================================== */
	/* === Tag → Attribute / Helper Maps === */
	/* ===================================== */
	static TMap<FGameplayTag, FGameplayAttribute> StatusEffectTagToAttributeMap;
	static TMap<FGameplayTag, FGameplayTag>        TagsMinMax;
	static TMap<FString, FGameplayAttribute>       BaseDamageToAttributesMap;
	static TMap<FString, FGameplayAttribute>       FlatDamageToAttributesMap;
	static TMap<FString, FGameplayAttribute>       PercentDamageToAttributesMap;
	static TMap<FString, FGameplayAttribute>       AllAttributesMap;
	static TMap<FGameplayAttribute, FGameplayTag> AttributeToTagMap;
	static TMap<FGameplayTag, FGameplayAttribute>  TagToAttributeMap;

private:
	static FPHGameplayTags GameplayTags;
	
};
