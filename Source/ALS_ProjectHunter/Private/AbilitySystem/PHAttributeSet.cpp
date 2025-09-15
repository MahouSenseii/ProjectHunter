// Copyright@2024 Quentin Davis 


#include "AbilitySystem/PHAttributeSet.h"
#include "GameplayEffectTypes.h"
#include "GameplayEffectExtension.h"
#include "GameFramework/Character.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Library/PHCharacterEnumLibrary.h"
#include "Library/PHTagUtilityLibrary.h"
#include "Net/UnrealNetwork.h"

UPHAttributeSet::UPHAttributeSet()
{
	
}


void UPHAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	/* ============================= */
	/* === Indicators ============== */
	/* ============================= */
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, CombatAlignment, COND_None, REPNOTIFY_Always);

	/* ============================= */
	/* === Primary Attributes ====== */
	/* ============================= */
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, Strength,     COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, Intelligence, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, Dexterity,    COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, Endurance,    COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, Affliction,   COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, Luck,         COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, Covenant,     COND_OwnerOnly, REPNOTIFY_Always);

	/* ============================= */
	/* === Vital Max (raw/effective) */
	/* ============================= */
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxHealth,            COND_None,      REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxEffectiveHealth,   COND_None,      REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxStamina,           COND_None,      REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxEffectiveStamina,  COND_None,      REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxMana,              COND_None,      REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxEffectiveMana,     COND_None,      REPNOTIFY_Always);

	/* ============================= */
	/* === Health Regen / Reserve == */
	/* ============================= */
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, HealthRegenRate,          COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, HealthRegenAmount,        COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ReservedHealth,           COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxReservedHealth,        COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, FlatReservedHealth,       COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, PercentageReservedHealth, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxHealthRegenRate,   COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxHealthRegenAmount, COND_OwnerOnly, REPNOTIFY_Always);


	/* ============================= */
	/* === Mana Regen / Reserve ==== */
	/* ============================= */
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ManaRegenRate,            COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ManaRegenAmount,          COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ReservedMana,             COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxReservedMana,          COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, FlatReservedMana,         COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, PercentageReservedMana,   COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxManaRegenRate,     COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxManaRegenAmount,   COND_OwnerOnly, REPNOTIFY_Always);

	/* ============================= */
	/* === Stamina Regen / Reserve = */
	/* ============================= */
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, StaminaRegenRate,         COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, StaminaRegenAmount,       COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, StaminaDegenRate,         COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, StaminaDegenAmount,       COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ReservedStamina,          COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxReservedStamina,       COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, FlatReservedStamina,      COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, PercentageReservedStamina,COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxStaminaRegenRate,  COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxStaminaRegenAmount,COND_OwnerOnly, REPNOTIFY_Always);

	/* ============================= */
	/* === Arcane Shield  ===== */
	/* ============================= */
	// Core pools (missing before)
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ArcaneShield,              COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxArcaneShield,           COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxEffectiveArcaneShield,  COND_None, REPNOTIFY_Always);
	
	// Regen / reserve (you already had most of these)
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ArcaneShieldRegenRate,        COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ArcaneShieldRegenAmount,      COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ReservedArcaneShield,         COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxReservedArcaneShield,      COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, FlatReservedArcaneShield,     COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, PercentageReservedArcaneShield,COND_OwnerOnly, REPNOTIFY_Always);

	/* ============================= */
	/* === Damage Ranges & Bonuses = */
	/* ============================= */
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, GlobalDamages,        COND_OwnerOnly, REPNOTIFY_Always);

	// Min
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MinPhysicalDamage,     COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MinFireDamage,         COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MinLightDamage,        COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MinLightningDamage,    COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MinCorruptionDamage,   COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MinIceDamage,          COND_OwnerOnly, REPNOTIFY_Always);

	// Max
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxPhysicalDamage,     COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxFireDamage,         COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxLightDamage,        COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxLightningDamage,    COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxCorruptionDamage,   COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxIceDamage,          COND_OwnerOnly, REPNOTIFY_Always);

	// Flat bonuses
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, PhysicalFlatBonus,     COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, FireFlatBonus,         COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, LightFlatBonus,        COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, LightningFlatBonus,    COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, CorruptionFlatBonus,   COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, IceFlatBonus,          COND_OwnerOnly, REPNOTIFY_Always);

	// Percent bonuses
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, PhysicalPercentBonus,  COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, FirePercentBonus,      COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, LightPercentBonus,     COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, LightningPercentBonus, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, CorruptionPercentBonus,COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, IcePercentBonus,       COND_OwnerOnly, REPNOTIFY_Always);

	// Situational
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, DamageBonusWhileAtFullHP, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, DamageBonusWhileAtLowHP,  COND_OwnerOnly, REPNOTIFY_Always);

	/* ============================= */
	/* === Other Offensive Stats === */
	/* ============================= */
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, AreaDamage,         COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, AreaOfEffect,       COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, AttackRange,        COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, AttackSpeed,        COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, CastSpeed,          COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, CritChance,         COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, CritMultiplier,     COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, DamageOverTime,     COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ElementalDamage,    COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, SpellsCritChance,   COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, SpellsCritMultiplier,COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MeleeDamage,        COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, SpellDamage,        COND_OwnerOnly, REPNOTIFY_Always); 
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ProjectileCount,    COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ProjectileSpeed,    COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, RangedDamage,       COND_OwnerOnly, REPNOTIFY_Always);

	/* ============================= */
	/* === Durations =============== */
	/* ============================= */
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, BurnDuration,           COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, BleedDuration,          COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, FreezeDuration,         COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, CorruptionDuration,     COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ShockDuration,          COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, PetrifyBuildUpDuration, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, PurifyDuration,         COND_OwnerOnly, REPNOTIFY_Always);

	/* ============================= */
	/* === Resistances ============= */
	/* ============================= */
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, GlobalDefenses,                COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, BlockStrength,                 COND_None,      REPNOTIFY_Always);

	// Armour
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, Armour,                        COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ArmourFlatBonus,               COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ArmourPercentBonus,            COND_OwnerOnly, REPNOTIFY_Always);

	// Fire
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, FireResistanceFlatBonus,       COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, FireResistancePercentBonus,    COND_OwnerOnly, REPNOTIFY_Always); 
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxFireResistance,             COND_OwnerOnly, REPNOTIFY_Always);

	// Light
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, LightResistanceFlatBonus,      COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, LightResistancePercentBonus,   COND_OwnerOnly, REPNOTIFY_Always); 
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxLightResistance,            COND_OwnerOnly, REPNOTIFY_Always);

	// Lightning
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, LightningResistanceFlatBonus,  COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, LightningResistancePercentBonus,COND_OwnerOnly, REPNOTIFY_Always); 
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxLightningResistance,        COND_OwnerOnly, REPNOTIFY_Always);

	// Corruption
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, CorruptionResistanceFlatBonus, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, CorruptionResistancePercentBonus, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxCorruptionResistance,       COND_OwnerOnly, REPNOTIFY_Always);

	// Ice
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, IceResistanceFlatBonus,        COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, IceResistancePercentBonus,     COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MaxIceResistance,              COND_OwnerOnly, REPNOTIFY_Always);


	// Physical Damage Conversions\
	// DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, PhysicalToFire, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, PhysicalToIce, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, PhysicalToLightning, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, PhysicalToLight, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, PhysicalToCorruption, COND_None, REPNOTIFY_Always);

	// Fire Damage Conversions
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, FireToPhysical, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, FireToIce, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, FireToLightning, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, FireToLight, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, FireToCorruption, COND_None, REPNOTIFY_Always);

	// Ice Damage Conversions
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, IceToPhysical, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, IceToFire, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, IceToLightning, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, IceToLight, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, IceToCorruption, COND_None, REPNOTIFY_Always);

	// Lightning Damage Conversions
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, LightningToPhysical, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, LightningToFire, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, LightningToIce, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, LightningToLight, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, LightningToCorruption, COND_None, REPNOTIFY_Always);

	// Light Damage Conversions
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, LightToPhysical, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, LightToFire, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, LightToIce, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, LightToLightning, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, LightToCorruption, COND_None, REPNOTIFY_Always);

	// Corruption Damage Conversions
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, CorruptionToPhysical, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, CorruptionToFire, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, CorruptionToIce, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, CorruptionToLightning, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, CorruptionToLight, COND_None, REPNOTIFY_Always);

	/* ============================= */
	/* === Piercing ================ */
	/* ============================= */
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ArmourPiercing,        COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, FirePiercing,          COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, LightPiercing,         COND_OwnerOnly, REPNOTIFY_Always); 
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, LightningPiercing,     COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, CorruptionPiercing,    COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, IcePiercing,           COND_OwnerOnly, REPNOTIFY_Always);

	/* ============================= */
	/* === Ailment Chances ========= */
	/* ============================= */
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ChanceToBleed,        COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ChanceToCorrupt,      COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ChanceToFreeze,       COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ChanceToIgnite,       COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ChanceToPetrify,      COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ChanceToPurify,       COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ChanceToShock,        COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ChanceToStun,         COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ChanceToKnockBack,    COND_OwnerOnly, REPNOTIFY_Always);

	/* ============================= */
	/* === Misc ==================== */
	/* ============================= */
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ComboCounter,     COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, CoolDown,         COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, LifeLeech,        COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ManaLeech,        COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, MovementSpeed,    COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, Poise,            COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, Weight,           COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, PoiseResistance,  COND_OwnerOnly, REPNOTIFY_Always); 
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, StunRecovery,     COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ManaCostChanges,  COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, LifeOnHit,        COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, ManaOnHit,        COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, StaminaOnHit,     COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, StaminaCostChanges,COND_OwnerOnly, REPNOTIFY_Always);

	/* ============================= */
	/* === Current Vitals ========= */
	/* ============================= */
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, Health,   COND_None,      REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, Mana,     COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, Stamina,  COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPHAttributeSet, Gems,     COND_OwnerOnly, REPNOTIFY_Always);
}


