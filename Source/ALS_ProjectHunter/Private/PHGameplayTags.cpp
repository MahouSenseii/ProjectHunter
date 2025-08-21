// Copyright@2024 Quentin Davis 

#include "PHGameplayTags.h"
#include "GameplayTagsManager.h"
#include "AbilitySystem/PHAttributeSet.h"



// === Static Map Definitions ===
TMap<FGameplayTag, FGameplayAttribute> FPHGameplayTags::StatusEffectTagToAttributeMap;
TMap<FGameplayTag, FGameplayTag> FPHGameplayTags::TagsMinMax;
TMap<FString, FGameplayAttribute> FPHGameplayTags::FlatDamageToAttributesMap;
TMap<FString, FGameplayAttribute> FPHGameplayTags::PercentDamageToAttributesMap;
TMap<FString, FGameplayAttribute> FPHGameplayTags::BaseDamageToAttributesMap;
TMap<FString, FGameplayAttribute> FPHGameplayTags::AllAttributesMap;



// Initialize the static singleton instance
FPHGameplayTags FPHGameplayTags::GameplayTags;

#define DEFINE_GAMEPLAY_TAG(TagName) FGameplayTag FPHGameplayTags::TagName;
// === Static FGameplayTag Definitions ===
// === Primary Attributes ===
DEFINE_GAMEPLAY_TAG(Attributes_Primary_Strength)
DEFINE_GAMEPLAY_TAG(Attributes_Primary_Intelligence)
DEFINE_GAMEPLAY_TAG(Attributes_Primary_Dexterity)
DEFINE_GAMEPLAY_TAG(Attributes_Primary_Endurance)
DEFINE_GAMEPLAY_TAG(Attributes_Primary_Affliction)
DEFINE_GAMEPLAY_TAG(Attributes_Primary_Luck)
DEFINE_GAMEPLAY_TAG(Attributes_Primary_Covenant)

// === Secondary Attributes: Defenses ===
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Defenses_Armor)

// === Secondary Attributes: Health
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_MaxHealth)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_HealthRegenRate)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_HealthRegenAmount)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_MaxHealthRegenAmount)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_MaxHealthRegenRate)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_HealthReservedAmount)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_MaxHealthReservedAmount)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_HealthFlatReservedAmount)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_HealthPercentageReserved)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_MaxEffectiveHealth)

// === Secondary Attributes: Mana
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_MaxMana)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_ManaRegenRate)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_ManaRegenAmount)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_MaxManaRegenRate)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_MaxManaRegenAmount)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_ManaReservedAmount)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_MaxManaReservedAmount)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_ManaFlatReservedAmount)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_ManaPercentageReserved)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_MaxEffectiveMana)

// === Secondary Attributes: Stamina
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_MaxStamina)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_MaxEffectiveStamina)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_StaminaRegenRate)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_StaminaRegenAmount)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_MaxStaminaRegenRate)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_StaminaFlatReservedAmount)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_StaminaPercentageReserved)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_MaxStaminaRegenAmount)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_StaminaReservedAmount)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_MaxStaminaReservedAmount)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_StaminaDegen)

// === Secondary Attributes: Arcane Shield
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_ArcaneShield)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_ArcaneShieldRegenRate)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_MaxArcaneShieldRegenRate)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_MaxArcaneShieldRegenAmount)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_MaxArcaneShieldReservedAmount)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_ArcaneShieldReservedAmount)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_ArcaneShieldRegenAmount)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_ArcaneShieldFlatReservedAmount)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Vital_ArcaneShieldPercentageReserved)

// === Damage: Min and Max
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

// === Damage: Flat & Percent Bonus
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

// === Resistances
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Resistances_GlobalDefenses)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Resistances_Armour)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Resistances_ArmourFlatBonus)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Resistances_ArmourPercentBonus)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Resistances_FireResistanceFlat)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Resistances_IceResistanceFlat)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Resistances_LightResistanceFlat)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Resistances_LightningResistanceFlat)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Resistances_CorruptionResistanceFlat)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Resistances_FireResistancePercentage)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Resistances_IceResistancePercentage)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Resistances_LightResistancePercentage)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Resistances_LightningResistancePercentage)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Resistances_CorruptionResistancePercentage)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Resistances_MaxFireResistance)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Resistances_MaxIceResistance)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Resistances_MaxLightResistance)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Resistances_MaxLightningResistance)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Resistances_MaxCorruptionResistance)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Resistances_BlockStrength)

// === Miscellaneous
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Misc_Poise)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Misc_Weight)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Misc_StunRecovery)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Misc_ManaCostChanges)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Misc_CoolDown)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Misc_LifeLeech)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Misc_ManaLeech)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Misc_MovementSpeed)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Misc_LifeOnHit)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Misc_ManaOnHit)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Misc_StaminaOnHit)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Misc_StaminaCostChanges)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Misc_CritChance)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Misc_CritMultiplier)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Money_Gems)
DEFINE_GAMEPLAY_TAG(Relation_HostileToSource)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Misc_CombatAlignment)

// === Vitals
DEFINE_GAMEPLAY_TAG(Attributes_Vital_Health)
DEFINE_GAMEPLAY_TAG(Attributes_Vital_Stamina)
DEFINE_GAMEPLAY_TAG(Attributes_Vital_Mana)

// === Status Effect Chances
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_ChanceToApply_ChanceToBleed)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_ChanceToApply_ChanceToIgnite)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_ChanceToApply_ChanceToFreeze)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_ChanceToApply_ChanceToShock)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_ChanceToApply_ChanceToStun)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_ChanceToApply_ChanceToKnockBack)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_ChanceToApply_ChanceToPetrify)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_ChanceToApply_ChanceToPurify)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_ChanceToApply_ChanceToCorrupt)

// === Status Effect Durations
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Duration_BleedDuration)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Duration_BurnDuration)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Duration_FreezeDuration)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Duration_ShockDuration)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Duration_CorruptionDuration)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Duration_PetrifyBuildUpDuration)
DEFINE_GAMEPLAY_TAG(Attributes_Secondary_Duration_PurifyDuration)

// === Conditions: Health, Mana, Stamina, ArcaneShield ===
DEFINE_GAMEPLAY_TAG(Condition_OnFullHealth)
DEFINE_GAMEPLAY_TAG(Condition_OnLowHealth)
DEFINE_GAMEPLAY_TAG(Condition_OnFullMana)
DEFINE_GAMEPLAY_TAG(Condition_OnLowMana)
DEFINE_GAMEPLAY_TAG(Condition_OnFullStamina)
DEFINE_GAMEPLAY_TAG(Condition_OnLowStamina)
DEFINE_GAMEPLAY_TAG(Condition_OnFullArcaneShield)
DEFINE_GAMEPLAY_TAG(Condition_OnLowArcaneShield)

// === Basic States ===
DEFINE_GAMEPLAY_TAG(Condition_Alive)
DEFINE_GAMEPLAY_TAG(Condition_Dead)
DEFINE_GAMEPLAY_TAG(Condition_NearDeathExperience)
DEFINE_GAMEPLAY_TAG(Condition_DeathPrevented)

