// Copyright@2024 Quentin Davis 

#include "PHGameplayTags.h"
#include "GameplayTagsManager.h"
#include "AbilitySystem/HunterAttributeSet.h"



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
	RegisterOffensiveTags();
	RegisterPiercingTags();
	RegisterReflectionTags();
	RegisterDamageConversionTags();
	RegisterStatusEffectAliases();
	RegisterAttributeToTagMappings();
	// Keep the lookup-map rebuild steps last: they clear and repopulate shared maps.
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
	Attributes_Secondary_Defenses_Armor             = T.AddNativeGameplayTag("Attributes.Secondary.Defenses.Armor",           TEXT("Armor. Legacy alias for Armour."));
	Attributes_Secondary_Resistances_GlobalDefenses       = T.AddNativeGameplayTag("Attributes.Secondary.Resistance.GlobalDefenses", TEXT("Global defenses."));
	Attributes_Secondary_Resistances_Armour               = T.AddNativeGameplayTag("Attributes.Secondary.Resistance.Armour",         TEXT("Armour."));
	Attributes_Secondary_Resistances_BlockStrength        = T.AddNativeGameplayTag("Attributes.Secondary.Resistance.BlockStrength",  TEXT("Block strength."));
	Attributes_Secondary_Resistances_ArmourFlatBonus      = T.AddNativeGameplayTag("Attributes.Secondary.Resistance.Armour.Flat",    TEXT("Flat armour."));
	Attributes_Secondary_Resistances_ArmourPercentBonus   = T.AddNativeGameplayTag("Attributes.Secondary.Resistance.Armour.Percent", TEXT("Percent armour."));
	T.AddNativeGameplayTag("Attributes.Secondary.Resistance.Armor",         TEXT("Armor. US spelling alias for Armour."));
	T.AddNativeGameplayTag("Attributes.Secondary.Resistance.Armor.Flat",    TEXT("Flat armor. US spelling alias for Armour."));
	T.AddNativeGameplayTag("Attributes.Secondary.Resistance.Armor.Percent", TEXT("Percent armor. US spelling alias for Armour."));

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
	Attributes_Secondary_Misc_CritChance       = T.AddNativeGameplayTag("Attributes.Secondary.Misc.CritChance",       TEXT("Crit chance (legacy alias of Offensive.CritChance)."));
	Attributes_Secondary_Misc_CritMultiplier   = T.AddNativeGameplayTag("Attributes.Secondary.Misc.CritMultiplier",   TEXT("Crit multiplier (legacy alias of Offensive.CritMultiplier)."));

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
	// Canonical StatusEffect.* tags are kept for content-facing usage.
	// C++ lookups use the Attributes.Secondary.* aliases registered below.
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
	T.AddNativeGameplayTag("Attributes.Secondary.Piercing.Armor",  TEXT("Armor piercing. US spelling alias for Armour."));
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
    AttributeToTagMap.Empty();

    // ===========================
    // Base Damage Attributes
    // ===========================
    AttributeToTagMap.Add(UHunterAttributeSet::GetMinPhysicalDamageAttribute(),   Attributes_Secondary_Damages_MinPhysicalDamage);
    AttributeToTagMap.Add(UHunterAttributeSet::GetMaxPhysicalDamageAttribute(),   Attributes_Secondary_Damages_MaxPhysicalDamage);
    AttributeToTagMap.Add(UHunterAttributeSet::GetMinFireDamageAttribute(),       Attributes_Secondary_Damages_MinFireDamage);
    AttributeToTagMap.Add(UHunterAttributeSet::GetMaxFireDamageAttribute(),       Attributes_Secondary_Damages_MaxFireDamage);
    AttributeToTagMap.Add(UHunterAttributeSet::GetMinIceDamageAttribute(),        Attributes_Secondary_Damages_MinIceDamage);
    AttributeToTagMap.Add(UHunterAttributeSet::GetMaxIceDamageAttribute(),        Attributes_Secondary_Damages_MaxIceDamage);
    AttributeToTagMap.Add(UHunterAttributeSet::GetMinLightningDamageAttribute(),  Attributes_Secondary_Damages_MinLightningDamage);
    AttributeToTagMap.Add(UHunterAttributeSet::GetMaxLightningDamageAttribute(),  Attributes_Secondary_Damages_MaxLightningDamage);
    AttributeToTagMap.Add(UHunterAttributeSet::GetMinLightDamageAttribute(),      Attributes_Secondary_Damages_MinLightDamage);
    AttributeToTagMap.Add(UHunterAttributeSet::GetMaxLightDamageAttribute(),      Attributes_Secondary_Damages_MaxLightDamage);
    AttributeToTagMap.Add(UHunterAttributeSet::GetMinCorruptionDamageAttribute(), Attributes_Secondary_Damages_MinCorruptionDamage);
    AttributeToTagMap.Add(UHunterAttributeSet::GetMaxCorruptionDamageAttribute(), Attributes_Secondary_Damages_MaxCorruptionDamage);
    
    // ===========================
    // Primary Attributes
    // ===========================
    AttributeToTagMap.Add(UHunterAttributeSet::GetStrengthAttribute(),     Attributes_Primary_Strength);
    AttributeToTagMap.Add(UHunterAttributeSet::GetIntelligenceAttribute(), Attributes_Primary_Intelligence);
    AttributeToTagMap.Add(UHunterAttributeSet::GetDexterityAttribute(),    Attributes_Primary_Dexterity);
    AttributeToTagMap.Add(UHunterAttributeSet::GetEnduranceAttribute(),    Attributes_Primary_Endurance);
    AttributeToTagMap.Add(UHunterAttributeSet::GetAfflictionAttribute(),   Attributes_Primary_Affliction);
    AttributeToTagMap.Add(UHunterAttributeSet::GetLuckAttribute(),         Attributes_Primary_Luck);
    AttributeToTagMap.Add(UHunterAttributeSet::GetCovenantAttribute(),     Attributes_Primary_Covenant);
    
    // ===========================
    // Defense Attributes
    // ===========================
    AttributeToTagMap.Add(UHunterAttributeSet::GetArmourAttribute(),    Attributes_Secondary_Resistances_Armour);
    AttributeToTagMap.Add(UHunterAttributeSet::GetPoiseAttribute(),     Attributes_Secondary_Misc_Poise);
    
    // ===========================
    // Resistance Attributes (Flat Bonus)
    // ===========================
    AttributeToTagMap.Add(UHunterAttributeSet::GetFireResistanceFlatBonusAttribute(),       Attributes_Secondary_Resistances_FireResistanceFlat);
    AttributeToTagMap.Add(UHunterAttributeSet::GetIceResistanceFlatBonusAttribute(),        Attributes_Secondary_Resistances_IceResistanceFlat);
    AttributeToTagMap.Add(UHunterAttributeSet::GetLightningResistanceFlatBonusAttribute(),  Attributes_Secondary_Resistances_LightningResistanceFlat);
    AttributeToTagMap.Add(UHunterAttributeSet::GetLightResistanceFlatBonusAttribute(),      Attributes_Secondary_Resistances_LightResistanceFlat);
    AttributeToTagMap.Add(UHunterAttributeSet::GetCorruptionResistanceFlatBonusAttribute(), Attributes_Secondary_Resistances_CorruptionResistanceFlat);
    
    // ===========================
    // Combat Attributes
    // ===========================
    AttributeToTagMap.Add(UHunterAttributeSet::GetCritChanceAttribute(),    Attributes_Secondary_Offensive_CritChance);
    AttributeToTagMap.Add(UHunterAttributeSet::GetCritMultiplierAttribute(), Attributes_Secondary_Offensive_CritMultiplier);
    AttributeToTagMap.Add(UHunterAttributeSet::GetAttackSpeedAttribute(),   Attributes_Secondary_Offensive_AttackSpeed);
    AttributeToTagMap.Add(UHunterAttributeSet::GetCastSpeedAttribute(),     Attributes_Secondary_Offensive_CastSpeed);
    AttributeToTagMap.Add(UHunterAttributeSet::GetAttackRangeAttribute(),   Attributes_Secondary_Offensive_AttackRange);
    
    // ===========================
    // Resource Cost Attributes
    // ===========================
    AttributeToTagMap.Add(UHunterAttributeSet::GetManaCostChangesAttribute(),    Attributes_Secondary_Misc_ManaCostChanges);
    AttributeToTagMap.Add(UHunterAttributeSet::GetStaminaCostChangesAttribute(), Attributes_Secondary_Misc_StaminaCostChanges);
    
    // ===========================
    // Flat Damage Bonus Attributes
    // ===========================
    AttributeToTagMap.Add(UHunterAttributeSet::GetPhysicalFlatDamageAttribute(),   Attributes_Secondary_BonusDamage_PhysicalFlatBonus);
    AttributeToTagMap.Add(UHunterAttributeSet::GetFireFlatDamageAttribute(),       Attributes_Secondary_BonusDamage_FireFlatBonus);
    AttributeToTagMap.Add(UHunterAttributeSet::GetIceFlatDamageAttribute(),        Attributes_Secondary_BonusDamage_IceFlatBonus);
    AttributeToTagMap.Add(UHunterAttributeSet::GetLightningFlatDamageAttribute(),  Attributes_Secondary_BonusDamage_LightningFlatBonus);
    AttributeToTagMap.Add(UHunterAttributeSet::GetLightFlatDamageAttribute(),      Attributes_Secondary_BonusDamage_LightFlatBonus);
    AttributeToTagMap.Add(UHunterAttributeSet::GetCorruptionFlatDamageAttribute(), Attributes_Secondary_BonusDamage_CorruptionFlatBonus);
    
    // ===========================
    // Percent Damage Bonus Attributes
    // ===========================
    AttributeToTagMap.Add(UHunterAttributeSet::GetPhysicalPercentDamageAttribute(),   Attributes_Secondary_BonusDamage_PhysicalPercentBonus);
    AttributeToTagMap.Add(UHunterAttributeSet::GetFirePercentDamageAttribute(),       Attributes_Secondary_BonusDamage_FirePercentBonus);
    AttributeToTagMap.Add(UHunterAttributeSet::GetIcePercentDamageAttribute(),        Attributes_Secondary_BonusDamage_IcePercentBonus);
    AttributeToTagMap.Add(UHunterAttributeSet::GetLightningPercentDamageAttribute(),  Attributes_Secondary_BonusDamage_LightningPercentBonus);
    AttributeToTagMap.Add(UHunterAttributeSet::GetLightPercentDamageAttribute(),      Attributes_Secondary_BonusDamage_LightPercentBonus);
    AttributeToTagMap.Add(UHunterAttributeSet::GetCorruptionPercentDamageAttribute(), Attributes_Secondary_BonusDamage_CorruptionPercentBonus);
}


