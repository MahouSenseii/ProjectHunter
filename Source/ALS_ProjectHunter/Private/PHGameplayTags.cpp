// Copyright@2024 Quentin Davis 

#include "PHGameplayTags.h"
#include "GameplayTagsManager.h"
#include "AbilitySystem/PHAttributeSet.h"



// ==============================
// Static maps / singleton
// ==============================
TMap<FGameplayTag, FGameplayAttribute> FPHGameplayTags::StatusEffectTagToAttributeMap;
TMap<FGameplayTag, FGameplayTag>        FPHGameplayTags::TagsMinMax;
TMap<FString, FGameplayAttribute>       FPHGameplayTags::FlatDamageToAttributesMap;
TMap<FString, FGameplayAttribute>       FPHGameplayTags::PercentDamageToAttributesMap;
TMap<FString, FGameplayAttribute>       FPHGameplayTags::BaseDamageToAttributesMap;
TMap<FString, FGameplayAttribute>       FPHGameplayTags::AllAttributesMap;
TMap<FGameplayAttribute, FGameplayTag>  FPHGameplayTags::AttributeToTagMap;
TMap<FGameplayTag, FGameplayAttribute>  FPHGameplayTags::TagToAttributeMap;

FPHGameplayTags FPHGameplayTags::GameplayTags;

#define DEFINE_GAMEPLAY_TAG(TAG) FGameplayTag FPHGameplayTags::TAG;

// ==============================
// Tag declarations (mirror .h)
// ==============================

// Primary
DEFINE_GAMEPLAY_TAG(Attributes_Primary_Strength)
DEFINE_GAMEPLAY_TAG(Attributes_Primary_Intelligence)
DEFINE_GAMEPLAY_TAG(Attributes_Primary_Dexterity)
DEFINE_GAMEPLAY_TAG(Attributes_Primary_Endurance)
DEFINE_GAMEPLAY_TAG(Attributes_Primary_Affliction)
DEFINE_GAMEPLAY_TAG(Attributes_Primary_Luck)
DEFINE_GAMEPLAY_TAG(Attributes_Primary_Covenant)

// Secondary: Defenses
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Defenses_Armor)

// Secondary: Vitals - Health
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_MaxHealth)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_MaxEffectiveHealth)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_HealthRegenRate)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_HealthRegenAmount)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_MaxHealthRegenRate)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_MaxHealthRegenAmount)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_HealthReservedAmount)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_MaxHealthReservedAmount)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_HealthFlatReservedAmount)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_HealthPercentageReserved)

// Secondary: Vitals - Mana
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_MaxMana)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_MaxEffectiveMana)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_ManaRegenRate)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_ManaRegenAmount)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_MaxManaRegenRate)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_MaxManaRegenAmount)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_ManaReservedAmount)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_MaxManaReservedAmount)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_ManaFlatReservedAmount)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_ManaPercentageReserved)

// Secondary: Vitals - Stamina
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_MaxStamina)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_MaxEffectiveStamina)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_StaminaRegenRate)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_StaminaRegenAmount)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_MaxStaminaRegenRate)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_MaxStaminaRegenAmount)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_StaminaReservedAmount)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_MaxStaminaReservedAmount)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_StaminaFlatReservedAmount)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_StaminaPercentageReserved)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_StaminaDegenRate)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_StaminaDegenAmount)

// Secondary: Vitals - Arcane Shield
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_ArcaneShield)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_MaxArcaneShield)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_MaxEffectiveArcaneShield)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_ArcaneShieldRegenRate)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_ArcaneShieldRegenAmount)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_MaxArcaneShieldRegenRate)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_MaxArcaneShieldRegenAmount)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_ArcaneShieldReservedAmount)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_MaxArcaneShieldReservedAmount)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_ArcaneShieldFlatReservedAmount)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_ArcaneShieldPercentageReserved)

// Damage: Base (Min/Max)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Damages_MinPhysicalDamage)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Damages_MinFireDamage)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Damages_MinIceDamage)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Damages_MinLightDamage)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Damages_MinLightningDamage)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Damages_MinCorruptionDamage)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Damages_MaxPhysicalDamage)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Damages_MaxFireDamage)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Damages_MaxIceDamage)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Damages_MaxLightDamage)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Damages_MaxLightningDamage)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Damages_MaxCorruptionDamage)

// Damage: Flat/Percent bonuses
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_BonusDamage_GlobalDamages)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_BonusDamage_PhysicalPercentBonus)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_BonusDamage_PhysicalFlatBonus)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_BonusDamage_FirePercentBonus)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_BonusDamage_FireFlatBonus)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_BonusDamage_IcePercentBonus)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_BonusDamage_IceFlatBonus)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_BonusDamage_LightPercentBonus)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_BonusDamage_LightFlatBonus)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_BonusDamage_LightningPercentBonus)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_BonusDamage_LightningFlatBonus)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_BonusDamage_CorruptionPercentBonus)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_BonusDamage_CorruptionFlatBonus)

// Resistances
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Resistances_GlobalDefenses)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Resistances_BlockStrength)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Resistances_Armour)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Resistances_ArmourFlatBonus)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Resistances_ArmourPercentBonus)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Resistances_FireResistanceFlat)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Resistances_FireResistancePercentage)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Resistances_MaxFireResistance)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Resistances_IceResistanceFlat)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Resistances_IceResistancePercentage)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Resistances_MaxIceResistance)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Resistances_LightResistanceFlat)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Resistances_LightResistancePercentage)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Resistances_MaxLightResistance)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Resistances_LightningResistanceFlat)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Resistances_LightningResistancePercentage)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Resistances_MaxLightningResistance)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Resistances_CorruptionResistanceFlat)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Resistances_CorruptionResistancePercentage)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Resistances_MaxCorruptionResistance)

// Secondary: Offensive
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Offensive_AreaDamage)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Offensive_AreaOfEffect)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Offensive_AttackRange)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Offensive_AttackSpeed)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Offensive_CastSpeed)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Offensive_CritChance)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Offensive_CritMultiplier)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Offensive_DamageOverTime)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Offensive_ElementalDamage)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Offensive_MeleeDamage)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Offensive_SpellDamage)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Offensive_ProjectileCount)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Offensive_ProjectileSpeed)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Offensive_RangedDamage)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Offensive_SpellsCritChance)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Offensive_SpellsCritMultiplier)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Offensive_ChainCount)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Offensive_ForkCount)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Offensive_ChainDamage)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Offensive_DamageBonusWhileAtFullHP)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Offensive_DamageBonusWhileAtLowHP)

// Secondary: Piercing
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Piercing_Armour)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Piercing_Fire)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Piercing_Ice)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Piercing_Light)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Piercing_Lightning)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Piercing_Corruption)

// Secondary: Reflection
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Reflection_Physical)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Reflection_Elemental)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Reflection_ChancePhysical)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Reflection_ChanceElemental)

// Secondary: Conversions
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Conversion_PhysicalToFire)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Conversion_PhysicalToIce)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Conversion_PhysicalToLightning)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Conversion_PhysicalToLight)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Conversion_PhysicalToCorruption)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Conversion_FireToPhysical)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Conversion_FireToIce)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Conversion_FireToLightning)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Conversion_FireToLight)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Conversion_FireToCorruption)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Conversion_IceToPhysical)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Conversion_IceToFire)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Conversion_IceToLightning)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Conversion_IceToLight)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Conversion_IceToCorruption)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Conversion_LightningToPhysical)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Conversion_LightningToFire)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Conversion_LightningToIce)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Conversion_LightningToLight)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Conversion_LightningToCorruption)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Conversion_LightToPhysical)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Conversion_LightToFire)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Conversion_LightToIce)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Conversion_LightToLightning)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Conversion_LightToCorruption)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Conversion_CorruptionToPhysical)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Conversion_CorruptionToFire)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Conversion_CorruptionToIce)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Conversion_CorruptionToLightning)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Conversion_CorruptionToLight)

// Misc
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Money_Gems)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Misc_Poise)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Misc_ComboCounter)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Misc_PoiseResistance)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Misc_Weight)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Misc_StunRecovery)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Misc_MovementSpeed)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Misc_CoolDown)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Misc_ManaCostChanges)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Misc_LifeLeech)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Misc_ManaLeech)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Misc_LifeOnHit)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Misc_ManaOnHit)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Misc_StaminaOnHit)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Misc_StaminaCostChanges)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Misc_CritChance)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Misc_CritMultiplier)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Misc_CombatAlignment)
DEFINE_GAMEPLAY_TAG(Relation_HostileToSource)

// Vitals (current values)
DEFINE_GAMEPLAY_TAG(Attributes_Vital_Health)
DEFINE_GAMEPLAY_TAG(Attributes_Vital_Stamina)
DEFINE_GAMEPLAY_TAG(Attributes_Vital_Mana)

// Status chances (aliases under Attributes.Secondary.*)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Ailments_ChanceToBleed)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Ailments_ChanceToIgnite)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Ailments_ChanceToFreeze)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Ailments_ChanceToShock)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Ailments_ChanceToStun)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Ailments_ChanceToKnockBack)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Ailments_ChanceToPetrify)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Ailments_ChanceToPurify)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Ailments_ChanceToCorrupt)

// Durations (aliases under Attributes.Secondary.*)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Duration_Bleed)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Duration_Burn)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Duration_Freeze)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Duration_Shock)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Duration_Corruption)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Duration_PetrifyBuildUp)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Duration_Purify)

// Conditions / Triggers / Effects
DEFINE_GAMEPLAY_TAG(Condition_Alive)
DEFINE_GAMEPLAY_TAG(Condition_Dead)
DEFINE_GAMEPLAY_TAG(Condition_NearDeathExperience)
DEFINE_GAMEPLAY_TAG(Condition_DeathPrevented)
DEFINE_GAMEPLAY_TAG(Condition_OnFullHealth)
DEFINE_GAMEPLAY_TAG(Condition_OnLowHealth)
DEFINE_GAMEPLAY_TAG(Condition_OnFullMana)
DEFINE_GAMEPLAY_TAG(Condition_OnLowMana)
DEFINE_GAMEPLAY_TAG(Condition_OnFullStamina)
DEFINE_GAMEPLAY_TAG(Condition_OnLowStamina)
DEFINE_GAMEPLAY_TAG(Condition_OnFullArcaneShield)
DEFINE_GAMEPLAY_TAG(Condition_OnLowArcaneShield)
DEFINE_GAMEPLAY_TAG(Condition_OnKill)
DEFINE_GAMEPLAY_TAG(Condition_OnCrit)
DEFINE_GAMEPLAY_TAG(Condition_RecentlyHit)
DEFINE_GAMEPLAY_TAG(Condition_RecentlyCrit)
DEFINE_GAMEPLAY_TAG(Condition_RecentlyBlocked)
DEFINE_GAMEPLAY_TAG(Condition_RecentlyReflected)
DEFINE_GAMEPLAY_TAG(Condition_TakingDamage)
DEFINE_GAMEPLAY_TAG(Condition_DealingDamage)
DEFINE_GAMEPLAY_TAG(Condition_RecentlyUsedSkill)
DEFINE_GAMEPLAY_TAG(Condition_RecentlyAppliedBuff)
DEFINE_GAMEPLAY_TAG(Condition_RecentlyDispelled)
DEFINE_GAMEPLAY_TAG(Condition_InCombat)
DEFINE_GAMEPLAY_TAG(Condition_OutOfCombat)
DEFINE_GAMEPLAY_TAG(Condition_UsingSkill)
DEFINE_GAMEPLAY_TAG(Condition_UsingMelee)
DEFINE_GAMEPLAY_TAG(Condition_UsingRanged)
DEFINE_GAMEPLAY_TAG(Condition_UsingSpell)
DEFINE_GAMEPLAY_TAG(Condition_UsingAura)
DEFINE_GAMEPLAY_TAG(Condition_UsingMovementSkill)
DEFINE_GAMEPLAY_TAG(Condition_WhileChanneling)
DEFINE_GAMEPLAY_TAG(Condition_WhileMoving)
DEFINE_GAMEPLAY_TAG(Condition_WhileStationary)
DEFINE_GAMEPLAY_TAG(Condition_Sprinting)
DEFINE_GAMEPLAY_TAG(Condition_BuffDurationBelow50)
DEFINE_GAMEPLAY_TAG(Condition_EffectDurationExpired)
DEFINE_GAMEPLAY_TAG(Condition_HasBuff)
DEFINE_GAMEPLAY_TAG(Condition_HasDebuff)
DEFINE_GAMEPLAY_TAG(Condition_TargetIsBoss)
DEFINE_GAMEPLAY_TAG(Condition_TargetIsMinion)
DEFINE_GAMEPLAY_TAG(Condition_TargetHasShield)
DEFINE_GAMEPLAY_TAG(Condition_TargetIsCasting)
DEFINE_GAMEPLAY_TAG(Condition_Target_IsBlocking)
DEFINE_GAMEPLAY_TAG(Condition_Target_Stunned)
DEFINE_GAMEPLAY_TAG(Condition_Target_Frozen)
DEFINE_GAMEPLAY_TAG(Condition_Target_Shocked)
DEFINE_GAMEPLAY_TAG(Condition_Target_Burned)
DEFINE_GAMEPLAY_TAG(Condition_Target_Corrupted)
DEFINE_GAMEPLAY_TAG(Condition_Target_Petrified)
DEFINE_GAMEPLAY_TAG(Condition_Target_Purified)
DEFINE_GAMEPLAY_TAG(Condition_Target_Bleeding)
DEFINE_GAMEPLAY_TAG(Condition_NearAllies)
DEFINE_GAMEPLAY_TAG(Condition_NearEnemies)
DEFINE_GAMEPLAY_TAG(Condition_Alone)
DEFINE_GAMEPLAY_TAG(Condition_InLight)
DEFINE_GAMEPLAY_TAG(Condition_InDark)
DEFINE_GAMEPLAY_TAG(Condition_InDangerZone)
DEFINE_GAMEPLAY_TAG(Condition_Self_Bleeding)
DEFINE_GAMEPLAY_TAG(Condition_Self_Stunned)
DEFINE_GAMEPLAY_TAG(Condition_Self_Frozen)
DEFINE_GAMEPLAY_TAG(Condition_Self_Shocked)
DEFINE_GAMEPLAY_TAG(Condition_Self_Burned)
DEFINE_GAMEPLAY_TAG(Condition_Self_Corrupted)
DEFINE_GAMEPLAY_TAG(Condition_Self_Purified)
DEFINE_GAMEPLAY_TAG(Condition_Self_Petrified)
DEFINE_GAMEPLAY_TAG(Condition_Self_CannotRegenHP)
DEFINE_GAMEPLAY_TAG(Condition_Self_CannotRegenStamina)
DEFINE_GAMEPLAY_TAG(Condition_Self_CannotRegenMana)
DEFINE_GAMEPLAY_TAG(Condition_Self_CannotHealHPAbove50Percent)
DEFINE_GAMEPLAY_TAG(Condition_Self_CannotHealStamina50Percent)
DEFINE_GAMEPLAY_TAG(Condition_Self_CannotHealMana50Percent)
DEFINE_GAMEPLAY_TAG(Condition_Self_LowArcaneShield)
DEFINE_GAMEPLAY_TAG(Condition_Self_ZeroArcaneShield)
DEFINE_GAMEPLAY_TAG(Condition_Self_IsBlocking)
DEFINE_GAMEPLAY_TAG(Condition_ImmuneToCC)
DEFINE_GAMEPLAY_TAG(Condition_CannotBeFrozen)
DEFINE_GAMEPLAY_TAG(Condition_CannotBeCorrupted)
DEFINE_GAMEPLAY_TAG(Condition_CannotBeBurned)
DEFINE_GAMEPLAY_TAG(Condition_CannotBeSlowed)
DEFINE_GAMEPLAY_TAG(Condition_CannotBeInterrupted)
DEFINE_GAMEPLAY_TAG(Condition_CannotBeKnockedBack)
DEFINE_GAMEPLAY_TAG(Condition_SkillRecentlyUsed)
DEFINE_GAMEPLAY_TAG(Condition_HitTakenRecently)
DEFINE_GAMEPLAY_TAG(Condition_CritTakenRecently)
DEFINE_GAMEPLAY_TAG(Condition_KilledRecently)
DEFINE_GAMEPLAY_TAG(Condition_EnemyKilledRecently)
DEFINE_GAMEPLAY_TAG(Condition_HitWithPhysicalDamage)
DEFINE_GAMEPLAY_TAG(Condition_HitWithFireDamage)
DEFINE_GAMEPLAY_TAG(Condition_HitWithLightningDamage)
DEFINE_GAMEPLAY_TAG(Condition_HitWithProjectile)
DEFINE_GAMEPLAY_TAG(Condition_HitWithAoE)
DEFINE_GAMEPLAY_TAG(Effect_Stamina_RegenActive)
DEFINE_GAMEPLAY_TAG(Effect_Stamina_DegenActive)
DEFINE_GAMEPLAY_TAG(Effect_Health_RegenActive)
DEFINE_GAMEPLAY_TAG(Effect_ArcaneShield_RegenActive)
DEFINE_GAMEPLAY_TAG(Effect_Mana_RegenActive)
DEFINE_GAMEPLAY_TAG(Effect_Health_DegenActive)
DEFINE_GAMEPLAY_TAG(Effect_Mana_DegenActive)