// === Combat States ===
DEFINE_GAMEPLAY_TAG(Condition_OnKill)
DEFINE_GAMEPLAY_TAG(Condition_OnCrit)
DEFINE_GAMEPLAY_TAG(Condition_RecentlyHit)
DEFINE_GAMEPLAY_TAG(Condition_RecentlyCrit)
DEFINE_GAMEPLAY_TAG(Condition_RecentlyBlocked)
DEFINE_GAMEPLAY_TAG(Condition_TakingDamage)
DEFINE_GAMEPLAY_TAG(Condition_DealingDamage)
DEFINE_GAMEPLAY_TAG(Condition_RecentlyUsedSkill)
DEFINE_GAMEPLAY_TAG(Condition_RecentlyAppliedBuff)
DEFINE_GAMEPLAY_TAG(Condition_RecentlyDispelled)

// === Action States ===
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

// === Buff/Debuff States ===
DEFINE_GAMEPLAY_TAG(Condition_HasBuff)
DEFINE_GAMEPLAY_TAG(Condition_HasDebuff)
DEFINE_GAMEPLAY_TAG(Condition_BuffDurationBelow50)
DEFINE_GAMEPLAY_TAG(Condition_EffectDurationExpired)

// === Target Conditions ===
DEFINE_GAMEPLAY_TAG(Condition_TargetIsBoss)
DEFINE_GAMEPLAY_TAG(Condition_TargetIsMinion)
DEFINE_GAMEPLAY_TAG(Condition_TargetHasShield)
DEFINE_GAMEPLAY_TAG(Condition_TargetIsCasting)

// === Position / Environment ===
DEFINE_GAMEPLAY_TAG(Condition_NearAllies)
DEFINE_GAMEPLAY_TAG(Condition_NearEnemies)
DEFINE_GAMEPLAY_TAG(Condition_Alone)
DEFINE_GAMEPLAY_TAG(Condition_InLight)
DEFINE_GAMEPLAY_TAG(Condition_InDark)
DEFINE_GAMEPLAY_TAG(Condition_InDangerZone)

// === Self Status Effects ===
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
DEFINE_GAMEPLAY_TAG(Condition_Self_ZeroArcaneShield)

// === Target Status Effects ===
DEFINE_GAMEPLAY_TAG(Condition_Target_Bleeding)
DEFINE_GAMEPLAY_TAG(Condition_Target_Stunned)
DEFINE_GAMEPLAY_TAG(Condition_Target_Frozen)
DEFINE_GAMEPLAY_TAG(Condition_Target_Shocked)
DEFINE_GAMEPLAY_TAG(Condition_Target_Burned)
DEFINE_GAMEPLAY_TAG(Condition_Target_Corrupted)
DEFINE_GAMEPLAY_TAG(Condition_Target_Purified)
DEFINE_GAMEPLAY_TAG(Condition_Target_Petrified)

// === Immunities & Restrictions ===
DEFINE_GAMEPLAY_TAG(Condition_ImmuneToCC)
DEFINE_GAMEPLAY_TAG(Condition_CannotBeFrozen)
DEFINE_GAMEPLAY_TAG(Condition_CannotBeCorrupted)
DEFINE_GAMEPLAY_TAG(Condition_CannotBeBurned)
DEFINE_GAMEPLAY_TAG(Condition_CannotBeSlowed)
DEFINE_GAMEPLAY_TAG(Condition_CannotBeInterrupted)
DEFINE_GAMEPLAY_TAG(Condition_CannotBeKnockedBack)

// === Triggers ===
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
DEFINE_GAMEPLAY_TAG(Condition_HasMeleeWeaponEquipped)
DEFINE_GAMEPLAY_TAG(Condition_HasBowEquipped)
DEFINE_GAMEPLAY_TAG(Condition_HasShieldEquipped)
DEFINE_GAMEPLAY_TAG(Condition_HasStaffEquipped)

#undef DEFINE_GAMEPLAY_TAG

/* ============================= */
/* === Initialize Gameplay Tags === */
/* ============================= */
void FPHGameplayTags::InitializeNativeGameplayTags()
{
	UGameplayTagsManager& TagsManager = UGameplayTagsManager::Get();

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
	RegisterAllAttribute();
}

void FPHGameplayTags::RegisterPrimaryAttributes()
{
	UGameplayTagsManager& TagsManager = UGameplayTagsManager::Get();

	Attributes_Primary_Strength = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Primary.Strength"),
		TEXT("Increases physical damage and slightly increases health."));

	Attributes_Primary_Intelligence = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Primary.Intelligence"),
		TEXT("Increases mana and slightly increases elemental damage."));

	Attributes_Primary_Dexterity = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Primary.Dexterity"),
		TEXT("Increases critical multiplier and slightly increases attack and cast speed."));

	Attributes_Primary_Endurance = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Primary.Endurance"),
		TEXT("Increases stamina and slightly increases resistances."));

	Attributes_Primary_Affliction = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Primary.Affliction"),
		TEXT("Increases damage over time and slightly increases effect duration."));

	Attributes_Primary_Luck = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Primary.Luck"),
		TEXT("Increases chance to apply status effects and improves item drop rates."));

	Attributes_Primary_Covenant = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Primary.Covenant"),
		TEXT("Improves the strength and durability of your summoned allies or minions."));
}

