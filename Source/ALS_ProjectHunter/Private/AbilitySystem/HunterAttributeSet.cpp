// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/HunterAttributeSet.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Net/UnrealNetwork.h"


UHunterAttributeSet::UHunterAttributeSet()
{
	GlobalXPGain.SetBaseValue(0.0f);       
	LocalXPGain.SetBaseValue(0.0f);        
	XPGainMultiplier.SetBaseValue(1.0f);   
	XPPenalty.SetBaseValue(1.0f); 
}

FGameplayAttribute UHunterAttributeSet::FindAttributeByName(FName AttributeName)
{
	// Uses Unreal's reflection system - works with ALL attributes automatically! ✅
	const UHunterAttributeSet* ClassCDO = GetDefault<UHunterAttributeSet>();
	
	if (!ClassCDO)
	{
		return FGameplayAttribute();
	}

	// Search through all properties
	for (TFieldIterator<FProperty> PropIt(UHunterAttributeSet::StaticClass()); PropIt; ++PropIt)
	{
		FProperty* Property = *PropIt;
		
		// Check if this is a FGameplayAttributeData property
		if (FStructProperty* StructProperty = CastField<FStructProperty>(Property))
		{
			if (StructProperty->Struct == FGameplayAttributeData::StaticStruct())
			{
				// Check if the name matches
				if (Property->GetFName() == AttributeName)
				{
					return FGameplayAttribute(Property);
				}
			}
		}
	}

	return FGameplayAttribute(); // Not found
}

void UHunterAttributeSet::GetAllAttributes(TArray<FGameplayAttribute>& OutAttributes)
{
	OutAttributes.Empty();

	for (TFieldIterator<FProperty> PropIt(UHunterAttributeSet::StaticClass()); PropIt; ++PropIt)
	{
		FProperty* Property = *PropIt;
		
		if (FStructProperty* StructProperty = CastField<FStructProperty>(Property))
		{
			if (StructProperty->Struct == FGameplayAttributeData::StaticStruct())
			{
				OutAttributes.Add(FGameplayAttribute(Property));
			}
		}
	}
}

void UHunterAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	/* ============================= */
	/* === Primary Attributes ====== */
	/* ============================= */
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, Strength,     COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, Intelligence, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, Dexterity,    COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, Endurance,    COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, Affliction,   COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, Luck,         COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, Covenant,     COND_OwnerOnly, REPNOTIFY_Always);


	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, GlobalXPGain,   COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, LocalXPGain,         COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, XPGainMultiplier,   COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, XPPenalty,         COND_OwnerOnly, REPNOTIFY_Always);
	
	/* ============================= */
	/* === Vital Max (raw/effective) */
	/* ============================= */
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, MaxHealth,            COND_None,      REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, MaxEffectiveHealth,   COND_None,      REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, MaxStamina,           COND_None,      REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, MaxEffectiveStamina,  COND_None,      REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, MaxMana,              COND_None,      REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, MaxEffectiveMana,     COND_None,      REPNOTIFY_Always);

	/* ============================= */
	/* === Health Regen / Reserve == */
	/* ============================= */
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, HealthRegenRate,          COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, HealthRegenAmount,        COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, ReservedHealth,           COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, MaxReservedHealth,        COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, FlatReservedHealth,       COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, PercentageReservedHealth, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, MaxHealthRegenRate,   COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, MaxHealthRegenAmount, COND_OwnerOnly, REPNOTIFY_Always);


	/* ============================= */
	/* === Mana Regen / Reserve ==== */
	/* ============================= */
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, ManaRegenRate,            COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, ManaRegenAmount,          COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, ReservedMana,             COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, MaxReservedMana,          COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, FlatReservedMana,         COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, PercentageReservedMana,   COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, MaxManaRegenRate,     COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, MaxManaRegenAmount,   COND_OwnerOnly, REPNOTIFY_Always);

	/* ============================= */
	/* === Stamina Regen / Reserve = */
	/* ============================= */
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, StaminaRegenRate,         COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, StaminaRegenAmount,       COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, StaminaDegenRate,         COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, StaminaDegenAmount,       COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, ReservedStamina,          COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, MaxReservedStamina,       COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, FlatReservedStamina,      COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, PercentageReservedStamina,COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, MaxStaminaRegenRate,  COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, MaxStaminaRegenAmount,COND_OwnerOnly, REPNOTIFY_Always);

	/* ============================= */
	/* === Arcane Shield  ===== */
	/* ============================= */
	// Core pools (missing before)
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, ArcaneShield,              COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, MaxArcaneShield,           COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, MaxEffectiveArcaneShield,  COND_None, REPNOTIFY_Always);
	
	// Regen / reserve (you already had most of these)
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, ArcaneShieldRegenRate,        COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, ArcaneShieldRegenAmount,      COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, ReservedArcaneShield,         COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, MaxReservedArcaneShield,      COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, FlatReservedArcaneShield,     COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, PercentageReservedArcaneShield,COND_OwnerOnly, REPNOTIFY_Always);

	/* ============================= */
	/* === Damage Ranges & Bonuses = */
	/* ============================= */
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, GlobalDamages,        COND_OwnerOnly, REPNOTIFY_Always);

	// Min
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, MinPhysicalDamage,     COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, MinFireDamage,         COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, MinLightDamage,        COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, MinLightningDamage,    COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, MinCorruptionDamage,   COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, MinIceDamage,          COND_OwnerOnly, REPNOTIFY_Always);

	// Max
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, MaxPhysicalDamage,     COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, MaxFireDamage,         COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, MaxLightDamage,        COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, MaxLightningDamage,    COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, MaxCorruptionDamage,   COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, MaxIceDamage,          COND_OwnerOnly, REPNOTIFY_Always);

	// Flat bonuses
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, PhysicalFlatDamage,     COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, FireFlatDamage,         COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, LightFlatDamage,        COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, LightningFlatDamage,    COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, CorruptionFlatDamage,   COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, IceFlatDamage,          COND_OwnerOnly, REPNOTIFY_Always);

	// Percent bonuses
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, PhysicalPercentDamage,  COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, FirePercentDamage,      COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, LightPercentDamage,     COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, LightningPercentDamage, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, CorruptionPercentDamage,COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, IcePercentDamage,       COND_OwnerOnly, REPNOTIFY_Always);

	// Situational
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, DamageBonusWhileAtFullHP, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, DamageBonusWhileAtLowHP,  COND_OwnerOnly, REPNOTIFY_Always);

	/* ============================= */
	/* === Other Offensive Stats === */
	/* ============================= */
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, AreaDamage,         COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, AreaOfEffect,       COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, AttackRange,        COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, AttackSpeed,        COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, CastSpeed,          COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, CritChance,         COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, CritMultiplier,     COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, DamageOverTime,     COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, ElementalDamage,    COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, SpellsCritChance,   COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, SpellsCritMultiplier,COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, MeleeDamage,        COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, SpellDamage,        COND_OwnerOnly, REPNOTIFY_Always); 
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, ProjectileCount,    COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, ProjectileSpeed,    COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, RangedDamage,       COND_OwnerOnly, REPNOTIFY_Always);

	/* ============================= */
	/* === Durations =============== */
	/* ============================= */
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, BurnDuration,           COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, BleedDuration,          COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, FreezeDuration,         COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, CorruptionDuration,     COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, ShockDuration,          COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, PetrifyBuildUpDuration, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, PurifyDuration,         COND_OwnerOnly, REPNOTIFY_Always);

	/* ============================= */
	/* === Resistances ============= */
	/* ============================= */
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, GlobalDefenses,                COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, BlockStrength,                 COND_None,      REPNOTIFY_Always);

	// Armour
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, Armour,                        COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, ArmourFlatBonus,               COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, ArmourPercentBonus,            COND_OwnerOnly, REPNOTIFY_Always);

	// Fire
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, FireResistanceFlatBonus,       COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, FireResistancePercentBonus,    COND_OwnerOnly, REPNOTIFY_Always); 
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, MaxFireResistance,             COND_OwnerOnly, REPNOTIFY_Always);

	// Light
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, LightResistanceFlatBonus,      COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, LightResistancePercentBonus,   COND_OwnerOnly, REPNOTIFY_Always); 
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, MaxLightResistance,            COND_OwnerOnly, REPNOTIFY_Always);

	// Lightning
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, LightningResistanceFlatBonus,  COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, LightningResistancePercentBonus,COND_OwnerOnly, REPNOTIFY_Always); 
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, MaxLightningResistance,        COND_OwnerOnly, REPNOTIFY_Always);

	// Corruption
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, CorruptionResistanceFlatBonus, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, CorruptionResistancePercentBonus, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, MaxCorruptionResistance,       COND_OwnerOnly, REPNOTIFY_Always);

	// Ice
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, IceResistanceFlatBonus,        COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, IceResistancePercentBonus,     COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, MaxIceResistance,              COND_OwnerOnly, REPNOTIFY_Always);


	// Physical Damage Conversions\
	// DOREPLIFETIME_CONDITION_NOTIFY(HunterAttributeSet, PhysicalToFire, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, PhysicalToIce, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, PhysicalToLightning, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, PhysicalToLight, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, PhysicalToCorruption, COND_None, REPNOTIFY_Always);

	// Fire Damage Conversions
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, FireToPhysical, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, FireToIce, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, FireToLightning, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, FireToLight, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, FireToCorruption, COND_None, REPNOTIFY_Always);

	// Ice Damage Conversions
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, IceToPhysical, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, IceToFire, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, IceToLightning, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, IceToLight, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, IceToCorruption, COND_None, REPNOTIFY_Always);

	// Lightning Damage Conversions
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, LightningToPhysical, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, LightningToFire, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, LightningToIce, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, LightningToLight, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, LightningToCorruption, COND_None, REPNOTIFY_Always);

	// Light Damage Conversions
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, LightToPhysical, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, LightToFire, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, LightToIce, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, LightToLightning, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, LightToCorruption, COND_None, REPNOTIFY_Always);

	// Corruption Damage Conversions
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, CorruptionToPhysical, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, CorruptionToFire, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, CorruptionToIce, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, CorruptionToLightning, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, CorruptionToLight, COND_None, REPNOTIFY_Always);

	/* ============================= */
	/* === Piercing ================ */
	/* ============================= */
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, ArmourPiercing,        COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, FirePiercing,          COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, LightPiercing,         COND_OwnerOnly, REPNOTIFY_Always); 
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, LightningPiercing,     COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, CorruptionPiercing,    COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, IcePiercing,           COND_OwnerOnly, REPNOTIFY_Always);

	/* ============================= */
	/* === Ailment Chances ========= */
	/* ============================= */
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, ChanceToBleed,        COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, ChanceToCorrupt,      COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, ChanceToFreeze,       COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, ChanceToIgnite,       COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, ChanceToPetrify,      COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, ChanceToPurify,       COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, ChanceToShock,        COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, ChanceToStun,         COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, ChanceToKnockBack,    COND_OwnerOnly, REPNOTIFY_Always);

	/* ============================= */
	/* === Misc ==================== */
	/* ============================= */
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, ComboCounter,     COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, Cooldown,         COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, LifeLeech,        COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, ManaLeech,        COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, MovementSpeed,    COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, Poise,            COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, Weight,           COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, PoiseResistance,  COND_OwnerOnly, REPNOTIFY_Always); 
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, StunRecovery,     COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, ManaCostChanges,  COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, HealthCostChanges,  COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, LifeOnHit,        COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, ManaOnHit,        COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, StaminaOnHit,     COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, StaminaCostChanges,COND_OwnerOnly, REPNOTIFY_Always);

	/* ============================= */
	/* === Current Vitals ========= */
	/* ============================= */
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, Health,   COND_None,      REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, Mana,     COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, Stamina,  COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UHunterAttributeSet, Gems,     COND_OwnerOnly, REPNOTIFY_Always);
}


float UHunterAttributeSet::GetAttributeValue(const FGameplayAttribute& Attribute) const 
{
	if (const UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent())
	{
		return ASC->GetNumericAttribute(Attribute);
	}
	return 0.0f;
}



void UHunterAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
    Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetGlobalXPGainAttribute() || Attribute == GetLocalXPGainAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, 500.0f); // 0-500%
	}
	else if (Attribute == GetXPGainMultiplierAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.01f); // Min 1%
	}
	else if (Attribute == GetXPPenaltyAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, 1.0f); // 0-100%
	}
	
	// Dispatch to category-specific validation functions
	ClampVitalAttributes(Attribute, NewValue);
	ClampPrimaryAttributes(Attribute, NewValue);
	ClampPercentageAttributes(Attribute, NewValue);
	ClampDamageAttributes(Attribute, NewValue);
	ClampResistanceAttributes(Attribute, NewValue);
	ClampRateAndAmountAttributes(Attribute, NewValue);
	ClampUtilityAttributes(Attribute, NewValue);
	ClampSpecialAttributes(Attribute, NewValue);
}



void UHunterAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
}


bool UHunterAttributeSet::ShouldUpdateThresholdTags(const FGameplayAttribute& Attribute)
{
	static const TSet<FGameplayAttribute> TrackedThresholdAttributes = {
		GetHealthAttribute(),
		GetManaAttribute(),
		GetStaminaAttribute(),
		GetArcaneShieldAttribute()
	};

	return TrackedThresholdAttributes.Contains(Attribute);
}

//Health
void UHunterAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,Health, OldHealth)
}

void UHunterAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,MaxHealth, OldMaxHealth)
}

void UHunterAttributeSet::OnRep_MaxEffectiveHealth(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,MaxEffectiveHealth, OldAmount);
}

void UHunterAttributeSet::OnRep_HealthRegenRate(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,HealthRegenRate, OldAmount);
}

void UHunterAttributeSet::OnRep_MaxHealthRegenRate(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,MaxHealthRegenRate, OldAmount);
}

void UHunterAttributeSet::OnRep_HealthRegenAmount(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,HealthRegenAmount, OldAmount);
}


void UHunterAttributeSet::OnRep_MaxHealthRegenAmount(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,MaxHealthRegenAmount, OldAmount);
}

void UHunterAttributeSet::OnRep_ReservedHealth(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,ReservedHealth, OldAmount);
}

void UHunterAttributeSet::OnRep_MaxReservedHealth(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,MaxReservedHealth, OldAmount);
}

void UHunterAttributeSet::OnRep_FlatReservedHealth(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet , FlatReservedHealth, OldAmount);
}

void UHunterAttributeSet::OnRep_PercentageReservedHealth(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet , PercentageReservedHealth, OldAmount);
}

//Health End

//Stamina 
void UHunterAttributeSet::OnRep_Stamina(const FGameplayAttributeData& OldStamina) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet , Stamina,OldStamina)
}

void UHunterAttributeSet::OnRep_MaxStamina(const FGameplayAttributeData& OldMaxStamina) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,MaxStamina, OldMaxStamina)
}

void UHunterAttributeSet::OnRep_MaxEffectiveStamina(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,MaxEffectiveStamina, OldAmount)

}

void UHunterAttributeSet::OnRep_StaminaRegenRate(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,StaminaRegenRate, OldAmount)
}

void UHunterAttributeSet::OnRep_StaminaDegenRate(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,StaminaDegenRate, OldAmount)
}

void UHunterAttributeSet::OnRep_MaxStaminaRegenRate(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,MaxStaminaRegenRate, OldAmount)
}

void UHunterAttributeSet::OnRep_StaminaRegenAmount(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,StaminaRegenAmount, OldAmount)
}

void UHunterAttributeSet::OnRep_StaminaDegenAmount(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,StaminaDegenAmount, OldAmount)
}

void UHunterAttributeSet::OnRep_MaxStaminaRegenAmount(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,MaxStaminaRegenAmount,OldAmount)
}

void UHunterAttributeSet::OnRep_ReservedStamina(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,ReservedStamina, OldAmount)
}

void UHunterAttributeSet::OnRep_MaxReservedStamina(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,MaxReservedStamina, OldAmount)
}


void UHunterAttributeSet::OnRep_FlatReservedStamina(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,FlatReservedStamina, OldAmount)
}

void UHunterAttributeSet::OnRep_PercentageReservedStamina(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,PercentageReservedStamina, OldAmount)
}

//Stamina End

//Mana
void UHunterAttributeSet::OnRep_Mana(const FGameplayAttributeData& OldMana) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,Mana, OldMana)
}

void UHunterAttributeSet::OnRep_MaxMana(const FGameplayAttributeData& OldMaxMana) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,MaxMana, OldMaxMana)

}

void UHunterAttributeSet::OnRep_MaxEffectiveMana(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,MaxEffectiveMana, OldAmount)
}

void UHunterAttributeSet::OnRep_ManaRegenRate(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,ManaRegenRate,OldAmount)
}

void UHunterAttributeSet::OnRep_MaxManaRegenRate(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,MaxManaRegenRate,OldAmount)
}

void UHunterAttributeSet::OnRep_ManaRegenAmount(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,ManaRegenAmount,OldAmount)
}

void UHunterAttributeSet::OnRep_MaxManaRegenAmount(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,MaxManaRegenAmount,OldAmount)
}

void UHunterAttributeSet::OnRep_ReservedMana(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,ReservedMana,OldAmount)
}

void UHunterAttributeSet::OnRep_MaxReservedMana(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,MaxReservedMana,OldAmount)
}

void UHunterAttributeSet::OnRep_FlatReservedMana(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet , FlatReservedMana,OldAmount)
}

void UHunterAttributeSet::OnRep_PercentageReservedMana(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet , PercentageReservedMana,OldAmount)
}

void UHunterAttributeSet::OnRep_ArcaneShield(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet , ArcaneShield ,OldAmount)
}

void UHunterAttributeSet::OnRep_MaxArcaneShield(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet , MaxArcaneShield ,OldAmount)
}

void UHunterAttributeSet::OnRep_MaxEffectiveArcaneShield(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet , MaxEffectiveArcaneShield ,OldAmount)
}

void UHunterAttributeSet::OnRep_ArcaneShieldRegenRate(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet , ArcaneShieldRegenRate ,OldAmount)
}

void UHunterAttributeSet::OnRep_MaxArcaneShieldRegenRate(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet , MaxArcaneShieldRegenRate ,OldAmount)
}

void UHunterAttributeSet::OnRep_ArcaneShieldRegenAmount(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet , ArcaneShieldRegenAmount ,OldAmount)
}