float UPHAttributeSet::GetAttributeValue(const FGameplayAttribute& Attribute) const 
{
	if (const UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent())
	{
		return ASC->GetNumericAttribute(Attribute);
	}
	return 0.0f;
}

// for use in BP will show enum not float values 
ECombatAlignment UPHAttributeSet::GetCombatAlignmentBP() const
{
	// Convert the float value stored in CombatAlignment to the ECombatAlignment enum
	return static_cast<ECombatAlignment>(CombatAlignment.GetCurrentValue());
}

// for use in BP will show enum not float values 
void UPHAttributeSet::SetCombatAlignmentBP(ECombatAlignment NewAlignment)
{
	// Set the CombatAlignment value using the float representation of the enum
	SetCombatAlignment(static_cast<float>(NewAlignment));
}

void UPHAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
    Super::PreAttributeChange(Attribute, NewValue);

    // ============================================================================
    // VITAL ATTRIBUTES - Clamp to their effective maximums
    // ============================================================================
    if (Attribute == GetHealthAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxEffectiveHealth());
    }
    else if (Attribute == GetManaAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxEffectiveMana());
    }
    else if (Attribute == GetStaminaAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxEffectiveStamina());
    }
    else if (Attribute == GetArcaneShieldAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxEffectiveArcaneShield());
    }

    // ============================================================================
    // PRIMARY ATTRIBUTES - Usually non-negative, reasonable upper bounds
    // ============================================================================
    else if (Attribute == GetStrengthAttribute() || Attribute == GetIntelligenceAttribute() || 
             Attribute == GetDexterityAttribute() || Attribute == GetEnduranceAttribute() ||
             Attribute == GetAfflictionAttribute() || Attribute == GetLuckAttribute() || 
             Attribute == GetCovenantAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.0f, 9999.0f);
    }

    // ============================================================================
    // PERCENTAGE-BASED ATTRIBUTES - Clamp to 0-100% (assuming 0-100 scale)
    // ============================================================================
    
    // Critical Chance and Multipliers
    else if (Attribute == GetCritChanceAttribute() || Attribute == GetSpellsCritChanceAttribute())
    {
    	// 0-100%
        NewValue = FMath::Clamp(NewValue, 0.0f, 100.0f);
    }
    else if (Attribute == GetCritMultiplierAttribute() || Attribute == GetSpellsCritMultiplierAttribute())
    {
    	// At least 1x, max 10x
        NewValue = FMath::Clamp(NewValue, 1.0f, 10.0f); 
    }

    // Resistance Percentages (usually 0-90% max to prevent immunity)
    else if (Attribute == GetFireResistancePercentBonusAttribute() || 
             Attribute == GetIceResistancePercentBonusAttribute() ||
             Attribute == GetLightResistancePercentBonusAttribute() ||
             Attribute == GetLightningResistancePercentBonusAttribute() ||
             Attribute == GetCorruptionResistancePercentBonusAttribute() ||
             Attribute == GetArmourPercentBonusAttribute())
    {
    	// Max 90% resistance
        NewValue = FMath::Clamp(NewValue, 0.0f, 90.0f); 
    }

    // Damage Percent Bonuses
    else if (Attribute == GetPhysicalPercentBonusAttribute() || 
             Attribute == GetFirePercentBonusAttribute() ||
             Attribute == GetIcePercentBonusAttribute() ||
             Attribute == GetLightPercentBonusAttribute() ||
             Attribute == GetLightningPercentBonusAttribute() ||
             Attribute == GetCorruptionPercentBonusAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.0f, 999.0f); 
    }

    // Ailment Chances
    else if (Attribute == GetChanceToBleedAttribute() || Attribute == GetChanceToIgniteAttribute() ||
             Attribute == GetChanceToFreezeAttribute() || Attribute == GetChanceToShockAttribute() ||
             Attribute == GetChanceToCorruptAttribute() || Attribute == GetChanceToPetrifyAttribute() ||
             Attribute == GetChanceToStunAttribute() || Attribute == GetChanceToKnockBackAttribute() ||
             Attribute == GetChanceToPurifyAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.0f, 100.0f); // 0-100%
    }

    // Conversion Percentages (total conversions shouldn't exceed 100%)
    else if (Attribute == GetPhysicalToFireAttribute() || Attribute == GetPhysicalToIceAttribute() ||
             Attribute == GetPhysicalToLightningAttribute() || Attribute == GetPhysicalToLightAttribute() ||
             Attribute == GetPhysicalToCorruptionAttribute() ||
             Attribute == GetFireToPhysicalAttribute() || Attribute == GetFireToIceAttribute() ||
             Attribute == GetFireToLightningAttribute() || Attribute == GetFireToLightAttribute() ||
             Attribute == GetFireToCorruptionAttribute() ||
             Attribute == GetIceToPhysicalAttribute() || Attribute == GetIceToFireAttribute() ||
             Attribute == GetIceToLightningAttribute() || Attribute == GetIceToLightAttribute() ||
             Attribute == GetIceToCorruptionAttribute() ||
             Attribute == GetLightningToPhysicalAttribute() || Attribute == GetLightningToFireAttribute() ||
             Attribute == GetLightningToIceAttribute() || Attribute == GetLightningToLightAttribute() ||
             Attribute == GetLightningToCorruptionAttribute() ||
             Attribute == GetLightToPhysicalAttribute() || Attribute == GetLightToFireAttribute() ||
             Attribute == GetLightToIceAttribute() || Attribute == GetLightToLightningAttribute() ||
             Attribute == GetLightToCorruptionAttribute() ||
             Attribute == GetCorruptionToPhysicalAttribute() || Attribute == GetCorruptionToFireAttribute() ||
             Attribute == GetCorruptionToIceAttribute() || Attribute == GetCorruptionToLightningAttribute() ||
             Attribute == GetCorruptionToLightAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.0f, 100.0f); // 0-100% conversion
    }

    // Reserved Percentages
    else if (Attribute == GetPercentageReservedHealthAttribute() || 
             Attribute == GetPercentageReservedManaAttribute() ||
             Attribute == GetPercentageReservedStaminaAttribute() ||
             Attribute == GetPercentageReservedArcaneShieldAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.0f, 95.0f); 
    }

    // Piercing Percentages
    else if (Attribute == GetArmourPiercingAttribute() || Attribute == GetFirePiercingAttribute() ||
             Attribute == GetIcePiercingAttribute() || Attribute == GetLightPiercingAttribute() ||
             Attribute == GetLightningPiercingAttribute() || Attribute == GetCorruptionPiercingAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.0f, 100.0f); // 0-100% piercing
    }

    // ============================================================================
    // DAMAGE ATTRIBUTES - Non-negative values
    // ============================================================================
    else if (Attribute == GetMinPhysicalDamageAttribute() || Attribute == GetMaxPhysicalDamageAttribute() ||
             Attribute == GetMinFireDamageAttribute() || Attribute == GetMaxFireDamageAttribute() ||
             Attribute == GetMinIceDamageAttribute() || Attribute == GetMaxIceDamageAttribute() ||
             Attribute == GetMinLightDamageAttribute() || Attribute == GetMaxLightDamageAttribute() ||
             Attribute == GetMinLightningDamageAttribute() || Attribute == GetMaxLightningDamageAttribute() ||
             Attribute == GetMinCorruptionDamageAttribute() || Attribute == GetMaxCorruptionDamageAttribute())
    {
        NewValue = FMath::Max(NewValue, 0.0f); // Damage can't be negative
        
       // Physical Damage Min/Max Validation
        if (Attribute == GetMinPhysicalDamageAttribute())
        {
            NewValue = FMath::Min(NewValue, GetMaxPhysicalDamage());
        }
        else if (Attribute == GetMaxPhysicalDamageAttribute())
        {
            NewValue = FMath::Max(NewValue, GetMinPhysicalDamage());
        }
        
        // Fire Damage Min/Max Validation
        else if (Attribute == GetMinFireDamageAttribute())
        {
            NewValue = FMath::Min(NewValue, GetMaxFireDamage());
        }
        else if (Attribute == GetMaxFireDamageAttribute())
        {
            NewValue = FMath::Max(NewValue, GetMinFireDamage());
        }
        
        // Ice Damage Min/Max Validation
        else if (Attribute == GetMinIceDamageAttribute())
        {
            NewValue = FMath::Min(NewValue, GetMaxIceDamage());
        }
        else if (Attribute == GetMaxIceDamageAttribute())
        {
            NewValue = FMath::Max(NewValue, GetMinIceDamage());
        }
        
        // Light Damage Min/Max Validation
        else if (Attribute == GetMinLightDamageAttribute())
        {
            NewValue = FMath::Min(NewValue, GetMaxLightDamage());
        }
        else if (Attribute == GetMaxLightDamageAttribute())
        {
            NewValue = FMath::Max(NewValue, GetMinLightDamage());
        }
        
        // Lightning Damage Min/Max Validation
        else if (Attribute == GetMinLightningDamageAttribute())
        {
            NewValue = FMath::Min(NewValue, GetMaxLightningDamage());
        }
        else if (Attribute == GetMaxLightningDamageAttribute())
        {
            NewValue = FMath::Max(NewValue, GetMinLightningDamage());
        }
        
        // Corruption Damage Min/Max Validation
        else if (Attribute == GetMinCorruptionDamageAttribute())
        {
            NewValue = FMath::Min(NewValue, GetMaxCorruptionDamage());
        }
        else if (Attribute == GetMaxCorruptionDamageAttribute())
        {
            NewValue = FMath::Max(NewValue, GetMinCorruptionDamage());
        }
    }

    // Flat Damage Bonuses
    else if (Attribute == GetPhysicalFlatBonusAttribute() || Attribute == GetFireFlatBonusAttribute() ||
             Attribute == GetIceFlatBonusAttribute() || Attribute == GetLightFlatBonusAttribute() ||
             Attribute == GetLightningFlatBonusAttribute() || Attribute == GetCorruptionFlatBonusAttribute())
    {
        NewValue = FMath::Max(NewValue, 0.0f); // Flat bonuses non-negative
    }

    // Global Damage Multipliers
    else if (Attribute == GetGlobalDamagesAttribute() || Attribute == GetElementalDamageAttribute() ||
             Attribute == GetMeleeDamageAttribute() || Attribute == GetSpellDamageAttribute() ||
             Attribute == GetRangedDamageAttribute() || Attribute == GetAreaDamageAttribute() ||
             Attribute == GetDamageOverTimeAttribute())
    {
        NewValue = FMath::Max(NewValue, 0.0f); // Damage multipliers non-negative
    }

    // ============================================================================
    // RESISTANCE ATTRIBUTES - Non-negative flat resistances
    // ============================================================================
    else if (Attribute == GetArmourAttribute() || Attribute == GetArmourFlatBonusAttribute() ||
             Attribute == GetFireResistanceFlatBonusAttribute() || Attribute == GetIceResistanceFlatBonusAttribute() ||
             Attribute == GetLightResistanceFlatBonusAttribute() || Attribute == GetLightningResistanceFlatBonusAttribute() ||
             Attribute == GetCorruptionResistanceFlatBonusAttribute())
    {
        NewValue = FMath::Max(NewValue, 0.0f); // Flat resistances non-negative
    }

    // Max Resistance Caps
    else if (Attribute == GetMaxFireResistanceAttribute() || Attribute == GetMaxIceResistanceAttribute() ||
             Attribute == GetMaxLightResistanceAttribute() || Attribute == GetMaxLightningResistanceAttribute() ||
             Attribute == GetMaxCorruptionResistanceAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.0f, 90.0f); // Max resistance caps
    }

    // ============================================================================
    // RATE ATTRIBUTES - Generally positive values (time-based)
    // ============================================================================
    else if (Attribute == GetHealthRegenRateAttribute() || Attribute == GetManaRegenRateAttribute() ||
             Attribute == GetStaminaRegenRateAttribute() || Attribute == GetArcaneShieldRegenRateAttribute() ||
             Attribute == GetStaminaDegenRateAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.1f, 60.0f); // Min 0.1s, max 60s between ticks
    }

    // Max Rate Caps
    else if (Attribute == GetMaxHealthRegenRateAttribute() || Attribute == GetMaxManaRegenRateAttribute() ||
             Attribute == GetMaxStaminaRegenRateAttribute() || Attribute == GetMaxArcaneShieldRegenRateAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.1f, 60.0f); // Same as regular rates
    }

    // ============================================================================
    // AMOUNT ATTRIBUTES - Non-negative values
    // ============================================================================
    else if (Attribute == GetHealthRegenAmountAttribute() || Attribute == GetManaRegenAmountAttribute() ||
             Attribute == GetStaminaRegenAmountAttribute() || Attribute == GetArcaneShieldRegenAmountAttribute() ||
             Attribute == GetStaminaDegenAmountAttribute() ||
             Attribute == GetMaxHealthRegenAmountAttribute() || Attribute == GetMaxManaRegenAmountAttribute() ||
             Attribute == GetMaxStaminaRegenAmountAttribute() || Attribute == GetMaxArcaneShieldRegenAmountAttribute())
    {
        NewValue = FMath::Max(NewValue, 0.0f); // Amounts non-negative
    }

    // ============================================================================
    // RESERVED AMOUNTS - Non-negative, reasonable caps
    // ============================================================================
    else if (Attribute == GetFlatReservedHealthAttribute() || Attribute == GetFlatReservedManaAttribute() ||
             Attribute == GetFlatReservedStaminaAttribute() || Attribute == GetFlatReservedArcaneShieldAttribute())
    {
        NewValue = FMath::Max(NewValue, 0.0f); // Reserved amounts non-negative
    }

    // ============================================================================
    // DURATION ATTRIBUTES - Non-negative time values
    // ============================================================================
    else if (Attribute == GetBurnDurationAttribute() || Attribute == GetBleedDurationAttribute() ||
             Attribute == GetFreezeDurationAttribute() || Attribute == GetShockDurationAttribute() ||
             Attribute == GetCorruptionDurationAttribute() || Attribute == GetPetrifyBuildUpDurationAttribute() ||
             Attribute == GetPurifyDurationAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.0f, 300.0f); // 0-300 seconds max duration
    }

    // ============================================================================
    // SPEED ATTRIBUTES - Non-negative values
    // ============================================================================
    else if (Attribute == GetMovementSpeedAttribute() || Attribute == GetAttackSpeedAttribute() ||
             Attribute == GetCastSpeedAttribute() || Attribute == GetProjectileSpeedAttribute())
    {
        NewValue = FMath::Max(NewValue, 0.0f); // Speeds non-negative
    }

    // ============================================================================
    // COUNT ATTRIBUTES - Non-negative integer-like values
    // ============================================================================
    else if (Attribute == GetProjectileCountAttribute() || Attribute == GetChainCountAttribute() ||
             Attribute == GetForkCountAttribute() || Attribute == GetComboCounterAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.0f, 99.0f); // 0-99 count limit
    }

    // ============================================================================
    // LEECH ATTRIBUTES - 0-100% typically
    // ============================================================================
    else if (Attribute == GetLifeLeechAttribute() || Attribute == GetManaLeechAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.0f, 100.0f); // 0-100% leech
    }

    // ============================================================================
    // ON-HIT ATTRIBUTES - Non-negative values
    // ============================================================================
    else if (Attribute == GetLifeOnHitAttribute() || Attribute == GetManaOnHitAttribute() ||
             Attribute == GetStaminaOnHitAttribute())
    {
        NewValue = FMath::Max(NewValue, 0.0f); // On-hit amounts non-negative
    }

    // ============================================================================
    // COST CHANGE ATTRIBUTES - Can be negative (cost reduction) or positive
    // ============================================================================
    else if (Attribute == GetManaCostChangesAttribute() || Attribute == GetStaminaCostChangesAttribute())
    {
        NewValue = FMath::Clamp(NewValue, -90.0f, 500.0f); // -90% to +500% cost changes
    }

    // ============================================================================
    // RANGE AND AREA ATTRIBUTES - Non-negative
    // ============================================================================
    else if (Attribute == GetAttackRangeAttribute() || Attribute == GetAreaOfEffectAttribute() ||
             Attribute == GetAuraRadiusAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.0f, 2000.0f); // Reasonable range limits
    }

    // ============================================================================
    // REFLECTION ATTRIBUTES - 0-100% typically
    // ============================================================================
    else if (Attribute == GetReflectChancePhysicalAttribute() || Attribute == GetReflectChanceElementalAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.0f, 100.0f); // 0-100% reflect chance
    }
    else if (Attribute == GetReflectPhysicalAttribute() || Attribute == GetReflectElementalAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.0f, 300.0f); // 0-300% reflect damage
    }

    // ============================================================================
    // SPECIAL ATTRIBUTES - Various constraints
    // ============================================================================
    else if (Attribute == GetPoiseAttribute() || Attribute == GetPoiseResistanceAttribute() ||
             Attribute == GetStunRecoveryAttribute())
    {
        NewValue = FMath::Max(NewValue, 0.0f); // Non-negative
    }
    else if (Attribute == GetWeightAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.0f, 999.0f); // 0-999 weight units
    }
    else if (Attribute == GetBlockStrengthAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.0f, 100.0f); // 0-100% block strength
    }
    else if (Attribute == GetCoolDownAttribute())
    {
        NewValue = FMath::Max(NewValue, 0.0f); // Cooldown can't be negative
    }
    else if (Attribute == GetGemsAttribute())
    {
        NewValue = FMath::Max(NewValue, 0.0f); // Currency non-negative
    }

    // ============================================================================
    // MAX VITAL ATTRIBUTES - Reasonable bounds
    // ============================================================================
    else if (Attribute == GetMaxHealthAttribute() || Attribute == GetMaxManaAttribute() ||
             Attribute == GetMaxStaminaAttribute() || Attribute == GetMaxArcaneShieldAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 1.0f, 99999.0f); // At least 1, reasonable max
    }

    // Effective Max Vitals (calculated values, may not need validation)
    else if (Attribute == GetMaxEffectiveHealthAttribute() || Attribute == GetMaxEffectiveManaAttribute() ||
             Attribute == GetMaxEffectiveStaminaAttribute() || Attribute == GetMaxEffectiveArcaneShieldAttribute())
    {
        NewValue = FMath::Max(NewValue, 1.0f); // At least 1 effective point
    }

    // ============================================================================
    // GLOBAL MODIFIERS - Usually positive multipliers
    // ============================================================================
    else if (Attribute == GetGlobalDefensesAttribute())
    {
        NewValue = FMath::Max(NewValue, 0.0f); // Global defense non-negative
    }

    // ============================================================================
    // SITUATIONAL DAMAGE BONUSES
    // ============================================================================
    else if (Attribute == GetDamageBonusWhileAtFullHPAttribute() || 
             Attribute == GetDamageBonusWhileAtLowHPAttribute() ||
             Attribute == GetChainDamageAttribute())
    {
        NewValue = FMath::Max(NewValue, 0.0f); // Damage bonuses non-negative
    }

    // ============================================================================
    // AURA AND EFFECT ATTRIBUTES
    // ============================================================================
    else if (Attribute == GetAuraEffectAttribute())
    {
        NewValue = FMath::Max(NewValue, 0.0f); // Aura effects non-negative
    }

    // ============================================================================
    // COMBAT ALIGNMENT - Special enum value
    // ============================================================================
    else if (Attribute == GetCombatAlignmentAttribute())
    {
        // Assuming ECombatAlignment enum values are 0, 1, 2, etc.
        NewValue = FMath::Clamp(NewValue, 0.0f, 10.0f); // Adjust based on your enum size
    }
}