void FPHGameplayTags::RegisterSecondaryVitals()
{
	UGameplayTagsManager& TagsManager = UGameplayTagsManager::Get();

	/* === Health & Regen === */
	Attributes_Secondary_Vital_MaxHealth = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Vital.MaxHealth"),
		TEXT("Maximum amount of health."));

	Attributes_Secondary_Vital_HealthRegenRate = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Vital.HealthRegenRate"),
		TEXT("Rate at which health regenerates."));

	Attributes_Secondary_Vital_HealthRegenAmount = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Vital.HealthRegenAmount"),
		TEXT("Amount of health restored per regeneration tick."));

	Attributes_Secondary_Vital_MaxHealthRegenAmount = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Vital.MaxHealthRegenAmount"),
		TEXT("Maximum health restored per tick."));

	Attributes_Secondary_Vital_MaxHealthRegenRate = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Vital.MaxHealthRegenRate"),
		TEXT("Maximum health regeneration rate."));

	Attributes_Secondary_Vital_HealthReservedAmount = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Vital.HealthReservedAmount"),
		TEXT("Total health reserved (not usable)."));

	Attributes_Secondary_Vital_MaxHealthReservedAmount = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Vital.MaxHealthReservedAmount"),
		TEXT("Maximum health that can be reserved."));

	Attributes_Secondary_Vital_HealthFlatReservedAmount = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Vital.HealthFlatReservedAmount"),
		TEXT("Flat amount of health reserved."));

	Attributes_Secondary_Vital_HealthPercentageReserved = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Vital.HealthPercentageReserved"),
		TEXT("Percentage of health reserved."));

	Attributes_Secondary_Vital_MaxEffectiveHealth = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Vital.MaxEffectiveHealth"),
		TEXT("Effective maximum health after reservations."));

	/* === Mana & Regen === */
	Attributes_Secondary_Vital_MaxMana = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Vital.MaxMana"),
		TEXT("Maximum amount of mana."));

	Attributes_Secondary_Vital_ManaRegenRate = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Vital.ManaRegenRate"),
		TEXT("Rate at which mana regenerates."));

	Attributes_Secondary_Vital_ManaRegenAmount = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Vital.ManaRegenAmount"),
		TEXT("Amount of mana restored per tick."));

	Attributes_Secondary_Vital_MaxManaRegenRate = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Vital.MaxManaRegenRate"),
		TEXT("Maximum rate at which mana regenerates."));

	Attributes_Secondary_Vital_MaxManaRegenAmount = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Vital.MaxManaRegenAmount"),
		TEXT("Maximum mana restored per tick."));

	Attributes_Secondary_Vital_ManaReservedAmount = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Vital.ManaReservedAmount"),
		TEXT("Total mana reserved (not usable)."));


	Attributes_Secondary_Vital_MaxManaReservedAmount = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Vital.MaxManaReservedAmount"),
		TEXT("Maximum mana that can be reserved."));

	Attributes_Secondary_Vital_ManaFlatReservedAmount = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Vital.ManaFlatReservedAmount"),
		TEXT("Flat mana reserved."));

	Attributes_Secondary_Vital_ManaPercentageReserved = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Vital.ManaPercentageReserved"),
		TEXT("Percentage of mana reserved."));

	Attributes_Secondary_Vital_MaxEffectiveMana = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Vital.MaxEffectiveMana"),
		TEXT("Effective max mana after reservation."));

	/* === Stamina & Regen === */
	Attributes_Secondary_Vital_MaxStamina = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Vital.MaxStamina"),
		TEXT("Maximum stamina capacity."));

	Attributes_Secondary_Vital_MaxEffectiveStamina = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Vital.MaxEffectiveStamina"),
		TEXT("Effective maximum stamina."));

	Attributes_Secondary_Vital_StaminaRegenRate = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Vital.StaminaRegenRate"),
		TEXT("Stamina regeneration rate."));

	Attributes_Secondary_Vital_StaminaRegenAmount = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Vital.StaminaRegenAmount"),
		TEXT("Amount of stamina regenerated per tick."));

	Attributes_Secondary_Vital_MaxStaminaRegenRate = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Vital.MaxStaminaRegenRate"),
		TEXT("Maximum stamina regen rate."));

	Attributes_Secondary_Vital_StaminaFlatReservedAmount = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Vital.StaminaFlatReservedAmount"),
		TEXT("Flat stamina reserved."));

	Attributes_Secondary_Vital_StaminaPercentageReserved = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Vital.StaminaPercentageReserved"),
		TEXT("Percentage of stamina reserved."));

	Attributes_Secondary_Vital_MaxStaminaRegenAmount = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Vital.MaxStaminaRegenAmount"),
		TEXT("Max stamina restored per tick."));

	Attributes_Secondary_Vital_StaminaReservedAmount = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Vital.StaminaReservedAmount"),
		TEXT("Stamina reserved (not usable)."));

	Attributes_Secondary_Vital_MaxStaminaReservedAmount = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Vital.MaxStaminaReservedAmount"),
		TEXT("Maximum stamina that can be reserved."));

	Attributes_Secondary_Vital_StaminaDegen = TagsManager.AddNativeGameplayTag(
	FName("Attribute.Secondary.Vital.StaminaDegen"),
	TEXT("Stamina Degen."));

	/* === Arcane Shield === */
	Attributes_Secondary_Vital_ArcaneShield = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Vital.ArcaneShield"),
		TEXT("Current arcane shield value."));

	Attributes_Secondary_Vital_ArcaneShieldRegenRate = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Vital.ArcaneShieldRegenRate"),
		TEXT("Rate of arcane shield regeneration."));

	Attributes_Secondary_Vital_MaxArcaneShieldRegenRate = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Vital.MaxArcaneShieldRegenRate"),
		TEXT("Maximum arcane shield regen rate."));

	Attributes_Secondary_Vital_MaxArcaneShieldRegenAmount = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Vital.MaxArcaneShieldRegenAmount"),
		TEXT("Maximum arcane shield restored per tick."));

	Attributes_Secondary_Vital_MaxArcaneShieldReservedAmount = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Vital.MaxArcaneShieldReservedAmount"),
		TEXT("Max arcane shield that can be reserved."));

	Attributes_Secondary_Vital_ArcaneShieldReservedAmount = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Vital.ArcaneShieldReservedAmount"),
		TEXT("Total arcane shield reserved."));

	Attributes_Secondary_Vital_ArcaneShieldRegenAmount = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Vital.ArcaneShieldRegenAmount"),
		TEXT("Arcane shield restored per tick."));

	Attributes_Secondary_Vital_ArcaneShieldFlatReservedAmount = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Vital.ArcaneShieldFlatReservedAmount"),
		TEXT("Flat arcane shield reserved."));

	Attributes_Secondary_Vital_ArcaneShieldPercentageReserved = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Vital.ArcaneShieldPercentageReserved"),
		TEXT("Percentage of arcane shield reserved."));
}