#undef DEFINE_GAMEPLAY_TAG

// ==============================
// Initialization entry points
// ==============================
void FPHGameplayTags::InitializeNativeGameplayTags()
{
	UGameplayTagsManager& TagsManager = UGameplayTagsManager::Get();

	InitRegister();
}

void FPHGameplayTags::InitRegister()
{
	RegisterPrimaryAttributes();
	RegisterSecondaryVitals();
	RegisterDamageTags();
	RegisterResistanceTags();
	RegisterMiscAttributes();
	RegisterVitals();
	RegisterStatusEffectChances();
	RegisterStatusEffectDurations();
	RegisterConditions();
	RegisterConditionTriggers();
	RegisterAttributeToTagMappings();
	RegisterOffensiveTags();
	RegisterPiercingTags();
	RegisterReflectionTags();
	RegisterDamageConversionTags();
	RegisterStatusEffectAliases();
	RegisterAllAttribute();
	RegisterTagToAttributeMappings();
}

// ==============================
// Registrars (grouped, tidy)
// ==============================
void FPHGameplayTags::RegisterPrimaryAttributes()
{
	UGameplayTagsManager& T = UGameplayTagsManager::Get();
	Attributes_Primary_Strength     = T.AddNativeGameplayTag("Attributes.Primary.Strength",     TEXT("Increases physical damage and slightly increases health."));
	Attributes_Primary_Intelligence = T.AddNativeGameplayTag("Attributes.Primary.Intelligence", TEXT("Increases mana and slightly increases elemental damage."));
	Attributes_Primary_Dexterity    = T.AddNativeGameplayTag("Attributes.Primary.Dexterity",    TEXT("Increases crit multi; slightly increases attack/cast speed."));
	Attributes_Primary_Endurance    = T.AddNativeGameplayTag("Attributes.Primary.Endurance",    TEXT("Increases stamina; slightly increases resistances."));
	Attributes_Primary_Affliction   = T.AddNativeGameplayTag("Attributes.Primary.Affliction",   TEXT("Increases damage over time; slightly increases effect duration."));
	Attributes_Primary_Luck         = T.AddNativeGameplayTag("Attributes.Primary.Luck",         TEXT("Increases ailment chance and drops."));
	Attributes_Primary_Covenant     = T.AddNativeGameplayTag("Attributes.Primary.Covenant",     TEXT("Improves summoned allies/minions."));
}

void FPHGameplayTags::RegisterSecondaryVitals()
{
	UGameplayTagsManager& T = UGameplayTagsManager::Get();

	// Health
	Attributes_Secondary_Vital_MaxHealth              = T.AddNativeGameplayTag("Attributes.Secondary.Vital.MaxHealth",              TEXT("Maximum health."));
	Attributes_Secondary_Vital_MaxEffectiveHealth     = T.AddNativeGameplayTag("Attributes.Secondary.Vital.MaxEffectiveHealth",     TEXT("Effective max health after reservations."));
	Attributes_Secondary_Vital_HealthRegenRate        = T.AddNativeGameplayTag("Attributes.Secondary.Vital.HealthRegenRate",        TEXT("Health regen rate."));
	Attributes_Secondary_Vital_HealthRegenAmount      = T.AddNativeGameplayTag("Attributes.Secondary.Vital.HealthRegenAmount",      TEXT("Health per tick."));
	Attributes_Secondary_Vital_MaxHealthRegenRate     = T.AddNativeGameplayTag("Attributes.Secondary.Vital.MaxHealthRegenRate",     TEXT("Max health regen rate."));
	Attributes_Secondary_Vital_MaxHealthRegenAmount   = T.AddNativeGameplayTag("Attributes.Secondary.Vital.MaxHealthRegenAmount",   TEXT("Max health per tick."));
	Attributes_Secondary_Vital_HealthReservedAmount   = T.AddNativeGameplayTag("Attributes.Secondary.Vital.HealthReservedAmount",   TEXT("Reserved health (unusable)."));
	Attributes_Secondary_Vital_MaxHealthReservedAmount= T.AddNativeGameplayTag("Attributes.Secondary.Vital.MaxHealthReservedAmount",TEXT("Max reserved health."));
	Attributes_Secondary_Vital_HealthFlatReservedAmount= T.AddNativeGameplayTag("Attributes.Secondary.Vital.HealthFlatReservedAmount",TEXT("Flat reserved health."));
	Attributes_Secondary_Vital_HealthPercentageReserved= T.AddNativeGameplayTag("Attributes.Secondary.Vital.HealthPercentageReserved",TEXT("% reserved health."));

	// Mana
	Attributes_Secondary_Vital_MaxMana                = T.AddNativeGameplayTag("Attributes.Secondary.Vital.MaxMana",                TEXT("Maximum mana."));
	Attributes_Secondary_Vital_MaxEffectiveMana       = T.AddNativeGameplayTag("Attributes.Secondary.Vital.MaxEffectiveMana",       TEXT("Effective max mana after reservations."));
	Attributes_Secondary_Vital_ManaRegenRate          = T.AddNativeGameplayTag("Attributes.Secondary.Vital.ManaRegenRate",          TEXT("Mana regen rate."));
	Attributes_Secondary_Vital_ManaRegenAmount        = T.AddNativeGameplayTag("Attributes.Secondary.Vital.ManaRegenAmount",        TEXT("Mana per tick."));
	Attributes_Secondary_Vital_MaxManaRegenRate       = T.AddNativeGameplayTag("Attributes.Secondary.Vital.MaxManaRegenRate",       TEXT("Max mana regen rate."));
	Attributes_Secondary_Vital_MaxManaRegenAmount     = T.AddNativeGameplayTag("Attributes.Secondary.Vital.MaxManaRegenAmount",     TEXT("Max mana per tick."));
	Attributes_Secondary_Vital_ManaReservedAmount     = T.AddNativeGameplayTag("Attributes.Secondary.Vital.ManaReservedAmount",     TEXT("Reserved mana (unusable)."));
	Attributes_Secondary_Vital_MaxManaReservedAmount  = T.AddNativeGameplayTag("Attributes.Secondary.Vital.MaxManaReservedAmount",  TEXT("Max reserved mana."));
	Attributes_Secondary_Vital_ManaFlatReservedAmount = T.AddNativeGameplayTag("Attributes.Secondary.Vital.ManaFlatReservedAmount", TEXT("Flat reserved mana."));
	Attributes_Secondary_Vital_ManaPercentageReserved = T.AddNativeGameplayTag("Attributes.Secondary.Vital.ManaPercentageReserved", TEXT("% reserved mana."));

	// Stamina
	Attributes_Secondary_Vital_MaxStamina             = T.AddNativeGameplayTag("Attributes.Secondary.Vital.MaxStamina",             TEXT("Max stamina."));
	Attributes_Secondary_Vital_MaxEffectiveStamina    = T.AddNativeGameplayTag("Attributes.Secondary.Vital.MaxEffectiveStamina",    TEXT("Effective max stamina."));
	Attributes_Secondary_Vital_StaminaRegenRate       = T.AddNativeGameplayTag("Attributes.Secondary.Vital.StaminaRegenRate",       TEXT("Stamina regen rate."));
	Attributes_Secondary_Vital_StaminaRegenAmount     = T.AddNativeGameplayTag("Attributes.Secondary.Vital.StaminaRegenAmount",     TEXT("Stamina per tick."));
	Attributes_Secondary_Vital_MaxStaminaRegenRate    = T.AddNativeGameplayTag("Attributes.Secondary.Vital.MaxStaminaRegenRate",    TEXT("Max stamina regen rate."));
	Attributes_Secondary_Vital_MaxStaminaRegenAmount  = T.AddNativeGameplayTag("Attributes.Secondary.Vital.MaxStaminaRegenAmount",  TEXT("Max stamina per tick."));
	Attributes_Secondary_Vital_StaminaReservedAmount  = T.AddNativeGameplayTag("Attributes.Secondary.Vital.StaminaReservedAmount",  TEXT("Reserved stamina (unusable)."));
	Attributes_Secondary_Vital_MaxStaminaReservedAmount= T.AddNativeGameplayTag("Attributes.Secondary.Vital.MaxStaminaReservedAmount",TEXT("Max reserved stamina."));
	Attributes_Secondary_Vital_StaminaFlatReservedAmount= T.AddNativeGameplayTag("Attributes.Secondary.Vital.StaminaFlatReservedAmount",TEXT("Flat reserved stamina."));
	Attributes_Secondary_Vital_StaminaPercentageReserved= T.AddNativeGameplayTag("Attributes.Secondary.Vital.StaminaPercentageReserved",TEXT("% reserved stamina."));
	Attributes_Secondary_Vital_StaminaDegenRate       = T.AddNativeGameplayTag("Attributes.Secondary.Vital.StaminaDegenRate",       TEXT("Stamina degeneration rate."));
	Attributes_Secondary_Vital_StaminaDegenAmount     = T.AddNativeGameplayTag("Attributes.Secondary.Vital.StaminaDegenAmount",     TEXT("Stamina degeneration amount."));

	// Arcane Shield
	Attributes_Secondary_Vital_ArcaneShield                 = T.AddNativeGameplayTag("Attributes.Secondary.Vital.ArcaneShield",                 TEXT("Current arcane shield."));
	Attributes_Secondary_Vital_MaxArcaneShield              = T.AddNativeGameplayTag("Attributes.Secondary.Vital.MaxArcaneShield",              TEXT("Max arcane shield."));
	Attributes_Secondary_Vital_MaxEffectiveArcaneShield     = T.AddNativeGameplayTag("Attributes.Secondary.Vital.MaxEffectiveArcaneShield",     TEXT("Effective max arcane shield."));
	Attributes_Secondary_Vital_ArcaneShieldRegenRate        = T.AddNativeGameplayTag("Attributes.Secondary.Vital.ArcaneShieldRegenRate",        TEXT("Arcane shield regen rate."));
	Attributes_Secondary_Vital_ArcaneShieldRegenAmount      = T.AddNativeGameplayTag("Attributes.Secondary.Vital.ArcaneShieldRegenAmount",      TEXT("Arcane shield per tick."));
	Attributes_Secondary_Vital_MaxArcaneShieldRegenRate     = T.AddNativeGameplayTag("Attributes.Secondary.Vital.MaxArcaneShieldRegenRate",     TEXT("Max arcane shield regen rate."));
	Attributes_Secondary_Vital_MaxArcaneShieldRegenAmount   = T.AddNativeGameplayTag("Attributes.Secondary.Vital.MaxArcaneShieldRegenAmount",   TEXT("Max arcane shield per tick."));
	Attributes_Secondary_Vital_ArcaneShieldReservedAmount   = T.AddNativeGameplayTag("Attributes.Secondary.Vital.ArcaneShieldReservedAmount",   TEXT("Reserved arcane shield."));
	Attributes_Secondary_Vital_MaxArcaneShieldReservedAmount= T.AddNativeGameplayTag("Attributes.Secondary.Vital.MaxArcaneShieldReservedAmount",TEXT("Max reserved arcane shield."));
	Attributes_Secondary_Vital_ArcaneShieldFlatReservedAmount= T.AddNativeGameplayTag("Attributes.Secondary.Vital.ArcaneShieldFlatReservedAmount",TEXT("Flat reserved arcane shield."));
	Attributes_Secondary_Vital_ArcaneShieldPercentageReserved= T.AddNativeGameplayTag("Attributes.Secondary.Vital.ArcaneShieldPercentageReserved",TEXT("% reserved arcane shield."));

	// Also register your regen/degeneration “effect” tags here (unchanged)
	Effect_Stamina_RegenActive = T.AddNativeGameplayTag("Effect.Stamina.RegenActive", TEXT("Stamina is regenerating."));
	Effect_Stamina_DegenActive = T.AddNativeGameplayTag("Effect.Stamina.DegenActive", TEXT("Stamina is degenerating."));
	Effect_Health_RegenActive  = T.AddNativeGameplayTag("Effect.Health.RegenActive",  TEXT("Health is regenerating."));
	Effect_Mana_RegenActive    = T.AddNativeGameplayTag("Effect.Mana.RegenActive",    TEXT("Mana is regenerating."));
	Effect_Health_DegenActive  = T.AddNativeGameplayTag("Effect.Health.DegenActive",  TEXT("Health is degenerating."));
	Effect_ArcaneShield_RegenActive = T.AddNativeGameplayTag("Effect.ArcaneShield.RegenActive", TEXT("Arcane shield is regenerating."));
	Effect_Mana_DegenActive    = T.AddNativeGameplayTag("Effect.Mana.DegenActive",    TEXT("Mana is degenerating."));
}