void UPHAttributeSet::ClampResource(const TFunctionRef<float()>& Getter, const TFunctionRef<float()>& MaxGetter, const TFunctionRef<void(float)>& Setter)
{
	Setter(FMath::Clamp(Getter(), 0.0f, MaxGetter()));
}




void UPHAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
    Super::PostGameplayEffectExecute(Data);

    FEffectProperties Props;
    SetEffectProperties(Data, Props);

    const FGameplayAttribute Attribute = Data.EvaluatedData.Attribute;

    // ============================================================================
    // RESERVED ATTRIBUTE CALCULATIONS - Recalculate when reserve sources change
    // ============================================================================
    
    // Health Reserve Calculation
    if (Attribute == GetFlatReservedHealthAttribute() || 
        Attribute == GetPercentageReservedHealthAttribute() ||
        Attribute == GetMaxHealthAttribute())
    {
        RecalculateHealthReserves();
    }
    
    // Mana Reserve Calculation  
    else if (Attribute == GetFlatReservedManaAttribute() || 
             Attribute == GetPercentageReservedManaAttribute() ||
             Attribute == GetMaxManaAttribute())
    {
        RecalculateManaReserves();
    }
    
    // Stamina Reserve Calculation
    else if (Attribute == GetFlatReservedStaminaAttribute() || 
             Attribute == GetPercentageReservedStaminaAttribute() ||
             Attribute == GetMaxStaminaAttribute())
    {
        RecalculateStaminaReserves();
    }
    
    // Arcane Shield Reserve Calculation
    else if (Attribute == GetFlatReservedArcaneShieldAttribute() || 
             Attribute == GetPercentageReservedArcaneShieldAttribute() ||
             Attribute == GetMaxArcaneShieldAttribute())
    {
        RecalculateArcaneShieldReserves();
    }

    // ============================================================================
    // DIRECT VITAL ATTRIBUTE CHANGES - Clamp to current effective maximums
    // ============================================================================
    
    // Only clamp if the attribute change was direct (not from reserve recalculation)
    else if (Attribute == GetHealthAttribute())
    {
        ClampResource(
            [this]() { return GetHealth(); },
            [this]() { return GetMaxEffectiveHealth(); },
            [this](const float V) { SetHealth(V); }
        );
        
        if (GetHealth() <= 0.0f)
        {
            // TODO: death handling
        }
    }
    else if (Attribute == GetManaAttribute())
    {
        ClampResource(
            [this]() { return GetMana(); },
            [this]() { return GetMaxEffectiveMana(); },
            [this](const float V) { SetMana(V); }
        );
    }
    else if (Attribute == GetStaminaAttribute())
    {
        ClampResource(
            [this]() { return GetStamina(); },
            [this]() { return GetMaxEffectiveStamina(); },
            [this](const float V) { SetStamina(V); }
        );
    }
    else if (Attribute == GetArcaneShieldAttribute())
    {
        ClampResource(
            [this]() { return GetArcaneShield(); },
            [this]() { return GetMaxEffectiveArcaneShield(); },
            [this](const float V) { SetArcaneShield(V); }
        );
    }

    // ============================================================================
    // THRESHOLD TAG UPDATES - Only once at the end
    // ============================================================================
    if (Props.TargetASC && ShouldUpdateThresholdTags(Attribute))
    {
        UPHTagUtilityLibrary::UpdateAttributeThresholdTags(Props.TargetASC, this);
    }
}