void FPHGameplayTags::RegisterDamageTags()
{
	UGameplayTagsManager& TagsManager = UGameplayTagsManager::Get();

	// === Min Damage ===
	Attributes_Secondary_Damages_MinPhysicalDamage = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Damage.Min.Physical"),
		TEXT("Minimum base physical damage."));

	Attributes_Secondary_Damages_MinFireDamage = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Damage.Min.Fire"),
		TEXT("Minimum base fire damage."));

	Attributes_Secondary_Damages_MinIceDamage = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Damage.Min.Ice"),
		TEXT("Minimum base ice damage."));

	Attributes_Secondary_Damages_MinLightDamage = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Damage.Min.Light"),
		TEXT("Minimum base light (holy) damage."));

	Attributes_Secondary_Damages_MinLightningDamage = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Damage.Min.Lightning"),
		TEXT("Minimum base lightning damage."));

	Attributes_Secondary_Damages_MinCorruptionDamage = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Damage.Min.Corruption"),
		TEXT("Minimum base corruption (dark) damage."));

	// === Max Damage ===
	Attributes_Secondary_Damages_MaxPhysicalDamage = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Damage.Max.Physical"),
		TEXT("Maximum base physical damage."));

	Attributes_Secondary_Damages_MaxFireDamage = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Damage.Max.Fire"),
		TEXT("Maximum base fire damage."));

	Attributes_Secondary_Damages_MaxIceDamage = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Damage.Max.Ice"),
		TEXT("Maximum base ice damage."));

	Attributes_Secondary_Damages_MaxLightDamage = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Damage.Max.Light"),
		TEXT("Maximum base light (holy) damage."));

	Attributes_Secondary_Damages_MaxLightningDamage = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Damage.Max.Lightning"),
		TEXT("Maximum base lightning damage."));

	Attributes_Secondary_Damages_MaxCorruptionDamage = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Damage.Max.Corruption"),
		TEXT("Maximum base corruption (dark) damage."));

	// === Global Damage Bonus ===
	Attributes_Secondary_BonusDamage_GlobalDamages = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Damage.GlobalBonus"),
		TEXT("Increases all types of damage."));

	// === Flat Damage Bonuses ===
	Attributes_Secondary_BonusDamage_PhysicalFlatBonus = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Damage.Flat.Physical"),
		TEXT("Flat physical damage bonus."));

	Attributes_Secondary_BonusDamage_FireFlatBonus = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Damage.Flat.Fire"),
		TEXT("Flat fire damage bonus."));

	Attributes_Secondary_BonusDamage_IceFlatBonus = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Damage.Flat.Ice"),
		TEXT("Flat ice damage bonus."));

	Attributes_Secondary_BonusDamage_LightFlatBonus = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Damage.Flat.Light"),
		TEXT("Flat light damage bonus."));

	Attributes_Secondary_BonusDamage_LightningFlatBonus = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Damage.Flat.Lightning"),
		TEXT("Flat lightning damage bonus."));

	Attributes_Secondary_BonusDamage_CorruptionFlatBonus = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Damage.Flat.Corruption"),
		TEXT("Flat corruption damage bonus."));

	// === Percent Damage Bonuses ===
	Attributes_Secondary_BonusDamage_PhysicalPercentBonus = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Damage.Percent.Physical"),
		TEXT("Percent bonus to physical damage."));

	Attributes_Secondary_BonusDamage_FirePercentBonus = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Damage.Percent.Fire"),
		TEXT("Percent bonus to fire damage."));

	Attributes_Secondary_BonusDamage_IcePercentBonus = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Damage.Percent.Ice"),
		TEXT("Percent bonus to ice damage."));

	Attributes_Secondary_BonusDamage_LightPercentBonus = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Damage.Percent.Light"),
		TEXT("Percent bonus to light damage."));

	Attributes_Secondary_BonusDamage_LightningPercentBonus = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Damage.Percent.Lightning"),
		TEXT("Percent bonus to lightning damage."));

	Attributes_Secondary_BonusDamage_CorruptionPercentBonus = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Damage.Percent.Corruption"),
		TEXT("Percent bonus to corruption damage."));
}


void FPHGameplayTags::RegisterResistanceTags()
{
	UGameplayTagsManager& TagsManager = UGameplayTagsManager::Get();

	// === General Defenses ===
	Attributes_Secondary_Resistances_GlobalDefenses = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Resistance.GlobalDefenses"),
		TEXT("Increases general defense against all types of damage."));

	Attributes_Secondary_Resistances_Armour = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Resistance.Armour"),
		TEXT("Reduces incoming physical damage."));

	Attributes_Secondary_Resistances_BlockStrength = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Resistance.BlockStrength"),
		TEXT("Reduces damage taken when blocking."));

	// === Armour Bonuses ===
	Attributes_Secondary_Resistances_ArmourFlatBonus = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Resistance.Armour.Flat"),
		TEXT("Flat increase to armour."));

	Attributes_Secondary_Resistances_ArmourPercentBonus = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Resistance.Armour.Percent"),
		TEXT("Percentage increase to armour."));

	// === Fire Resistance ===
	Attributes_Secondary_Resistances_FireResistanceFlat = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Resistance.Fire.Flat"),
		TEXT("Flat fire resistance bonus."));

	Attributes_Secondary_Resistances_FireResistancePercentage = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Resistance.Fire.Percent"),
		TEXT("Percentage fire resistance bonus."));

	Attributes_Secondary_Resistances_MaxFireResistance = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Resistance.Fire.Max"),
		TEXT("Maximum fire resistance cap."));

	// === Ice Resistance ===
	Attributes_Secondary_Resistances_IceResistanceFlat = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Resistance.Ice.Flat"),
		TEXT("Flat ice resistance bonus."));

	Attributes_Secondary_Resistances_IceResistancePercentage = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Resistance.Ice.Percent"),
		TEXT("Percentage ice resistance bonus."));

	Attributes_Secondary_Resistances_MaxIceResistance = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Resistance.Ice.Max"),
		TEXT("Maximum ice resistance cap."));

	// === Light Resistance ===
	Attributes_Secondary_Resistances_LightResistanceFlat = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Resistance.Light.Flat"),
		TEXT("Flat light resistance bonus."));

	Attributes_Secondary_Resistances_LightResistancePercentage = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Resistance.Light.Percent"),
		TEXT("Percentage light resistance bonus."));

	Attributes_Secondary_Resistances_MaxLightResistance = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Resistance.Light.Max"),
		TEXT("Maximum light resistance cap."));

	// === Lightning Resistance ===
	Attributes_Secondary_Resistances_LightningResistanceFlat = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Resistance.Lightning.Flat"),
		TEXT("Flat lightning resistance bonus."));

	Attributes_Secondary_Resistances_LightningResistancePercentage = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Resistance.Lightning.Percent"),
		TEXT("Percentage lightning resistance bonus."));

	Attributes_Secondary_Resistances_MaxLightningResistance = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Resistance.Lightning.Max"),
		TEXT("Maximum lightning resistance cap."));

	// === Corruption Resistance ===
	Attributes_Secondary_Resistances_CorruptionResistanceFlat = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Resistance.Corruption.Flat"),
		TEXT("Flat corruption resistance bonus."));

	Attributes_Secondary_Resistances_CorruptionResistancePercentage = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Resistance.Corruption.Percent"),
		TEXT("Percentage corruption resistance bonus."));

	Attributes_Secondary_Resistances_MaxCorruptionResistance = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Resistance.Corruption.Max"),
		TEXT("Maximum corruption resistance cap."));
}


void FPHGameplayTags::RegisterMiscAttributes()
{
		UGameplayTagsManager& TagsManager = UGameplayTagsManager::Get();

	Attributes_Secondary_Misc_Poise = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Misc.Poise"),
		TEXT("Prevents staggering until depleted."));

	Attributes_Secondary_Misc_StunRecovery = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Misc.StunRecovery"),
		TEXT("Reduces stun duration."));

	Attributes_Secondary_Misc_CoolDown = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Misc.CoolDown"),
		TEXT("Cooldown reduction for abilities."));

	Attributes_Secondary_Misc_ManaCostChanges = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Misc.ManaCostChanges"),
		TEXT("Changes in mana cost for spells or skills."));

	Attributes_Secondary_Misc_LifeLeech = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Misc.LifeLeech"),
		TEXT("Percent of damage returned as health."));

	Attributes_Secondary_Misc_ManaLeech = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Misc.ManaLeech"),
		TEXT("Percent of damage returned as mana."));

	Attributes_Secondary_Misc_MovementSpeed = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Misc.MovementSpeed"),
		TEXT("Speed at which the character moves."));

	Attributes_Secondary_Misc_LifeOnHit = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Misc.LifeOnHit"),
		TEXT("Health gained on hitting an enemy."));

	Attributes_Secondary_Misc_ManaOnHit = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Misc.ManaOnHit"),
		TEXT("Mana gained on hitting an enemy."));

	Attributes_Secondary_Misc_StaminaOnHit = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Misc.StaminaOnHit"),
		TEXT("Stamina gained on hitting an enemy."));

	Attributes_Secondary_Misc_StaminaCostChanges = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Misc.StaminaCostChanges"),
		TEXT("Changes in stamina cost for actions."));

	Attributes_Secondary_Money_Gems = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Money.Gems"),
		TEXT("Currency used in trade or upgrades."));

	Attributes_Secondary_Misc_CombatAlignment = TagsManager.AddNativeGameplayTag(
		FName("Attribute.Secondary.Misc.CombatAlignment"),
		TEXT("Allegiance or team designation for combat logic."));
}