void UHunterAttributeSet::OnRep_MaxArcaneShieldRegenAmount(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet , MaxArcaneShieldRegenAmount ,OldAmount)
}

void UHunterAttributeSet::OnRep_ReservedArcaneShield(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet , ReservedArcaneShield ,OldAmount)
}

void UHunterAttributeSet::OnRep_MaxReservedArcaneShield(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet , MaxReservedArcaneShield ,OldAmount)
}

void UHunterAttributeSet::OnRep_FlatReservedArcaneShield(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet , FlatReservedArcaneShield ,OldAmount)
}

//Mana End

void UHunterAttributeSet::OnRep_PercentageReservedArcaneShield(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet , PercentageReservedArcaneShield ,OldAmount)
}

//Gems
void UHunterAttributeSet::OnRep_Gems(const FGameplayAttributeData& OldGems) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,Gems, OldGems)
}

void UHunterAttributeSet::OnRep_Strength(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,Strength, OldAmount)
}

void UHunterAttributeSet::OnRep_Intelligence(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,Intelligence, OldAmount)
}

void UHunterAttributeSet::OnRep_Dexterity(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,Dexterity, OldAmount)
}

void UHunterAttributeSet::OnRep_Endurance(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,Endurance, OldAmount)
}

void UHunterAttributeSet::OnRep_Affliction(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,Affliction, OldAmount)
}

void UHunterAttributeSet::OnRep_Luck(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,Luck, OldAmount)
}

void UHunterAttributeSet::OnRep_Covenant(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,Covenant, OldAmount)
}

void UHunterAttributeSet::OnRep_GlobalXPGain(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,GlobalXPGain, OldValue)
}

void UHunterAttributeSet::OnRep_LocalXPGain(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,LocalXPGain, OldValue)
}

void UHunterAttributeSet::OnRep_XPGainMultiplier(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,XPGainMultiplier, OldValue)
}

void UHunterAttributeSet::OnRep_XPPenalty(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,XPPenalty, OldValue)
}

void UHunterAttributeSet::OnRep_GlobalDamages(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,GlobalDamages, OldAmount)
}

void UHunterAttributeSet::OnRep_MinCorruptionDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,MinCorruptionDamage, OldAmount)
}

void UHunterAttributeSet::OnRep_MaxCorruptionDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,MaxCorruptionDamage, OldAmount)
}

void UHunterAttributeSet::OnRep_CorruptionFlatDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,CorruptionFlatDamage, OldAmount)
}

void UHunterAttributeSet::OnRep_CorruptionPercentDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,CorruptionPercentDamage, OldAmount)
}

void UHunterAttributeSet::OnRep_MinFireDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,MinFireDamage, OldAmount)
}

void UHunterAttributeSet::OnRep_MaxFireDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,MaxFireDamage, OldAmount)
}

void UHunterAttributeSet::OnRep_FireFlatDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,FireFlatDamage, OldAmount)
}

void UHunterAttributeSet::OnRep_FirePercentDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,FirePercentDamage, OldAmount)
}


void UHunterAttributeSet::OnRep_MinIceDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,MinIceDamage, OldAmount)
}

void UHunterAttributeSet::OnRep_MaxIceDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,MaxIceDamage, OldAmount)
}

void UHunterAttributeSet::OnRep_IceFlatDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,IceFlatDamage, OldAmount)
}

void UHunterAttributeSet::OnRep_IcePercentDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,IcePercentDamage, OldAmount)
}

void UHunterAttributeSet::OnRep_MinLightDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,MinLightDamage, OldAmount)
}

void UHunterAttributeSet::OnRep_MaxLightDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,MaxLightDamage, OldAmount)
}

void UHunterAttributeSet::OnRep_LightFlatDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,LightFlatDamage, OldAmount)
}

void UHunterAttributeSet::OnRep_LightPercentDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,LightPercentDamage, OldAmount)
}

void UHunterAttributeSet::OnRep_ReflectPhysical(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,ReflectPhysical, OldAmount)
}

void UHunterAttributeSet::OnRep_ReflectElemental(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,ReflectElemental, OldAmount)
}

void UHunterAttributeSet::OnRep_ReflectChancePhysical(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,ReflectChancePhysical, OldAmount)
}

void UHunterAttributeSet::OnRep_ReflectChanceElemental(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,ReflectChanceElemental, OldAmount)
}

void UHunterAttributeSet::OnRep_MinLightningDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,MinLightningDamage, OldAmount)
}

void UHunterAttributeSet::OnRep_MaxLightningDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,MaxLightningDamage, OldAmount)
}

void UHunterAttributeSet::OnRep_LightningFlatDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,LightningFlatDamage, OldAmount)
}

void UHunterAttributeSet::OnRep_LightningPercentDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,LightningPercentDamage, OldAmount)
}

void UHunterAttributeSet::OnRep_MinPhysicalDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,MinPhysicalDamage, OldAmount)
}

void UHunterAttributeSet::OnRep_MaxPhysicalDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,MaxPhysicalDamage, OldAmount)
}

void UHunterAttributeSet::OnRep_PhysicalFlatDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,PhysicalFlatDamage, OldAmount)
}

void UHunterAttributeSet::OnRep_PhysicalPercentDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,PhysicalPercentDamage, OldAmount)
}

void UHunterAttributeSet::OnRep_PhysicalToFire(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,PhysicalToFire, OldAmount)
}

void UHunterAttributeSet::OnRep_PhysicalToIce(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,PhysicalToIce, OldAmount)
}

void UHunterAttributeSet::OnRep_PhysicalToLightning(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,PhysicalToLightning, OldAmount)
}

void UHunterAttributeSet::OnRep_PhysicalToLight(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,PhysicalToLight, OldAmount)
}

void UHunterAttributeSet::OnRep_PhysicalToCorruption(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,PhysicalToCorruption, OldAmount)
}

void UHunterAttributeSet::OnRep_FireToPhysical(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,FireToPhysical, OldAmount)
}

void UHunterAttributeSet::OnRep_FireToIce(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,FireToIce, OldAmount)
}

void UHunterAttributeSet::OnRep_FireToLightning(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,FireToLightning, OldAmount)
}

void UHunterAttributeSet::OnRep_FireToLight(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,FireToLight, OldAmount)
}

void UHunterAttributeSet::OnRep_FireToCorruption(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,FireToCorruption, OldAmount)
}