bool UPHAttributeSet::ShouldUpdateThresholdTags(const FGameplayAttribute& Attribute)
{
	static const TSet<FGameplayAttribute> TrackedThresholdAttributes = {
		GetHealthAttribute(),
		GetManaAttribute(),
		GetStaminaAttribute(),
		GetArcaneShieldAttribute()
	};

	return TrackedThresholdAttributes.Contains(Attribute);
}

void UPHAttributeSet::SetEffectProperties(const FGameplayEffectModCallbackData& Data, FEffectProperties& Props)
{
	Props.EffectContextHandle = Data.EffectSpec.GetContext();
	Props.SourceAsc = Props.EffectContextHandle.GetOriginalInstigatorAbilitySystemComponent();

	if(IsValid(Props.SourceAsc) && Props.SourceAsc->AbilityActorInfo.IsValid() && Props.SourceAsc->AbilityActorInfo->AvatarActor.IsValid())
	{
		Props.SourceAvatarActor = Props.SourceAsc->AbilityActorInfo->AvatarActor.Get();
		Props.SourceController = Props.SourceAsc->AbilityActorInfo->PlayerController.Get();
		if (Props.SourceController == nullptr && Props.SourceAvatarActor != nullptr)
		{
			if (const APawn* Pawn = Cast<APawn>(Props.SourceAvatarActor))
			{
				Props.SourceController = Pawn->GetController();
			}
		}
		if(Props.SourceController)
		{
			Props.SourceCharacter = Cast<ACharacter>(Props.SourceController->GetPawn());
		}
	}

	if(Data.Target.AbilityActorInfo.IsValid() && Data.Target.AbilityActorInfo->AvatarActor.IsValid())
	{
		Props.TargetAvatarActor = Data.Target.AbilityActorInfo->AvatarActor.Get();
		Props.TargetController = Data.Target.AbilityActorInfo->PlayerController.Get();
		Props.TargetCharacter = Cast<ACharacter>(Props.TargetAvatarActor);
		Props.TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Props.TargetAvatarActor);
	}
}