void FPHGameplayTags::RegisterVitals()
{
		UGameplayTagsManager& TagsManager = UGameplayTagsManager::Get();

		Attributes_Vital_Health = TagsManager.AddNativeGameplayTag(
			FName("Attribute.Vital.Health"),
			TEXT("Current health value of the character."));

		Attributes_Vital_Stamina = TagsManager.AddNativeGameplayTag(
			FName("Attribute.Vital.Stamina"),
			TEXT("Current stamina value used for movement and physical actions."));

		Attributes_Vital_Mana = TagsManager.AddNativeGameplayTag(
			FName("Attribute.Vital.Mana"),
			TEXT("Current mana value used for spells and skills."));
}

void FPHGameplayTags::RegisterStatusEffectChances()
{
	UGameplayTagsManager& TagsManager = UGameplayTagsManager::Get();

	Attributes_Secondary_ChanceToApply_ChanceToBleed = TagsManager.AddNativeGameplayTag(
		FName("StatusEffect.ChanceToApply.Bleed"),
		TEXT("Chance to apply Bleed based on physical damage and Luck."));

	Attributes_Secondary_ChanceToApply_ChanceToIgnite = TagsManager.AddNativeGameplayTag(
		FName("StatusEffect.ChanceToApply.Ignite"),
		TEXT("Chance to apply Ignite (Burn) based on fire damage and Luck."));

	Attributes_Secondary_ChanceToApply_ChanceToFreeze = TagsManager.AddNativeGameplayTag(
		FName("StatusEffect.ChanceToApply.Freeze"),
		TEXT("Chance to apply Freeze based on ice damage and Luck."));

	Attributes_Secondary_ChanceToApply_ChanceToShock = TagsManager.AddNativeGameplayTag(
		FName("StatusEffect.ChanceToApply.Shock"),
		TEXT("Chance to apply Shock based on lightning damage and Luck."));

	Attributes_Secondary_ChanceToApply_ChanceToStun = TagsManager.AddNativeGameplayTag(
		FName("StatusEffect.ChanceToApply.Stun"),
		TEXT("Chance to apply Stun based on Dexterity and Affliction."));

	Attributes_Secondary_ChanceToApply_ChanceToKnockBack = TagsManager.AddNativeGameplayTag(
		FName("StatusEffect.ChanceToApply.KnockBack"),
		TEXT("Chance to Knock Back the enemy."));

	Attributes_Secondary_ChanceToApply_ChanceToPetrify = TagsManager.AddNativeGameplayTag(
		FName("StatusEffect.ChanceToApply.Petrify"),
		TEXT("Chance to apply Petrify effect."));

	Attributes_Secondary_ChanceToApply_ChanceToPurify = TagsManager.AddNativeGameplayTag(
		FName("StatusEffect.ChanceToApply.Purify"),
		TEXT("Chance to apply Purification effect (e.g., cleanse debuffs)."));

	Attributes_Secondary_ChanceToApply_ChanceToCorrupt = TagsManager.AddNativeGameplayTag(
		FName("StatusEffect.ChanceToApply.Corrupt"),
		TEXT("Chance to apply Corruption-based DOT."));
}

void FPHGameplayTags::RegisterStatusEffectDurations()
{
	UGameplayTagsManager& TagsManager = UGameplayTagsManager::Get();

	Attributes_Secondary_Duration_BleedDuration = TagsManager.AddNativeGameplayTag(
		FName("StatusEffect.Duration.Bleed"),
		TEXT("Duration of Bleed (physical DOT) effects."));

	Attributes_Secondary_Duration_BurnDuration = TagsManager.AddNativeGameplayTag(
		FName("StatusEffect.Duration.Burn"),
		TEXT("Duration of Burn (fire DOT) effects."));

	Attributes_Secondary_Duration_FreezeDuration = TagsManager.AddNativeGameplayTag(
		FName("StatusEffect.Duration.Freeze"),
		TEXT("Duration of Freeze (immobilization) effects."));

	Attributes_Secondary_Duration_ShockDuration = TagsManager.AddNativeGameplayTag(
		FName("StatusEffect.Duration.Shock"),
		TEXT("Duration of Shock (stun/lightning) effects."));

	Attributes_Secondary_Duration_CorruptionDuration = TagsManager.AddNativeGameplayTag(
		FName("StatusEffect.Duration.Corruption"),
		TEXT("Duration of Corruption (DOT and debuffs)."));

	Attributes_Secondary_Duration_PetrifyBuildUpDuration = TagsManager.AddNativeGameplayTag(
		FName("StatusEffect.Duration.PetrifyBuildUp"),
		TEXT("Time required to trigger full Petrify."));

	Attributes_Secondary_Duration_PurifyDuration = TagsManager.AddNativeGameplayTag(
		FName("StatusEffect.Duration.Purify"),
		TEXT("Duration of Purification buff effect."));
}