void UHunterAttributeSet::OnRep_IceToPhysical(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,IceToPhysical, OldAmount)
}

void UHunterAttributeSet::OnRep_IceToFire(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,IceToFire, OldAmount)
}

void UHunterAttributeSet::OnRep_IceToLightning(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,IceToLightning, OldAmount)
}

void UHunterAttributeSet::OnRep_IceToLight(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,IceToLight, OldAmount)
}

void UHunterAttributeSet::OnRep_IceToCorruption(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,IceToCorruption, OldAmount)
}

void UHunterAttributeSet::OnRep_LightningToPhysical(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,LightningToPhysical, OldAmount)
}

void UHunterAttributeSet::OnRep_LightningToFire(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,LightningToFire, OldAmount)
}

void UHunterAttributeSet::OnRep_LightningToIce(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,LightningToIce, OldAmount)
}

void UHunterAttributeSet::OnRep_LightningToLight(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,LightningToLight, OldAmount)
}

void UHunterAttributeSet::OnRep_LightningToCorruption(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,LightningToCorruption, OldAmount)
}

void UHunterAttributeSet::OnRep_LightToPhysical(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,LightToPhysical, OldAmount)
}

void UHunterAttributeSet::OnRep_LightToFire(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,LightToFire, OldAmount)
}

void UHunterAttributeSet::OnRep_LightToIce(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,LightToIce, OldAmount)
}

void UHunterAttributeSet::OnRep_LightToLightning(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,LightToLightning, OldAmount)
}

void UHunterAttributeSet::OnRep_LightToCorruption(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,LightToCorruption, OldAmount)
}

void UHunterAttributeSet::OnRep_CorruptionToPhysical(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,CorruptionToPhysical, OldAmount)
}

void UHunterAttributeSet::OnRep_CorruptionToFire(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,CorruptionToFire, OldAmount)
}

void UHunterAttributeSet::OnRep_CorruptionToIce(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,CorruptionToIce, OldAmount)
}

void UHunterAttributeSet::OnRep_CorruptionToLightning(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,CorruptionToLightning, OldAmount)
}

void UHunterAttributeSet::OnRep_CorruptionToLight(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,CorruptionToLight, OldAmount)
}

void UHunterAttributeSet::OnRep_AreaDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,AreaDamage, OldAmount)
}

void UHunterAttributeSet::OnRep_AreaOfEffect(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,AreaOfEffect, OldAmount)
}

void UHunterAttributeSet::OnRep_AttackRange(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,AttackRange, OldAmount)
}

void UHunterAttributeSet::OnRep_AttackSpeed(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,AttackSpeed, OldAmount)
}

void UHunterAttributeSet::OnRep_CastSpeed(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,CastSpeed, OldAmount)
}

void UHunterAttributeSet::OnRep_CritChance(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,CritChance, OldAmount)
}

void UHunterAttributeSet::OnRep_CritMultiplier(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,CritMultiplier, OldAmount)
}

void UHunterAttributeSet::OnRep_DamageOverTime(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,DamageOverTime, OldAmount)
}

void UHunterAttributeSet::OnRep_ElementalDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,ElementalDamage, OldAmount)
}

void UHunterAttributeSet::OnRep_SpellsCritChance(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,SpellsCritChance, OldAmount)
}

void UHunterAttributeSet::OnRep_SpellsCritMultiplier(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,SpellsCritMultiplier, OldAmount)
}

void UHunterAttributeSet::OnRep_DamageBonusWhileAtFullHP(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,DamageBonusWhileAtFullHP, OldAmount)
}

void UHunterAttributeSet::OnRep_MaxCorruptionResistance(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,MaxCorruptionResistance, OldAmount)
}

void UHunterAttributeSet::OnRep_ArmourPiercing(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,ArmourPiercing, OldAmount)
}

void UHunterAttributeSet::OnRep_FirePiercing(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet , FirePiercing, OldAmount)
}

void UHunterAttributeSet::OnRep_LightPiercing(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,LightPiercing, OldAmount)
}

void UHunterAttributeSet::OnRep_LightningPiercing(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,LightningPiercing, OldAmount)
}

void UHunterAttributeSet::OnRep_CorruptionPiercing(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,CorruptionPiercing, OldAmount)
}

void UHunterAttributeSet::OnRep_IcePiercing(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,IcePiercing, OldAmount)
}

void UHunterAttributeSet::OnRep_MaxFireResistance(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,MaxFireResistance, OldAmount)
}

void UHunterAttributeSet::OnRep_MaxIceResistance(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,MaxIceResistance, OldAmount)
}

void UHunterAttributeSet::OnRep_MaxLightResistance(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,MaxLightResistance, OldAmount)
}

void UHunterAttributeSet::OnRep_MaxLightningResistance(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,MaxLightningResistance, OldAmount)
}

void UHunterAttributeSet::OnRep_DamageBonusWhileAtLowHP(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,DamageBonusWhileAtLowHP, OldAmount)
}

void UHunterAttributeSet::OnRep_MeleeDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,MeleeDamage, OldAmount)
}

void UHunterAttributeSet::OnRep_SpellDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet , SpellDamage, OldAmount);
}

void UHunterAttributeSet::OnRep_ProjectileCount(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,ProjectileCount, OldAmount)
}

void UHunterAttributeSet::OnRep_ProjectileSpeed(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,ProjectileSpeed, OldAmount)
}

void UHunterAttributeSet::OnRep_RangedDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,RangedDamage, OldAmount)
}

void UHunterAttributeSet::OnRep_ChainCount(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,ChainCount, OldAmount);
}

void UHunterAttributeSet::OnRep_ForkCount(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,ForkCount, OldAmount);
}

void UHunterAttributeSet::OnRep_ChainDamage(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,ChainDamage, OldAmount);
}

void UHunterAttributeSet::OnRep_GlobalDefenses(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,GlobalDefenses, OldAmount)
}

void UHunterAttributeSet::OnRep_Armour(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,Armour, OldAmount)
}

void UHunterAttributeSet::OnRep_ArmourFlatBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,ArmourFlatBonus, OldAmount)
}

void UHunterAttributeSet::OnRep_ArmourPercentBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,ArmourPercentBonus, OldAmount)
}

void UHunterAttributeSet::OnRep_CorruptionResistanceFlatBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,CorruptionResistanceFlatBonus, OldAmount)
}