//
void UPHAttributeSet::OnRep_CombatAlignment(const FGameplayAttributeData& OldCombatAlignment) const
{
	// Notify listeners of the attribute change
	OnCombatAlignmentChange.Broadcast(GetCombatAlignment());
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet, CombatAlignment, OldCombatAlignment);
}

//Health
void UPHAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,Health, OldHealth)
}

void UPHAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxHealth, OldMaxHealth)
}

void UPHAttributeSet::OnRep_MaxEffectiveHealth(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxEffectiveHealth, OldAmount);
}

void UPHAttributeSet::OnRep_HealthRegenRate(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,HealthRegenRate, OldAmount);
}

void UPHAttributeSet::OnRep_MaxHealthRegenRate(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxHealthRegenRate, OldAmount);
}

void UPHAttributeSet::OnRep_HealthRegenAmount(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,HealthRegenAmount, OldAmount);
}


void UPHAttributeSet::OnRep_MaxHealthRegenAmount(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxHealthRegenAmount, OldAmount);
}

void UPHAttributeSet::OnRep_ReservedHealth(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ReservedHealth, OldAmount);
}

void UPHAttributeSet::OnRep_MaxReservedHealth(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxReservedHealth, OldAmount);
}

void UPHAttributeSet::OnRep_FlatReservedHealth(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet , FlatReservedHealth, OldAmount);
}

void UPHAttributeSet::OnRep_PercentageReservedHealth(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet , PercentageReservedHealth, OldAmount);
}

//Health End

//Stamina 
void UPHAttributeSet::OnRep_Stamina(const FGameplayAttributeData& OldStamina) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet , Stamina,OldStamina)
}

void UPHAttributeSet::OnRep_MaxStamina(const FGameplayAttributeData& OldMaxStamina) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxStamina, OldMaxStamina)
}

void UPHAttributeSet::OnRep_MaxEffectiveStamina(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxEffectiveStamina, OldAmount)

}

void UPHAttributeSet::OnRep_StaminaRegenRate(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,StaminaRegenRate, OldAmount)
}

void UPHAttributeSet::OnRep_StaminaDegenRate(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,StaminaDegenRate, OldAmount)
}

void UPHAttributeSet::OnRep_MaxStaminaRegenRate(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxStaminaRegenRate, OldAmount)
}

void UPHAttributeSet::OnRep_StaminaRegenAmount(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,StaminaRegenAmount, OldAmount)
}

void UPHAttributeSet::OnRep_StaminaDegenAmount(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,StaminaDegenAmount, OldAmount)
}

void UPHAttributeSet::OnRep_MaxStaminaRegenAmount(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxStaminaRegenAmount,OldAmount)
}

void UPHAttributeSet::OnRep_ReservedStamina(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ReservedStamina, OldAmount)
}

void UPHAttributeSet::OnRep_MaxReservedStamina(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxReservedStamina, OldAmount)
}


void UPHAttributeSet::OnRep_FlatReservedStamina(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,FlatReservedStamina, OldAmount)
}