void FPHGameplayTags::RegisterConditions()
{
		UGameplayTagsManager& TagsManager = UGameplayTagsManager::Get();

	Condition_Alive = TagsManager.AddNativeGameplayTag(FName("Condition.State.Alive"), TEXT("The entity is currently alive."));
	Condition_Dead = TagsManager.AddNativeGameplayTag(FName("Condition.State.Dead"), TEXT("The entity is dead and cannot act."));
	Condition_NearDeathExperience = TagsManager.AddNativeGameplayTag(FName("Condition.State.NearDeathExperience"), TEXT("Health is critically low."));
	Condition_DeathPrevented = TagsManager.AddNativeGameplayTag(FName("Condition.State.DeathPrevented"), TEXT("A death-preventing effect has occurred."));

	Condition_OnFullHealth = TagsManager.AddNativeGameplayTag(FName("Condition.Threshold.OnFullHealth"), TEXT("Health is at maximum."));
	Condition_OnLowHealth = TagsManager.AddNativeGameplayTag(FName("Condition.Threshold.OnLowHealth"), TEXT("Health is low."));
	Condition_OnFullMana = TagsManager.AddNativeGameplayTag(FName("Condition.Threshold.OnFullMana"), TEXT("Mana is at maximum."));
	Condition_OnLowMana = TagsManager.AddNativeGameplayTag(FName("Condition.Threshold.OnLowMana"), TEXT("Mana is low."));
	Condition_OnFullStamina = TagsManager.AddNativeGameplayTag(FName("Condition.Threshold.OnFullStamina"), TEXT("Stamina is at maximum."));
	Condition_OnLowStamina = TagsManager.AddNativeGameplayTag(FName("Condition.Threshold.OnLowStamina"), TEXT("Stamina is low."));
	Condition_OnFullArcaneShield = TagsManager.AddNativeGameplayTag(FName("Condition.Threshold.OnFullArcaneShield"), TEXT("Arcane shield is full."));
	Condition_OnLowArcaneShield = TagsManager.AddNativeGameplayTag(FName("Condition.Threshold.OnLowArcaneShield"), TEXT("Arcane shield is low."));

	Condition_OnKill = TagsManager.AddNativeGameplayTag(FName("Condition.Trigger.OnKill"), TEXT("Triggered after killing an enemy."));
	Condition_OnCrit = TagsManager.AddNativeGameplayTag(FName("Condition.Trigger.OnCrit"), TEXT("Triggered after landing a critical hit."));
	Condition_RecentlyHit = TagsManager.AddNativeGameplayTag(FName("Condition.Recently.ReceivedHit"), TEXT("Was hit recently."));
	Condition_RecentlyCrit = TagsManager.AddNativeGameplayTag(FName("Condition.Recently.ReceivedCrit"), TEXT("Was critically hit recently."));
	Condition_RecentlyBlocked = TagsManager.AddNativeGameplayTag(FName("Condition.Recently.Blocked"), TEXT("Blocked damage recently."));
	Condition_TakingDamage = TagsManager.AddNativeGameplayTag(FName("Condition.State.TakingDamage"), TEXT("Currently receiving damage."));
	Condition_DealingDamage = TagsManager.AddNativeGameplayTag(FName("Condition.State.DealingDamage"), TEXT("Currently dealing damage."));
	Condition_RecentlyUsedSkill = TagsManager.AddNativeGameplayTag(FName("Condition.Recently.UsedSkill"), TEXT("Used a skill recently."));
	Condition_RecentlyAppliedBuff = TagsManager.AddNativeGameplayTag(FName("Condition.Recently.AppliedBuff"), TEXT("Applied a buff recently."));
	Condition_RecentlyDispelled = TagsManager.AddNativeGameplayTag(FName("Condition.Recently.Dispelled"), TEXT("Dispelled a buff or debuff recently."));
	Condition_BuffDurationBelow50 = TagsManager.AddNativeGameplayTag(FName("Condition.Buff.DurationBelow50"), TEXT("Buff has less than 50% duration remaining."));
	Condition_EffectDurationExpired = TagsManager.AddNativeGameplayTag(FName("Condition.Effect.Expired"), TEXT("Effect duration has ended."));
	Condition_HasBuff = TagsManager.AddNativeGameplayTag(FName("Condition.Has.Buff"), TEXT("Has at least one buff active."));
	Condition_HasDebuff = TagsManager.AddNativeGameplayTag(FName("Condition.Has.Debuff"), TEXT("Has at least one debuff active."));
	Condition_ImmuneToCC = TagsManager.AddNativeGameplayTag(FName("Condition.Immune.CC"), TEXT("Immune to crowd control."));
	Condition_CannotBeFrozen = TagsManager.AddNativeGameplayTag(FName("Condition.Immune.Frozen"), TEXT("Cannot be frozen."));
	Condition_CannotBeCorrupted = TagsManager.AddNativeGameplayTag(FName("Condition.Immune.Corrupted"), TEXT("Cannot be corrupted."));
	Condition_CannotBeBurned = TagsManager.AddNativeGameplayTag(FName("Condition.Immune.Burned"), TEXT("Cannot be burned."));
	Condition_CannotBeSlowed = TagsManager.AddNativeGameplayTag(FName("Condition.Immune.Slowed"), TEXT("Cannot be slowed."));
	Condition_CannotBeInterrupted = TagsManager.AddNativeGameplayTag(FName("Condition.Immune.Interrupted"), TEXT("Cannot be interrupted."));
	Condition_CannotBeKnockedBack = TagsManager.AddNativeGameplayTag(FName("Condition.Immune.KnockBack"), TEXT("Cannot be knocked back."));
	Condition_Self_Bleeding = TagsManager.AddNativeGameplayTag(FName("Condition.Self.Bleeding"), TEXT("Self is affected by Bleed."));
	Condition_Self_Stunned = TagsManager.AddNativeGameplayTag(FName("Condition.Self.Stunned"), TEXT("Self is stunned."));
	Condition_Self_Frozen = TagsManager.AddNativeGameplayTag(FName("Condition.Self.Frozen"), TEXT("Self is frozen."));
	Condition_Self_Shocked = TagsManager.AddNativeGameplayTag(FName("Condition.Self.Shocked"), TEXT("Self is shocked."));
	Condition_Self_Burned = TagsManager.AddNativeGameplayTag(FName("Condition.Self.Burned"), TEXT("Self is burned."));
	Condition_Self_Corrupted = TagsManager.AddNativeGameplayTag(FName("Condition.Self.Corrupted"), TEXT("Self is corrupted."));
	Condition_Self_Purified = TagsManager.AddNativeGameplayTag(FName("Condition.Self.Purified"), TEXT("Self is purified."));
	Condition_Self_Petrified = TagsManager.AddNativeGameplayTag(FName("Condition.Self.Petrified"), TEXT("Self is petrified."));
	Condition_Self_CannotRegenHP = TagsManager.AddNativeGameplayTag(FName("Condition.Self.CannotRegenHP"), TEXT("Self cannot regenerate health."));
	Condition_Self_CannotRegenStamina = TagsManager.AddNativeGameplayTag(FName("Condition.Self.CannotRegenStamina"), TEXT("Self cannot regenerate stamina."));
	Condition_Self_CannotRegenMana = TagsManager.AddNativeGameplayTag(FName("Condition.Self.CannotRegenMana"), TEXT("Self cannot regenerate mana."));
	Condition_Self_CannotHealHPAbove50Percent = TagsManager.AddNativeGameplayTag(FName("Condition.Self.CannotHealHPAbove50Percent"), TEXT("Self cannot heal above 50% health."));
	Condition_Self_CannotHealStamina50Percent = TagsManager.AddNativeGameplayTag(FName("Condition.Self.CannotHealStamina50Percent"), TEXT("Self cannot heal above 50% stamina."));
	Condition_Self_CannotHealMana50Percent = TagsManager.AddNativeGameplayTag(FName("Condition.Self.CannotHealMana50Percent"), TEXT("Self cannot heal above 50% mana."));
	Condition_Self_ZeroArcaneShield = TagsManager.AddNativeGameplayTag(FName("Condition.Self.ZeroArcaneShield"), TEXT("Self has no arcane shield remaining."));

	// === Ailment & Status Effects (Target) ===
	Condition_Target_Bleeding = TagsManager.AddNativeGameplayTag(FName("Condition.Target.Bleeding"), TEXT("Target is bleeding."));
	Condition_Target_Stunned = TagsManager.AddNativeGameplayTag(FName("Condition.Target.Stunned"), TEXT("Target is stunned."));
	Condition_Target_Frozen = TagsManager.AddNativeGameplayTag(FName("Condition.Target.Frozen"), TEXT("Target is frozen."));
	Condition_Target_Shocked = TagsManager.AddNativeGameplayTag(FName("Condition.Target.Shocked"), TEXT("Target is shocked."));
	Condition_Target_Burned = TagsManager.AddNativeGameplayTag(FName("Condition.Target.Burned"), TEXT("Target is burned."));
	Condition_Target_Corrupted = TagsManager.AddNativeGameplayTag(FName("Condition.Target.Corrupted"), TEXT("Target is corrupted."));
	Condition_Target_Purified = TagsManager.AddNativeGameplayTag(FName("Condition.Target.Purified"), TEXT("Target is purified."));
	Condition_Target_Petrified = TagsManager.AddNativeGameplayTag(FName("Condition.Target.Petrified"), TEXT("Target is petrified."));
}