void UHunterAttributeSet::OnRep_FireResistanceFlatBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,FireResistanceFlatBonus, OldAmount)
}

void UHunterAttributeSet::OnRep_IceResistanceFlatBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,IceResistanceFlatBonus, OldAmount)
}


void UHunterAttributeSet::OnRep_LightResistanceFlatBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,LightResistanceFlatBonus, OldAmount)
}



void UHunterAttributeSet::OnRep_LightningResistanceFlatBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,LightningResistanceFlatBonus, OldAmount)
}

void UHunterAttributeSet::OnRep_CorruptionResistancePercentBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,CorruptionResistancePercentBonus, OldAmount)
}

void UHunterAttributeSet::OnRep_FireResistancePercentBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,FireResistancePercentBonus, OldAmount)
}

void UHunterAttributeSet::OnRep_IceResistancePercentBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,IceResistancePercentBonus, OldAmount)
}

void UHunterAttributeSet::OnRep_LightResistancePercentBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,LightResistancePercentBonus, OldAmount)
}

void UHunterAttributeSet::OnRep_LightningResistancePercentBonus(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,LightningResistancePercentBonus, OldAmount)
}

void UHunterAttributeSet::OnRep_BlockStrength(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,BlockStrength, OldAmount)
}

void UHunterAttributeSet::OnRep_ComboCounter(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,ComboCounter, OldAmount)
}

void UHunterAttributeSet::OnRep_Cooldown(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet , Cooldown, OldAmount)
}

void UHunterAttributeSet::OnRep_LifeLeech(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,LifeLeech, OldAmount)
}

void UHunterAttributeSet::OnRep_ManaLeech(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,ManaLeech, OldAmount)
}

void UHunterAttributeSet::OnRep_MovementSpeed(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,MovementSpeed, OldAmount)
}

void UHunterAttributeSet::OnRep_Poise(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,Poise, OldAmount)
}

void UHunterAttributeSet::OnRep_Weight(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,Weight, OldAmount)
}

void UHunterAttributeSet::OnRep_PoiseResistance(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,PoiseResistance, OldAmount)
}

void UHunterAttributeSet::OnRep_StunRecovery(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,StunRecovery, OldAmount)
}

void UHunterAttributeSet::OnRep_ManaCostChanges(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,ManaCostChanges, OldAmount)
}

void UHunterAttributeSet::OnRep_HealthCostChanges(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,HealthCostChanges, OldAmount)
}

void UHunterAttributeSet::OnRep_LifeOnHit(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,LifeOnHit, OldAmount)
}

void UHunterAttributeSet::OnRep_ManaOnHit(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,ManaOnHit, OldAmount)
}

void UHunterAttributeSet::OnRep_StaminaOnHit(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,StaminaOnHit, OldAmount)
}

void UHunterAttributeSet::OnRep_StaminaCostChanges(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,StaminaCostChanges, OldAmount)
}

void UHunterAttributeSet::OnRep_AuraEffect(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,AuraEffect, OldAmount)
}

void UHunterAttributeSet::OnRep_AuraRadius(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,AuraRadius, OldAmount)
}

void UHunterAttributeSet::OnRep_ChanceToBleed(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,ChanceToBleed, OldAmount)
}

void UHunterAttributeSet::OnRep_ChanceToCorrupt(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,ChanceToCorrupt, OldAmount)
}

void UHunterAttributeSet::OnRep_ChanceToFreeze(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,ChanceToFreeze, OldAmount)
}

void UHunterAttributeSet::OnRep_ChanceToIgnite(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,ChanceToIgnite, OldAmount)
}

void UHunterAttributeSet::OnRep_ChanceToKnockBack(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,ChanceToKnockBack, OldAmount)
}

void UHunterAttributeSet::OnRep_ChanceToPetrify(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,ChanceToPetrify, OldAmount)
}

void UHunterAttributeSet::OnRep_ChanceToShock(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,ChanceToShock, OldAmount)
}

void UHunterAttributeSet::OnRep_ChanceToStun(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,ChanceToStun, OldAmount)
}

void UHunterAttributeSet::OnRep_ChanceToPurify(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,ChanceToPurify, OldAmount)
}

void UHunterAttributeSet::OnRep_BurnDuration(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,BurnDuration, OldAmount)
}

void UHunterAttributeSet::OnRep_BleedDuration(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,BleedDuration, OldAmount)
}

void UHunterAttributeSet::OnRep_FreezeDuration(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,FreezeDuration, OldAmount)
}

void UHunterAttributeSet::OnRep_CorruptionDuration(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,CorruptionDuration, OldAmount)
}

void UHunterAttributeSet::OnRep_ShockDuration(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,ShockDuration, OldAmount)
}

void UHunterAttributeSet::OnRep_PetrifyBuildUpDuration(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,PetrifyBuildUpDuration, OldAmount)
}

void UHunterAttributeSet::OnRep_PurifyDuration(const FGameplayAttributeData& OldAmount) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UHunterAttributeSet ,PurifyDuration, OldAmount)
}


// ============================================================================
// VITAL ATTRIBUTES
// ============================================================================
void UHunterAttributeSet::ClampVitalAttributes(const FGameplayAttribute& Attribute, float& NewValue) const
{
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
    // Max vitals
    else if (Attribute == GetMaxHealthAttribute() || Attribute == GetMaxManaAttribute() ||
             Attribute == GetMaxStaminaAttribute() || Attribute == GetMaxArcaneShieldAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 1.0f, 99999.0f);
    }
    // Effective max vitals
    else if (Attribute == GetMaxEffectiveHealthAttribute() || Attribute == GetMaxEffectiveManaAttribute() ||
             Attribute == GetMaxEffectiveStaminaAttribute() || Attribute == GetMaxEffectiveArcaneShieldAttribute())
    {
        NewValue = FMath::Max(NewValue, 1.0f);
    }
}

// ============================================================================
// PRIMARY ATTRIBUTES
// ============================================================================
void UHunterAttributeSet::ClampPrimaryAttributes(const FGameplayAttribute& Attribute, float& NewValue) const
{
    if (Attribute == GetStrengthAttribute() || Attribute == GetIntelligenceAttribute() || 
        Attribute == GetDexterityAttribute() || Attribute == GetEnduranceAttribute() ||
        Attribute == GetAfflictionAttribute() || Attribute == GetLuckAttribute() || 
        Attribute == GetCovenantAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.0f, 9999.0f);
    }
}