void UPHAttributeSet::OnRep_PercentageReservedStamina(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,PercentageReservedStamina, OldAmount)
}

//Stamina End

//Mana
void UPHAttributeSet::OnRep_Mana(const FGameplayAttributeData& OldMana) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,Mana, OldMana)
}

void UPHAttributeSet::OnRep_MaxMana(const FGameplayAttributeData& OldMaxMana) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxMana, OldMaxMana)

}

void UPHAttributeSet::OnRep_MaxEffectiveMana(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxEffectiveMana, OldAmount)
}

void UPHAttributeSet::OnRep_ManaRegenRate(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ManaRegenRate,OldAmount)
}

void UPHAttributeSet::OnRep_MaxManaRegenRate(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxManaRegenRate,OldAmount)
}

void UPHAttributeSet::OnRep_ManaRegenAmount(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ManaRegenAmount,OldAmount)
}

void UPHAttributeSet::OnRep_MaxManaRegenAmount(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxManaRegenAmount,OldAmount)
}

void UPHAttributeSet::OnRep_ReservedMana(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ReservedMana,OldAmount)
}

void UPHAttributeSet::OnRep_MaxReservedMana(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxReservedMana,OldAmount)
}

void UPHAttributeSet::OnRep_FlatReservedMana(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet , FlatReservedMana,OldAmount)
}

void UPHAttributeSet::OnRep_PercentageReservedMana(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet , PercentageReservedMana,OldAmount)
}

void UPHAttributeSet::OnRep_ArcaneShield(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet , ArcaneShield ,OldAmount)
}

void UPHAttributeSet::OnRep_MaxArcaneShield(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet , MaxArcaneShield ,OldAmount)
}

void UPHAttributeSet::OnRep_MaxEffectiveArcaneShield(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet , MaxEffectiveArcaneShield ,OldAmount)
}

void UPHAttributeSet::OnRep_ArcaneShieldRegenRate(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet , ArcaneShieldRegenRate ,OldAmount)
}

void UPHAttributeSet::OnRep_MaxArcaneShieldRegenRate(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet , MaxArcaneShieldRegenRate ,OldAmount)
}

void UPHAttributeSet::OnRep_ArcaneShieldRegenAmount(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet , ArcaneShieldRegenAmount ,OldAmount)
}

void UPHAttributeSet::OnRep_MaxArcaneShieldRegenAmount(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet , MaxArcaneShieldRegenAmount ,OldAmount)
}

void UPHAttributeSet::OnRep_ReservedArcaneShield(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet , ReservedArcaneShield ,OldAmount)
}

void UPHAttributeSet::OnRep_MaxReservedArcaneShield(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet , MaxReservedArcaneShield ,OldAmount)
}

void UPHAttributeSet::OnRep_FlatReservedArcaneShield(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet , FlatReservedArcaneShield ,OldAmount)
}

//Mana End

void UPHAttributeSet::OnRep_PercentageReservedArcaneShield(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet , PercentageReservedArcaneShield ,OldAmount)
}

//Gems
void UPHAttributeSet::OnRep_Gems(const FGameplayAttributeData& OldGems) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,Gems, OldGems)
}

void UPHAttributeSet::OnRep_Strength(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,Strength, OldAmount)
}

void UPHAttributeSet::OnRep_Intelligence(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,Intelligence, OldAmount)
}

void UPHAttributeSet::OnRep_Dexterity(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,Dexterity, OldAmount)
}

void UPHAttributeSet::OnRep_Endurance(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,Endurance, OldAmount)
}

void UPHAttributeSet::OnRep_Affliction(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,Affliction, OldAmount)
}

void UPHAttributeSet::OnRep_Luck(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,Luck, OldAmount)
}

void UPHAttributeSet::OnRep_Covenant(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,Covenant, OldAmount)
}

void UPHAttributeSet::OnRep_GlobalDamages(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,GlobalDamages, OldAmount)
}

void UPHAttributeSet::OnRep_MinCorruptionDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MinCorruptionDamage, OldAmount)
}

void UPHAttributeSet::OnRep_MaxCorruptionDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxCorruptionDamage, OldAmount)
}

void UPHAttributeSet::OnRep_CorruptionFlatBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,CorruptionFlatBonus, OldAmount)
}

void UPHAttributeSet::OnRep_CorruptionPercentBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,CorruptionPercentBonus, OldAmount)
}

void UPHAttributeSet::OnRep_MinFireDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MinFireDamage, OldAmount)
}

void UPHAttributeSet::OnRep_MaxFireDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxFireDamage, OldAmount)
}

void UPHAttributeSet::OnRep_FireFlatBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,FireFlatBonus, OldAmount)
}

void UPHAttributeSet::OnRep_FirePercentBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,FirePercentBonus, OldAmount)
}


void UPHAttributeSet::OnRep_MinIceDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MinIceDamage, OldAmount)
}

void UPHAttributeSet::OnRep_MaxIceDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxIceDamage, OldAmount)
}

void UPHAttributeSet::OnRep_IceFlatBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,IceFlatBonus, OldAmount)
}

void UPHAttributeSet::OnRep_IcePercentBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,IcePercentBonus, OldAmount)
}

void UPHAttributeSet::OnRep_MinLightDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MinLightDamage, OldAmount)
}

void UPHAttributeSet::OnRep_MaxLightDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxLightDamage, OldAmount)
}

void UPHAttributeSet::OnRep_LightFlatBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,LightFlatBonus, OldAmount)
}

void UPHAttributeSet::OnRep_LightPercentBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,LightPercentBonus, OldAmount)
}

void UPHAttributeSet::OnRep_ReflectPhysical(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ReflectPhysical, OldAmount)
}

void UPHAttributeSet::OnRep_ReflectElemental(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ReflectElemental, OldAmount)
}

void UPHAttributeSet::OnRep_ReflectChancePhysical(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ReflectChancePhysical, OldAmount)
}

void UPHAttributeSet::OnRep_ReflectChanceElemental(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ReflectChanceElemental, OldAmount)
}

void UPHAttributeSet::OnRep_MinLightningDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MinLightningDamage, OldAmount)
}

void UPHAttributeSet::OnRep_MaxLightningDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxLightningDamage, OldAmount)
}

void UPHAttributeSet::OnRep_LightningFlatBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,LightningFlatBonus, OldAmount)
}

void UPHAttributeSet::OnRep_LightningPercentBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,LightningPercentBonus, OldAmount)
}

void UPHAttributeSet::OnRep_MinPhysicalDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MinPhysicalDamage, OldAmount)
}

void UPHAttributeSet::OnRep_MaxPhysicalDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxPhysicalDamage, OldAmount)
}

void UPHAttributeSet::OnRep_PhysicalFlatBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,PhysicalFlatBonus, OldAmount)
}

void UPHAttributeSet::OnRep_PhysicalPercentBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,PhysicalPercentBonus, OldAmount)
}

void UPHAttributeSet::OnRep_PhysicalToFire(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,PhysicalToFire, OldAmount)
}

void UPHAttributeSet::OnRep_PhysicalToIce(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,PhysicalToIce, OldAmount)
}

void UPHAttributeSet::OnRep_PhysicalToLightning(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,PhysicalToLightning, OldAmount)
}

void UPHAttributeSet::OnRep_PhysicalToLight(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,PhysicalToLight, OldAmount)
}

void UPHAttributeSet::OnRep_PhysicalToCorruption(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,PhysicalToCorruption, OldAmount)
}

void UPHAttributeSet::OnRep_FireToPhysical(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,FireToPhysical, OldAmount)
}

void UPHAttributeSet::OnRep_FireToIce(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,FireToIce, OldAmount)
}