void FPHGameplayTags::RegisterDamageTags()
{
	UGameplayTagsManager& T = UGameplayTagsManager::Get();

	// Min
	Attributes_Secondary_Damages_MinPhysicalDamage   = T.AddNativeGameplayTag("Attributes.Secondary.Damage.Min.Physical",   TEXT("Min physical damage."));
	Attributes_Secondary_Damages_MinFireDamage       = T.AddNativeGameplayTag("Attributes.Secondary.Damage.Min.Fire",       TEXT("Min fire damage."));
	Attributes_Secondary_Damages_MinIceDamage        = T.AddNativeGameplayTag("Attributes.Secondary.Damage.Min.Ice",        TEXT("Min ice damage."));
	Attributes_Secondary_Damages_MinLightDamage      = T.AddNativeGameplayTag("Attributes.Secondary.Damage.Min.Light",      TEXT("Min light damage."));
	Attributes_Secondary_Damages_MinLightningDamage  = T.AddNativeGameplayTag("Attributes.Secondary.Damage.Min.Lightning",  TEXT("Min lightning damage."));
	Attributes_Secondary_Damages_MinCorruptionDamage = T.AddNativeGameplayTag("Attributes.Secondary.Damage.Min.Corruption", TEXT("Min corruption damage."));

	// Max
	Attributes_Secondary_Damages_MaxPhysicalDamage   = T.AddNativeGameplayTag("Attributes.Secondary.Damage.Max.Physical",   TEXT("Max physical damage."));
	Attributes_Secondary_Damages_MaxFireDamage       = T.AddNativeGameplayTag("Attributes.Secondary.Damage.Max.Fire",       TEXT("Max fire damage."));
	Attributes_Secondary_Damages_MaxIceDamage        = T.AddNativeGameplayTag("Attributes.Secondary.Damage.Max.Ice",        TEXT("Max ice damage."));
	Attributes_Secondary_Damages_MaxLightDamage      = T.AddNativeGameplayTag("Attributes.Secondary.Damage.Max.Light",      TEXT("Max light damage."));
	Attributes_Secondary_Damages_MaxLightningDamage  = T.AddNativeGameplayTag("Attributes.Secondary.Damage.Max.Lightning",  TEXT("Max lightning damage."));
	Attributes_Secondary_Damages_MaxCorruptionDamage = T.AddNativeGameplayTag("Attributes.Secondary.Damage.Max.Corruption", TEXT("Max corruption damage."));

	// Flat
	Attributes_Secondary_BonusDamage_PhysicalFlatBonus   = T.AddNativeGameplayTag("Attributes.Secondary.Damage.Flat.Physical",   TEXT("Flat physical bonus."));
	Attributes_Secondary_BonusDamage_FireFlatBonus       = T.AddNativeGameplayTag("Attributes.Secondary.Damage.Flat.Fire",       TEXT("Flat fire bonus."));
	Attributes_Secondary_BonusDamage_IceFlatBonus        = T.AddNativeGameplayTag("Attributes.Secondary.Damage.Flat.Ice",        TEXT("Flat ice bonus."));
	Attributes_Secondary_BonusDamage_LightFlatBonus      = T.AddNativeGameplayTag("Attributes.Secondary.Damage.Flat.Light",      TEXT("Flat light bonus."));
	Attributes_Secondary_BonusDamage_LightningFlatBonus  = T.AddNativeGameplayTag("Attributes.Secondary.Damage.Flat.Lightning",  TEXT("Flat lightning bonus."));
	Attributes_Secondary_BonusDamage_CorruptionFlatBonus = T.AddNativeGameplayTag("Attributes.Secondary.Damage.Flat.Corruption", TEXT("Flat corruption bonus."));

	// Percent
	Attributes_Secondary_BonusDamage_PhysicalPercentBonus   = T.AddNativeGameplayTag("Attributes.Secondary.Damage.Percent.Physical",   TEXT("Percent physical bonus."));
	Attributes_Secondary_BonusDamage_FirePercentBonus       = T.AddNativeGameplayTag("Attributes.Secondary.Damage.Percent.Fire",       TEXT("Percent fire bonus."));
	Attributes_Secondary_BonusDamage_IcePercentBonus        = T.AddNativeGameplayTag("Attributes.Secondary.Damage.Percent.Ice",        TEXT("Percent ice bonus."));
	Attributes_Secondary_BonusDamage_LightPercentBonus      = T.AddNativeGameplayTag("Attributes.Secondary.Damage.Percent.Light",      TEXT("Percent light bonus."));
	Attributes_Secondary_BonusDamage_LightningPercentBonus  = T.AddNativeGameplayTag("Attributes.Secondary.Damage.Percent.Lightning",  TEXT("Percent lightning bonus."));
	Attributes_Secondary_BonusDamage_CorruptionPercentBonus = T.AddNativeGameplayTag("Attributes.Secondary.Damage.Percent.Corruption", TEXT("Percent corruption bonus."));

	// Global
	Attributes_Secondary_BonusDamage_GlobalDamages = T.AddNativeGameplayTag("Attributes.Secondary.Damage.GlobalBonus", TEXT("Global damage bonus."));
}

void FPHGameplayTags::RegisterResistanceTags()
{
	UGameplayTagsManager& T = UGameplayTagsManager::Get();

	// Global & Armour
	Attributes_Secondary_Resistances_GlobalDefenses       = T.AddNativeGameplayTag("Attributes.Secondary.Resistance.GlobalDefenses", TEXT("Global defenses."));
	Attributes_Secondary_Resistances_Armour               = T.AddNativeGameplayTag("Attributes.Secondary.Resistance.Armour",         TEXT("Armour."));
	Attributes_Secondary_Resistances_BlockStrength        = T.AddNativeGameplayTag("Attributes.Secondary.Resistance.BlockStrength",  TEXT("Block strength."));
	Attributes_Secondary_Resistances_ArmourFlatBonus      = T.AddNativeGameplayTag("Attributes.Secondary.Resistance.Armour.Flat",    TEXT("Flat armour."));
	Attributes_Secondary_Resistances_ArmourPercentBonus   = T.AddNativeGameplayTag("Attributes.Secondary.Resistance.Armour.Percent", TEXT("Percent armour."));

	// Fire
	Attributes_Secondary_Resistances_FireResistanceFlat        = T.AddNativeGameplayTag("Attributes.Secondary.Resistance.Fire.Flat",     TEXT("Flat fire res."));
	Attributes_Secondary_Resistances_FireResistancePercentage  = T.AddNativeGameplayTag("Attributes.Secondary.Resistance.Fire.Percent",  TEXT("Percent fire res."));
	Attributes_Secondary_Resistances_MaxFireResistance         = T.AddNativeGameplayTag("Attributes.Secondary.Resistance.Fire.Max",      TEXT("Max fire res."));

	// Ice
	Attributes_Secondary_Resistances_IceResistanceFlat         = T.AddNativeGameplayTag("Attributes.Secondary.Resistance.Ice.Flat",      TEXT("Flat ice res."));
	Attributes_Secondary_Resistances_IceResistancePercentage   = T.AddNativeGameplayTag("Attributes.Secondary.Resistance.Ice.Percent",   TEXT("Percent ice res."));
	Attributes_Secondary_Resistances_MaxIceResistance          = T.AddNativeGameplayTag("Attributes.Secondary.Resistance.Ice.Max",       TEXT("Max ice res."));

	// Light
	Attributes_Secondary_Resistances_LightResistanceFlat       = T.AddNativeGameplayTag("Attributes.Secondary.Resistance.Light.Flat",    TEXT("Flat light res."));
	Attributes_Secondary_Resistances_LightResistancePercentage = T.AddNativeGameplayTag("Attributes.Secondary.Resistance.Light.Percent", TEXT("Percent light res."));
	Attributes_Secondary_Resistances_MaxLightResistance        = T.AddNativeGameplayTag("Attributes.Secondary.Resistance.Light.Max",     TEXT("Max light res."));

	// Lightning
	Attributes_Secondary_Resistances_LightningResistanceFlat       = T.AddNativeGameplayTag("Attributes.Secondary.Resistance.Lightning.Flat",    TEXT("Flat lightning res."));
	Attributes_Secondary_Resistances_LightningResistancePercentage = T.AddNativeGameplayTag("Attributes.Secondary.Resistance.Lightning.Percent", TEXT("Percent lightning res."));
	Attributes_Secondary_Resistances_MaxLightningResistance        = T.AddNativeGameplayTag("Attributes.Secondary.Resistance.Lightning.Max",     TEXT("Max lightning res."));

	// Corruption
	Attributes_Secondary_Resistances_CorruptionResistanceFlat       = T.AddNativeGameplayTag("Attributes.Secondary.Resistance.Corruption.Flat",    TEXT("Flat corruption res."));
	Attributes_Secondary_Resistances_CorruptionResistancePercentage = T.AddNativeGameplayTag("Attributes.Secondary.Resistance.Corruption.Percent", TEXT("Percent corruption res."));
	Attributes_Secondary_Resistances_MaxCorruptionResistance        = T.AddNativeGameplayTag("Attributes.Secondary.Resistance.Corruption.Max",     TEXT("Max corruption res."));
}

void FPHGameplayTags::RegisterMiscAttributes()
{
	UGameplayTagsManager& T = UGameplayTagsManager::Get();
	Attributes_Secondary_Misc_Poise            = T.AddNativeGameplayTag("Attributes.Secondary.Misc.Poise",            TEXT("Poise."));
	
	Attributes_Secondary_Misc_ComboCounter     = T.AddNativeGameplayTag("Attributes.Secondary.Misc.ComboCounter",            TEXT("Combo count."));
	Attributes_Secondary_Misc_StunRecovery     = T.AddNativeGameplayTag("Attributes.Secondary.Misc.StunRecovery",     TEXT("Stun recovery."));
	Attributes_Secondary_Misc_CoolDown         = T.AddNativeGameplayTag("Attributes.Secondary.Misc.CoolDown",         TEXT("Cooldown changes."));
	Attributes_Secondary_Misc_ManaCostChanges  = T.AddNativeGameplayTag("Attributes.Secondary.Misc.ManaCostChanges",  TEXT("Mana cost changes."));
	Attributes_Secondary_Misc_LifeLeech        = T.AddNativeGameplayTag("Attributes.Secondary.Misc.LifeLeech",        TEXT("Life leech."));
	Attributes_Secondary_Misc_ManaLeech        = T.AddNativeGameplayTag("Attributes.Secondary.Misc.ManaLeech",        TEXT("Mana leech."));
	Attributes_Secondary_Misc_MovementSpeed    = T.AddNativeGameplayTag("Attributes.Secondary.Misc.MovementSpeed",    TEXT("Movement speed."));
	Attributes_Secondary_Misc_LifeOnHit        = T.AddNativeGameplayTag("Attributes.Secondary.Misc.LifeOnHit",        TEXT("Life on hit."));
	Attributes_Secondary_Misc_ManaOnHit        = T.AddNativeGameplayTag("Attributes.Secondary.Misc.ManaOnHit",        TEXT("Mana on hit."));
	Attributes_Secondary_Misc_StaminaOnHit     = T.AddNativeGameplayTag("Attributes.Secondary.Misc.StaminaOnHit",     TEXT("Stamina on hit."));
	Attributes_Secondary_Misc_StaminaCostChanges= T.AddNativeGameplayTag("Attributes.Secondary.Misc.StaminaCostChanges", TEXT("Stamina cost changes."));
	Attributes_Secondary_Money_Gems            = T.AddNativeGameplayTag("Attributes.Secondary.Money.Gems",            TEXT("Gems."));
	Attributes_Secondary_Misc_CritChance       = T.AddNativeGameplayTag("Attributes.Secondary.Misc.CritChance",       TEXT("Crit chance (misc)."));
	Attributes_Secondary_Misc_CritMultiplier   = T.AddNativeGameplayTag("Attributes.Secondary.Misc.CritMultiplier",   TEXT("Crit multiplier (misc)."));

	// Ensure this is registered once (remove any duplicate you might have had)
	Attributes_Secondary_Misc_CombatAlignment  = T.AddNativeGameplayTag("Attributes.Secondary.Misc.CombatAlignment",  TEXT("Combat alignment."));
	Relation_HostileToSource                   = T.AddNativeGameplayTag("Relation.HostileToSource",                   TEXT("Hostile relation to source."));
}

void FPHGameplayTags::RegisterVitals()
{
	UGameplayTagsManager& T = UGameplayTagsManager::Get();
	Attributes_Vital_Health  = T.AddNativeGameplayTag("Attributes.Vital.Health",  TEXT("Current health."));
	Attributes_Vital_Stamina = T.AddNativeGameplayTag("Attributes.Vital.Stamina", TEXT("Current stamina."));
	Attributes_Vital_Mana    = T.AddNativeGameplayTag("Attributes.Vital.Mana",    TEXT("Current mana."));
}

void FPHGameplayTags::RegisterStatusEffectChances()
{
	UGameplayTagsManager& T = UGameplayTagsManager::Get();
	// Canonical StatusEffect.* tags (kept for UI/content)
	T.AddNativeGameplayTag("StatusEffect.ChanceToApply.Bleed",     TEXT("Chance to Bleed."));
	T.AddNativeGameplayTag("StatusEffect.ChanceToApply.Ignite",    TEXT("Chance to Ignite."));
	T.AddNativeGameplayTag("StatusEffect.ChanceToApply.Freeze",    TEXT("Chance to Freeze."));
	T.AddNativeGameplayTag("StatusEffect.ChanceToApply.Shock",     TEXT("Chance to Shock."));
	T.AddNativeGameplayTag("StatusEffect.ChanceToApply.Stun",      TEXT("Chance to Stun."));
	T.AddNativeGameplayTag("StatusEffect.ChanceToApply.KnockBack", TEXT("Chance to KnockBack."));
	T.AddNativeGameplayTag("StatusEffect.ChanceToApply.Petrify",   TEXT("Chance to Petrify."));
	T.AddNativeGameplayTag("StatusEffect.ChanceToApply.Purify",    TEXT("Chance to Purify."));
	T.AddNativeGameplayTag("StatusEffect.ChanceToApply.Corrupt",   TEXT("Chance to Corrupt."));
}

void FPHGameplayTags::RegisterStatusEffectDurations()
{
	UGameplayTagsManager& T = UGameplayTagsManager::Get();
	// Canonical StatusEffect.* tags (kept for UI/content)
	T.AddNativeGameplayTag("StatusEffect.Duration.Bleed",          TEXT("Bleed duration."));
	T.AddNativeGameplayTag("StatusEffect.Duration.Burn",           TEXT("Burn duration."));
	T.AddNativeGameplayTag("StatusEffect.Duration.Freeze",         TEXT("Freeze duration."));
	T.AddNativeGameplayTag("StatusEffect.Duration.Shock",          TEXT("Shock duration."));
	T.AddNativeGameplayTag("StatusEffect.Duration.Corruption",     TEXT("Corruption duration."));
	T.AddNativeGameplayTag("StatusEffect.Duration.PetrifyBuildUp", TEXT("Petrify buildup duration."));
	T.AddNativeGameplayTag("StatusEffect.Duration.Purify",         TEXT("Purify duration."));
}