// ============================================================================
// PERCENTAGE-BASED ATTRIBUTES
// ============================================================================
void UHunterAttributeSet::ClampPercentageAttributes(const FGameplayAttribute& Attribute, float& NewValue) const
{
    // Critical Chance
    if (Attribute == GetCritChanceAttribute() || Attribute == GetSpellsCritChanceAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.0f, 100.0f);
    }
    // Critical Multipliers
    else if (Attribute == GetCritMultiplierAttribute() || Attribute == GetSpellsCritMultiplierAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 1.0f, 10.0f);
    }
    // Resistance Percentages
    else if (Attribute == GetFireResistancePercentBonusAttribute() || 
             Attribute == GetIceResistancePercentBonusAttribute() ||
             Attribute == GetLightResistancePercentBonusAttribute() ||
             Attribute == GetLightningResistancePercentBonusAttribute() ||
             Attribute == GetCorruptionResistancePercentBonusAttribute() ||
             Attribute == GetArmourPercentBonusAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.0f, 90.0f);
    }
    // Damage Percent Bonuses
    else if (Attribute == GetPhysicalPercentDamageAttribute() || 
             Attribute == GetFirePercentDamageAttribute() ||
             Attribute == GetIcePercentDamageAttribute() ||
             Attribute == GetLightPercentDamageAttribute() ||
             Attribute == GetLightningPercentDamageAttribute() ||
             Attribute == GetCorruptionPercentDamageAttribute())
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
        NewValue = FMath::Clamp(NewValue, 0.0f, 100.0f);
    }
    // Damage Conversions
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
        NewValue = FMath::Clamp(NewValue, 0.0f, 100.0f);
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
        NewValue = FMath::Clamp(NewValue, 0.0f, 100.0f);
    }
    // Leech
    else if (Attribute == GetLifeLeechAttribute() || Attribute == GetManaLeechAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.0f, 100.0f);
    }
    // Reflection Chances
    else if (Attribute == GetReflectChancePhysicalAttribute() || Attribute == GetReflectChanceElementalAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.0f, 100.0f);
    }
    else if (Attribute == GetReflectPhysicalAttribute() || Attribute == GetReflectElementalAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.0f, 300.0f);
    }
    // Block Strength
    else if (Attribute == GetBlockStrengthAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.0f, 100.0f);
    }
}

// ============================================================================
// DAMAGE ATTRIBUTES
// ============================================================================
void UHunterAttributeSet::ClampDamageAttributes(const FGameplayAttribute& Attribute, float& NewValue) const
{
    // Min/Max damage ranges with validation
    if (Attribute == GetMinPhysicalDamageAttribute() || Attribute == GetMaxPhysicalDamageAttribute() ||
        Attribute == GetMinFireDamageAttribute() || Attribute == GetMaxFireDamageAttribute() ||
        Attribute == GetMinIceDamageAttribute() || Attribute == GetMaxIceDamageAttribute() ||
        Attribute == GetMinLightDamageAttribute() || Attribute == GetMaxLightDamageAttribute() ||
        Attribute == GetMinLightningDamageAttribute() || Attribute == GetMaxLightningDamageAttribute() ||
        Attribute == GetMinCorruptionDamageAttribute() || Attribute == GetMaxCorruptionDamageAttribute())
    {
        NewValue = FMath::Max(NewValue, 0.0f);
        ValidateMinMaxDamage(Attribute, NewValue);
    }
    // Flat Damage Bonuses
    else if (Attribute == GetPhysicalFlatDamageAttribute() || Attribute == GetFireFlatDamageAttribute() ||
             Attribute == GetIceFlatDamageAttribute() || Attribute == GetLightFlatDamageAttribute() ||
             Attribute == GetLightningFlatDamageAttribute() || Attribute == GetCorruptionFlatDamageAttribute())
    {
        NewValue = FMath::Max(NewValue, 0.0f);
    }
    // Global Damage Multipliers
    else if (Attribute == GetGlobalDamagesAttribute() || Attribute == GetElementalDamageAttribute() ||
             Attribute == GetMeleeDamageAttribute() || Attribute == GetSpellDamageAttribute() ||
             Attribute == GetRangedDamageAttribute() || Attribute == GetAreaDamageAttribute() ||
             Attribute == GetDamageOverTimeAttribute() ||
             Attribute == GetDamageBonusWhileAtFullHPAttribute() || 
             Attribute == GetDamageBonusWhileAtLowHPAttribute() ||
             Attribute == GetChainDamageAttribute())
    {
        NewValue = FMath::Max(NewValue, 0.0f);
    }
}

void UHunterAttributeSet::ValidateMinMaxDamage(const FGameplayAttribute& Attribute, float& NewValue) const
{
    // Physical
    if (Attribute == GetMinPhysicalDamageAttribute())
        NewValue = FMath::Min(NewValue, GetMaxPhysicalDamage());
    else if (Attribute == GetMaxPhysicalDamageAttribute())
        NewValue = FMath::Max(NewValue, GetMinPhysicalDamage());
    // Fire
    else if (Attribute == GetMinFireDamageAttribute())
        NewValue = FMath::Min(NewValue, GetMaxFireDamage());
    else if (Attribute == GetMaxFireDamageAttribute())
        NewValue = FMath::Max(NewValue, GetMinFireDamage());
    // Ice
    else if (Attribute == GetMinIceDamageAttribute())
        NewValue = FMath::Min(NewValue, GetMaxIceDamage());
    else if (Attribute == GetMaxIceDamageAttribute())
        NewValue = FMath::Max(NewValue, GetMinIceDamage());
    // Light
    else if (Attribute == GetMinLightDamageAttribute())
        NewValue = FMath::Min(NewValue, GetMaxLightDamage());
    else if (Attribute == GetMaxLightDamageAttribute())
        NewValue = FMath::Max(NewValue, GetMinLightDamage());
    // Lightning
    else if (Attribute == GetMinLightningDamageAttribute())
        NewValue = FMath::Min(NewValue, GetMaxLightningDamage());
    else if (Attribute == GetMaxLightningDamageAttribute())
        NewValue = FMath::Max(NewValue, GetMinLightningDamage());
    // Corruption
    else if (Attribute == GetMinCorruptionDamageAttribute())
        NewValue = FMath::Min(NewValue, GetMaxCorruptionDamage());
    else if (Attribute == GetMaxCorruptionDamageAttribute())
        NewValue = FMath::Max(NewValue, GetMinCorruptionDamage());
}