void UPHAttributeSet::OnRep_FireToLightning(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,FireToLightning, OldAmount)
}

void UPHAttributeSet::OnRep_FireToLight(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,FireToLight, OldAmount)
}

void UPHAttributeSet::OnRep_FireToCorruption(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,FireToCorruption, OldAmount)
}

void UPHAttributeSet::OnRep_IceToPhysical(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,IceToPhysical, OldAmount)
}

void UPHAttributeSet::OnRep_IceToFire(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,IceToFire, OldAmount)
}

void UPHAttributeSet::OnRep_IceToLightning(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,IceToLightning, OldAmount)
}

void UPHAttributeSet::OnRep_IceToLight(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,IceToLight, OldAmount)
}

void UPHAttributeSet::OnRep_IceToCorruption(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,IceToCorruption, OldAmount)
}

void UPHAttributeSet::OnRep_LightningToPhysical(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,LightningToPhysical, OldAmount)
}

void UPHAttributeSet::OnRep_LightningToFire(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,LightningToFire, OldAmount)
}

void UPHAttributeSet::OnRep_LightningToIce(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,LightningToIce, OldAmount)
}

void UPHAttributeSet::OnRep_LightningToLight(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,LightningToLight, OldAmount)
}

void UPHAttributeSet::OnRep_LightningToCorruption(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,LightningToCorruption, OldAmount)
}

void UPHAttributeSet::OnRep_LightToPhysical(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,LightToPhysical, OldAmount)
}

void UPHAttributeSet::OnRep_LightToFire(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,LightToFire, OldAmount)
}

void UPHAttributeSet::OnRep_LightToIce(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,LightToIce, OldAmount)
}

void UPHAttributeSet::OnRep_LightToLightning(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,LightToLightning, OldAmount)
}

void UPHAttributeSet::OnRep_LightToCorruption(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,LightToCorruption, OldAmount)
}

void UPHAttributeSet::OnRep_CorruptionToPhysical(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,CorruptionToPhysical, OldAmount)
}

void UPHAttributeSet::OnRep_CorruptionToFire(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,CorruptionToFire, OldAmount)
}

void UPHAttributeSet::OnRep_CorruptionToIce(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,CorruptionToIce, OldAmount)
}

void UPHAttributeSet::OnRep_CorruptionToLightning(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,CorruptionToLightning, OldAmount)
}

void UPHAttributeSet::OnRep_CorruptionToLight(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,CorruptionToLight, OldAmount)
}

void UPHAttributeSet::OnRep_AreaDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,AreaDamage, OldAmount)
}

void UPHAttributeSet::OnRep_AreaOfEffect(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,AreaOfEffect, OldAmount)
}

void UPHAttributeSet::OnRep_AttackRange(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,AttackRange, OldAmount)
}

void UPHAttributeSet::OnRep_AttackSpeed(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,AttackSpeed, OldAmount)
}

void UPHAttributeSet::OnRep_CastSpeed(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,CastSpeed, OldAmount)
}

void UPHAttributeSet::OnRep_CritChance(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,CritChance, OldAmount)
}

void UPHAttributeSet::OnRep_CritMultiplier(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,CritMultiplier, OldAmount)
}

void UPHAttributeSet::OnRep_DamageOverTime(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,DamageOverTime, OldAmount)
}

void UPHAttributeSet::OnRep_ElementalDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ElementalDamage, OldAmount)
}

void UPHAttributeSet::OnRep_SpellsCritChance(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,SpellsCritChance, OldAmount)
}

void UPHAttributeSet::OnRep_SpellsCritMultiplier(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,SpellsCritMultiplier, OldAmount)
}

void UPHAttributeSet::OnRep_DamageBonusWhileAtFullHP(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,DamageBonusWhileAtFullHP, OldAmount)
}

void UPHAttributeSet::OnRep_MaxCorruptionResistance(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxCorruptionResistance, OldAmount)
}

void UPHAttributeSet::OnRep_ArmourPiercing(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ArmourPiercing, OldAmount)
}

void UPHAttributeSet::OnRep_FirePiercing(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet , FirePiercing, OldAmount)
}

void UPHAttributeSet::OnRep_LightPiercing(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,LightPiercing, OldAmount)
}

void UPHAttributeSet::OnRep_LightningPiercing(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,LightningPiercing, OldAmount)
}

void UPHAttributeSet::OnRep_CorruptionPiercing(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,CorruptionPiercing, OldAmount)
}

void UPHAttributeSet::OnRep_IcePiercing(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,IcePiercing, OldAmount)
}

void UPHAttributeSet::OnRep_MaxFireResistance(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxFireResistance, OldAmount)
}

void UPHAttributeSet::OnRep_MaxIceResistance(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxIceResistance, OldAmount)
}

void UPHAttributeSet::OnRep_MaxLightResistance(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxLightResistance, OldAmount)
}

void UPHAttributeSet::OnRep_MaxLightningResistance(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MaxLightningResistance, OldAmount)
}

void UPHAttributeSet::OnRep_DamageBonusWhileAtLowHP(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,DamageBonusWhileAtLowHP, OldAmount)
}

void UPHAttributeSet::OnRep_MeleeDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MeleeDamage, OldAmount)
}

void UPHAttributeSet::OnRep_SpellDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet , SpellDamage, OldAmount);
}

void UPHAttributeSet::OnRep_ProjectileCount(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ProjectileCount, OldAmount)
}

void UPHAttributeSet::OnRep_ProjectileSpeed(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ProjectileSpeed, OldAmount)
}

void UPHAttributeSet::OnRep_RangedDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,RangedDamage, OldAmount)
}

void UPHAttributeSet::OnRep_ChainCount(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ChainCount, OldAmount);
}

void UPHAttributeSet::OnRep_ForkCount(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ForkCount, OldAmount);
}

void UPHAttributeSet::OnRep_ChainDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ChainDamage, OldAmount);
}

void UPHAttributeSet::OnRep_GlobalDefenses(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,GlobalDefenses, OldAmount)
}

void UPHAttributeSet::OnRep_Armour(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,Armour, OldAmount)
}

void UPHAttributeSet::OnRep_ArmourFlatBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ArmourFlatBonus, OldAmount)
}

void UPHAttributeSet::OnRep_ArmourPercentBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ArmourPercentBonus, OldAmount)
}

void UPHAttributeSet::OnRep_CorruptionResistanceFlatBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,CorruptionResistanceFlatBonus, OldAmount)
}

void UPHAttributeSet::OnRep_FireResistanceFlatBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,FireResistanceFlatBonus, OldAmount)
}

void UPHAttributeSet::OnRep_IceResistanceFlatBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,IceResistanceFlatBonus, OldAmount)
}


void UPHAttributeSet::OnRep_LightResistanceFlatBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,LightResistanceFlatBonus, OldAmount)
}



void UPHAttributeSet::OnRep_LightningResistanceFlatBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,LightningResistanceFlatBonus, OldAmount)
}

void UPHAttributeSet::OnRep_CorruptionResistancePercentBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,CorruptionResistancePercentBonus, OldAmount)
}

void UPHAttributeSet::OnRep_FireResistancePercentBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,FireResistancePercentBonus, OldAmount)
}

void UPHAttributeSet::OnRep_IceResistancePercentBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,IceResistancePercentBonus, OldAmount)
}

void UPHAttributeSet::OnRep_LightResistancePercentBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,LightResistancePercentBonus, OldAmount)
}

void UPHAttributeSet::OnRep_LightningResistancePercentBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,LightningResistancePercentBonus, OldAmount)
}

void UPHAttributeSet::OnRep_BlockStrength(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,BlockStrength, OldAmount)
}

void UPHAttributeSet::OnRep_ComboCounter(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ComboCounter, OldAmount)
}