void FPHGameplayTags::RegisterConditions()
{
	UGameplayTagsManager& T = UGameplayTagsManager::Get();

	// Life/Death
	Condition_Alive               = T.AddNativeGameplayTag("Condition.State.Alive",               TEXT("Alive."));
	Condition_Dead                = T.AddNativeGameplayTag("Condition.State.Dead",                TEXT("Dead."));
	Condition_NearDeathExperience = T.AddNativeGameplayTag("Condition.State.NearDeathExperience", TEXT("Near death."));
	Condition_DeathPrevented      = T.AddNativeGameplayTag("Condition.State.DeathPrevented",      TEXT("Death prevented."));

	// Thresholds
	Condition_OnFullHealth        = T.AddNativeGameplayTag("Condition.Threshold.OnFullHealth",     TEXT("Full health."));
	Condition_OnLowHealth         = T.AddNativeGameplayTag("Condition.Threshold.OnLowHealth",      TEXT("Low health."));
	Condition_OnFullMana          = T.AddNativeGameplayTag("Condition.Threshold.OnFullMana",       TEXT("Full mana."));
	Condition_OnLowMana           = T.AddNativeGameplayTag("Condition.Threshold.OnLowMana",        TEXT("Low mana."));
	Condition_OnFullStamina       = T.AddNativeGameplayTag("Condition.Threshold.OnFullStamina",    TEXT("Full stamina."));
	Condition_OnLowStamina        = T.AddNativeGameplayTag("Condition.Threshold.OnLowStamina",     TEXT("Low stamina."));
	Condition_OnFullArcaneShield  = T.AddNativeGameplayTag("Condition.Threshold.OnFullArcaneShield", TEXT("Full arcane shield."));
	Condition_OnLowArcaneShield   = T.AddNativeGameplayTag("Condition.Threshold.OnLowArcaneShield",  TEXT("Low arcane shield."));

	// Combat interaction states
	Condition_OnKill              = T.AddNativeGameplayTag("Condition.Trigger.OnKill",            TEXT("On kill."));
	Condition_OnCrit              = T.AddNativeGameplayTag("Condition.Trigger.OnCrit",            TEXT("On crit."));
	Condition_RecentlyHit         = T.AddNativeGameplayTag("Condition.Recently.ReceivedHit",      TEXT("Recently hit."));
	Condition_RecentlyCrit        = T.AddNativeGameplayTag("Condition.Recently.ReceivedCrit",     TEXT("Recently crit."));
	Condition_RecentlyBlocked     = T.AddNativeGameplayTag("Condition.Recently.Blocked",          TEXT("Recently blocked."));
	Condition_RecentlyReflected   = T.AddNativeGameplayTag("Condition.Recently.Reflected",        TEXT("Recently reflected."));
	Condition_TakingDamage        = T.AddNativeGameplayTag("Condition.State.TakingDamage",        TEXT("Taking damage."));
	Condition_DealingDamage       = T.AddNativeGameplayTag("Condition.State.DealingDamage",       TEXT("Dealing damage."));
	Condition_RecentlyUsedSkill   = T.AddNativeGameplayTag("Condition.Recently.UsedSkill",        TEXT("Recently used skill."));
	Condition_RecentlyAppliedBuff = T.AddNativeGameplayTag("Condition.Recently.AppliedBuff",      TEXT("Recently applied buff."));
	Condition_RecentlyDispelled   = T.AddNativeGameplayTag("Condition.Recently.Dispelled",        TEXT("Recently dispelled."));
	Condition_InCombat            = T.AddNativeGameplayTag("Condition.State.InCombat",            TEXT("In combat."));
	Condition_OutOfCombat         = T.AddNativeGameplayTag("Condition.State.OutOfCombat",         TEXT("Out of combat."));

	// Action states
	Condition_UsingSkill          = T.AddNativeGameplayTag("Condition.State.UsingSkill",          TEXT("Using skill."));
	Condition_UsingMelee          = T.AddNativeGameplayTag("Condition.State.UsingMelee",          TEXT("Using melee."));
	Condition_UsingRanged         = T.AddNativeGameplayTag("Condition.State.UsingRanged",         TEXT("Using ranged."));
	Condition_UsingSpell          = T.AddNativeGameplayTag("Condition.State.UsingSpell",          TEXT("Using spell."));
	Condition_UsingAura           = T.AddNativeGameplayTag("Condition.State.UsingAura",           TEXT("Using aura."));
	Condition_UsingMovementSkill  = T.AddNativeGameplayTag("Condition.State.UsingMovementSkill",  TEXT("Using movement skill."));
	Condition_WhileChanneling     = T.AddNativeGameplayTag("Condition.State.WhileChanneling",     TEXT("While channeling."));
	Condition_WhileMoving         = T.AddNativeGameplayTag("Condition.State.WhileMoving",         TEXT("While moving."));
	Condition_WhileStationary     = T.AddNativeGameplayTag("Condition.State.WhileStationary",     TEXT("While stationary."));
	Condition_Sprinting           = T.AddNativeGameplayTag("Condition.State.Sprinting",           TEXT("Sprinting."));

	// Buff/Debuff & effect states
	Condition_BuffDurationBelow50 = T.AddNativeGameplayTag("Condition.Buff.DurationBelow50",      TEXT("Buff < 50% duration."));
	Condition_EffectDurationExpired= T.AddNativeGameplayTag("Condition.Effect.Expired",           TEXT("Effect expired."));
	Condition_HasBuff             = T.AddNativeGameplayTag("Condition.Has.Buff",                  TEXT("Has buff."));
	Condition_HasDebuff           = T.AddNativeGameplayTag("Condition.Has.Debuff",                TEXT("Has debuff."));

	// Enemy target states
	Condition_TargetIsBoss        = T.AddNativeGameplayTag("Condition.Target.IsBoss",             TEXT("Target is boss."));
	Condition_TargetIsMinion      = T.AddNativeGameplayTag("Condition.Target.IsMinion",           TEXT("Target is minion."));
	Condition_TargetHasShield     = T.AddNativeGameplayTag("Condition.Target.HasShield",          TEXT("Target has shield."));
	Condition_TargetIsCasting     = T.AddNativeGameplayTag("Condition.Target.IsCasting",          TEXT("Target casting."));
	Condition_Target_IsBlocking   = T.AddNativeGameplayTag("Condition.Target.IsBlocking",         TEXT("Target blocking."));

	// Positional / environmental
	Condition_NearAllies          = T.AddNativeGameplayTag("Condition.Proximity.NearAllies",      TEXT("Near allies."));
	Condition_NearEnemies         = T.AddNativeGameplayTag("Condition.Proximity.NearEnemies",     TEXT("Near enemies."));
	Condition_Alone               = T.AddNativeGameplayTag("Condition.Proximity.Alone",           TEXT("Alone."));
	Condition_InLight             = T.AddNativeGameplayTag("Condition.Environment.InLight",       TEXT("In light."));
	Condition_InDark              = T.AddNativeGameplayTag("Condition.Environment.InDark",        TEXT("In dark."));
	Condition_InDangerZone        = T.AddNativeGameplayTag("Condition.Environment.InDangerZone",  TEXT("In danger zone."));

	// Ailment & status (self)
	Condition_Self_Bleeding                   = T.AddNativeGameplayTag("Condition.Self.Bleeding",                   TEXT("Self bleeding."));
	Condition_Self_Stunned                    = T.AddNativeGameplayTag("Condition.Self.Stunned",                    TEXT("Self stunned."));
	Condition_Self_Frozen                     = T.AddNativeGameplayTag("Condition.Self.Frozen",                     TEXT("Self frozen."));
	Condition_Self_Shocked                    = T.AddNativeGameplayTag("Condition.Self.Shocked",                    TEXT("Self shocked."));
	Condition_Self_Burned                     = T.AddNativeGameplayTag("Condition.Self.Burned",                     TEXT("Self burned."));
	Condition_Self_Corrupted                  = T.AddNativeGameplayTag("Condition.Self.Corrupted",                  TEXT("Self corrupted."));
	Condition_Self_Purified                   = T.AddNativeGameplayTag("Condition.Self.Purified",                   TEXT("Self purified."));
	Condition_Self_Petrified                  = T.AddNativeGameplayTag("Condition.Self.Petrified",                  TEXT("Self petrified."));
	Condition_Self_CannotRegenHP              = T.AddNativeGameplayTag("Condition.Self.CannotRegenHP",              TEXT("Cannot regen HP."));
	Condition_Self_CannotRegenStamina         = T.AddNativeGameplayTag("Condition.Self.CannotRegenStamina",         TEXT("Cannot regen Stamina."));
	Condition_Self_CannotRegenMana            = T.AddNativeGameplayTag("Condition.Self.CannotRegenMana",            TEXT("Cannot regen Mana."));
	Condition_Self_CannotHealHPAbove50Percent = T.AddNativeGameplayTag("Condition.Self.CannotHealHPAbove50Percent", TEXT("Cannot heal HP > 50%."));
	Condition_Self_CannotHealStamina50Percent = T.AddNativeGameplayTag("Condition.Self.CannotHealStamina50Percent", TEXT("Cannot heal Stamina > 50%."));
	Condition_Self_CannotHealMana50Percent    = T.AddNativeGameplayTag("Condition.Self.CannotHealMana50Percent",    TEXT("Cannot heal Mana > 50%."));
	Condition_Self_LowArcaneShield            = T.AddNativeGameplayTag("Condition.Self.LowArcaneShield",            TEXT("Low arcane shield."));
	Condition_Self_ZeroArcaneShield           = T.AddNativeGameplayTag("Condition.Self.ZeroArcaneShield",           TEXT("Zero arcane shield."));
	Condition_Self_IsBlocking                 = T.AddNativeGameplayTag("Condition.Self.IsBlocking",                 TEXT("Self is blocking."));

	// Ailment & status (target)
	Condition_Target_Bleeding  = T.AddNativeGameplayTag("Condition.Target.Bleeding",  TEXT("Target bleeding."));
	Condition_Target_Stunned   = T.AddNativeGameplayTag("Condition.Target.Stunned",   TEXT("Target stunned."));
	Condition_Target_Frozen    = T.AddNativeGameplayTag("Condition.Target.Frozen",    TEXT("Target frozen."));
	Condition_Target_Shocked   = T.AddNativeGameplayTag("Condition.Target.Shocked",   TEXT("Target shocked."));
	Condition_Target_Burned    = T.AddNativeGameplayTag("Condition.Target.Burned",    TEXT("Target burned."));
	Condition_Target_Corrupted = T.AddNativeGameplayTag("Condition.Target.Corrupted", TEXT("Target corrupted."));
	Condition_Target_Petrified = T.AddNativeGameplayTag("Condition.Target.Petrified", TEXT("Target petrified."));
	Condition_Target_Purified  = T.AddNativeGameplayTag("Condition.Target.Purified",  TEXT("Target purified."));
}

void FPHGameplayTags::RegisterConditionTriggers()
{
	UGameplayTagsManager& T = UGameplayTagsManager::Get();
	Condition_SkillRecentlyUsed   = T.AddNativeGameplayTag("Condition.Trigger.SkillRecentlyUsed",   TEXT("Skill recently used."));
	Condition_HitTakenRecently    = T.AddNativeGameplayTag("Condition.Trigger.HitTakenRecently",    TEXT("Hit taken recently."));
	Condition_CritTakenRecently   = T.AddNativeGameplayTag("Condition.Trigger.CritTakenRecently",   TEXT("Crit taken recently."));
	Condition_KilledRecently      = T.AddNativeGameplayTag("Condition.Trigger.KilledRecently",      TEXT("Killed recently."));
	Condition_EnemyKilledRecently = T.AddNativeGameplayTag("Condition.Trigger.EnemyKilledRecently", TEXT("Enemy killed recently."));
	Condition_HitWithPhysicalDamage= T.AddNativeGameplayTag("Condition.Trigger.HitWith.Physical",    TEXT("Hit with physical."));
	Condition_HitWithFireDamage   = T.AddNativeGameplayTag("Condition.Trigger.HitWith.Fire",        TEXT("Hit with fire."));
	Condition_HitWithLightningDamage= T.AddNativeGameplayTag("Condition.Trigger.HitWith.Lightning", TEXT("Hit with lightning."));
	Condition_HitWithProjectile   = T.AddNativeGameplayTag("Condition.Trigger.HitWith.Projectile",  TEXT("Hit with projectile."));
	Condition_HitWithAoE          = T.AddNativeGameplayTag("Condition.Trigger.HitWith.AoE",         TEXT("Hit with AoE."));
}

void FPHGameplayTags::RegisterOffensiveTags()
{
	UGameplayTagsManager& T = UGameplayTagsManager::Get();
	Attributes_Secondary_Offensive_AreaDamage              = T.AddNativeGameplayTag("Attributes.Secondary.Offensive.AreaDamage",              TEXT("Area damage."));
	Attributes_Secondary_Offensive_AreaOfEffect            = T.AddNativeGameplayTag("Attributes.Secondary.Offensive.AreaOfEffect",            TEXT("Area of effect."));
	Attributes_Secondary_Offensive_AttackRange             = T.AddNativeGameplayTag("Attributes.Secondary.Offensive.AttackRange",             TEXT("Attack range."));
	Attributes_Secondary_Offensive_AttackSpeed             = T.AddNativeGameplayTag("Attributes.Secondary.Offensive.AttackSpeed",             TEXT("Attack speed."));
	Attributes_Secondary_Offensive_CastSpeed               = T.AddNativeGameplayTag("Attributes.Secondary.Offensive.CastSpeed",               TEXT("Cast speed."));
	Attributes_Secondary_Offensive_CritChance              = T.AddNativeGameplayTag("Attributes.Secondary.Offensive.CritChance",              TEXT("Crit chance."));
	Attributes_Secondary_Offensive_CritMultiplier          = T.AddNativeGameplayTag("Attributes.Secondary.Offensive.CritMultiplier",          TEXT("Crit multiplier."));
	Attributes_Secondary_Offensive_DamageOverTime          = T.AddNativeGameplayTag("Attributes.Secondary.Offensive.DamageOverTime",          TEXT("Damage over time."));
	Attributes_Secondary_Offensive_ElementalDamage         = T.AddNativeGameplayTag("Attributes.Secondary.Offensive.ElementalDamage",         TEXT("Elemental damage."));
	Attributes_Secondary_Offensive_MeleeDamage             = T.AddNativeGameplayTag("Attributes.Secondary.Offensive.MeleeDamage",             TEXT("Melee damage."));
	Attributes_Secondary_Offensive_SpellDamage             = T.AddNativeGameplayTag("Attributes.Secondary.Offensive.SpellDamage",             TEXT("Spell damage."));
	Attributes_Secondary_Offensive_ProjectileCount         = T.AddNativeGameplayTag("Attributes.Secondary.Offensive.ProjectileCount",         TEXT("Projectile count."));
	Attributes_Secondary_Offensive_ProjectileSpeed         = T.AddNativeGameplayTag("Attributes.Secondary.Offensive.ProjectileSpeed",         TEXT("Projectile speed."));
	Attributes_Secondary_Offensive_RangedDamage            = T.AddNativeGameplayTag("Attributes.Secondary.Offensive.RangedDamage",            TEXT("Ranged damage."));
	Attributes_Secondary_Offensive_SpellsCritChance        = T.AddNativeGameplayTag("Attributes.Secondary.Offensive.SpellsCritChance",        TEXT("Spells crit chance."));
	Attributes_Secondary_Offensive_SpellsCritMultiplier    = T.AddNativeGameplayTag("Attributes.Secondary.Offensive.SpellsCritMultiplier",    TEXT("Spells crit multiplier."));
	Attributes_Secondary_Offensive_ChainCount              = T.AddNativeGameplayTag("Attributes.Secondary.Offensive.ChainCount",              TEXT("Chain count."));
	Attributes_Secondary_Offensive_ForkCount               = T.AddNativeGameplayTag("Attributes.Secondary.Offensive.ForkCount",               TEXT("Fork count."));
	Attributes_Secondary_Offensive_ChainDamage             = T.AddNativeGameplayTag("Attributes.Secondary.Offensive.ChainDamage",             TEXT("Chain damage."));
	Attributes_Secondary_Offensive_DamageBonusWhileAtFullHP= T.AddNativeGameplayTag("Attributes.Secondary.Offensive.DamageBonusWhileAtFullHP",TEXT("Bonus at full HP."));
	Attributes_Secondary_Offensive_DamageBonusWhileAtLowHP = T.AddNativeGameplayTag("Attributes.Secondary.Offensive.DamageBonusWhileAtLowHP", TEXT("Bonus at low HP."));
}