void FPHGameplayTags::RegisterTagToAttributeMappings()
{
    TagToAttributeMap.Empty();

    // Safety check - ensure AttributeSet is ready
    if (!UHunterAttributeSet::GetHealthAttribute().IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("❌ AttributeSet not ready - skipping tag mappings"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("=== Registering Tag-to-Attribute Mappings ==="));

    // ===========================
    // Vitals - Current Values 
    // ===========================
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Vital.Health")), UHunterAttributeSet::GetHealthAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Vital.Mana")), UHunterAttributeSet::GetManaAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Vital.Stamina")), UHunterAttributeSet::GetStaminaAttribute());

    // ===========================
    // Vitals - Max Values 
    // ===========================
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Vital.MaxHealth")), UHunterAttributeSet::GetMaxHealthAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Vital.MaxMana")), UHunterAttributeSet::GetMaxManaAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Vital.MaxStamina")), UHunterAttributeSet::GetMaxStaminaAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Vital.ArcaneShield")), UHunterAttributeSet::GetArcaneShieldAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Vital.MaxArcaneShield")), UHunterAttributeSet::GetMaxArcaneShieldAttribute());

    // ===========================
    // Damage Types
    // ===========================
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Damage.GlobalBonus")), UHunterAttributeSet::GetGlobalDamagesAttribute());
    
    // Max Damage
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Damage.Max.Physical")), UHunterAttributeSet::GetMaxPhysicalDamageAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Damage.Max.Fire")), UHunterAttributeSet::GetMaxFireDamageAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Damage.Max.Ice")), UHunterAttributeSet::GetMaxIceDamageAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Damage.Max.Lightning")), UHunterAttributeSet::GetMaxLightningDamageAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Damage.Max.Light")), UHunterAttributeSet::GetMaxLightDamageAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Damage.Max.Corruption")), UHunterAttributeSet::GetMaxCorruptionDamageAttribute());

    // Min Damage
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Damage.Min.Physical")), UHunterAttributeSet::GetMinPhysicalDamageAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Damage.Min.Fire")), UHunterAttributeSet::GetMinFireDamageAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Damage.Min.Ice")), UHunterAttributeSet::GetMinIceDamageAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Damage.Min.Lightning")), UHunterAttributeSet::GetMinLightningDamageAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Damage.Min.Light")), UHunterAttributeSet::GetMinLightDamageAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Damage.Min.Corruption")), UHunterAttributeSet::GetMinCorruptionDamageAttribute());

    // ===========================
    // Damage Bonuses
    // ===========================
    
    // Flat Damage Bonuses
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Damage.Flat.Physical")), UHunterAttributeSet::GetPhysicalFlatDamageAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Damage.Flat.Fire")), UHunterAttributeSet::GetFireFlatDamageAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Damage.Flat.Ice")), UHunterAttributeSet::GetIceFlatDamageAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Damage.Flat.Lightning")), UHunterAttributeSet::GetLightningFlatDamageAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Damage.Flat.Light")), UHunterAttributeSet::GetLightFlatDamageAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Damage.Flat.Corruption")), UHunterAttributeSet::GetCorruptionFlatDamageAttribute());

    // Percent Damage Bonuses
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Damage.Percent.Physical")), UHunterAttributeSet::GetPhysicalPercentDamageAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Damage.Percent.Fire")), UHunterAttributeSet::GetFirePercentDamageAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Damage.Percent.Ice")), UHunterAttributeSet::GetIcePercentDamageAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Damage.Percent.Lightning")), UHunterAttributeSet::GetLightningPercentDamageAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Damage.Percent.Light")), UHunterAttributeSet::GetLightPercentDamageAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Damage.Percent.Corruption")), UHunterAttributeSet::GetCorruptionPercentDamageAttribute());

    // ===========================
    // Resistances
    // ===========================
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Resistance.GlobalDefenses")), UHunterAttributeSet::GetGlobalDefensesAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Defenses.Armor")), UHunterAttributeSet::GetArmourAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Resistance.Armour")), UHunterAttributeSet::GetArmourAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Resistance.Armor")), UHunterAttributeSet::GetArmourAttribute());
    
    // Flat Resistances
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Resistance.Armour.Flat")), UHunterAttributeSet::GetArmourFlatBonusAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Resistance.Armor.Flat")), UHunterAttributeSet::GetArmourFlatBonusAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Resistance.Fire.Flat")), UHunterAttributeSet::GetFireResistanceFlatBonusAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Resistance.Ice.Flat")), UHunterAttributeSet::GetIceResistanceFlatBonusAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Resistance.Lightning.Flat")), UHunterAttributeSet::GetLightningResistanceFlatBonusAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Resistance.Light.Flat")), UHunterAttributeSet::GetLightResistanceFlatBonusAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Resistance.Corruption.Flat")), UHunterAttributeSet::GetCorruptionResistanceFlatBonusAttribute());

    // Percent Resistances
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Resistance.Armour.Percent")), UHunterAttributeSet::GetArmourPercentBonusAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Resistance.Armor.Percent")), UHunterAttributeSet::GetArmourPercentBonusAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Resistance.Fire.Percent")), UHunterAttributeSet::GetFireResistancePercentBonusAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Resistance.Ice.Percent")), UHunterAttributeSet::GetIceResistancePercentBonusAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Resistance.Lightning.Percent")), UHunterAttributeSet::GetLightningResistancePercentBonusAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Resistance.Light.Percent")), UHunterAttributeSet::GetLightResistancePercentBonusAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Resistance.Corruption.Percent")), UHunterAttributeSet::GetCorruptionResistancePercentBonusAttribute());

    // ===========================
    // Primary Stats
    // ===========================
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Primary.Strength")), UHunterAttributeSet::GetStrengthAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Primary.Intelligence")), UHunterAttributeSet::GetIntelligenceAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Primary.Dexterity")), UHunterAttributeSet::GetDexterityAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Primary.Endurance")), UHunterAttributeSet::GetEnduranceAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Primary.Affliction")), UHunterAttributeSet::GetAfflictionAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Primary.Luck")), UHunterAttributeSet::GetLuckAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Primary.Covenant")), UHunterAttributeSet::GetCovenantAttribute());

    // ===========================
    // Regeneration
    // ===========================
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Vital.HealthRegenAmount")), UHunterAttributeSet::GetHealthRegenAmountAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Vital.HealthRegenRate")), UHunterAttributeSet::GetHealthRegenRateAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Vital.ManaRegenAmount")), UHunterAttributeSet::GetManaRegenAmountAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Vital.ManaRegenRate")), UHunterAttributeSet::GetManaRegenRateAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Vital.StaminaRegenAmount")), UHunterAttributeSet::GetStaminaRegenAmountAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Vital.StaminaRegenRate")), UHunterAttributeSet::GetStaminaRegenRateAttribute());

    // ===========================
    // Degeneration
    // ===========================
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Vital.StaminaDegenAmount")), UHunterAttributeSet::GetStaminaDegenAmountAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Vital.StaminaDegenRate")), UHunterAttributeSet::GetStaminaDegenRateAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Offensive.CritChance")), UHunterAttributeSet::GetCritChanceAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Offensive.CritMultiplier")), UHunterAttributeSet::GetCritMultiplierAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Misc.CritChance")), UHunterAttributeSet::GetCritChanceAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Misc.CritMultiplier")), UHunterAttributeSet::GetCritMultiplierAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Offensive.SpellDamage")), UHunterAttributeSet::GetSpellDamageAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Piercing.Armour")), UHunterAttributeSet::GetArmourPiercingAttribute());
    TagToAttributeMap.Add(FGameplayTag::RequestGameplayTag(FName("Attributes.Secondary.Piercing.Armor")), UHunterAttributeSet::GetArmourPiercingAttribute());

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
	StatusEffectTagToAttributeMap.Add(Attributes_Secondary_Ailments_ChanceToBleed,     UHunterAttributeSet::GetChanceToBleedAttribute());
	StatusEffectTagToAttributeMap.Add(Attributes_Secondary_Ailments_ChanceToIgnite,    UHunterAttributeSet::GetChanceToIgniteAttribute());
	StatusEffectTagToAttributeMap.Add(Attributes_Secondary_Ailments_ChanceToFreeze,    UHunterAttributeSet::GetChanceToFreezeAttribute());
	StatusEffectTagToAttributeMap.Add(Attributes_Secondary_Ailments_ChanceToShock,     UHunterAttributeSet::GetChanceToShockAttribute());
	StatusEffectTagToAttributeMap.Add(Attributes_Secondary_Ailments_ChanceToStun,      UHunterAttributeSet::GetChanceToStunAttribute());
	StatusEffectTagToAttributeMap.Add(Attributes_Secondary_Ailments_ChanceToKnockBack, UHunterAttributeSet::GetChanceToKnockBackAttribute());
	StatusEffectTagToAttributeMap.Add(Attributes_Secondary_Ailments_ChanceToPetrify,   UHunterAttributeSet::GetChanceToPetrifyAttribute());
	StatusEffectTagToAttributeMap.Add(Attributes_Secondary_Ailments_ChanceToPurify,    UHunterAttributeSet::GetChanceToPurifyAttribute());
	StatusEffectTagToAttributeMap.Add(Attributes_Secondary_Ailments_ChanceToCorrupt,   UHunterAttributeSet::GetChanceToCorruptAttribute());

	// Durations → FGameplayAttribute
	StatusEffectTagToAttributeMap.Add(Attributes_Secondary_Duration_Bleed,          UHunterAttributeSet::GetBleedDurationAttribute());
	StatusEffectTagToAttributeMap.Add(Attributes_Secondary_Duration_Burn,           UHunterAttributeSet::GetBurnDurationAttribute());
	StatusEffectTagToAttributeMap.Add(Attributes_Secondary_Duration_Freeze,         UHunterAttributeSet::GetFreezeDurationAttribute());
	StatusEffectTagToAttributeMap.Add(Attributes_Secondary_Duration_Shock,          UHunterAttributeSet::GetShockDurationAttribute());
	StatusEffectTagToAttributeMap.Add(Attributes_Secondary_Duration_Corruption,     UHunterAttributeSet::GetCorruptionDurationAttribute());
	StatusEffectTagToAttributeMap.Add(Attributes_Secondary_Duration_PetrifyBuildUp, UHunterAttributeSet::GetPetrifyBuildUpDurationAttribute());
	StatusEffectTagToAttributeMap.Add(Attributes_Secondary_Duration_Purify,         UHunterAttributeSet::GetPurifyDurationAttribute());
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
	FlatDamageToAttributesMap.Add("Physical",   UHunterAttributeSet::GetPhysicalFlatDamageAttribute());
	FlatDamageToAttributesMap.Add("Fire",       UHunterAttributeSet::GetFireFlatDamageAttribute());
	FlatDamageToAttributesMap.Add("Ice",        UHunterAttributeSet::GetIceFlatDamageAttribute());
	FlatDamageToAttributesMap.Add("Lightning",  UHunterAttributeSet::GetLightningFlatDamageAttribute());
	FlatDamageToAttributesMap.Add("Light",      UHunterAttributeSet::GetLightFlatDamageAttribute());
	FlatDamageToAttributesMap.Add("Corruption", UHunterAttributeSet::GetCorruptionFlatDamageAttribute());
}