void UPHAttributeSet::OnRep_CoolDown(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,CoolDown, OldAmount)
}

void UPHAttributeSet::OnRep_LifeLeech(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,LifeLeech, OldAmount)
}

void UPHAttributeSet::OnRep_ManaLeech(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ManaLeech, OldAmount)
}

void UPHAttributeSet::OnRep_MovementSpeed(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,MovementSpeed, OldAmount)
}

void UPHAttributeSet::OnRep_Poise(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,Poise, OldAmount)
}

void UPHAttributeSet::OnRep_Weight(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,Weight, OldAmount)
}

void UPHAttributeSet::OnRep_PoiseResistance(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,PoiseResistance, OldAmount)
}

void UPHAttributeSet::OnRep_StunRecovery(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,StunRecovery, OldAmount)
}

void UPHAttributeSet::OnRep_ManaCostChanges(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ManaCostChanges, OldAmount)
}

void UPHAttributeSet::OnRep_LifeOnHit(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,LifeOnHit, OldAmount)
}

void UPHAttributeSet::OnRep_ManaOnHit(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ManaOnHit, OldAmount)
}

void UPHAttributeSet::OnRep_StaminaOnHit(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,StaminaOnHit, OldAmount)
}

void UPHAttributeSet::OnRep_StaminaCostChanges(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,StaminaCostChanges, OldAmount)
}

void UPHAttributeSet::OnRep_AuraEffect(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,AuraEffect, OldAmount)
}

void UPHAttributeSet::OnRep_AuraRadius(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,AuraRadius, OldAmount)
}

void UPHAttributeSet::OnRep_ChanceToBleed(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ChanceToBleed, OldAmount)
}

void UPHAttributeSet::OnRep_ChanceToCorrupt(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ChanceToCorrupt, OldAmount)
}

void UPHAttributeSet::OnRep_ChanceToFreeze(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ChanceToFreeze, OldAmount)
}

void UPHAttributeSet::OnRep_ChanceToIgnite(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ChanceToIgnite, OldAmount)
}

void UPHAttributeSet::OnRep_ChanceToKnockBack(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ChanceToKnockBack, OldAmount)
}

void UPHAttributeSet::OnRep_ChanceToPetrify(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ChanceToPetrify, OldAmount)
}

void UPHAttributeSet::OnRep_ChanceToShock(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ChanceToShock, OldAmount)
}

void UPHAttributeSet::OnRep_ChanceToStun(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ChanceToStun, OldAmount)
}

void UPHAttributeSet::OnRep_ChanceToPurify(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ChanceToPurify, OldAmount)
}

void UPHAttributeSet::OnRep_BurnDuration(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,BurnDuration, OldAmount)
}

void UPHAttributeSet::OnRep_BleedDuration(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,BleedDuration, OldAmount)
}

void UPHAttributeSet::OnRep_FreezeDuration(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,FreezeDuration, OldAmount)
}

void UPHAttributeSet::OnRep_CorruptionDuration(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,CorruptionDuration, OldAmount)
}

void UPHAttributeSet::OnRep_ShockDuration(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,ShockDuration, OldAmount)
}

void UPHAttributeSet::OnRep_PetrifyBuildUpDuration(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,PetrifyBuildUpDuration, OldAmount)
}

void UPHAttributeSet::OnRep_PurifyDuration(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPHAttributeSet ,PurifyDuration, OldAmount)
}

void UPHAttributeSet::RecalculateHealthReserves()
{
	const float MaxHP = GetMaxHealth();
	const float FlatReserved = GetFlatReservedHealth();
	const float PercentReserved = GetPercentageReservedHealth();
    
	// Calculate total reserved: Flat + (Percentage of Max)
	const float CalculatedReserved = FlatReserved + (MaxHP * (PercentReserved / 100.0f));
    
	// Clamp reserved to not exceed max health (leave at least 1 HP)
	const float ClampedReserved = FMath::Clamp(CalculatedReserved, 0.0f, MaxHP - 1.0f);
    
	// Set the calculated reserved amount
	SetReservedHealth(ClampedReserved);
    
	// Calculate and set effective max health
	const float EffectiveMax = FMath::Max(1.0f, MaxHP - ClampedReserved);
	SetMaxEffectiveHealth(EffectiveMax);
    
	// Clamp current health to new effective max
	if (GetHealth() > EffectiveMax)
	{
		SetHealth(EffectiveMax);
	}
    
	UE_LOG(LogTemp, Verbose, TEXT("Health Reserves: Max=%.1f, Flat=%.1f, Percent=%.1f%%, Reserved=%.1f, Effective=%.1f"), 
		   MaxHP, FlatReserved, PercentReserved, ClampedReserved, EffectiveMax);
}

void UPHAttributeSet::RecalculateManaReserves()
{
	const float MaxMP = GetMaxMana();
	const float FlatReserved = GetFlatReservedMana();
	const float PercentReserved = GetPercentageReservedMana();
    
	const float CalculatedReserved = FlatReserved + (MaxMP * (PercentReserved / 100.0f));
	const float ClampedReserved = FMath::Clamp(CalculatedReserved, 0.0f, MaxMP);
    
	SetReservedMana(ClampedReserved);
    
	const float EffectiveMax = FMath::Max(0.0f, MaxMP - ClampedReserved);
	SetMaxEffectiveMana(EffectiveMax);
    
	if (GetMana() > EffectiveMax)
	{
		SetMana(EffectiveMax);
	}
    
	UE_LOG(LogTemp, Verbose, TEXT("Mana Reserves: Max=%.1f, Reserved=%.1f, Effective=%.1f"), 
		   MaxMP, ClampedReserved, EffectiveMax);
}

void UPHAttributeSet::RecalculateStaminaReserves()
{
	const float MaxSP = GetMaxStamina();
	const float FlatReserved = GetFlatReservedStamina();
	const float PercentReserved = GetPercentageReservedStamina();
    
	const float CalculatedReserved = FlatReserved + (MaxSP * (PercentReserved / 100.0f));
	const float ClampedReserved = FMath::Clamp(CalculatedReserved, 0.0f, MaxSP);
    
	SetReservedStamina(ClampedReserved);
    
	const float EffectiveMax = FMath::Max(0.0f, MaxSP - ClampedReserved);
	SetMaxEffectiveStamina(EffectiveMax);
    
	if (GetStamina() > EffectiveMax)
	{
		SetStamina(EffectiveMax);
	}
    
	UE_LOG(LogTemp, Verbose, TEXT("Stamina Reserves: Max=%.1f, Reserved=%.1f, Effective=%.1f"), 
		   MaxSP, ClampedReserved, EffectiveMax);
}

void UPHAttributeSet::RecalculateArcaneShieldReserves()
{
	const float MaxAS = GetMaxArcaneShield();
	const float FlatReserved = GetFlatReservedArcaneShield();
	const float PercentReserved = GetPercentageReservedArcaneShield();
    
	const float CalculatedReserved = FlatReserved + (MaxAS * (PercentReserved / 100.0f));
	const float ClampedReserved = FMath::Clamp(CalculatedReserved, 0.0f, MaxAS);
    
	SetReservedArcaneShield(ClampedReserved);
    
	const float EffectiveMax = FMath::Max(0.0f, MaxAS - ClampedReserved);
	SetMaxEffectiveArcaneShield(EffectiveMax);
    
	if (GetArcaneShield() > EffectiveMax)
	{
		SetArcaneShield(EffectiveMax);
	}
    
	UE_LOG(LogTemp, Verbose, TEXT("Arcane Shield Reserves: Max=%.1f, Reserved=%.1f, Effective=%.1f"), 
		   MaxAS, ClampedReserved, EffectiveMax);
}