void FPHGameplayTags::RegisterConditionTriggers()
{
	UGameplayTagsManager& TagsManager = UGameplayTagsManager::Get();

	Condition_SkillRecentlyUsed = TagsManager.AddNativeGameplayTag(
		FName("Condition.Trigger.SkillRecentlyUsed"),
		TEXT("Triggered when a skill was recently used."));

	Condition_HitTakenRecently = TagsManager.AddNativeGameplayTag(
		FName("Condition.Trigger.HitTakenRecently"),
		TEXT("Triggered when damage was recently taken."));

	Condition_CritTakenRecently = TagsManager.AddNativeGameplayTag(
		FName("Condition.Trigger.CritTakenRecently"),
		TEXT("Triggered when a critical hit was recently taken."));

	Condition_KilledRecently = TagsManager.AddNativeGameplayTag(
		FName("Condition.Trigger.KilledRecently"),
		TEXT("Triggered after killing an enemy recently."));

	Condition_EnemyKilledRecently = TagsManager.AddNativeGameplayTag(
		FName("Condition.Trigger.EnemyKilledRecently"),
		TEXT("Triggered when an enemy was recently killed."));

	Condition_HitWithPhysicalDamage = TagsManager.AddNativeGameplayTag(
		FName("Condition.Trigger.HitWith.Physical"),
		TEXT("Triggered when hit with physical damage."));

	Condition_HitWithFireDamage = TagsManager.AddNativeGameplayTag(
		FName("Condition.Trigger.HitWith.Fire"),
		TEXT("Triggered when hit with fire damage."));

	Condition_HitWithLightningDamage = TagsManager.AddNativeGameplayTag(
		FName("Condition.Trigger.HitWith.Lightning"),
		TEXT("Triggered when hit with lightning damage."));

	Condition_HitWithProjectile = TagsManager.AddNativeGameplayTag(
		FName("Condition.Trigger.HitWith.Projectile"),
		TEXT("Triggered when hit by a projectile."));

	Condition_HitWithAoE = TagsManager.AddNativeGameplayTag(
		FName("Condition.Trigger.HitWith.AoE"),
		TEXT("Triggered when hit by an area-of-effect attack."));
}

void FPHGameplayTags::RegisterStatusEffectAttributes()
{
	StatusEffectTagToAttributeMap.Add(Attributes_Secondary_ChanceToApply_ChanceToBleed, UPHAttributeSet::GetChanceToBleedAttribute());
		StatusEffectTagToAttributeMap.Add(Attributes_Secondary_ChanceToApply_ChanceToIgnite, UPHAttributeSet::GetChanceToIgniteAttribute());
		StatusEffectTagToAttributeMap.Add(Attributes_Secondary_ChanceToApply_ChanceToFreeze, UPHAttributeSet::GetChanceToFreezeAttribute());
		StatusEffectTagToAttributeMap.Add(Attributes_Secondary_ChanceToApply_ChanceToShock, UPHAttributeSet::GetChanceToShockAttribute());
		StatusEffectTagToAttributeMap.Add(Attributes_Secondary_ChanceToApply_ChanceToStun, UPHAttributeSet::GetChanceToStunAttribute());
		StatusEffectTagToAttributeMap.Add(Attributes_Secondary_ChanceToApply_ChanceToKnockBack, UPHAttributeSet::GetChanceToKnockBackAttribute());
		StatusEffectTagToAttributeMap.Add(Attributes_Secondary_ChanceToApply_ChanceToPetrify, UPHAttributeSet::GetChanceToPetrifyAttribute());
		StatusEffectTagToAttributeMap.Add(Attributes_Secondary_ChanceToApply_ChanceToPurify, UPHAttributeSet::GetChanceToPurifyAttribute());
		StatusEffectTagToAttributeMap.Add(Attributes_Secondary_ChanceToApply_ChanceToCorrupt, UPHAttributeSet::GetChanceToCorruptAttribute());

		StatusEffectTagToAttributeMap.Add(Attributes_Secondary_Duration_BleedDuration, UPHAttributeSet::GetBleedDurationAttribute());
		StatusEffectTagToAttributeMap.Add(Attributes_Secondary_Duration_BurnDuration, UPHAttributeSet::GetBurnDurationAttribute());
		StatusEffectTagToAttributeMap.Add(Attributes_Secondary_Duration_FreezeDuration, UPHAttributeSet::GetFreezeDurationAttribute());
		StatusEffectTagToAttributeMap.Add(Attributes_Secondary_Duration_ShockDuration, UPHAttributeSet::GetShockDurationAttribute());
		StatusEffectTagToAttributeMap.Add(Attributes_Secondary_Duration_CorruptionDuration, UPHAttributeSet::GetCorruptionDurationAttribute());
		StatusEffectTagToAttributeMap.Add(Attributes_Secondary_Duration_PetrifyBuildUpDuration, UPHAttributeSet::GetPetrifyBuildUpDurationAttribute());
		StatusEffectTagToAttributeMap.Add(Attributes_Secondary_Duration_PurifyDuration, UPHAttributeSet::GetPurifyDurationAttribute());
}