void FPHGameplayTags::RegisterPiercingTags()
{
	UGameplayTagsManager& T = UGameplayTagsManager::Get();
	Attributes_Secondary_Piercing_Armour     = T.AddNativeGameplayTag("Attributes.Secondary.Piercing.Armour",     TEXT("Armour piercing."));
	Attributes_Secondary_Piercing_Fire       = T.AddNativeGameplayTag("Attributes.Secondary.Piercing.Fire",       TEXT("Fire piercing."));
	Attributes_Secondary_Piercing_Ice        = T.AddNativeGameplayTag("Attributes.Secondary.Piercing.Ice",        TEXT("Ice piercing."));
	Attributes_Secondary_Piercing_Light      = T.AddNativeGameplayTag("Attributes.Secondary.Piercing.Light",      TEXT("Light piercing."));
	Attributes_Secondary_Piercing_Lightning  = T.AddNativeGameplayTag("Attributes.Secondary.Piercing.Lightning",  TEXT("Lightning piercing."));
	Attributes_Secondary_Piercing_Corruption = T.AddNativeGameplayTag("Attributes.Secondary.Piercing.Corruption", TEXT("Corruption piercing."));
}

void FPHGameplayTags::RegisterReflectionTags()
{
	UGameplayTagsManager& T = UGameplayTagsManager::Get();
	Attributes_Secondary_Reflection_Physical        = T.AddNativeGameplayTag("Attributes.Secondary.Reflection.Physical",        TEXT("Reflect physical."));
	Attributes_Secondary_Reflection_Elemental       = T.AddNativeGameplayTag("Attributes.Secondary.Reflection.Elemental",       TEXT("Reflect elemental."));
	Attributes_Secondary_Reflection_ChancePhysical  = T.AddNativeGameplayTag("Attributes.Secondary.Reflection.ChancePhysical",  TEXT("Chance to reflect physical."));
	Attributes_Secondary_Reflection_ChanceElemental = T.AddNativeGameplayTag("Attributes.Secondary.Reflection.ChanceElemental", TEXT("Chance to reflect elemental."));
}

void FPHGameplayTags::RegisterDamageConversionTags()
{
	UGameplayTagsManager& T = UGameplayTagsManager::Get();

	// Physical ->
	Attributes_Secondary_Conversion_PhysicalToFire       = T.AddNativeGameplayTag("Attributes.Secondary.Conversion.PhysicalToFire",       TEXT(""));
	Attributes_Secondary_Conversion_PhysicalToIce        = T.AddNativeGameplayTag("Attributes.Secondary.Conversion.PhysicalToIce",        TEXT(""));
	Attributes_Secondary_Conversion_PhysicalToLightning  = T.AddNativeGameplayTag("Attributes.Secondary.Conversion.PhysicalToLightning",  TEXT(""));
	Attributes_Secondary_Conversion_PhysicalToLight      = T.AddNativeGameplayTag("Attributes.Secondary.Conversion.PhysicalToLight",      TEXT(""));
	Attributes_Secondary_Conversion_PhysicalToCorruption = T.AddNativeGameplayTag("Attributes.Secondary.Conversion.PhysicalToCorruption", TEXT(""));

	// Fire ->
	Attributes_Secondary_Conversion_FireToPhysical       = T.AddNativeGameplayTag("Attributes.Secondary.Conversion.FireToPhysical",       TEXT(""));
	Attributes_Secondary_Conversion_FireToIce            = T.AddNativeGameplayTag("Attributes.Secondary.Conversion.FireToIce",            TEXT(""));
	Attributes_Secondary_Conversion_FireToLightning      = T.AddNativeGameplayTag("Attributes.Secondary.Conversion.FireToLightning",      TEXT(""));
	Attributes_Secondary_Conversion_FireToLight          = T.AddNativeGameplayTag("Attributes.Secondary.Conversion.FireToLight",          TEXT(""));
	Attributes_Secondary_Conversion_FireToCorruption     = T.AddNativeGameplayTag("Attributes.Secondary.Conversion.FireToCorruption",     TEXT(""));

	// Ice ->
	Attributes_Secondary_Conversion_IceToPhysical        = T.AddNativeGameplayTag("Attributes.Secondary.Conversion.IceToPhysical",        TEXT(""));
	Attributes_Secondary_Conversion_IceToFire            = T.AddNativeGameplayTag("Attributes.Secondary.Conversion.IceToFire",            TEXT(""));
	Attributes_Secondary_Conversion_IceToLightning       = T.AddNativeGameplayTag("Attributes.Secondary.Conversion.IceToLightning",       TEXT(""));
	Attributes_Secondary_Conversion_IceToLight           = T.AddNativeGameplayTag("Attributes.Secondary.Conversion.IceToLight",           TEXT(""));
	Attributes_Secondary_Conversion_IceToCorruption      = T.AddNativeGameplayTag("Attributes.Secondary.Conversion.IceToCorruption",      TEXT(""));

	// Lightning ->
	Attributes_Secondary_Conversion_LightningToPhysical  = T.AddNativeGameplayTag("Attributes.Secondary.Conversion.LightningToPhysical",  TEXT(""));
	Attributes_Secondary_Conversion_LightningToFire      = T.AddNativeGameplayTag("Attributes.Secondary.Conversion.LightningToFire",      TEXT(""));
	Attributes_Secondary_Conversion_LightningToIce       = T.AddNativeGameplayTag("Attributes.Secondary.Conversion.LightningToIce",       TEXT(""));
	Attributes_Secondary_Conversion_LightningToLight     = T.AddNativeGameplayTag("Attributes.Secondary.Conversion.LightningToLight",     TEXT(""));
	Attributes_Secondary_Conversion_LightningToCorruption= T.AddNativeGameplayTag("Attributes.Secondary.Conversion.LightningToCorruption",TEXT(""));

	// Light ->
	Attributes_Secondary_Conversion_LightToPhysical      = T.AddNativeGameplayTag("Attributes.Secondary.Conversion.LightToPhysical",      TEXT(""));
	Attributes_Secondary_Conversion_LightToFire          = T.AddNativeGameplayTag("Attributes.Secondary.Conversion.LightToFire",          TEXT(""));
	Attributes_Secondary_Conversion_LightToIce           = T.AddNativeGameplayTag("Attributes.Secondary.Conversion.LightToIce",           TEXT(""));
	Attributes_Secondary_Conversion_LightToLightning     = T.AddNativeGameplayTag("Attributes.Secondary.Conversion.LightToLightning",     TEXT(""));
	Attributes_Secondary_Conversion_LightToCorruption    = T.AddNativeGameplayTag("Attributes.Secondary.Conversion.LightToCorruption",    TEXT(""));

	// Corruption ->
	Attributes_Secondary_Conversion_CorruptionToPhysical = T.AddNativeGameplayTag("Attributes.Secondary.Conversion.CorruptionToPhysical", TEXT(""));
	Attributes_Secondary_Conversion_CorruptionToFire     = T.AddNativeGameplayTag("Attributes.Secondary.Conversion.CorruptionToFire",     TEXT(""));
	Attributes_Secondary_Conversion_CorruptionToIce      = T.AddNativeGameplayTag("Attributes.Secondary.Conversion.CorruptionToIce",      TEXT(""));
	Attributes_Secondary_Conversion_CorruptionToLightning= T.AddNativeGameplayTag("Attributes.Secondary.Conversion.CorruptionToLightning",TEXT(""));
	Attributes_Secondary_Conversion_CorruptionToLight    = T.AddNativeGameplayTag("Attributes.Secondary.Conversion.CorruptionToLight",    TEXT(""));
}

void FPHGameplayTags::RegisterStatusEffectAliases()
{
	// Bridge: expose aliases under Attributes.Secondary.* to match FindAttributeByTag()
	UGameplayTagsManager& T = UGameplayTagsManager::Get();

	// Chances
	Attributes_Secondary_Ailments_ChanceToBleed     = T.AddNativeGameplayTag("Attributes.Secondary.Ailments.ChanceToBleed",     TEXT(""));
	Attributes_Secondary_Ailments_ChanceToIgnite    = T.AddNativeGameplayTag("Attributes.Secondary.Ailments.ChanceToIgnite",    TEXT(""));
	Attributes_Secondary_Ailments_ChanceToFreeze    = T.AddNativeGameplayTag("Attributes.Secondary.Ailments.ChanceToFreeze",    TEXT(""));
	Attributes_Secondary_Ailments_ChanceToShock     = T.AddNativeGameplayTag("Attributes.Secondary.Ailments.ChanceToShock",     TEXT(""));
	Attributes_Secondary_Ailments_ChanceToStun      = T.AddNativeGameplayTag("Attributes.Secondary.Ailments.ChanceToStun",      TEXT(""));
	Attributes_Secondary_Ailments_ChanceToKnockBack = T.AddNativeGameplayTag("Attributes.Secondary.Ailments.ChanceToKnockBack", TEXT(""));
	Attributes_Secondary_Ailments_ChanceToPetrify   = T.AddNativeGameplayTag("Attributes.Secondary.Ailments.ChanceToPetrify",   TEXT(""));
	Attributes_Secondary_Ailments_ChanceToPurify    = T.AddNativeGameplayTag("Attributes.Secondary.Ailments.ChanceToPurify",    TEXT(""));
	Attributes_Secondary_Ailments_ChanceToCorrupt   = T.AddNativeGameplayTag("Attributes.Secondary.Ailments.ChanceToCorrupt",   TEXT(""));

	// Durations
	Attributes_Secondary_Duration_Bleed           = T.AddNativeGameplayTag("Attributes.Secondary.Duration.Bleed",           TEXT(""));
	Attributes_Secondary_Duration_Burn            = T.AddNativeGameplayTag("Attributes.Secondary.Duration.Burn",            TEXT(""));
	Attributes_Secondary_Duration_Freeze          = T.AddNativeGameplayTag("Attributes.Secondary.Duration.Freeze",          TEXT(""));
	Attributes_Secondary_Duration_Shock           = T.AddNativeGameplayTag("Attributes.Secondary.Duration.Shock",           TEXT(""));
	Attributes_Secondary_Duration_Corruption      = T.AddNativeGameplayTag("Attributes.Secondary.Duration.Corruption",      TEXT(""));
	Attributes_Secondary_Duration_PetrifyBuildUp  = T.AddNativeGameplayTag("Attributes.Secondary.Duration.PetrifyBuildUp",  TEXT(""));
	Attributes_Secondary_Duration_Purify          = T.AddNativeGameplayTag("Attributes.Secondary.Duration.Purify",          TEXT(""));
}

void FPHGameplayTags::RegisterAttributeToTagMappings()
{
	// Map attributes to their corresponding gameplay tags
	AttributeToTagMap.Add(UPHAttributeSet::GetMinPhysicalDamageAttribute(),   Attributes_Secondary_Damages_MinPhysicalDamage);
	AttributeToTagMap.Add(UPHAttributeSet::GetMaxPhysicalDamageAttribute(),   Attributes_Secondary_Damages_MaxPhysicalDamage);
	AttributeToTagMap.Add(UPHAttributeSet::GetMinFireDamageAttribute(),       Attributes_Secondary_Damages_MinFireDamage);
	AttributeToTagMap.Add(UPHAttributeSet::GetMaxFireDamageAttribute(),       Attributes_Secondary_Damages_MaxFireDamage);
	AttributeToTagMap.Add(UPHAttributeSet::GetMinIceDamageAttribute(),        Attributes_Secondary_Damages_MinIceDamage);
	AttributeToTagMap.Add(UPHAttributeSet::GetMaxIceDamageAttribute(),        Attributes_Secondary_Damages_MaxIceDamage);
	AttributeToTagMap.Add(UPHAttributeSet::GetMinLightningDamageAttribute(),  Attributes_Secondary_Damages_MinLightningDamage);
	AttributeToTagMap.Add(UPHAttributeSet::GetMaxLightningDamageAttribute(),  Attributes_Secondary_Damages_MaxLightningDamage);
	AttributeToTagMap.Add(UPHAttributeSet::GetMinLightDamageAttribute(),      Attributes_Secondary_Damages_MinLightDamage);
	AttributeToTagMap.Add(UPHAttributeSet::GetMaxLightDamageAttribute(),      Attributes_Secondary_Damages_MaxLightDamage);
	AttributeToTagMap.Add(UPHAttributeSet::GetMinCorruptionDamageAttribute(), Attributes_Secondary_Damages_MinCorruptionDamage);
	AttributeToTagMap.Add(UPHAttributeSet::GetMaxCorruptionDamageAttribute(), Attributes_Secondary_Damages_MaxCorruptionDamage);
}