// ============================================================================
// RESISTANCE ATTRIBUTES
// ============================================================================
void UHunterAttributeSet::ClampResistanceAttributes(const FGameplayAttribute& Attribute, float& NewValue) const
{
    // Flat resistances
    if (Attribute == GetArmourAttribute() || Attribute == GetArmourFlatBonusAttribute() ||
        Attribute == GetFireResistanceFlatBonusAttribute() || Attribute == GetIceResistanceFlatBonusAttribute() ||
        Attribute == GetLightResistanceFlatBonusAttribute() || Attribute == GetLightningResistanceFlatBonusAttribute() ||
        Attribute == GetCorruptionResistanceFlatBonusAttribute())
    {
        NewValue = FMath::Max(NewValue, 0.0f);
    }
    // Max resistance caps
    else if (Attribute == GetMaxFireResistanceAttribute() || Attribute == GetMaxIceResistanceAttribute() ||
             Attribute == GetMaxLightResistanceAttribute() || Attribute == GetMaxLightningResistanceAttribute() ||
             Attribute == GetMaxCorruptionResistanceAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.0f, 90.0f);
    }
    // Global defenses
    else if (Attribute == GetGlobalDefensesAttribute())
    {
        NewValue = FMath::Max(NewValue, 0.0f);
    }
}

// ============================================================================
// RATE AND AMOUNT ATTRIBUTES
// ============================================================================
void UHunterAttributeSet::ClampRateAndAmountAttributes(const FGameplayAttribute& Attribute, float& NewValue) const
{
    // Regen rates
    if (Attribute == GetHealthRegenRateAttribute() || Attribute == GetManaRegenRateAttribute() ||
        Attribute == GetStaminaRegenRateAttribute() || Attribute == GetArcaneShieldRegenRateAttribute() ||
        Attribute == GetStaminaDegenRateAttribute() ||
        Attribute == GetMaxHealthRegenRateAttribute() || Attribute == GetMaxManaRegenRateAttribute() ||
        Attribute == GetMaxStaminaRegenRateAttribute() || Attribute == GetMaxArcaneShieldRegenRateAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.1f, 60.0f);
    }
    // Regen/degen amounts
    else if (Attribute == GetHealthRegenAmountAttribute() || Attribute == GetManaRegenAmountAttribute() ||
             Attribute == GetStaminaRegenAmountAttribute() || Attribute == GetArcaneShieldRegenAmountAttribute() ||
             Attribute == GetStaminaDegenAmountAttribute() ||
             Attribute == GetMaxHealthRegenAmountAttribute() || Attribute == GetMaxManaRegenAmountAttribute() ||
             Attribute == GetMaxStaminaRegenAmountAttribute() || Attribute == GetMaxArcaneShieldRegenAmountAttribute())
    {
        NewValue = FMath::Max(NewValue, 0.0f);
    }
    // Reserved amounts
    else if (Attribute == GetFlatReservedHealthAttribute() || Attribute == GetFlatReservedManaAttribute() ||
             Attribute == GetFlatReservedStaminaAttribute() || Attribute == GetFlatReservedArcaneShieldAttribute())
    {
        NewValue = FMath::Max(NewValue, 0.0f);
    }
    // Durations
    else if (Attribute == GetBurnDurationAttribute() || Attribute == GetBleedDurationAttribute() ||
             Attribute == GetFreezeDurationAttribute() || Attribute == GetShockDurationAttribute() ||
             Attribute == GetCorruptionDurationAttribute() || Attribute == GetPetrifyBuildUpDurationAttribute() ||
             Attribute == GetPurifyDurationAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.0f, 300.0f);
    }
}

// ============================================================================
// UTILITY ATTRIBUTES
// ============================================================================
void UHunterAttributeSet::ClampUtilityAttributes(const FGameplayAttribute& Attribute, float& NewValue) const
{
    // Speeds
    if (Attribute == GetMovementSpeedAttribute() || Attribute == GetAttackSpeedAttribute() ||
        Attribute == GetCastSpeedAttribute() || Attribute == GetProjectileSpeedAttribute())
    {
        NewValue = FMath::Max(NewValue, 0.0f);
    }
    // Counts
    else if (Attribute == GetProjectileCountAttribute() || Attribute == GetChainCountAttribute() ||
             Attribute == GetForkCountAttribute() || Attribute == GetComboCounterAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.0f, 99.0f);
    }
    // On-hit gains
    else if (Attribute == GetLifeOnHitAttribute() || Attribute == GetManaOnHitAttribute() ||
             Attribute == GetStaminaOnHitAttribute())
    {
        NewValue = FMath::Max(NewValue, 0.0f);
    }
    // Cost changes (can be negative)
    else if (Attribute == GetManaCostChangesAttribute() || Attribute == GetStaminaCostChangesAttribute() ||
             Attribute == GetHealthCostChangesAttribute())
    {
        NewValue = FMath::Clamp(NewValue, -99.0f, 9000.0f);
    }
    // Ranges and areas
    else if (Attribute == GetAttackRangeAttribute() || Attribute == GetAreaOfEffectAttribute() ||
             Attribute == GetAuraRadiusAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.0f, 2000.0f);
    }
    // Cooldown reduction
    else if (Attribute == GetCooldownAttribute())
    {
        NewValue = FMath::Max(NewValue, 0.0f);
    }
    // Aura effects
    else if (Attribute == GetAuraEffectAttribute())
    {
        NewValue = FMath::Max(NewValue, 0.0f);
    }
}

// ============================================================================
// SPECIAL ATTRIBUTES
// ============================================================================
void UHunterAttributeSet::ClampSpecialAttributes(const FGameplayAttribute& Attribute, float& NewValue) const
{
    if (Attribute == GetPoiseAttribute() || Attribute == GetPoiseResistanceAttribute() ||
        Attribute == GetStunRecoveryAttribute())
    {
        NewValue = FMath::Max(NewValue, 0.0f);
    }
    else if (Attribute == GetWeightAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.0f, 999.0f);
    }
    else if (Attribute == GetGemsAttribute())
    {
        NewValue = FMath::Max(NewValue, 0.0f);
    }
}