void FPHGameplayTags::RegisterMinMaxTagMap()
{
	// Adding primary attribute tags
    TagsMinMax.Add(GameplayTags.Attributes_Primary_Strength, GameplayTags.Attributes_Primary_Strength);
    TagsMinMax.Add(GameplayTags.Attributes_Primary_Intelligence, GameplayTags.Attributes_Primary_Intelligence);
    TagsMinMax.Add(GameplayTags.Attributes_Primary_Endurance, GameplayTags.Attributes_Primary_Endurance);
    TagsMinMax.Add(GameplayTags.Attributes_Primary_Affliction, GameplayTags.Attributes_Primary_Affliction);
    TagsMinMax.Add(GameplayTags.Attributes_Primary_Dexterity, GameplayTags.Attributes_Primary_Dexterity);
    TagsMinMax.Add(GameplayTags.Attributes_Primary_Luck, GameplayTags.Attributes_Primary_Luck);
    TagsMinMax.Add(GameplayTags.Attributes_Primary_Covenant, GameplayTags.Attributes_Primary_Covenant);

    // Adding secondary attribute tags
    TagsMinMax.Add(GameplayTags.Attributes_Secondary_Vital_HealthReservedAmount, GameplayTags.Attributes_Secondary_Vital_MaxHealthReservedAmount);
    TagsMinMax.Add(GameplayTags.Attributes_Secondary_Vital_HealthRegenRate, GameplayTags.Attributes_Secondary_Vital_MaxHealthRegenRate);
    TagsMinMax.Add(GameplayTags.Attributes_Secondary_Vital_HealthRegenAmount, GameplayTags.Attributes_Secondary_Vital_MaxHealthRegenAmount);
    
    TagsMinMax.Add(GameplayTags.Attributes_Secondary_Vital_ManaRegenRate, GameplayTags.Attributes_Secondary_Vital_MaxManaRegenRate);
    TagsMinMax.Add(GameplayTags.Attributes_Secondary_Vital_ManaRegenAmount, GameplayTags.Attributes_Secondary_Vital_MaxManaRegenAmount);
    TagsMinMax.Add(GameplayTags.Attributes_Secondary_Vital_ManaReservedAmount, GameplayTags.Attributes_Secondary_Vital_MaxManaReservedAmount);
    
    TagsMinMax.Add(GameplayTags.Attributes_Secondary_Vital_StaminaRegenRate, GameplayTags.Attributes_Secondary_Vital_MaxStaminaRegenRate);
    TagsMinMax.Add(GameplayTags.Attributes_Secondary_Vital_StaminaRegenAmount, GameplayTags.Attributes_Secondary_Vital_MaxStaminaRegenAmount);
    TagsMinMax.Add(GameplayTags.Attributes_Secondary_Vital_StaminaReservedAmount, GameplayTags.Attributes_Secondary_Vital_MaxStaminaReservedAmount);
    
    TagsMinMax.Add(GameplayTags.Attributes_Secondary_Vital_ArcaneShieldRegenRate, GameplayTags.Attributes_Secondary_Vital_MaxArcaneShieldRegenRate);
    TagsMinMax.Add(GameplayTags.Attributes_Secondary_Vital_ArcaneShieldRegenAmount, GameplayTags.Attributes_Secondary_Vital_MaxArcaneShieldRegenAmount);
    TagsMinMax.Add(GameplayTags.Attributes_Secondary_Vital_ArcaneShieldReservedAmount, GameplayTags.Attributes_Secondary_Vital_MaxArcaneShieldReservedAmount);

}


void FPHGameplayTags::RegisterFlatDamageAttributes()
{
	FlatDamageToAttributesMap.Add("Physical", UPHAttributeSet::GetPhysicalFlatBonusAttribute());
	FlatDamageToAttributesMap.Add("Fire", UPHAttributeSet::GetFireFlatBonusAttribute());
	FlatDamageToAttributesMap.Add("Ice", UPHAttributeSet::GetIceFlatBonusAttribute());
	FlatDamageToAttributesMap.Add("Lightning", UPHAttributeSet::GetLightningFlatBonusAttribute());
	FlatDamageToAttributesMap.Add("Light", UPHAttributeSet::GetLightFlatBonusAttribute());
	FlatDamageToAttributesMap.Add("Corruption", UPHAttributeSet::GetCorruptionFlatBonusAttribute());
}


void FPHGameplayTags::RegisterPercentDamageAttributes()
{
	PercentDamageToAttributesMap.Add("Physical", UPHAttributeSet::GetPhysicalPercentBonusAttribute());
	PercentDamageToAttributesMap.Add("Fire", UPHAttributeSet::GetFirePercentBonusAttribute());
	PercentDamageToAttributesMap.Add("Ice", UPHAttributeSet::GetIcePercentBonusAttribute());
	PercentDamageToAttributesMap.Add("Lightning", UPHAttributeSet::GetLightningPercentBonusAttribute());
	PercentDamageToAttributesMap.Add("Light", UPHAttributeSet::GetLightPercentBonusAttribute());
	PercentDamageToAttributesMap.Add("Corruption", UPHAttributeSet::GetCorruptionPercentBonusAttribute());
}

void FPHGameplayTags::RegisterBaseDamageAttributes()
{
	BaseDamageToAttributesMap.Add("Min Physical", UPHAttributeSet::GetMinPhysicalDamageAttribute());
	BaseDamageToAttributesMap.Add("Min Fire", UPHAttributeSet::GetMinFireDamageAttribute());
	BaseDamageToAttributesMap.Add("Min Ice", UPHAttributeSet::GetMinIceDamageAttribute());
	BaseDamageToAttributesMap.Add("Min Lightning", UPHAttributeSet::GetMinLightningDamageAttribute());
	BaseDamageToAttributesMap.Add("Min Light", UPHAttributeSet::GetMinLightDamageAttribute());
	BaseDamageToAttributesMap.Add("Min Corruption", UPHAttributeSet::GetMinCorruptionDamageAttribute());

	BaseDamageToAttributesMap.Add("Max Physical", UPHAttributeSet::GetMaxPhysicalDamageAttribute());
	BaseDamageToAttributesMap.Add("Max Fire", UPHAttributeSet::GetMaxFireDamageAttribute());
	BaseDamageToAttributesMap.Add("Max Ice", UPHAttributeSet::GetMaxIceDamageAttribute());
	BaseDamageToAttributesMap.Add("Max Lightning", UPHAttributeSet::GetMaxLightningDamageAttribute());
	BaseDamageToAttributesMap.Add("Max Light", UPHAttributeSet::GetMaxLightDamageAttribute());
	BaseDamageToAttributesMap.Add("Max Corruption", UPHAttributeSet::GetMaxCorruptionDamageAttribute());
}

void FPHGameplayTags::RegisterAllAttributes()
{
	AllAttributesMap.Append(FlatDamageToAttributesMap);
	AllAttributesMap.Append(PercentDamageToAttributesMap);
	AllAttributesMap.Append(BaseDamageToAttributesMap);

	AllAttributesMap.Add("Strength", UPHAttributeSet::GetStrengthAttribute());
	AllAttributesMap.Add("Intelligence", UPHAttributeSet::GetIntelligenceAttribute());
	AllAttributesMap.Add("Dexterity", UPHAttributeSet::GetDexterityAttribute());
	AllAttributesMap.Add("Endurance", UPHAttributeSet::GetEnduranceAttribute());
	AllAttributesMap.Add("Affliction", UPHAttributeSet::GetAfflictionAttribute());
	AllAttributesMap.Add("Luck", UPHAttributeSet::GetLuckAttribute());
	AllAttributesMap.Add("Covenant", UPHAttributeSet::GetCovenantAttribute());

	// You can add other global or rare attributes here too
}


void FPHGameplayTags::RegisterAllAttribute()
{
	RegisterStatusEffectAttributes();
	RegisterMinMaxTagMap();
	RegisterFlatDamageAttributes();
	RegisterPercentDamageAttributes();
	RegisterBaseDamageAttributes();
	RegisterAllAttributes();
}