void FPHGameplayTags::RegisterTagToAttributeMappings()
{
    TagToAttributeMap.Empty();

    // Safety check - ensure AttributeSet is ready
    if (!UPHAttributeSet::GetHealthAttribute().IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("❌ AttributeSet not ready - skipping tag mappings"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("=== Registering Tag-to-Attribute Mappings ==="));

    // ===========================
    // Vitals - Current Values 
    // ===========================
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Vital.Health")), UPHAttributeSet::GetHealthAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Vital.Mana")), UPHAttributeSet::GetManaAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Vital.Stamina")), UPHAttributeSet::GetStaminaAttribute());

    // ===========================
    // Vitals - Max Values 
    // ===========================
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Vital.MaxHealth")), UPHAttributeSet::GetMaxHealthAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Vital.MaxMana")), UPHAttributeSet::GetMaxManaAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Vital.MaxStamina")), UPHAttributeSet::GetMaxStaminaAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Vital.ArcaneShield")), UPHAttributeSet::GetArcaneShieldAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Vital.MaxArcaneShield")), UPHAttributeSet::GetMaxArcaneShieldAttribute());

    // ===========================
    // Damage Types
    // ===========================
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Damage.GlobalBonus")), UPHAttributeSet::GetGlobalDamagesAttribute());
    
    // Max Damage
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Damage.Max.Physical")), UPHAttributeSet::GetMaxPhysicalDamageAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Damage.Max.Fire")), UPHAttributeSet::GetMaxFireDamageAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Damage.Max.Ice")), UPHAttributeSet::GetMaxIceDamageAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Damage.Max.Lightning")), UPHAttributeSet::GetMaxLightningDamageAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Damage.Max.Light")), UPHAttributeSet::GetMaxLightDamageAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Damage.Max.Corruption")), UPHAttributeSet::GetMaxCorruptionDamageAttribute());

    // Min Damage
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Damage.Min.Physical")), UPHAttributeSet::GetMinPhysicalDamageAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Damage.Min.Fire")), UPHAttributeSet::GetMinFireDamageAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Damage.Min.Ice")), UPHAttributeSet::GetMinIceDamageAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Damage.Min.Lightning")), UPHAttributeSet::GetMinLightningDamageAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Damage.Min.Light")), UPHAttributeSet::GetMinLightDamageAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Damage.Min.Corruption")), UPHAttributeSet::GetMinCorruptionDamageAttribute());

    // ===========================
    // Damage Bonuses
    // ===========================
    
    // Flat Damage Bonuses
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Damage.Flat.Physical")), UPHAttributeSet::GetPhysicalFlatBonusAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Damage.Flat.Fire")), UPHAttributeSet::GetFireFlatBonusAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Damage.Flat.Ice")), UPHAttributeSet::GetIceFlatBonusAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Damage.Flat.Lightning")), UPHAttributeSet::GetLightningFlatBonusAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Damage.Flat.Light")), UPHAttributeSet::GetLightFlatBonusAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Damage.Flat.Corruption")), UPHAttributeSet::GetCorruptionFlatBonusAttribute());

    // Percent Damage Bonuses
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Damage.Percent.Physical")), UPHAttributeSet::GetPhysicalPercentBonusAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Damage.Percent.Fire")), UPHAttributeSet::GetFirePercentBonusAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Damage.Percent.Ice")), UPHAttributeSet::GetIcePercentBonusAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Damage.Percent.Lightning")), UPHAttributeSet::GetLightningPercentBonusAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Damage.Percent.Light")), UPHAttributeSet::GetLightPercentBonusAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Damage.Percent.Corruption")), UPHAttributeSet::GetCorruptionPercentBonusAttribute());

    // ===========================
    // Resistances
    // ===========================
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Resistance.GlobalDefenses")), UPHAttributeSet::GetGlobalDefensesAttribute());
    
    // Flat Resistances
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Resistance.Armour.Flat")), UPHAttributeSet::GetArmourFlatBonusAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Resistance.Fire.Flat")), UPHAttributeSet::GetFireResistanceFlatBonusAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Resistance.Ice.Flat")), UPHAttributeSet::GetIceResistanceFlatBonusAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Resistance.Lightning.Flat")), UPHAttributeSet::GetLightningResistanceFlatBonusAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Resistance.Light.Flat")), UPHAttributeSet::GetLightResistanceFlatBonusAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Resistance.Corruption.Flat")), UPHAttributeSet::GetCorruptionResistanceFlatBonusAttribute());

    // Percent Resistances
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Resistance.Armour.Percent")), UPHAttributeSet::GetArmourPercentBonusAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Resistance.Fire.Percent")), UPHAttributeSet::GetFireResistancePercentBonusAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Resistance.Ice.Percent")), UPHAttributeSet::GetIceResistancePercentBonusAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Resistance.Lightning.Percent")), UPHAttributeSet::GetLightningResistancePercentBonusAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Resistance.Light.Percent")), UPHAttributeSet::GetLightResistancePercentBonusAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Resistance.Corruption.Percent")), UPHAttributeSet::GetCorruptionResistancePercentBonusAttribute());

    // ===========================
    // Primary Stats
    // ===========================
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Primary.Strength")), UPHAttributeSet::GetStrengthAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Primary.Intelligence")), UPHAttributeSet::GetIntelligenceAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Primary.Dexterity")), UPHAttributeSet::GetDexterityAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Primary.Endurance")), UPHAttributeSet::GetEnduranceAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Primary.Affliction")), UPHAttributeSet::GetAfflictionAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Primary.Luck")), UPHAttributeSet::GetLuckAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Primary.Covenant")), UPHAttributeSet::GetCovenantAttribute());

    // ===========================
    // Regeneration
    // ===========================
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Vital.HealthRegenAmount")), UPHAttributeSet::GetHealthRegenAmountAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Vital.HealthRegenRate")), UPHAttributeSet::GetHealthRegenRateAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Vital.ManaRegenAmount")), UPHAttributeSet::GetManaRegenAmountAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Vital.ManaRegenRate")), UPHAttributeSet::GetManaRegenRateAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Vital.StaminaRegenAmount")), UPHAttributeSet::GetStaminaRegenAmountAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Vital.StaminaRegenRate")), UPHAttributeSet::GetStaminaRegenRateAttribute());

    // ===========================
    // Degeneration
    // ===========================
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Vital.StaminaDegenAmount")), UPHAttributeSet::GetStaminaDegenAmountAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Vital.StaminaDegenRate")), UPHAttributeSet::GetStaminaDegenRateAttribute());

    UE_LOG(LogTemp, Log, TEXT("✓ Tag-to-Attribute mappings initialized with %d entries"), TagToAttributeMap.Num());
}

FGameplayAttribute FPHGameplayTags::GetAttributeFromTag(const FGameplayTag& Tag)
{
	if (const FGameplayAttribute* Attr = TagToAttributeMap.Find(Tag))
	{
		return *Attr;
	}

	return FGameplayAttribute();
}

// ==============================
// Helper maps
// ==============================
void FPHGameplayTags::RegisterStatusEffectAttributes()
{
	// Chances → FGameplayAttribute
	StatusEffectTagToAttributeMap.Add(Attributes_Secondary_Ailments_ChanceToBleed,     UPHAttributeSet::GetChanceToBleedAttribute());
	StatusEffectTagToAttributeMap.Add(Attributes_Secondary_Ailments_ChanceToIgnite,    UPHAttributeSet::GetChanceToIgniteAttribute());
	StatusEffectTagToAttributeMap.Add(Attributes_Secondary_Ailments_ChanceToFreeze,    UPHAttributeSet::GetChanceToFreezeAttribute());
	StatusEffectTagToAttributeMap.Add(Attributes_Secondary_Ailments_ChanceToShock,     UPHAttributeSet::GetChanceToShockAttribute());
	StatusEffectTagToAttributeMap.Add(Attributes_Secondary_Ailments_ChanceToStun,      UPHAttributeSet::GetChanceToStunAttribute());
	StatusEffectTagToAttributeMap.Add(Attributes_Secondary_Ailments_ChanceToKnockBack, UPHAttributeSet::GetChanceToKnockBackAttribute());
	StatusEffectTagToAttributeMap.Add(Attributes_Secondary_Ailments_ChanceToPetrify,   UPHAttributeSet::GetChanceToPetrifyAttribute());
	StatusEffectTagToAttributeMap.Add(Attributes_Secondary_Ailments_ChanceToPurify,    UPHAttributeSet::GetChanceToPurifyAttribute());
	StatusEffectTagToAttributeMap.Add(Attributes_Secondary_Ailments_ChanceToCorrupt,   UPHAttributeSet::GetChanceToCorruptAttribute());

	// Durations → FGameplayAttribute
	StatusEffectTagToAttributeMap.Add(Attributes_Secondary_Duration_Bleed,          UPHAttributeSet::GetBleedDurationAttribute());
	StatusEffectTagToAttributeMap.Add(Attributes_Secondary_Duration_Burn,           UPHAttributeSet::GetBurnDurationAttribute());
	StatusEffectTagToAttributeMap.Add(Attributes_Secondary_Duration_Freeze,         UPHAttributeSet::GetFreezeDurationAttribute());
	StatusEffectTagToAttributeMap.Add(Attributes_Secondary_Duration_Shock,          UPHAttributeSet::GetShockDurationAttribute());
	StatusEffectTagToAttributeMap.Add(Attributes_Secondary_Duration_Corruption,     UPHAttributeSet::GetCorruptionDurationAttribute());
	StatusEffectTagToAttributeMap.Add(Attributes_Secondary_Duration_PetrifyBuildUp, UPHAttributeSet::GetPetrifyBuildUpDurationAttribute());
	StatusEffectTagToAttributeMap.Add(Attributes_Secondary_Duration_Purify,         UPHAttributeSet::GetPurifyDurationAttribute());
}

void FPHGameplayTags::RegisterMinMaxTagMap()
{
	// Primary map to itself (no min/max pair)
	TagsMinMax.Add(Attributes_Primary_Strength,     Attributes_Primary_Strength);
	TagsMinMax.Add(Attributes_Primary_Intelligence, Attributes_Primary_Intelligence);
	TagsMinMax.Add(Attributes_Primary_Endurance,    Attributes_Primary_Endurance);
	TagsMinMax.Add(Attributes_Primary_Affliction,   Attributes_Primary_Affliction);
	TagsMinMax.Add(Attributes_Primary_Dexterity,    Attributes_Primary_Dexterity);
	TagsMinMax.Add(Attributes_Primary_Luck,         Attributes_Primary_Luck);
	TagsMinMax.Add(Attributes_Primary_Covenant,     Attributes_Primary_Covenant);

	// Vitals pairs
	TagsMinMax.Add(Attributes_Secondary_Vital_HealthRegenRate,       Attributes_Secondary_Vital_MaxHealthRegenRate);
	TagsMinMax.Add(Attributes_Secondary_Vital_HealthRegenAmount,     Attributes_Secondary_Vital_MaxHealthRegenAmount);
	TagsMinMax.Add(Attributes_Secondary_Vital_HealthReservedAmount,  Attributes_Secondary_Vital_MaxHealthReservedAmount);

	TagsMinMax.Add(Attributes_Secondary_Vital_ManaRegenRate,         Attributes_Secondary_Vital_MaxManaRegenRate);
	TagsMinMax.Add(Attributes_Secondary_Vital_ManaRegenAmount,       Attributes_Secondary_Vital_MaxManaRegenAmount);
	TagsMinMax.Add(Attributes_Secondary_Vital_ManaReservedAmount,    Attributes_Secondary_Vital_MaxManaReservedAmount);

	TagsMinMax.Add(Attributes_Secondary_Vital_StaminaRegenRate,      Attributes_Secondary_Vital_MaxStaminaRegenRate);
	TagsMinMax.Add(Attributes_Secondary_Vital_StaminaRegenAmount,    Attributes_Secondary_Vital_MaxStaminaRegenAmount);
	TagsMinMax.Add(Attributes_Secondary_Vital_StaminaReservedAmount, Attributes_Secondary_Vital_MaxStaminaReservedAmount);

	TagsMinMax.Add(Attributes_Secondary_Vital_ArcaneShieldRegenRate,     Attributes_Secondary_Vital_MaxArcaneShieldRegenRate);
	TagsMinMax.Add(Attributes_Secondary_Vital_ArcaneShieldRegenAmount,   Attributes_Secondary_Vital_MaxArcaneShieldRegenAmount);
	TagsMinMax.Add(Attributes_Secondary_Vital_ArcaneShieldReservedAmount,Attributes_Secondary_Vital_MaxArcaneShieldReservedAmount);
}

void FPHGameplayTags::RegisterFlatDamageAttributes()
{
	FlatDamageToAttributesMap.Add("Physical",   UPHAttributeSet::GetPhysicalFlatBonusAttribute());
	FlatDamageToAttributesMap.Add("Fire",       UPHAttributeSet::GetFireFlatBonusAttribute());
	FlatDamageToAttributesMap.Add("Ice",        UPHAttributeSet::GetIceFlatBonusAttribute());
	FlatDamageToAttributesMap.Add("Lightning",  UPHAttributeSet::GetLightningFlatBonusAttribute());
	FlatDamageToAttributesMap.Add("Light",      UPHAttributeSet::GetLightFlatBonusAttribute());
	FlatDamageToAttributesMap.Add("Corruption", UPHAttributeSet::GetCorruptionFlatBonusAttribute());
}

void FPHGameplayTags::RegisterPercentDamageAttributes()
{
	PercentDamageToAttributesMap.Add("Physical",   UPHAttributeSet::GetPhysicalPercentBonusAttribute());
	PercentDamageToAttributesMap.Add("Fire",       UPHAttributeSet::GetFirePercentBonusAttribute());
	PercentDamageToAttributesMap.Add("Ice",        UPHAttributeSet::GetIcePercentBonusAttribute());
	PercentDamageToAttributesMap.Add("Lightning",  UPHAttributeSet::GetLightningPercentBonusAttribute());
	PercentDamageToAttributesMap.Add("Light",      UPHAttributeSet::GetLightPercentBonusAttribute());
	PercentDamageToAttributesMap.Add("Corruption", UPHAttributeSet::GetCorruptionPercentBonusAttribute());
}

void FPHGameplayTags::RegisterBaseDamageAttributes()
{
	BaseDamageToAttributesMap.Add("Min Physical",   UPHAttributeSet::GetMinPhysicalDamageAttribute());
	BaseDamageToAttributesMap.Add("Min Fire",       UPHAttributeSet::GetMinFireDamageAttribute());
	BaseDamageToAttributesMap.Add("Min Ice",        UPHAttributeSet::GetMinIceDamageAttribute());
	BaseDamageToAttributesMap.Add("Min Lightning",  UPHAttributeSet::GetMinLightningDamageAttribute());
	BaseDamageToAttributesMap.Add("Min Light",      UPHAttributeSet::GetMinLightDamageAttribute());
	BaseDamageToAttributesMap.Add("Min Corruption", UPHAttributeSet::GetMinCorruptionDamageAttribute());

	BaseDamageToAttributesMap.Add("Max Physical",   UPHAttributeSet::GetMaxPhysicalDamageAttribute());
	BaseDamageToAttributesMap.Add("Max Fire",       UPHAttributeSet::GetMaxFireDamageAttribute());
	BaseDamageToAttributesMap.Add("Max Ice",        UPHAttributeSet::GetMaxIceDamageAttribute());
	BaseDamageToAttributesMap.Add("Max Lightning",  UPHAttributeSet::GetMaxLightningDamageAttribute());
	BaseDamageToAttributesMap.Add("Max Light",      UPHAttributeSet::GetMaxLightDamageAttribute());
	BaseDamageToAttributesMap.Add("Max Corruption", UPHAttributeSet::GetMaxCorruptionDamageAttribute());
}