void FPHGameplayTags::RegisterPercentDamageAttributes()
{
	PercentDamageToAttributesMap.Add("Physical",   UHunterAttributeSet::GetPhysicalPercentDamageAttribute());
	PercentDamageToAttributesMap.Add("Fire",       UHunterAttributeSet::GetFirePercentDamageAttribute());
	PercentDamageToAttributesMap.Add("Ice",        UHunterAttributeSet::GetIcePercentDamageAttribute());
	PercentDamageToAttributesMap.Add("Lightning",  UHunterAttributeSet::GetLightningPercentDamageAttribute());
	PercentDamageToAttributesMap.Add("Light",      UHunterAttributeSet::GetLightPercentDamageAttribute());
	PercentDamageToAttributesMap.Add("Corruption", UHunterAttributeSet::GetCorruptionPercentDamageAttribute());
}

void FPHGameplayTags::RegisterBaseDamageAttributes()
{
	BaseDamageToAttributesMap.Add("Min Physical",   UHunterAttributeSet::GetMinPhysicalDamageAttribute());
	BaseDamageToAttributesMap.Add("Min Fire",       UHunterAttributeSet::GetMinFireDamageAttribute());
	BaseDamageToAttributesMap.Add("Min Ice",        UHunterAttributeSet::GetMinIceDamageAttribute());
	BaseDamageToAttributesMap.Add("Min Lightning",  UHunterAttributeSet::GetMinLightningDamageAttribute());
	BaseDamageToAttributesMap.Add("Min Light",      UHunterAttributeSet::GetMinLightDamageAttribute());
	BaseDamageToAttributesMap.Add("Min Corruption", UHunterAttributeSet::GetMinCorruptionDamageAttribute());

	BaseDamageToAttributesMap.Add("Max Physical",   UHunterAttributeSet::GetMaxPhysicalDamageAttribute());
	BaseDamageToAttributesMap.Add("Max Fire",       UHunterAttributeSet::GetMaxFireDamageAttribute());
	BaseDamageToAttributesMap.Add("Max Ice",        UHunterAttributeSet::GetMaxIceDamageAttribute());
	BaseDamageToAttributesMap.Add("Max Lightning",  UHunterAttributeSet::GetMaxLightningDamageAttribute());
	BaseDamageToAttributesMap.Add("Max Light",      UHunterAttributeSet::GetMaxLightDamageAttribute());
	BaseDamageToAttributesMap.Add("Max Corruption", UHunterAttributeSet::GetMaxCorruptionDamageAttribute());
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
	Add(TEXT("Attributes.Primary.Strength"),      UHunterAttributeSet::GetStrengthAttribute());
	Add(TEXT("Attributes.Primary.Intelligence"),  UHunterAttributeSet::GetIntelligenceAttribute());
	Add(TEXT("Attributes.Primary.Dexterity"),     UHunterAttributeSet::GetDexterityAttribute());
	Add(TEXT("Attributes.Primary.Endurance"),     UHunterAttributeSet::GetEnduranceAttribute());
	Add(TEXT("Attributes.Primary.Affliction"),    UHunterAttributeSet::GetAfflictionAttribute());
	Add(TEXT("Attributes.Primary.Luck"),          UHunterAttributeSet::GetLuckAttribute());
	Add(TEXT("Attributes.Primary.Covenant"),      UHunterAttributeSet::GetCovenantAttribute());

	// ===========================
	// Vitals (current values)
	// ===========================
	Add(TEXT("Attributes.Vital.Health"),   UHunterAttributeSet::GetHealthAttribute());
	Add(TEXT("Attributes.Vital.Mana"),     UHunterAttributeSet::GetManaAttribute());
	Add(TEXT("Attributes.Vital.Stamina"),  UHunterAttributeSet::GetStaminaAttribute());

	// ===========================
	// Secondary → Vitals: Health
	// ===========================
	Add(TEXT("Attributes.Secondary.Vital.MaxHealth"),               UHunterAttributeSet::GetMaxHealthAttribute());
	Add(TEXT("Attributes.Secondary.Vital.MaxEffectiveHealth"),      UHunterAttributeSet::GetMaxEffectiveHealthAttribute());
	Add(TEXT("Attributes.Secondary.Vital.HealthRegenRate"),         UHunterAttributeSet::GetHealthRegenRateAttribute());
	Add(TEXT("Attributes.Secondary.Vital.HealthRegenAmount"),       UHunterAttributeSet::GetHealthRegenAmountAttribute());
	Add(TEXT("Attributes.Secondary.Vital.MaxHealthRegenRate"),      UHunterAttributeSet::GetMaxHealthRegenRateAttribute());
	Add(TEXT("Attributes.Secondary.Vital.MaxHealthRegenAmount"),    UHunterAttributeSet::GetMaxHealthRegenAmountAttribute());
	Add(TEXT("Attributes.Secondary.Vital.HealthReservedAmount"),    UHunterAttributeSet::GetReservedHealthAttribute());
	Add(TEXT("Attributes.Secondary.Vital.MaxHealthReservedAmount"), UHunterAttributeSet::GetMaxReservedHealthAttribute());
	Add(TEXT("Attributes.Secondary.Vital.HealthFlatReservedAmount"),UHunterAttributeSet::GetFlatReservedHealthAttribute());
	Add(TEXT("Attributes.Secondary.Vital.HealthPercentageReserved"),UHunterAttributeSet::GetPercentageReservedHealthAttribute());

	// ===========================
	// Secondary → Vitals: Mana
	// ===========================
	Add(TEXT("Attributes.Secondary.Vital.MaxMana"),               UHunterAttributeSet::GetMaxManaAttribute());
	Add(TEXT("Attributes.Secondary.Vital.MaxEffectiveMana"),      UHunterAttributeSet::GetMaxEffectiveManaAttribute());
	Add(TEXT("Attributes.Secondary.Vital.ManaRegenRate"),         UHunterAttributeSet::GetManaRegenRateAttribute());
	Add(TEXT("Attributes.Secondary.Vital.ManaRegenAmount"),       UHunterAttributeSet::GetManaRegenAmountAttribute());
	Add(TEXT("Attributes.Secondary.Vital.MaxManaRegenRate"),      UHunterAttributeSet::GetMaxManaRegenRateAttribute());
	Add(TEXT("Attributes.Secondary.Vital.MaxManaRegenAmount"),    UHunterAttributeSet::GetMaxManaRegenAmountAttribute());
	Add(TEXT("Attributes.Secondary.Vital.ManaReservedAmount"),    UHunterAttributeSet::GetReservedManaAttribute());
	Add(TEXT("Attributes.Secondary.Vital.MaxManaReservedAmount"), UHunterAttributeSet::GetMaxReservedManaAttribute());
	Add(TEXT("Attributes.Secondary.Vital.ManaFlatReservedAmount"),UHunterAttributeSet::GetFlatReservedManaAttribute());
	Add(TEXT("Attributes.Secondary.Vital.ManaPercentageReserved"),UHunterAttributeSet::GetPercentageReservedManaAttribute());

	// ===========================
	// Secondary → Vitals: Stamina
	// ===========================
	Add(TEXT("Attributes.Secondary.Vital.MaxStamina"),               UHunterAttributeSet::GetMaxStaminaAttribute());
	Add(TEXT("Attributes.Secondary.Vital.MaxEffectiveStamina"),      UHunterAttributeSet::GetMaxEffectiveStaminaAttribute());
	Add(TEXT("Attributes.Secondary.Vital.StaminaRegenRate"),         UHunterAttributeSet::GetStaminaRegenRateAttribute());
	Add(TEXT("Attributes.Secondary.Vital.StaminaRegenAmount"),       UHunterAttributeSet::GetStaminaRegenAmountAttribute());
	Add(TEXT("Attributes.Secondary.Vital.MaxStaminaRegenRate"),      UHunterAttributeSet::GetMaxStaminaRegenRateAttribute());
	Add(TEXT("Attributes.Secondary.Vital.MaxStaminaRegenAmount"),    UHunterAttributeSet::GetMaxStaminaRegenAmountAttribute());
	Add(TEXT("Attributes.Secondary.Vital.StaminaReservedAmount"),    UHunterAttributeSet::GetReservedStaminaAttribute());
	Add(TEXT("Attributes.Secondary.Vital.MaxStaminaReservedAmount"), UHunterAttributeSet::GetMaxReservedStaminaAttribute());
	Add(TEXT("Attributes.Secondary.Vital.StaminaFlatReservedAmount"),UHunterAttributeSet::GetFlatReservedStaminaAttribute());
	Add(TEXT("Attributes.Secondary.Vital.StaminaPercentageReserved"),UHunterAttributeSet::GetPercentageReservedStaminaAttribute());
	Add(TEXT("Attributes.Secondary.Vital.StaminaDegenRate"),         UHunterAttributeSet::GetStaminaDegenRateAttribute());
	Add(TEXT("Attributes.Secondary.Vital.StaminaDegenAmount"),       UHunterAttributeSet::GetStaminaDegenAmountAttribute());

	// ===========================
	// Secondary → Vitals: Arcane Shield
	// ===========================
	Add(TEXT("Attributes.Secondary.Vital.ArcaneShield"),                 UHunterAttributeSet::GetArcaneShieldAttribute());
	Add(TEXT("Attributes.Secondary.Vital.MaxArcaneShield"),              UHunterAttributeSet::GetMaxArcaneShieldAttribute());
	Add(TEXT("Attributes.Secondary.Vital.MaxEffectiveArcaneShield"),     UHunterAttributeSet::GetMaxEffectiveArcaneShieldAttribute());
	Add(TEXT("Attributes.Secondary.Vital.ArcaneShieldRegenRate"),        UHunterAttributeSet::GetArcaneShieldRegenRateAttribute());
	Add(TEXT("Attributes.Secondary.Vital.ArcaneShieldRegenAmount"),      UHunterAttributeSet::GetArcaneShieldRegenAmountAttribute());
	Add(TEXT("Attributes.Secondary.Vital.MaxArcaneShieldRegenRate"),     UHunterAttributeSet::GetMaxArcaneShieldRegenRateAttribute());
	Add(TEXT("Attributes.Secondary.Vital.MaxArcaneShieldRegenAmount"),   UHunterAttributeSet::GetMaxArcaneShieldRegenAmountAttribute());
	Add(TEXT("Attributes.Secondary.Vital.ArcaneShieldReservedAmount"),   UHunterAttributeSet::GetReservedArcaneShieldAttribute());
	Add(TEXT("Attributes.Secondary.Vital.MaxArcaneShieldReservedAmount"),UHunterAttributeSet::GetMaxReservedArcaneShieldAttribute());
	Add(TEXT("Attributes.Secondary.Vital.ArcaneShieldFlatReservedAmount"),UHunterAttributeSet::GetFlatReservedArcaneShieldAttribute());
	Add(TEXT("Attributes.Secondary.Vital.ArcaneShieldPercentageReserved"),UHunterAttributeSet::GetPercentageReservedArcaneShieldAttribute());

	// ===========================
	// Damage (min/max)
	// ===========================
	Add(TEXT("Attributes.Secondary.Damage.Min.Physical"),   UHunterAttributeSet::GetMinPhysicalDamageAttribute());
	Add(TEXT("Attributes.Secondary.Damage.Max.Physical"),   UHunterAttributeSet::GetMaxPhysicalDamageAttribute());
	Add(TEXT("Attributes.Secondary.Damage.Min.Fire"),       UHunterAttributeSet::GetMinFireDamageAttribute());
	Add(TEXT("Attributes.Secondary.Damage.Max.Fire"),       UHunterAttributeSet::GetMaxFireDamageAttribute());
	Add(TEXT("Attributes.Secondary.Damage.Min.Ice"),        UHunterAttributeSet::GetMinIceDamageAttribute());
	Add(TEXT("Attributes.Secondary.Damage.Max.Ice"),        UHunterAttributeSet::GetMaxIceDamageAttribute());
	Add(TEXT("Attributes.Secondary.Damage.Min.Light"),      UHunterAttributeSet::GetMinLightDamageAttribute());
	Add(TEXT("Attributes.Secondary.Damage.Max.Light"),      UHunterAttributeSet::GetMaxLightDamageAttribute());
	Add(TEXT("Attributes.Secondary.Damage.Min.Lightning"),  UHunterAttributeSet::GetMinLightningDamageAttribute());
	Add(TEXT("Attributes.Secondary.Damage.Max.Lightning"),  UHunterAttributeSet::GetMaxLightningDamageAttribute());
	Add(TEXT("Attributes.Secondary.Damage.Min.Corruption"), UHunterAttributeSet::GetMinCorruptionDamageAttribute());
	Add(TEXT("Attributes.Secondary.Damage.Max.Corruption"), UHunterAttributeSet::GetMaxCorruptionDamageAttribute());

	// Damage (flat/percent + global)
	Add(TEXT("Attributes.Secondary.Damage.GlobalBonus"),       UHunterAttributeSet::GetGlobalDamagesAttribute());
	Add(TEXT("Attributes.Secondary.Damage.Flat.Physical"),     UHunterAttributeSet::GetPhysicalFlatDamageAttribute());
	Add(TEXT("Attributes.Secondary.Damage.Percent.Physical"),  UHunterAttributeSet::GetPhysicalPercentDamageAttribute());
	Add(TEXT("Attributes.Secondary.Damage.Flat.Fire"),         UHunterAttributeSet::GetFireFlatDamageAttribute());
	Add(TEXT("Attributes.Secondary.Damage.Percent.Fire"),      UHunterAttributeSet::GetFirePercentDamageAttribute());
	Add(TEXT("Attributes.Secondary.Damage.Flat.Ice"),          UHunterAttributeSet::GetIceFlatDamageAttribute());
	Add(TEXT("Attributes.Secondary.Damage.Percent.Ice"),       UHunterAttributeSet::GetIcePercentDamageAttribute());
	Add(TEXT("Attributes.Secondary.Damage.Flat.Light"),        UHunterAttributeSet::GetLightFlatDamageAttribute());
	Add(TEXT("Attributes.Secondary.Damage.Percent.Light"),     UHunterAttributeSet::GetLightPercentDamageAttribute());
	Add(TEXT("Attributes.Secondary.Damage.Flat.Lightning"),    UHunterAttributeSet::GetLightningFlatDamageAttribute());
	Add(TEXT("Attributes.Secondary.Damage.Percent.Lightning"), UHunterAttributeSet::GetLightningPercentDamageAttribute());
	Add(TEXT("Attributes.Secondary.Damage.Flat.Corruption"),   UHunterAttributeSet::GetCorruptionFlatDamageAttribute());
	Add(TEXT("Attributes.Secondary.Damage.Percent.Corruption"),UHunterAttributeSet::GetCorruptionPercentDamageAttribute());

	// ===========================
	// Resistances
	// ===========================
	Add(TEXT("Attributes.Secondary.Resistance.GlobalDefenses"),   UHunterAttributeSet::GetGlobalDefensesAttribute());
	Add(TEXT("Attributes.Secondary.Resistance.BlockStrength"),    UHunterAttributeSet::GetBlockStrengthAttribute());
	Add(TEXT("Attributes.Secondary.Defenses.Armor"),             UHunterAttributeSet::GetArmourAttribute());
	Add(TEXT("Attributes.Secondary.Resistance.Armour"),           UHunterAttributeSet::GetArmourAttribute());
	Add(TEXT("Attributes.Secondary.Resistance.Armor"),            UHunterAttributeSet::GetArmourAttribute());
	Add(TEXT("Attributes.Secondary.Resistance.Armour.Flat"),      UHunterAttributeSet::GetArmourFlatBonusAttribute());
	Add(TEXT("Attributes.Secondary.Resistance.Armor.Flat"),       UHunterAttributeSet::GetArmourFlatBonusAttribute());
	Add(TEXT("Attributes.Secondary.Resistance.Armour.Percent"),   UHunterAttributeSet::GetArmourPercentBonusAttribute());
	Add(TEXT("Attributes.Secondary.Resistance.Armor.Percent"),    UHunterAttributeSet::GetArmourPercentBonusAttribute());

	Add(TEXT("Attributes.Secondary.Resistance.Fire.Flat"),        UHunterAttributeSet::GetFireResistanceFlatBonusAttribute());
	Add(TEXT("Attributes.Secondary.Resistance.Fire.Percent"),     UHunterAttributeSet::GetFireResistancePercentBonusAttribute());
	Add(TEXT("Attributes.Secondary.Resistance.Fire.Max"),         UHunterAttributeSet::GetMaxFireResistanceAttribute());

	Add(TEXT("Attributes.Secondary.Resistance.Ice.Flat"),         UHunterAttributeSet::GetIceResistanceFlatBonusAttribute());
	Add(TEXT("Attributes.Secondary.Resistance.Ice.Percent"),      UHunterAttributeSet::GetIceResistancePercentBonusAttribute());
	Add(TEXT("Attributes.Secondary.Resistance.Ice.Max"),          UHunterAttributeSet::GetMaxIceResistanceAttribute());

	Add(TEXT("Attributes.Secondary.Resistance.Light.Flat"),       UHunterAttributeSet::GetLightResistanceFlatBonusAttribute());
	Add(TEXT("Attributes.Secondary.Resistance.Light.Percent"),    UHunterAttributeSet::GetLightResistancePercentBonusAttribute());
	Add(TEXT("Attributes.Secondary.Resistance.Light.Max"),        UHunterAttributeSet::GetMaxLightResistanceAttribute());

	Add(TEXT("Attributes.Secondary.Resistance.Lightning.Flat"),   UHunterAttributeSet::GetLightningResistanceFlatBonusAttribute());
	Add(TEXT("Attributes.Secondary.Resistance.Lightning.Percent"),UHunterAttributeSet::GetLightningResistancePercentBonusAttribute());
	Add(TEXT("Attributes.Secondary.Resistance.Lightning.Max"),    UHunterAttributeSet::GetMaxLightningResistanceAttribute());

	Add(TEXT("Attributes.Secondary.Resistance.Corruption.Flat"),  UHunterAttributeSet::GetCorruptionResistanceFlatBonusAttribute());
	Add(TEXT("Attributes.Secondary.Resistance.Corruption.Percent"),UHunterAttributeSet::GetCorruptionResistancePercentBonusAttribute());
	Add(TEXT("Attributes.Secondary.Resistance.Corruption.Max"),   UHunterAttributeSet::GetMaxCorruptionResistanceAttribute());

	// ===========================
	// Offensive
	// ===========================
	Add(TEXT("Attributes.Secondary.Offensive.AreaDamage"),                 UHunterAttributeSet::GetAreaDamageAttribute());
	Add(TEXT("Attributes.Secondary.Offensive.AreaOfEffect"),               UHunterAttributeSet::GetAreaOfEffectAttribute());
	Add(TEXT("Attributes.Secondary.Offensive.AttackRange"),                UHunterAttributeSet::GetAttackRangeAttribute());
	Add(TEXT("Attributes.Secondary.Offensive.AttackSpeed"),                UHunterAttributeSet::GetAttackSpeedAttribute());
	Add(TEXT("Attributes.Secondary.Offensive.CastSpeed"),                  UHunterAttributeSet::GetCastSpeedAttribute());
	Add(TEXT("Attributes.Secondary.Offensive.CritChance"),                 UHunterAttributeSet::GetCritChanceAttribute());
	Add(TEXT("Attributes.Secondary.Offensive.CritMultiplier"),             UHunterAttributeSet::GetCritMultiplierAttribute());
	Add(TEXT("Attributes.Secondary.Offensive.DamageOverTime"),             UHunterAttributeSet::GetDamageOverTimeAttribute());
	Add(TEXT("Attributes.Secondary.Offensive.ElementalDamage"),            UHunterAttributeSet::GetElementalDamageAttribute());
	Add(TEXT("Attributes.Secondary.Offensive.MeleeDamage"),                UHunterAttributeSet::GetMeleeDamageAttribute());
	Add(TEXT("Attributes.Secondary.Offensive.SpellDamage"),                UHunterAttributeSet::GetSpellDamageAttribute());
	Add(TEXT("Attributes.Secondary.Offensive.ProjectileCount"),            UHunterAttributeSet::GetProjectileCountAttribute());
	Add(TEXT("Attributes.Secondary.Offensive.ProjectileSpeed"),            UHunterAttributeSet::GetProjectileSpeedAttribute());
	Add(TEXT("Attributes.Secondary.Offensive.RangedDamage"),               UHunterAttributeSet::GetRangedDamageAttribute());
	Add(TEXT("Attributes.Secondary.Offensive.SpellsCritChance"),           UHunterAttributeSet::GetSpellsCritChanceAttribute());
	Add(TEXT("Attributes.Secondary.Offensive.SpellsCritMultiplier"),       UHunterAttributeSet::GetSpellsCritMultiplierAttribute());
	Add(TEXT("Attributes.Secondary.Offensive.ChainCount"),                 UHunterAttributeSet::GetChainCountAttribute());
	Add(TEXT("Attributes.Secondary.Offensive.ForkCount"),                  UHunterAttributeSet::GetForkCountAttribute());
	Add(TEXT("Attributes.Secondary.Offensive.ChainDamage"),                UHunterAttributeSet::GetChainDamageAttribute());
	Add(TEXT("Attributes.Secondary.Offensive.DamageBonusWhileAtFullHP"),   UHunterAttributeSet::GetDamageBonusWhileAtFullHPAttribute());
	Add(TEXT("Attributes.Secondary.Offensive.DamageBonusWhileAtLowHP"),    UHunterAttributeSet::GetDamageBonusWhileAtLowHPAttribute());

	// ===========================
	// Piercing
	// ===========================
	Add(TEXT("Attributes.Secondary.Piercing.Armour"),     UHunterAttributeSet::GetArmourPiercingAttribute());
	Add(TEXT("Attributes.Secondary.Piercing.Armor"),      UHunterAttributeSet::GetArmourPiercingAttribute());
	Add(TEXT("Attributes.Secondary.Piercing.Fire"),       UHunterAttributeSet::GetFirePiercingAttribute());
	Add(TEXT("Attributes.Secondary.Piercing.Ice"),        UHunterAttributeSet::GetIcePiercingAttribute());
	Add(TEXT("Attributes.Secondary.Piercing.Light"),      UHunterAttributeSet::GetLightPiercingAttribute());
	Add(TEXT("Attributes.Secondary.Piercing.Lightning"),  UHunterAttributeSet::GetLightningPiercingAttribute());
	Add(TEXT("Attributes.Secondary.Piercing.Corruption"), UHunterAttributeSet::GetCorruptionPiercingAttribute());

	// ===========================
	// Reflection
	// ===========================
	Add(TEXT("Attributes.Secondary.Reflection.Physical"),       UHunterAttributeSet::GetReflectPhysicalAttribute());
	Add(TEXT("Attributes.Secondary.Reflection.Elemental"),      UHunterAttributeSet::GetReflectElementalAttribute());
	Add(TEXT("Attributes.Secondary.Reflection.ChancePhysical"), UHunterAttributeSet::GetReflectChancePhysicalAttribute());
	Add(TEXT("Attributes.Secondary.Reflection.ChanceElemental"),UHunterAttributeSet::GetReflectChanceElementalAttribute());

	// ===========================
	// Damage Conversions
	// ===========================
	Add(TEXT("Attributes.Secondary.Conversion.PhysicalToFire"),        UHunterAttributeSet::GetPhysicalToFireAttribute());
	Add(TEXT("Attributes.Secondary.Conversion.PhysicalToIce"),         UHunterAttributeSet::GetPhysicalToIceAttribute());
	Add(TEXT("Attributes.Secondary.Conversion.PhysicalToLightning"),   UHunterAttributeSet::GetPhysicalToLightningAttribute());
	Add(TEXT("Attributes.Secondary.Conversion.PhysicalToLight"),       UHunterAttributeSet::GetPhysicalToLightAttribute());
	Add(TEXT("Attributes.Secondary.Conversion.PhysicalToCorruption"),  UHunterAttributeSet::GetPhysicalToCorruptionAttribute());

	Add(TEXT("Attributes.Secondary.Conversion.FireToPhysical"),        UHunterAttributeSet::GetFireToPhysicalAttribute());
	Add(TEXT("Attributes.Secondary.Conversion.FireToIce"),             UHunterAttributeSet::GetFireToIceAttribute());
	Add(TEXT("Attributes.Secondary.Conversion.FireToLightning"),       UHunterAttributeSet::GetFireToLightningAttribute());
	Add(TEXT("Attributes.Secondary.Conversion.FireToLight"),           UHunterAttributeSet::GetFireToLightAttribute());
	Add(TEXT("Attributes.Secondary.Conversion.FireToCorruption"),      UHunterAttributeSet::GetFireToCorruptionAttribute());

	Add(TEXT("Attributes.Secondary.Conversion.IceToPhysical"),         UHunterAttributeSet::GetIceToPhysicalAttribute());
	Add(TEXT("Attributes.Secondary.Conversion.IceToFire"),             UHunterAttributeSet::GetIceToFireAttribute());
	Add(TEXT("Attributes.Secondary.Conversion.IceToLightning"),        UHunterAttributeSet::GetIceToLightningAttribute());
	Add(TEXT("Attributes.Secondary.Conversion.IceToLight"),            UHunterAttributeSet::GetIceToLightAttribute());
	Add(TEXT("Attributes.Secondary.Conversion.IceToCorruption"),       UHunterAttributeSet::GetIceToCorruptionAttribute());

	Add(TEXT("Attributes.Secondary.Conversion.LightningToPhysical"),   UHunterAttributeSet::GetLightningToPhysicalAttribute());
	Add(TEXT("Attributes.Secondary.Conversion.LightningToFire"),       UHunterAttributeSet::GetLightningToFireAttribute());
	Add(TEXT("Attributes.Secondary.Conversion.LightningToIce"),        UHunterAttributeSet::GetLightningToIceAttribute());
	Add(TEXT("Attributes.Secondary.Conversion.LightningToLight"),      UHunterAttributeSet::GetLightningToLightAttribute());
	Add(TEXT("Attributes.Secondary.Conversion.LightningToCorruption"), UHunterAttributeSet::GetLightningToCorruptionAttribute());

	Add(TEXT("Attributes.Secondary.Conversion.LightToPhysical"),       UHunterAttributeSet::GetLightToPhysicalAttribute());
	Add(TEXT("Attributes.Secondary.Conversion.LightToFire"),           UHunterAttributeSet::GetLightToFireAttribute());
	Add(TEXT("Attributes.Secondary.Conversion.LightToIce"),            UHunterAttributeSet::GetLightToIceAttribute());
	Add(TEXT("Attributes.Secondary.Conversion.LightToLightning"),      UHunterAttributeSet::GetLightToLightningAttribute());
	Add(TEXT("Attributes.Secondary.Conversion.LightToCorruption"),     UHunterAttributeSet::GetLightToCorruptionAttribute());

	Add(TEXT("Attributes.Secondary.Conversion.CorruptionToPhysical"),  UHunterAttributeSet::GetCorruptionToPhysicalAttribute());
	Add(TEXT("Attributes.Secondary.Conversion.CorruptionToFire"),      UHunterAttributeSet::GetCorruptionToFireAttribute());
	Add(TEXT("Attributes.Secondary.Conversion.CorruptionToIce"),       UHunterAttributeSet::GetCorruptionToIceAttribute());
	Add(TEXT("Attributes.Secondary.Conversion.CorruptionToLightning"), UHunterAttributeSet::GetCorruptionToLightningAttribute());
	Add(TEXT("Attributes.Secondary.Conversion.CorruptionToLight"),     UHunterAttributeSet::GetCorruptionToLightAttribute());

	// ===========================
	// Misc
	// ===========================
	Add(TEXT("Attributes.Secondary.Money.Gems"),           UHunterAttributeSet::GetGemsAttribute());
	Add(TEXT("Attributes.Secondary.Misc.Poise"),           UHunterAttributeSet::GetPoiseAttribute());
	Add(TEXT("Attributes.Secondary.Misc.Weight"),          UHunterAttributeSet::GetWeightAttribute());
	Add(TEXT("Attributes.Secondary.Misc.StunRecovery"),    UHunterAttributeSet::GetStunRecoveryAttribute());
	Add(TEXT("Attributes.Secondary.Misc.MovementSpeed"),   UHunterAttributeSet::GetMovementSpeedAttribute());
	Add(TEXT("Attributes.Secondary.Misc.CoolDown"),        UHunterAttributeSet::GetCooldownAttribute());
	Add(TEXT("Attributes.Secondary.Misc.ManaCostChanges"), UHunterAttributeSet::GetManaCostChangesAttribute());
	Add(TEXT("Attributes.Secondary.Misc.LifeLeech"),       UHunterAttributeSet::GetLifeLeechAttribute());
	Add(TEXT("Attributes.Secondary.Misc.ManaLeech"),       UHunterAttributeSet::GetManaLeechAttribute());
	Add(TEXT("Attributes.Secondary.Misc.LifeOnHit"),       UHunterAttributeSet::GetLifeOnHitAttribute());
	Add(TEXT("Attributes.Secondary.Misc.ManaOnHit"),       UHunterAttributeSet::GetManaOnHitAttribute());
	Add(TEXT("Attributes.Secondary.Misc.StaminaOnHit"),    UHunterAttributeSet::GetStaminaOnHitAttribute());
	Add(TEXT("Attributes.Secondary.Misc.StaminaCostChanges"),UHunterAttributeSet::GetStaminaCostChangesAttribute());
	Add(TEXT("Attributes.Secondary.Misc.CritChance"),      UHunterAttributeSet::GetCritChanceAttribute());     // mirrors Misc tag
	Add(TEXT("Attributes.Secondary.Misc.CritMultiplier"),  UHunterAttributeSet::GetCritMultiplierAttribute()); // mirrors Misc tag

	// ===========================
	// Status Effects (aliases)
	// ===========================
	Add(TEXT("Attributes.Secondary.Ailments.ChanceToBleed"),     UHunterAttributeSet::GetChanceToBleedAttribute());
	Add(TEXT("Attributes.Secondary.Ailments.ChanceToIgnite"),    UHunterAttributeSet::GetChanceToIgniteAttribute());
	Add(TEXT("Attributes.Secondary.Ailments.ChanceToFreeze"),    UHunterAttributeSet::GetChanceToFreezeAttribute());
	Add(TEXT("Attributes.Secondary.Ailments.ChanceToShock"),     UHunterAttributeSet::GetChanceToShockAttribute());
	Add(TEXT("Attributes.Secondary.Ailments.ChanceToStun"),      UHunterAttributeSet::GetChanceToStunAttribute());
	Add(TEXT("Attributes.Secondary.Ailments.ChanceToKnockBack"), UHunterAttributeSet::GetChanceToKnockBackAttribute());
	Add(TEXT("Attributes.Secondary.Ailments.ChanceToPetrify"),   UHunterAttributeSet::GetChanceToPetrifyAttribute());
	Add(TEXT("Attributes.Secondary.Ailments.ChanceToPurify"),    UHunterAttributeSet::GetChanceToPurifyAttribute());
	Add(TEXT("Attributes.Secondary.Ailments.ChanceToCorrupt"),   UHunterAttributeSet::GetChanceToCorruptAttribute());

	Add(TEXT("Attributes.Secondary.Duration.Bleed"),          UHunterAttributeSet::GetBleedDurationAttribute());
	Add(TEXT("Attributes.Secondary.Duration.Burn"),           UHunterAttributeSet::GetBurnDurationAttribute());
	Add(TEXT("Attributes.Secondary.Duration.Freeze"),         UHunterAttributeSet::GetFreezeDurationAttribute());
	Add(TEXT("Attributes.Secondary.Duration.Shock"),          UHunterAttributeSet::GetShockDurationAttribute());
	Add(TEXT("Attributes.Secondary.Duration.Corruption"),     UHunterAttributeSet::GetCorruptionDurationAttribute());
	Add(TEXT("Attributes.Secondary.Duration.PetrifyBuildUp"), UHunterAttributeSet::GetPetrifyBuildUpDurationAttribute());
	Add(TEXT("Attributes.Secondary.Duration.Purify"),         UHunterAttributeSet::GetPurifyDurationAttribute());

	// (Optional) Log summary for sanity
	UE_LOG(LogTemp, Log, TEXT("[PHGameplayTags] RegisterAllAttribute(): %d attributes in AllAttributesMap, %d status tags, %d min/max pairs."),
		AllAttributesMap.Num(),
		StatusEffectTagToAttributeMap.Num(),
		TagsMinMax.Num());
}