void FPHGameplayTags::RegisterAllAttribute()
{
	// 1) Reset all helper maps so hot-reload or multiple calls are safe.
	StatusEffectTagToAttributeMap.Empty();
	TagsMinMax.Empty();
	BaseDamageToAttributesMap.Empty();
	FlatDamageToAttributesMap.Empty();
	PercentDamageToAttributesMap.Empty();
	AllAttributesMap.Empty();

	// 2) Rebuild the small, focused maps.
	RegisterStatusEffectAttributes();
	RegisterMinMaxTagMap();
	RegisterBaseDamageAttributes();
	RegisterFlatDamageAttributes();
	RegisterPercentDamageAttributes();

	// 3) Build a single, comprehensive TagString -> FGameplayAttribute map.
	//    NOTE: use canonical tag paths (exactly what FindAttributeByTag uses).
	auto Add = [&](const TCHAR* Tag, const FGameplayAttribute& Attr)
	{
		if (Attr.IsValid())
		{
			AllAttributesMap.Add(Tag, Attr);
		}
	};

	// ===========================
	// Primary
	// ===========================
	Add(TEXT("Attributes.Primary.Strength"),      UPHAttributeSet::GetStrengthAttribute());
	Add(TEXT("Attributes.Primary.Intelligence"),  UPHAttributeSet::GetIntelligenceAttribute());
	Add(TEXT("Attributes.Primary.Dexterity"),     UPHAttributeSet::GetDexterityAttribute());
	Add(TEXT("Attributes.Primary.Endurance"),     UPHAttributeSet::GetEnduranceAttribute());
	Add(TEXT("Attributes.Primary.Affliction"),    UPHAttributeSet::GetAfflictionAttribute());
	Add(TEXT("Attributes.Primary.Luck"),          UPHAttributeSet::GetLuckAttribute());
	Add(TEXT("Attributes.Primary.Covenant"),      UPHAttributeSet::GetCovenantAttribute());

	// ===========================
	// Vitals (current values)
	// ===========================
	Add(TEXT("Attributes.Vital.Health"),   UPHAttributeSet::GetHealthAttribute());
	Add(TEXT("Attributes.Vital.Mana"),     UPHAttributeSet::GetManaAttribute());
	Add(TEXT("Attributes.Vital.Stamina"),  UPHAttributeSet::GetStaminaAttribute());

	// ===========================
	// Secondary → Vitals: Health
	// ===========================
	Add(TEXT("Attributes.Secondary.Vital.MaxHealth"),               UPHAttributeSet::GetMaxHealthAttribute());
	Add(TEXT("Attributes.Secondary.Vital.MaxEffectiveHealth"),      UPHAttributeSet::GetMaxEffectiveHealthAttribute());
	Add(TEXT("Attributes.Secondary.Vital.HealthRegenRate"),         UPHAttributeSet::GetHealthRegenRateAttribute());
	Add(TEXT("Attributes.Secondary.Vital.HealthRegenAmount"),       UPHAttributeSet::GetHealthRegenAmountAttribute());
	Add(TEXT("Attributes.Secondary.Vital.MaxHealthRegenRate"),      UPHAttributeSet::GetMaxHealthRegenRateAttribute());
	Add(TEXT("Attributes.Secondary.Vital.MaxHealthRegenAmount"),    UPHAttributeSet::GetMaxHealthRegenAmountAttribute());
	Add(TEXT("Attributes.Secondary.Vital.HealthReservedAmount"),    UPHAttributeSet::GetReservedHealthAttribute());
	Add(TEXT("Attributes.Secondary.Vital.MaxHealthReservedAmount"), UPHAttributeSet::GetMaxReservedHealthAttribute());
	Add(TEXT("Attributes.Secondary.Vital.HealthFlatReservedAmount"),UPHAttributeSet::GetFlatReservedHealthAttribute());
	Add(TEXT("Attributes.Secondary.Vital.HealthPercentageReserved"),UPHAttributeSet::GetPercentageReservedHealthAttribute());

	// ===========================
	// Secondary → Vitals: Mana
	// ===========================
	Add(TEXT("Attributes.Secondary.Vital.MaxMana"),               UPHAttributeSet::GetMaxManaAttribute());
	Add(TEXT("Attributes.Secondary.Vital.MaxEffectiveMana"),      UPHAttributeSet::GetMaxEffectiveManaAttribute());
	Add(TEXT("Attributes.Secondary.Vital.ManaRegenRate"),         UPHAttributeSet::GetManaRegenRateAttribute());
	Add(TEXT("Attributes.Secondary.Vital.ManaRegenAmount"),       UPHAttributeSet::GetManaRegenAmountAttribute());
	Add(TEXT("Attributes.Secondary.Vital.MaxManaRegenRate"),      UPHAttributeSet::GetMaxManaRegenRateAttribute());
	Add(TEXT("Attributes.Secondary.Vital.MaxManaRegenAmount"),    UPHAttributeSet::GetMaxManaRegenAmountAttribute());
	Add(TEXT("Attributes.Secondary.Vital.ManaReservedAmount"),    UPHAttributeSet::GetReservedManaAttribute());
	Add(TEXT("Attributes.Secondary.Vital.MaxManaReservedAmount"), UPHAttributeSet::GetMaxReservedManaAttribute());
	Add(TEXT("Attributes.Secondary.Vital.ManaFlatReservedAmount"),UPHAttributeSet::GetFlatReservedManaAttribute());
	Add(TEXT("Attributes.Secondary.Vital.ManaPercentageReserved"),UPHAttributeSet::GetPercentageReservedManaAttribute());

	// ===========================
	// Secondary → Vitals: Stamina
	// ===========================
	Add(TEXT("Attributes.Secondary.Vital.MaxStamina"),               UPHAttributeSet::GetMaxStaminaAttribute());
	Add(TEXT("Attributes.Secondary.Vital.MaxEffectiveStamina"),      UPHAttributeSet::GetMaxEffectiveStaminaAttribute());
	Add(TEXT("Attributes.Secondary.Vital.StaminaRegenRate"),         UPHAttributeSet::GetStaminaRegenRateAttribute());
	Add(TEXT("Attributes.Secondary.Vital.StaminaRegenAmount"),       UPHAttributeSet::GetStaminaRegenAmountAttribute());
	Add(TEXT("Attributes.Secondary.Vital.MaxStaminaRegenRate"),      UPHAttributeSet::GetMaxStaminaRegenRateAttribute());
	Add(TEXT("Attributes.Secondary.Vital.MaxStaminaRegenAmount"),    UPHAttributeSet::GetMaxStaminaRegenAmountAttribute());
	Add(TEXT("Attributes.Secondary.Vital.StaminaReservedAmount"),    UPHAttributeSet::GetReservedStaminaAttribute());
	Add(TEXT("Attributes.Secondary.Vital.MaxStaminaReservedAmount"), UPHAttributeSet::GetMaxReservedStaminaAttribute());
	Add(TEXT("Attributes.Secondary.Vital.StaminaFlatReservedAmount"),UPHAttributeSet::GetFlatReservedStaminaAttribute());
	Add(TEXT("Attributes.Secondary.Vital.StaminaPercentageReserved"),UPHAttributeSet::GetPercentageReservedStaminaAttribute());
	Add(TEXT("Attributes.Secondary.Vital.StaminaDegenRate"),         UPHAttributeSet::GetStaminaDegenRateAttribute());
	Add(TEXT("Attributes.Secondary.Vital.StaminaDegenAmount"),       UPHAttributeSet::GetStaminaDegenAmountAttribute());

	// ===========================
	// Secondary → Vitals: Arcane Shield
	// ===========================
	Add(TEXT("Attributes.Secondary.Vital.ArcaneShield"),                 UPHAttributeSet::GetArcaneShieldAttribute());
	Add(TEXT("Attributes.Secondary.Vital.MaxArcaneShield"),              UPHAttributeSet::GetMaxArcaneShieldAttribute());
	Add(TEXT("Attributes.Secondary.Vital.MaxEffectiveArcaneShield"),     UPHAttributeSet::GetMaxEffectiveArcaneShieldAttribute());
	Add(TEXT("Attributes.Secondary.Vital.ArcaneShieldRegenRate"),        UPHAttributeSet::GetArcaneShieldRegenRateAttribute());
	Add(TEXT("Attributes.Secondary.Vital.ArcaneShieldRegenAmount"),      UPHAttributeSet::GetArcaneShieldRegenAmountAttribute());
	Add(TEXT("Attributes.Secondary.Vital.MaxArcaneShieldRegenRate"),     UPHAttributeSet::GetMaxArcaneShieldRegenRateAttribute());
	Add(TEXT("Attributes.Secondary.Vital.MaxArcaneShieldRegenAmount"),   UPHAttributeSet::GetMaxArcaneShieldRegenAmountAttribute());
	Add(TEXT("Attributes.Secondary.Vital.ArcaneShieldReservedAmount"),   UPHAttributeSet::GetReservedArcaneShieldAttribute());
	Add(TEXT("Attributes.Secondary.Vital.MaxArcaneShieldReservedAmount"),UPHAttributeSet::GetMaxReservedArcaneShieldAttribute());
	Add(TEXT("Attributes.Secondary.Vital.ArcaneShieldFlatReservedAmount"),UPHAttributeSet::GetFlatReservedArcaneShieldAttribute());
	Add(TEXT("Attributes.Secondary.Vital.ArcaneShieldPercentageReserved"),UPHAttributeSet::GetPercentageReservedArcaneShieldAttribute());

	// ===========================
	// Damage (min/max)
	// ===========================
	Add(TEXT("Attributes.Secondary.Damage.Min.Physical"),   UPHAttributeSet::GetMinPhysicalDamageAttribute());
	Add(TEXT("Attributes.Secondary.Damage.Max.Physical"),   UPHAttributeSet::GetMaxPhysicalDamageAttribute());
	Add(TEXT("Attributes.Secondary.Damage.Min.Fire"),       UPHAttributeSet::GetMinFireDamageAttribute());
	Add(TEXT("Attributes.Secondary.Damage.Max.Fire"),       UPHAttributeSet::GetMaxFireDamageAttribute());
	Add(TEXT("Attributes.Secondary.Damage.Min.Ice"),        UPHAttributeSet::GetMinIceDamageAttribute());
	Add(TEXT("Attributes.Secondary.Damage.Max.Ice"),        UPHAttributeSet::GetMaxIceDamageAttribute());
	Add(TEXT("Attributes.Secondary.Damage.Min.Light"),      UPHAttributeSet::GetMinLightDamageAttribute());
	Add(TEXT("Attributes.Secondary.Damage.Max.Light"),      UPHAttributeSet::GetMaxLightDamageAttribute());
	Add(TEXT("Attributes.Secondary.Damage.Min.Lightning"),  UPHAttributeSet::GetMinLightningDamageAttribute());
	Add(TEXT("Attributes.Secondary.Damage.Max.Lightning"),  UPHAttributeSet::GetMaxLightningDamageAttribute());
	Add(TEXT("Attributes.Secondary.Damage.Min.Corruption"), UPHAttributeSet::GetMinCorruptionDamageAttribute());
	Add(TEXT("Attributes.Secondary.Damage.Max.Corruption"), UPHAttributeSet::GetMaxCorruptionDamageAttribute());

	// Damage (flat/percent + global)
	Add(TEXT("Attributes.Secondary.Damage.GlobalBonus"),       UPHAttributeSet::GetGlobalDamagesAttribute());
	Add(TEXT("Attributes.Secondary.Damage.Flat.Physical"),     UPHAttributeSet::GetPhysicalFlatBonusAttribute());
	Add(TEXT("Attributes.Secondary.Damage.Percent.Physical"),  UPHAttributeSet::GetPhysicalPercentBonusAttribute());
	Add(TEXT("Attributes.Secondary.Damage.Flat.Fire"),         UPHAttributeSet::GetFireFlatBonusAttribute());
	Add(TEXT("Attributes.Secondary.Damage.Percent.Fire"),      UPHAttributeSet::GetFirePercentBonusAttribute());
	Add(TEXT("Attributes.Secondary.Damage.Flat.Ice"),          UPHAttributeSet::GetIceFlatBonusAttribute());
	Add(TEXT("Attributes.Secondary.Damage.Percent.Ice"),       UPHAttributeSet::GetIcePercentBonusAttribute());
	Add(TEXT("Attributes.Secondary.Damage.Flat.Light"),        UPHAttributeSet::GetLightFlatBonusAttribute());
	Add(TEXT("Attributes.Secondary.Damage.Percent.Light"),     UPHAttributeSet::GetLightPercentBonusAttribute());
	Add(TEXT("Attributes.Secondary.Damage.Flat.Lightning"),    UPHAttributeSet::GetLightningFlatBonusAttribute());
	Add(TEXT("Attributes.Secondary.Damage.Percent.Lightning"), UPHAttributeSet::GetLightningPercentBonusAttribute());
	Add(TEXT("Attributes.Secondary.Damage.Flat.Corruption"),   UPHAttributeSet::GetCorruptionFlatBonusAttribute());
	Add(TEXT("Attributes.Secondary.Damage.Percent.Corruption"),UPHAttributeSet::GetCorruptionPercentBonusAttribute());

	// ===========================
	// Resistances
	// ===========================
	Add(TEXT("Attributes.Secondary.Resistance.GlobalDefenses"),   UPHAttributeSet::GetGlobalDefensesAttribute());
	Add(TEXT("Attributes.Secondary.Resistance.BlockStrength"),    UPHAttributeSet::GetBlockStrengthAttribute());
	Add(TEXT("Attributes.Secondary.Resistance.Armour"),           UPHAttributeSet::GetArmourAttribute());
	Add(TEXT("Attributes.Secondary.Resistance.Armour.Flat"),      UPHAttributeSet::GetArmourFlatBonusAttribute());
	Add(TEXT("Attributes.Secondary.Resistance.Armour.Percent"),   UPHAttributeSet::GetArmourPercentBonusAttribute());

	Add(TEXT("Attributes.Secondary.Resistance.Fire.Flat"),        UPHAttributeSet::GetFireResistanceFlatBonusAttribute());
	Add(TEXT("Attributes.Secondary.Resistance.Fire.Percent"),     UPHAttributeSet::GetFireResistancePercentBonusAttribute());
	Add(TEXT("Attributes.Secondary.Resistance.Fire.Max"),         UPHAttributeSet::GetMaxFireResistanceAttribute());

	Add(TEXT("Attributes.Secondary.Resistance.Ice.Flat"),         UPHAttributeSet::GetIceResistanceFlatBonusAttribute());
	Add(TEXT("Attributes.Secondary.Resistance.Ice.Percent"),      UPHAttributeSet::GetIceResistancePercentBonusAttribute());
	Add(TEXT("Attributes.Secondary.Resistance.Ice.Max"),          UPHAttributeSet::GetMaxIceResistanceAttribute());

	Add(TEXT("Attributes.Secondary.Resistance.Light.Flat"),       UPHAttributeSet::GetLightResistanceFlatBonusAttribute());
	Add(TEXT("Attributes.Secondary.Resistance.Light.Percent"),    UPHAttributeSet::GetLightResistancePercentBonusAttribute());
	Add(TEXT("Attributes.Secondary.Resistance.Light.Max"),        UPHAttributeSet::GetMaxLightResistanceAttribute());

	Add(TEXT("Attributes.Secondary.Resistance.Lightning.Flat"),   UPHAttributeSet::GetLightningResistanceFlatBonusAttribute());
	Add(TEXT("Attributes.Secondary.Resistance.Lightning.Percent"),UPHAttributeSet::GetLightningResistancePercentBonusAttribute());
	Add(TEXT("Attributes.Secondary.Resistance.Lightning.Max"),    UPHAttributeSet::GetMaxLightningResistanceAttribute());

	Add(TEXT("Attributes.Secondary.Resistance.Corruption.Flat"),  UPHAttributeSet::GetCorruptionResistanceFlatBonusAttribute());
	Add(TEXT("Attributes.Secondary.Resistance.Corruption.Percent"),UPHAttributeSet::GetCorruptionResistancePercentBonusAttribute());
	Add(TEXT("Attributes.Secondary.Resistance.Corruption.Max"),   UPHAttributeSet::GetMaxCorruptionResistanceAttribute());

	// ===========================
	// Offensive
	// ===========================
	Add(TEXT("Attributes.Secondary.Offensive.AreaDamage"),                 UPHAttributeSet::GetAreaDamageAttribute());
	Add(TEXT("Attributes.Secondary.Offensive.AreaOfEffect"),               UPHAttributeSet::GetAreaOfEffectAttribute());
	Add(TEXT("Attributes.Secondary.Offensive.AttackRange"),                UPHAttributeSet::GetAttackRangeAttribute());
	Add(TEXT("Attributes.Secondary.Offensive.AttackSpeed"),                UPHAttributeSet::GetAttackSpeedAttribute());
	Add(TEXT("Attributes.Secondary.Offensive.CastSpeed"),                  UPHAttributeSet::GetCastSpeedAttribute());
	Add(TEXT("Attributes.Secondary.Offensive.CritChance"),                 UPHAttributeSet::GetCritChanceAttribute());
	Add(TEXT("Attributes.Secondary.Offensive.CritMultiplier"),             UPHAttributeSet::GetCritMultiplierAttribute());
	Add(TEXT("Attributes.Secondary.Offensive.DamageOverTime"),             UPHAttributeSet::GetDamageOverTimeAttribute());
	Add(TEXT("Attributes.Secondary.Offensive.ElementalDamage"),            UPHAttributeSet::GetElementalDamageAttribute());
	Add(TEXT("Attributes.Secondary.Offensive.MeleeDamage"),                UPHAttributeSet::GetMeleeDamageAttribute());
	Add(TEXT("Attributes.Secondary.Offensive.Spelldamage"),                UPHAttributeSet::GetSpellDamageAttribute());
	Add(TEXT("Attributes.Secondary.Offensive.ProjectileCount"),            UPHAttributeSet::GetProjectileCountAttribute());
	Add(TEXT("Attributes.Secondary.Offensive.ProjectileSpeed"),            UPHAttributeSet::GetProjectileSpeedAttribute());
	Add(TEXT("Attributes.Secondary.Offensive.RangedDamage"),               UPHAttributeSet::GetRangedDamageAttribute());
	Add(TEXT("Attributes.Secondary.Offensive.SpellsCritChance"),           UPHAttributeSet::GetSpellsCritChanceAttribute());
	Add(TEXT("Attributes.Secondary.Offensive.SpellsCritMultiplier"),       UPHAttributeSet::GetSpellsCritMultiplierAttribute());
	Add(TEXT("Attributes.Secondary.Offensive.ChainCount"),                 UPHAttributeSet::GetChainCountAttribute());
	Add(TEXT("Attributes.Secondary.Offensive.ForkCount"),                  UPHAttributeSet::GetForkCountAttribute());
	Add(TEXT("Attributes.Secondary.Offensive.ChainDamage"),                UPHAttributeSet::GetChainDamageAttribute());
	Add(TEXT("Attributes.Secondary.Offensive.DamageBonusWhileAtFullHP"),   UPHAttributeSet::GetDamageBonusWhileAtFullHPAttribute());
	Add(TEXT("Attributes.Secondary.Offensive.DamageBonusWhileAtLowHP"),    UPHAttributeSet::GetDamageBonusWhileAtLowHPAttribute());

	// ===========================
	// Piercing
	// ===========================
	Add(TEXT("Attributes.Secondary.Piercing.Armour"),     UPHAttributeSet::GetArmourPiercingAttribute());
	Add(TEXT("Attributes.Secondary.Piercing.Fire"),       UPHAttributeSet::GetFirePiercingAttribute());
	Add(TEXT("Attributes.Secondary.Piercing.Ice"),        UPHAttributeSet::GetIcePiercingAttribute());
	Add(TEXT("Attributes.Secondary.Piercing.Light"),      UPHAttributeSet::GetLightPiercingAttribute());
	Add(TEXT("Attributes.Secondary.Piercing.Lightning"),  UPHAttributeSet::GetLightningPiercingAttribute());
	Add(TEXT("Attributes.Secondary.Piercing.Corruption"), UPHAttributeSet::GetCorruptionPiercingAttribute());

	// ===========================
	// Reflection
	// ===========================
	Add(TEXT("Attributes.Secondary.Reflection.Physical"),       UPHAttributeSet::GetReflectPhysicalAttribute());
	Add(TEXT("Attributes.Secondary.Reflection.Elemental"),      UPHAttributeSet::GetReflectElementalAttribute());
	Add(TEXT("Attributes.Secondary.Reflection.ChancePhysical"), UPHAttributeSet::GetReflectChancePhysicalAttribute());
	Add(TEXT("Attributes.Secondary.Reflection.ChanceElemental"),UPHAttributeSet::GetReflectChanceElementalAttribute());

	// ===========================
	// Damage Conversions
	// ===========================
	Add(TEXT("Attributes.Secondary.Conversion.PhysicalToFire"),        UPHAttributeSet::GetPhysicalToFireAttribute());
	Add(TEXT("Attributes.Secondary.Conversion.PhysicalToIce"),         UPHAttributeSet::GetPhysicalToIceAttribute());
	Add(TEXT("Attributes.Secondary.Conversion.PhysicalToLightning"),   UPHAttributeSet::GetPhysicalToLightningAttribute());
	Add(TEXT("Attributes.Secondary.Conversion.PhysicalToLight"),       UPHAttributeSet::GetPhysicalToLightAttribute());
	Add(TEXT("Attributes.Secondary.Conversion.PhysicalToCorruption"),  UPHAttributeSet::GetPhysicalToCorruptionAttribute());

	Add(TEXT("Attributes.Secondary.Conversion.FireToPhysical"),        UPHAttributeSet::GetFireToPhysicalAttribute());
	Add(TEXT("Attributes.Secondary.Conversion.FireToIce"),             UPHAttributeSet::GetFireToIceAttribute());
	Add(TEXT("Attributes.Secondary.Conversion.FireToLightning"),       UPHAttributeSet::GetFireToLightningAttribute());
	Add(TEXT("Attributes.Secondary.Conversion.FireToLight"),           UPHAttributeSet::GetFireToLightAttribute());
	Add(TEXT("Attributes.Secondary.Conversion.FireToCorruption"),      UPHAttributeSet::GetFireToCorruptionAttribute());

	Add(TEXT("Attributes.Secondary.Conversion.IceToPhysical"),         UPHAttributeSet::GetIceToPhysicalAttribute());
	Add(TEXT("Attributes.Secondary.Conversion.IceToFire"),             UPHAttributeSet::GetIceToFireAttribute());
	Add(TEXT("Attributes.Secondary.Conversion.IceToLightning"),        UPHAttributeSet::GetIceToLightningAttribute());
	Add(TEXT("Attributes.Secondary.Conversion.IceToLight"),            UPHAttributeSet::GetIceToLightAttribute());
	Add(TEXT("Attributes.Secondary.Conversion.IceToCorruption"),       UPHAttributeSet::GetIceToCorruptionAttribute());

	Add(TEXT("Attributes.Secondary.Conversion.LightningToPhysical"),   UPHAttributeSet::GetLightningToPhysicalAttribute());
	Add(TEXT("Attributes.Secondary.Conversion.LightningToFire"),       UPHAttributeSet::GetLightningToFireAttribute());
	Add(TEXT("Attributes.Secondary.Conversion.LightningToIce"),        UPHAttributeSet::GetLightningToIceAttribute());
	Add(TEXT("Attributes.Secondary.Conversion.LightningToLight"),      UPHAttributeSet::GetLightningToLightAttribute());
	Add(TEXT("Attributes.Secondary.Conversion.LightningToCorruption"), UPHAttributeSet::GetLightningToCorruptionAttribute());

	Add(TEXT("Attributes.Secondary.Conversion.LightToPhysical"),       UPHAttributeSet::GetLightToPhysicalAttribute());
	Add(TEXT("Attributes.Secondary.Conversion.LightToFire"),           UPHAttributeSet::GetLightToFireAttribute());
	Add(TEXT("Attributes.Secondary.Conversion.LightToIce"),            UPHAttributeSet::GetLightToIceAttribute());
	Add(TEXT("Attributes.Secondary.Conversion.LightToLightning"),      UPHAttributeSet::GetLightToLightningAttribute());
	Add(TEXT("Attributes.Secondary.Conversion.LightToCorruption"),     UPHAttributeSet::GetLightToCorruptionAttribute());

	Add(TEXT("Attributes.Secondary.Conversion.CorruptionToPhysical"),  UPHAttributeSet::GetCorruptionToPhysicalAttribute());
	Add(TEXT("Attributes.Secondary.Conversion.CorruptionToFire"),      UPHAttributeSet::GetCorruptionToFireAttribute());
	Add(TEXT("Attributes.Secondary.Conversion.CorruptionToIce"),       UPHAttributeSet::GetCorruptionToIceAttribute());
	Add(TEXT("Attributes.Secondary.Conversion.CorruptionToLightning"), UPHAttributeSet::GetCorruptionToLightningAttribute());
	Add(TEXT("Attributes.Secondary.Conversion.CorruptionToLight"),     UPHAttributeSet::GetCorruptionToLightAttribute());

	// ===========================
	// Misc
	// ===========================
	Add(TEXT("Attributes.Secondary.Money.Gems"),           UPHAttributeSet::GetGemsAttribute());
	Add(TEXT("Attributes.Secondary.Misc.Poise"),           UPHAttributeSet::GetPoiseAttribute());
	Add(TEXT("Attributes.Secondary.Misc.Weight"),          UPHAttributeSet::GetWeightAttribute());
	Add(TEXT("Attributes.Secondary.Misc.StunRecovery"),    UPHAttributeSet::GetStunRecoveryAttribute());
	Add(TEXT("Attributes.Secondary.Misc.MovementSpeed"),   UPHAttributeSet::GetMovementSpeedAttribute());
	Add(TEXT("Attributes.Secondary.Misc.CoolDown"),        UPHAttributeSet::GetCoolDownAttribute());
	Add(TEXT("Attributes.Secondary.Misc.ManaCostChanges"), UPHAttributeSet::GetManaCostChangesAttribute());
	Add(TEXT("Attributes.Secondary.Misc.LifeLeech"),       UPHAttributeSet::GetLifeLeechAttribute());
	Add(TEXT("Attributes.Secondary.Misc.ManaLeech"),       UPHAttributeSet::GetManaLeechAttribute());
	Add(TEXT("Attributes.Secondary.Misc.LifeOnHit"),       UPHAttributeSet::GetLifeOnHitAttribute());
	Add(TEXT("Attributes.Secondary.Misc.ManaOnHit"),       UPHAttributeSet::GetManaOnHitAttribute());
	Add(TEXT("Attributes.Secondary.Misc.StaminaOnHit"),    UPHAttributeSet::GetStaminaOnHitAttribute());
	Add(TEXT("Attributes.Secondary.Misc.StaminaCostChanges"),UPHAttributeSet::GetStaminaCostChangesAttribute());
	Add(TEXT("Attributes.Secondary.Misc.CritChance"),      UPHAttributeSet::GetCritChanceAttribute());     // mirrors Misc tag
	Add(TEXT("Attributes.Secondary.Misc.CritMultiplier"),  UPHAttributeSet::GetCritMultiplierAttribute()); // mirrors Misc tag

	// ===========================
	// Status Effects (aliases)
	// ===========================
	Add(TEXT("Attributes.Secondary.Ailments.ChanceToBleed"),     UPHAttributeSet::GetChanceToBleedAttribute());
	Add(TEXT("Attributes.Secondary.Ailments.ChanceToIgnite"),    UPHAttributeSet::GetChanceToIgniteAttribute());
	Add(TEXT("Attributes.Secondary.Ailments.ChanceToFreeze"),    UPHAttributeSet::GetChanceToFreezeAttribute());
	Add(TEXT("Attributes.Secondary.Ailments.ChanceToShock"),     UPHAttributeSet::GetChanceToShockAttribute());
	Add(TEXT("Attributes.Secondary.Ailments.ChanceToStun"),      UPHAttributeSet::GetChanceToStunAttribute());
	Add(TEXT("Attributes.Secondary.Ailments.ChanceToKnockBack"), UPHAttributeSet::GetChanceToKnockBackAttribute());
	Add(TEXT("Attributes.Secondary.Ailments.ChanceToPetrify"),   UPHAttributeSet::GetChanceToPetrifyAttribute());
	Add(TEXT("Attributes.Secondary.Ailments.ChanceToPurify"),    UPHAttributeSet::GetChanceToPurifyAttribute());
	Add(TEXT("Attributes.Secondary.Ailments.ChanceToCorrupt"),   UPHAttributeSet::GetChanceToCorruptAttribute());

	Add(TEXT("Attributes.Secondary.Duration.Bleed"),          UPHAttributeSet::GetBleedDurationAttribute());
	Add(TEXT("Attributes.Secondary.Duration.Burn"),           UPHAttributeSet::GetBurnDurationAttribute());
	Add(TEXT("Attributes.Secondary.Duration.Freeze"),         UPHAttributeSet::GetFreezeDurationAttribute());
	Add(TEXT("Attributes.Secondary.Duration.Shock"),          UPHAttributeSet::GetShockDurationAttribute());
	Add(TEXT("Attributes.Secondary.Duration.Corruption"),     UPHAttributeSet::GetCorruptionDurationAttribute());
	Add(TEXT("Attributes.Secondary.Duration.PetrifyBuildUp"), UPHAttributeSet::GetPetrifyBuildUpDurationAttribute());
	Add(TEXT("Attributes.Secondary.Duration.Purify"),         UPHAttributeSet::GetPurifyDurationAttribute());

	// (Optional) Log summary for sanity
	UE_LOG(LogTemp, Log, TEXT("[PHGameplayTags] RegisterAllAttribute(): %d attributes in AllAttributesMap, %d status tags, %d min/max pairs."),
		AllAttributesMap.Num(),
		StatusEffectTagToAttributeMap.Num(),
		TagsMinMax.Num());
}


