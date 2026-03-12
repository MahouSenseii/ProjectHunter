// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "HunterAttributeSet.generated.h"


#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)


struct FGameplayAttribute;
template<class T>
using TStaticFuncPtr = TBaseStaticDelegateInstance<FGameplayAttribute(), FDefaultTSDelegateUserPolicy>::FFuncPtr;

/**
 * Main attributes set
 * (the main part of GAS any added Attributes should be added here.)
 */
UCLASS()
class ALS_PROJECTHUNTER_API UHunterAttributeSet : public UAttributeSet
{
	GENERATED_BODY()
	
public:
	
	UHunterAttributeSet();

	/**
	 * Find a FGameplayAttribute by its name
	 * Works with all 187 attributes automatically!
	 */
	UFUNCTION(BlueprintPure, Category = "Attributes")
	static FGameplayAttribute FindAttributeByName(FName AttributeName);

	/**
	 * Get all attributes in this set
	 */
	static void GetAllAttributes(TArray<FGameplayAttribute>& OutAttributes);
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/* ============================= */
	/* === Attribute Utility Functions === */
	/* ============================= */

	float GetAttributeValue(const FGameplayAttribute& Attribute) const;	
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
	static bool ShouldUpdateThresholdTags(const FGameplayAttribute& Attribute);
	TMap<FGameplayTag, TStaticFuncPtr<FGameplayAttribute()>> TagsToAttributes;

	UPROPERTY(BlueprintReadOnly, Category = "Attribute Maps")
	TMap<FGameplayTag, FGameplayTag> TagsMinMax;

	UPROPERTY(BlueprintReadOnly, Category = "Attribute Maps")
	TMap<FString, FGameplayAttribute> BaseDamageAttributesMap;
	
	UPROPERTY(BlueprintReadOnly, Category = "Attribute Maps")
	TMap<FString, FGameplayAttribute> FlatDamageAttributesMap;
	
	UPROPERTY(BlueprintReadOnly, Category = "Attribute Maps")
	TMap<FString, FGameplayAttribute> PercentDamageAttributesMap;

	UPROPERTY(BlueprintReadOnly, Category = "Attribute Maps")
	TMap<FString, FGameplayAttribute> AllAttributesMap;
 
	/* ============================= */
	/* === Primary Attributes === */
	/* ============================= */

	/** Determines the character’s strength, increasing max health and physical damage. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_Strength, Category = "Primary Attribute" , meta = (DisplayName = "Strength"))
	FGameplayAttributeData Strength;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, Strength); // +5 Max Health, +2% Physical Damage

	/** Determines the character’s intelligence, increasing mana and elemental damage. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_Intelligence, Category = "Primary Attribute", meta = (DisplayName = "Intelligence"))
	FGameplayAttributeData Intelligence;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, Intelligence); // +5 Mana, +1.3% Elemental Damage

	/** Determines agility and precision, increasing crit multiplier and attack/cast speed. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_Dexterity, Category = "Primary Attribute", meta = (DisplayName = "Dexterity"))
	FGameplayAttributeData Dexterity;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, Dexterity); // +0.5 Crit Multiplier, +0.5 Attack/Cast Speed

	/** Determines stamina and resilience, increasing stamina and resistances. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_Endurance, Category = "Primary Attribute", meta = (DisplayName = "Endurance"))
	FGameplayAttributeData Endurance;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, Endurance); // +10 Stamina, +0.01 Resistance

	/** Determines affliction-based effectiveness, increasing DOT and duration. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_Affliction, Category = "Primary Attribute" , meta = (DisplayName = "Affliction"))
	FGameplayAttributeData Affliction;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, Affliction); // +Damage Over Time, +0.05 Duration, +0.01

	/** Determines luck, affecting drop rates, and status affect applications. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_Luck, Category = "Primary Attribute", meta = (DisplayName = "Luck"))
	FGameplayAttributeData Luck;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, Luck); // +0.01 Chance to Apply, +0.02 Drop Chance

	/** Determines covenant affinity, increasing minion strength and durability. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_Covenant, Category = "Primary Attribute", meta = (DisplayName = "Covenant"))
	FGameplayAttributeData Covenant;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, Covenant); // +0.02 Minion Damage, +0.01 Minion Health

	/* === EXPERIENCE GAIN MODIFIERS === */

	/** Global XP Gain (additive from party/events) */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_GlobalXPGain, Category = "Experience", meta = (DisplayName = "Global XP Gain"))
	FGameplayAttributeData GlobalXPGain;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, GlobalXPGain);

	/** Local XP Gain (additive from equipment) */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_LocalXPGain, Category = "Experience", meta = (DisplayName = "Local XP Gain"))
	FGameplayAttributeData LocalXPGain;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, LocalXPGain);

	/** XP Gain Multiplier (multiplicative "More" bonus) */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_XPGainMultiplier, Category = "Experience", meta = (DisplayName = "XP Gain Multiplier"))
	FGameplayAttributeData XPGainMultiplier;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, XPGainMultiplier);

	/** XP Penalty (for over-leveling, death, etc.) */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_XPGainMultiplier, Category = "Experience", meta = (DisplayName = "XP Penalty"))
	FGameplayAttributeData XPPenalty;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, XPPenalty);
	
	/* ============================= */
	/* === Secondary Attributes === */
	/* ============================= */

	/** 
	 * Increases all forms of damage globally. 
	 * Applied as a multiplier to all damage types.
	 */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_GlobalDamages, Category = "Secondary Attribute|Damage", meta = (DisplayName = "GlobalDamages"))
	FGameplayAttributeData GlobalDamages;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, GlobalDamages);

	/* ========================= */
	/* === Damage Attributes === */
	/* ========================= */

	/** Physical Damage */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MinPhysicalDamage, Category = "Secondary Attribute|Damage", meta = (DisplayName = "Min Physical Damage"))
	FGameplayAttributeData MinPhysicalDamage;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, MinPhysicalDamage);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxPhysicalDamage, Category = "Secondary Attribute|Damage" , meta = (DisplayName = "Max Physical Damage"))
	FGameplayAttributeData MaxPhysicalDamage;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, MaxPhysicalDamage);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_PhysicalFlatDamage, Category = "Secondary Attribute|Damage" , meta = (DisplayName = "Physical Flat Damage"))
	FGameplayAttributeData PhysicalFlatDamage;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, PhysicalFlatDamage);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_PhysicalPercentDamage, Category = "Secondary Attribute|Damage" , meta = (DisplayName = "Physical Percent Damage"))
	FGameplayAttributeData PhysicalPercentDamage;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, PhysicalPercentDamage);

	/** Fire Damage */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MinFireDamage, Category = "Secondary Attribute|Damage", meta = (DisplayName = "Min Fire Damage "))
	FGameplayAttributeData MinFireDamage;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, MinFireDamage);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxFireDamage, Category = "Secondary Attribute|Damage", meta = (DisplayName = "Max Fire Damage "))
	FGameplayAttributeData MaxFireDamage;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, MaxFireDamage);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_FireFlatDamage, Category = "Secondary Attribute|Damage", meta = (DisplayName = "Fire Flat Damage"))
	FGameplayAttributeData FireFlatDamage;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, FireFlatDamage);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_FirePercentDamage, Category = "Secondary Attribute|Damage", meta = (DisplayName = "Fire Percent Damage"))
	FGameplayAttributeData FirePercentDamage;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, FirePercentDamage);

	/** Ice Damage */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MinIceDamage, Category = "Secondary Attribute|Damage", meta = (DisplayName = "Min Ice Damage"))
	FGameplayAttributeData MinIceDamage;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, MinIceDamage);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxIceDamage, Category = "Secondary Attribute|Damage", meta = (DisplayName = "Max Ice Damage"))
	FGameplayAttributeData MaxIceDamage;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, MaxIceDamage);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_IceFlatDamage, Category = "Secondary Attribute|Damage", meta = (DisplayName = "Ice Flat Damage"))
	FGameplayAttributeData IceFlatDamage;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, IceFlatDamage);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_IcePercentDamage, Category = "Secondary Attribute|Damage", meta = (DisplayName = "Ice Percent Damage"))
	FGameplayAttributeData IcePercentDamage;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, IcePercentDamage);

	/** Light Damage */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MinLightDamage, Category = "Secondary Attribute|Damage", meta = (DisplayName = "Min Light Damage"))
	FGameplayAttributeData MinLightDamage;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, MinLightDamage);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxLightDamage, Category = "Secondary Attribute|Damage", meta = (DisplayName = "Max Light Damage"))
	FGameplayAttributeData MaxLightDamage;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, MaxLightDamage);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_LightFlatDamage, Category = "Secondary Attribute|Damage", meta = (DisplayName = "Light Flat Damage"))
	FGameplayAttributeData LightFlatDamage;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, LightFlatDamage);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_LightPercentDamage, Category = "Secondary Attribute|Damage", meta = (DisplayName = "Light Percent Damage"))
	FGameplayAttributeData LightPercentDamage;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, LightPercentDamage);

	/** Lightning Damage */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MinLightningDamage, Category = "Secondary Attribute|Damage", meta = (DisplayName = "Min Lightning Damage"))
	FGameplayAttributeData MinLightningDamage;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, MinLightningDamage);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxLightningDamage, Category = "Secondary Attribute|Damage", meta = (DisplayName = "Max Lightning Damage"))
	FGameplayAttributeData MaxLightningDamage;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, MaxLightningDamage);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_LightningFlatDamage, Category = "Secondary Attribute|Damage", meta = (DisplayName = "Max Lightning Damage"))
	FGameplayAttributeData LightningFlatDamage;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, LightningFlatDamage);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_LightningPercentDamage, Category = "Secondary Attribute|Damage", meta = (DisplayName = "Max Percent Damage"))
	FGameplayAttributeData LightningPercentDamage;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, LightningPercentDamage);

	/** Corruption Damage */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MinCorruptionDamage, Category = "Secondary Attribute|Damage", meta = (DisplayName = "Min Corruption Damage"))
	FGameplayAttributeData MinCorruptionDamage;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, MinCorruptionDamage);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxCorruptionDamage, Category = "Secondary Attribute|Damage", meta = (DisplayName = "Max Corruption Damage"))
	FGameplayAttributeData MaxCorruptionDamage;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, MaxCorruptionDamage);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_CorruptionFlatDamage, Category = "Secondary Attribute|Damage", meta = (DisplayName = "Corruption Flat Damage"))
	FGameplayAttributeData CorruptionFlatDamage;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, CorruptionFlatDamage);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_CorruptionPercentDamage, Category = "Secondary Attribute|Damage", meta = (DisplayName = "Max Corruption Damage"))
	FGameplayAttributeData CorruptionPercentDamage;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, CorruptionPercentDamage);

	/** Special Damage Modifiers */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_DamageBonusWhileAtFullHP, Category = "Secondary Attribute|Damage", meta = (DisplayName = "Damage Bonus While At Full HP"))
	FGameplayAttributeData DamageBonusWhileAtFullHP;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, DamageBonusWhileAtFullHP);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_DamageBonusWhileAtLowHP, Category = "Secondary Attribute|Damage", meta = (DisplayName = "Damage Bonus While At Low HP"))
	FGameplayAttributeData DamageBonusWhileAtLowHP;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, DamageBonusWhileAtLowHP);



	/* ============================= */
	/* === Other Offensive Stats === */
	/* ============================= */

	/** Increases area-based damage output. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_AreaDamage, Category = "Secondary Attribute|Offensive Stats", meta = (DisplayName = "Area Damage"))
	FGameplayAttributeData AreaDamage;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, AreaDamage);

	/** Increases the radius of area-based effects (AoE). */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_AreaOfEffect, Category = "Secondary Attribute|Offensive Stats", meta = (DisplayName = "Area Of Effect"))
	FGameplayAttributeData AreaOfEffect;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, AreaOfEffect);

	/** Increases mêlée and ranged attack reach. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_AttackRange, Category = "Secondary Attribute|Offensive Stats", meta = (DisplayName = "Attack Range"))
	FGameplayAttributeData AttackRange;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, AttackRange);

	/** Increases the speed of physical mêlée attacks. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_AttackSpeed, Category = "Secondary Attribute|Offensive Stats", meta = (DisplayName = "Attack Speed"))
	FGameplayAttributeData AttackSpeed;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, AttackSpeed);

	/** Increases the speed of spell casting. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_CastSpeed, Category = "Secondary Attribute|Offensive Stats", meta = (DisplayName = "Cast Speed"))
	FGameplayAttributeData CastSpeed;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, CastSpeed);

	/** Increases chance for critical hits. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_CritChance, Category = "Secondary Attribute|Offensive Stats", meta = (DisplayName = "Crit Chance"))
	FGameplayAttributeData CritChance;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, CritChance);

	/** Increases the multiplier for critical hits. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_CritMultiplier, Category = "Secondary Attribute|Offensive Stats", meta = (DisplayName = "Crit Multiplier"))
	FGameplayAttributeData CritMultiplier;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, CritMultiplier);

	/** Increases damage-over-time effectiveness. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_DamageOverTime, Category = "Secondary Attribute|Offensive Stats", meta = (DisplayName = "Damage Over Time"))
	FGameplayAttributeData DamageOverTime;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, DamageOverTime);

	/** Increases overall elemental damage. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ElementalDamage, Category = "Secondary Attribute|Offensive Stats", meta = (DisplayName = "Elemental Damage"))
	FGameplayAttributeData ElementalDamage;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, ElementalDamage);

	/** Increases mêlée attack damage. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MeleeDamage, Category = "Secondary Attribute|Offensive Stats", meta = (DisplayName = "Melee Damage"))
	FGameplayAttributeData MeleeDamage;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, MeleeDamage);

	/** Increases Spell attack damage. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_SpellDamage, Category = "Secondary Attribute|Offensive Stats", meta = (DisplayName = "Spell Damage"))
	FGameplayAttributeData SpellDamage;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, SpellDamage);

	/** Increases the number of projectiles fired at once. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ProjectileCount, Category = "Secondary Attribute|Offensive Stats", meta = (DisplayName = "Projectile Count"))
	FGameplayAttributeData ProjectileCount;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, ProjectileCount);

	/** Increases projectile speed for ranged attacks and spells. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ProjectileSpeed, Category = "Secondary Attribute|Offensive Stats", meta = (DisplayName = "Projectile Speed"))
	FGameplayAttributeData ProjectileSpeed;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, ProjectileSpeed);

	/** Increases damage for all ranged attacks. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_RangedDamage, Category = "Secondary Attribute|Offensive Stats", meta = (DisplayName = "Ranged Damage"))
	FGameplayAttributeData RangedDamage;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, RangedDamage);

	/** Increases spell critical chance. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_SpellsCritChance, Category = "Secondary Attribute|Offensive Stats", meta = (DisplayName = "Spells Crit Chance"))
	FGameplayAttributeData SpellsCritChance;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, SpellsCritChance);

	/** Increases spell critical multiplier. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_SpellsCritMultiplier, Category = "Secondary Attribute|Offensive Stats", meta = (DisplayName = "Spells Crit Multiplier"))
	FGameplayAttributeData SpellsCritMultiplier;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, SpellsCritMultiplier);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ChainCount, Category = "Secondary Attribute|Offensive", meta = (DisplayName = "Chain Count"))
	FGameplayAttributeData ChainCount;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, ChainCount);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ForkCount, Category = "Secondary Attribute|Offensive", meta = (DisplayName = "Fork Count"))
	FGameplayAttributeData ForkCount;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, ForkCount);


	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ChainDamage, Category = "Secondary Attribute|Offensive", meta = (DisplayName = "Chain Damage"))
	FGameplayAttributeData ChainDamage;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, ChainDamage);

	/* ============================= */
	/* === Damage Conversion === */
	/* ============================= */

	/** Physical Damage Conversions */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_PhysicalToFire, Category = "Secondary Attribute|Conversion", meta = (DisplayName = "Physical To Fire"))
	FGameplayAttributeData PhysicalToFire;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, PhysicalToFire);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_PhysicalToIce, Category = "Secondary Attribute|Conversion", meta = (DisplayName = "Physical To Ice"))
	FGameplayAttributeData PhysicalToIce;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, PhysicalToIce);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_PhysicalToLightning, Category = "Secondary Attribute|Conversion", meta = (DisplayName = "Physical To Lightning"))
	FGameplayAttributeData PhysicalToLightning;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, PhysicalToLightning);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_PhysicalToLight, Category = "Secondary Attribute|Conversion", meta = (DisplayName = "Physical To Light"))
	FGameplayAttributeData PhysicalToLight;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, PhysicalToLight);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_PhysicalToCorruption, Category = "Secondary Attribute|Conversion", meta = (DisplayName = "Physical To Corruption"))
	FGameplayAttributeData PhysicalToCorruption;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, PhysicalToCorruption);

	/** Fire Damage Conversions */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_FireToPhysical, Category = "Secondary Attribute|Conversion", meta = (DisplayName = "Fire To Physical"))
	FGameplayAttributeData FireToPhysical;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, FireToPhysical);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_FireToIce, Category = "Secondary Attribute|Conversion", meta = (DisplayName = "Fire To Ice"))
	FGameplayAttributeData FireToIce;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, FireToIce);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_FireToLightning, Category = "Secondary Attribute|Conversion", meta = (DisplayName = "Fire To Lightning"))
	FGameplayAttributeData FireToLightning;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, FireToLightning);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_FireToLight, Category = "Secondary Attribute|Conversion", meta = (DisplayName = "Fire To Light"))
	FGameplayAttributeData FireToLight;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, FireToLight);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_FireToCorruption, Category = "Secondary Attribute|Conversion", meta = (DisplayName = "Fire To Corruption"))
	FGameplayAttributeData FireToCorruption;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, FireToCorruption);

	/** Ice Damage Conversions */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_IceToPhysical, Category = "Secondary Attribute|Conversion", meta = (DisplayName = "Ice To Physical"))
	FGameplayAttributeData IceToPhysical;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, IceToPhysical);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_IceToFire, Category = "Secondary Attribute|Conversion", meta = (DisplayName = "Ice To Fire"))
	FGameplayAttributeData IceToFire;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, IceToFire);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_IceToLightning, Category = "Secondary Attribute|Conversion", meta = (DisplayName = "Ice To Fire"))
	FGameplayAttributeData IceToLightning;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, IceToLightning);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_IceToLight, Category = "Secondary Attribute|Conversion", meta = (DisplayName = "Ice To Light"))
	FGameplayAttributeData IceToLight;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, IceToLight);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_IceToCorruption, Category = "Secondary Attribute|Conversion", meta = (DisplayName = "Ice To Corruption"))
	FGameplayAttributeData IceToCorruption;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, IceToCorruption);

	/** Lightning Damage Conversions */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_LightningToPhysical, Category = "Secondary Attribute|Conversion", meta = (DisplayName = "Lightning To Physical"))
	FGameplayAttributeData LightningToPhysical;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, LightningToPhysical);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_LightningToFire, Category = "Secondary Attribute|Conversion", meta = (DisplayName = "Lightning To Fire"))
	FGameplayAttributeData LightningToFire;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, LightningToFire);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_LightningToIce, Category = "Secondary Attribute|Conversion", meta = (DisplayName = "Lightning To Ice"))
	FGameplayAttributeData LightningToIce;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, LightningToIce);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_LightningToLight, Category = "Secondary Attribute|Conversion", meta = (DisplayName = "Lightning To Light"))
	FGameplayAttributeData LightningToLight;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, LightningToLight);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_LightningToCorruption, Category = "Secondary Attribute|Conversion", meta = (DisplayName = "Lightning To Corruption"))
	FGameplayAttributeData LightningToCorruption;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, LightningToCorruption);

	/** Light Damage Conversions */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_LightToPhysical, Category = "Secondary Attribute|Conversion", meta = (DisplayName = "Lightning To Physical"))
	FGameplayAttributeData LightToPhysical;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, LightToPhysical);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_LightToFire, Category = "Secondary Attribute|Conversion", meta = (DisplayName = "Light To Fire"))
	FGameplayAttributeData LightToFire;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, LightToFire);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_LightToIce, Category = "Secondary Attribute|Conversion", meta = (DisplayName = "Light To Ice"))
	FGameplayAttributeData LightToIce;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, LightToIce);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_LightToLightning, Category = "Secondary Attribute|Conversion", meta = (DisplayName = "Light To Lightning"))
	FGameplayAttributeData LightToLightning;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, LightToLightning);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_LightToCorruption, Category = "Secondary Attribute|Conversion", meta = (DisplayName = "Light To Corruption"))
	FGameplayAttributeData LightToCorruption;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, LightToCorruption);

	/** Corruption Damage Conversions */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_CorruptionToPhysical, Category = "Secondary Attribute|Conversion", meta = (DisplayName = "Corruption To Physical"))
	FGameplayAttributeData CorruptionToPhysical;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, CorruptionToPhysical);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_CorruptionToFire, Category = "Secondary Attribute|Conversion", meta = (DisplayName = "Corruption To Physical"))
	FGameplayAttributeData CorruptionToFire;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, CorruptionToFire);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_CorruptionToIce, Category = "Secondary Attribute|Conversion", meta = (DisplayName = "Corruption To Ice"))
	FGameplayAttributeData CorruptionToIce;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, CorruptionToIce);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_CorruptionToLightning, Category = "Secondary Attribute|Conversion", meta = (DisplayName = "Corruption To Lightning"))
	FGameplayAttributeData CorruptionToLightning;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, CorruptionToLightning);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_CorruptionToLight, Category = "Secondary Attribute|Conversion", meta = (DisplayName = "Corruption To Light"))
	FGameplayAttributeData CorruptionToLight;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, CorruptionToLight);

	/* =================================== */
	/* === Chance to Apply Ailments === */
	/* =================================== */
	
	/** Increases the chance to apply bleed effects. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ChanceToBleed, Category = "Secondary Attribute|Ailments", meta = (DisplayName = "Chance To Bleed"))
	FGameplayAttributeData ChanceToBleed;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, ChanceToBleed);
	
	/** Increases the chance to apply corruption effects. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ChanceToCorrupt, Category = "Secondary Attribute|Ailments", meta = (DisplayName = "Chance To Corrupt"))
	FGameplayAttributeData ChanceToCorrupt;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, ChanceToCorrupt);

	/** Increases the chance to apply freeze effects. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ChanceToFreeze, Category = "Secondary Attribute|Ailments", meta = (DisplayName = "Chance To Freeze"))
	FGameplayAttributeData ChanceToFreeze;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, ChanceToFreeze);

	/** Increases the chance to apply purification effects. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ChanceToPurify, Category = "Secondary Attribute|Ailments", meta = (DisplayName = "Chance To Purify"))
	FGameplayAttributeData ChanceToPurify;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, ChanceToPurify);

	/** Increases the chance to apply to ignite effects (burning). */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ChanceToIgnite, Category = "Secondary Attribute|Ailments", meta = (DisplayName = "Chance To Ignite"))
	FGameplayAttributeData ChanceToIgnite;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, ChanceToIgnite);

	/** Increases the chance to knock back an enemy. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ChanceToKnockBack, Category = "Secondary Attribute|Ailments", meta = (DisplayName = "Chance To Knock Back"))
	FGameplayAttributeData ChanceToKnockBack;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, ChanceToKnockBack);

	/** Increases the chance to apply to petrify effects. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ChanceToPetrify, Category = "Secondary Attribute|Ailments", meta = (DisplayName = "Chance To Petrify"))
	FGameplayAttributeData ChanceToPetrify;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, ChanceToPetrify);

	/** Increases the chance to apply shock effects. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ChanceToShock, Category = "Secondary Attribute|Ailments", meta = (DisplayName = "Chance To Shock"))
	FGameplayAttributeData ChanceToShock;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, ChanceToShock);

	/** Increases the chance to apply stun effects. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ChanceToStun, Category = "Secondary Attribute|Ailments", meta = (DisplayName = "Chance To Stun"))
	FGameplayAttributeData ChanceToStun;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, ChanceToStun);


	/* ============================= */
	/* === Duration Attributes === */
	/* ============================= */

	/** Duration of burn effects (fire damage over time). */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_BurnDuration, Category = "Secondary Attribute|Duration", meta = (DisplayName = "Burn Duration"))
	FGameplayAttributeData BurnDuration;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, BurnDuration);

	/** Duration of bleed effects (physical damage over time). */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_BleedDuration, Category = "Secondary Attribute|Duration", meta = (DisplayName = "Bleed Duration"))
	FGameplayAttributeData BleedDuration;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, BleedDuration);

	/** Duration of freeze effects (ice immobilization). */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_FreezeDuration, Category = "Secondary Attribute|Duration", meta = (DisplayName = "Freeze Duration"))
	FGameplayAttributeData FreezeDuration;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, FreezeDuration);

	/** Duration of corruption effects (dark magic damage over time). */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_CorruptionDuration, Category = "Secondary Attribute|Duration", meta = (DisplayName = "Corruption Duration"))
	FGameplayAttributeData CorruptionDuration;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, CorruptionDuration);

	/** Duration of shock effects (lightning stun and damage over time). */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ShockDuration, Category = "Secondary Attribute|Duration", meta = (DisplayName = "Shock Duration"))
	FGameplayAttributeData ShockDuration;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, ShockDuration);

	/** Build-up duration for petrification effects. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_PetrifyBuildUpDuration, Category = "Secondary Attribute|Duration", meta = (DisplayName = "Petrify Build Up Duration"))
	FGameplayAttributeData PetrifyBuildUpDuration;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, PetrifyBuildUpDuration);

	/** Duration of purification effects (removal of debuffs). */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_PurifyDuration, Category = "Secondary Attribute|Duration", meta = (DisplayName = "Purify Duration"))
	FGameplayAttributeData PurifyDuration;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, PurifyDuration);

	/* ============================= */
	/* === Resistance Attributes === */
	/* ============================= */

	/** Increases all defensive resistances. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_GlobalDefenses, Category = "Secondary Attribute|Resistances", meta = (DisplayName = "Global Defenses"))
	FGameplayAttributeData GlobalDefenses;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, GlobalDefenses);

	/** Strength of blocking incoming attacks. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_BlockStrength, Category = "Secondary Attribute|Resistances", meta = (DisplayName = "Block Strength"))
	FGameplayAttributeData BlockStrength;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, BlockStrength);

	/** Armor rating for physical damage mitigation. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_Armour, Category = "Secondary Attribute|Resistances", meta = (DisplayName = "Armour"))
	FGameplayAttributeData Armour;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, Armour);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ArmourFlatBonus, Category = "Secondary Attribute|Resistances", meta = (DisplayName = "Armour Flat Bonus"))
	FGameplayAttributeData ArmourFlatBonus;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, ArmourFlatBonus);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ArmourPercentBonus, Category = "Secondary Attribute|Resistances", meta = (DisplayName = "Armour Percent Bonus"))
	FGameplayAttributeData ArmourPercentBonus;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, ArmourPercentBonus);

	/** Corruption Resistance - Reduces corruption-based damage and curses. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_CorruptionResistanceFlatBonus, Category = "Secondary Attribute|Resistances", meta = (DisplayName = "Corruption Resistance Flat Bonus"))
	FGameplayAttributeData CorruptionResistanceFlatBonus;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, CorruptionResistanceFlatBonus);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_CorruptionResistancePercentBonus, Category = "Secondary Attribute|Resistances", meta = (DisplayName = "Corruption Resistance Percent Bonus"))
	FGameplayAttributeData CorruptionResistancePercentBonus;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, CorruptionResistancePercentBonus);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxCorruptionResistance, Category = "Secondary Attribute|Resistances", meta = (DisplayName = "Max Corruption Resistance"))
	FGameplayAttributeData MaxCorruptionResistance;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, MaxCorruptionResistance);

	/** Fire Resistance - Reduces fire damage and burn effects. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_FireResistanceFlatBonus, Category = "Secondary Attribute|Resistances", meta = (DisplayName = "Fire Resistance Flat Bonus"))
	FGameplayAttributeData FireResistanceFlatBonus;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, FireResistanceFlatBonus);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_FireResistancePercentBonus, Category = "Secondary Attribute|Resistances", meta = (DisplayName = "Fire Resistance Percent Bonus"))
	FGameplayAttributeData FireResistancePercentBonus;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, FireResistancePercentBonus);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxFireResistance, Category = "Secondary Attribute|Resistances", meta = (DisplayName = "Max Fire Resistance"))
	FGameplayAttributeData MaxFireResistance;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, MaxFireResistance);

	/** Ice Resistance - Reduces ice damage and freeze effects. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_IceResistanceFlatBonus, Category = "Secondary Attribute|Resistances", meta = (DisplayName = "Ice Resistance Flat Bonus"))
	FGameplayAttributeData IceResistanceFlatBonus;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, IceResistanceFlatBonus);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_IceResistancePercentBonus, Category = "Secondary Attribute|Resistances", meta = (DisplayName = "Max Fire Resistance"))
	FGameplayAttributeData IceResistancePercentBonus;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, IceResistancePercentBonus);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxIceResistance, Category = "Secondary Attribute|Resistances", meta = (DisplayName = "Max Ice Resistance"))
	FGameplayAttributeData MaxIceResistance;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, MaxIceResistance);

	/** Light Resistance - Reduces light damage and holy-based effects. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_LightResistanceFlatBonus, Category = "Secondary Attribute|Resistances", meta = (DisplayName = "Light Resistance Flat Bonus"))
	FGameplayAttributeData LightResistanceFlatBonus;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, LightResistanceFlatBonus);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_LightResistancePercentBonus, Category = "Secondary Attribute|Resistances", meta = (DisplayName = "Light Resistance Percent Bonus"))
	FGameplayAttributeData LightResistancePercentBonus;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, LightResistancePercentBonus);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxLightResistance, Category = "Secondary Attribute|Resistances", meta = (DisplayName = "Max Light Resistance"))
	FGameplayAttributeData MaxLightResistance;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, MaxLightResistance);

	/** Lightning Resistance - Reduces lightning damage and shock effects. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_LightningResistanceFlatBonus, Category = "Secondary Attribute|Resistances", meta = (DisplayName = "Lightning Resistance Flat Bonus"))
	FGameplayAttributeData LightningResistanceFlatBonus;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, LightningResistanceFlatBonus);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_LightningResistancePercentBonus, Category = "Secondary Attribute|Resistances", meta = (DisplayName = "Lightning Resistance Percent Bonus"))
	FGameplayAttributeData LightningResistancePercentBonus;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, LightningResistancePercentBonus);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxLightningResistance, Category = "Secondary Attribute|Resistances", meta = (DisplayName = "Max Lightning Resistance"))
	FGameplayAttributeData MaxLightningResistance;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, MaxLightningResistance);


	/*Reflection*/

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ReflectPhysical, Category = "Secondary Attribute|Reflection", meta = (DisplayName = "Reflect Physical"))
	FGameplayAttributeData ReflectPhysical;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, ReflectPhysical);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ReflectElemental, Category = "Secondary Attribute|Reflection", meta = (DisplayName = "Reflect Elemental"))
	FGameplayAttributeData ReflectElemental;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, ReflectElemental);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ReflectChancePhysical, Category = "Secondary Attribute|Reflection", meta = (DisplayName = "Reflect Chance Physical"))
	FGameplayAttributeData ReflectChancePhysical;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, ReflectChancePhysical);


	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ReflectChanceElemental, Category = "Secondary Attribute|Reflection", meta = (DisplayName = "Reflect Chance Elemental"))
	FGameplayAttributeData ReflectChanceElemental;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, ReflectChanceElemental);
	

	/* ============================= */
	/* === Piercing Attributes === */
	/* ============================= */

	/** Ignores a percentage of the target’s armor. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ArmourPiercing, Category = "Secondary Attribute|Piercing", meta = (DisplayName = "Armour Piercing"))
	FGameplayAttributeData ArmourPiercing;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, ArmourPiercing);

	/** Ignores a percentage of the target’s fire resistance. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_FirePiercing, Category = "Secondary Attribute|Piercing", meta = (DisplayName = "Fire Piercing"))
	FGameplayAttributeData FirePiercing;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, FirePiercing);

	/** Ignores a percentage of the target’s light resistance. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_LightPiercing, Category = "Secondary Attribute|Piercing", meta = (DisplayName = "Light Piercing"))
	FGameplayAttributeData LightPiercing;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, LightPiercing);

	/** Ignores a percentage of the target’s lightning resistance. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_LightningPiercing, Category = "Secondary Attribute|Piercing", meta = (DisplayName = "Lightning Piercing"))
	FGameplayAttributeData LightningPiercing;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, LightningPiercing);

	/** Ignores a percentage of the target’s corruption resistance. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_CorruptionPiercing, Category = "Secondary Attribute|Piercing", meta = (DisplayName = "Corruption Piercing"))
	FGameplayAttributeData CorruptionPiercing;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, CorruptionPiercing);

	/** Ignores a percentage of the target’s ice resistance. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_IcePiercing, Category = "Secondary Attribute|Piercing", meta = (DisplayName = "Ice Piercing"))
	FGameplayAttributeData IcePiercing;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, IcePiercing);


	/*
	* Misc Attributes
	*/

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ComboCounter, Category = "Vital Attribute|Misc", meta = (DisplayName = "Combo Counter"))
	FGameplayAttributeData ComboCounter;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, ComboCounter);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_Cooldown, Category = "Vital Attribute|Misc", meta = (DisplayName = "Cooldown"))
	FGameplayAttributeData Cooldown;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, Cooldown);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_Gems, Category = "Vital Attribute|Misc|Gems", meta = (DisplayName = "Gems"))
	FGameplayAttributeData Gems;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, Gems);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_LifeLeech, Category = "Vital Attribute|Misc", meta = (DisplayName = "Life Leech"))
	FGameplayAttributeData LifeLeech;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, LifeLeech);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ManaLeech, Category = "Vital Attribute|Misc", meta = (DisplayName = "Mana Leech"))
	FGameplayAttributeData ManaLeech;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, ManaLeech);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MovementSpeed, Category = "Vital Attribute|Misc", meta = (DisplayName = "Movement Speed"))
	FGameplayAttributeData MovementSpeed;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, MovementSpeed);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_Poise, Category = "Vital Attribute|Misc", meta = (DisplayName = "Poise"))
	FGameplayAttributeData Poise;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, Poise);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_Weight, Category = "Vital Attribute|Misc", meta = (DisplayName = "Weight"))
	FGameplayAttributeData Weight;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, Weight);
	
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_PoiseResistance, Category = "Vital Attribute|Misc", meta = (DisplayName = "Poise Resistance"))
	FGameplayAttributeData PoiseResistance;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, PoiseResistance);
	
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_StunRecovery, Category = "Vital Attribute|Misc", meta = (DisplayName = "Stun Recovery"))
	FGameplayAttributeData StunRecovery;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, StunRecovery);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ManaCostChanges, Category = "Vital Attribute|Misc", meta = (DisplayName = "Mana Cost Changes"))
	FGameplayAttributeData ManaCostChanges;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, ManaCostChanges);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_HealthCostChanges, Category = "Vital Attribute|Misc", meta = (DisplayName = "Health Cost Changes"))
	FGameplayAttributeData HealthCostChanges;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, HealthCostChanges);
	
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_LifeOnHit, Category = "Vital Attribute|Misc", meta = (DisplayName = "Life OnH it"))
	FGameplayAttributeData LifeOnHit;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, LifeOnHit);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ManaOnHit, Category = "Vital Attribute|Misc", meta = (DisplayName = "Mana On Hit"))
	FGameplayAttributeData ManaOnHit;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, ManaOnHit);
	
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_StaminaOnHit, Category = "Vital Attribute|Misc", meta = (DisplayName = "Stamina On Hit"))
	FGameplayAttributeData StaminaOnHit;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, StaminaOnHit);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_StaminaCostChanges, Category = "Vital Attribute|Misc", meta = (DisplayName = "Stamina Cost Changes"))
	FGameplayAttributeData StaminaCostChanges;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, StaminaCostChanges);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_AuraEffect, Category = "Vital Attribute|Misc", meta = (DisplayName = "Aura Effect"))
	FGameplayAttributeData AuraEffect;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, AuraEffect);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_AuraRadius, Category = "Vital Attribute|Misc", meta = (DisplayName = "Aura Radius"))
	FGameplayAttributeData AuraRadius;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, AuraRadius);
	

	/* ============================= */
	/* === Vital Attributes === */
	/* ============================= */

	/* ============================= */
	/* === Health Attributes === */
	/* ============================= */

/** Current health of the character. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_Health, Category = "Vital Attribute|Health", meta = (DisplayName = "Health"))
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, Health);

	/** Maximum health capacity. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxHealth, Category = "Vital Attribute|Health", meta = (DisplayName = "Max Health"))
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, MaxHealth);

	/** Effective max health after considering reserved health. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxEffectiveHealth, Category = "Vital Attribute|Health", meta = (DisplayName = "Max Effective Health"))
	FGameplayAttributeData MaxEffectiveHealth;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, MaxEffectiveHealth);

	/** Health regeneration rate per second. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_HealthRegenRate, Category = "Vital Attribute|Health", meta = (DisplayName = "Health Regen Rate"))
	FGameplayAttributeData HealthRegenRate;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, HealthRegenRate);

	/** Maximum rate at which health can regenerate per second. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxHealthRegenRate, Category = "Vital Attribute|Health", meta = (DisplayName = "Max Health Regen Rate"))
	FGameplayAttributeData MaxHealthRegenRate;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, MaxHealthRegenRate);

	/** Amount of health restored per tick. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_HealthRegenAmount, Category = "Vital Attribute|Health", meta = (DisplayName = "Health Regen Amount"))
	FGameplayAttributeData HealthRegenAmount;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, HealthRegenAmount);

	/** Maximum health regeneration per tick. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxHealthRegenAmount, Category = "Vital Attribute|Health", meta = (DisplayName = "Max Health Regen Amount"))
	FGameplayAttributeData MaxHealthRegenAmount;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, MaxHealthRegenAmount);

	/** Reserved health, reducing max effective health. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ReservedHealth, Category = "Vital Attribute|Health", meta = (DisplayName = "Reserved Health"))
	FGameplayAttributeData ReservedHealth;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, ReservedHealth);

	/** Maximum reserved health possible. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxReservedHealth, Category = "Vital Attribute|Health", meta = (DisplayName = "Max Reserved Health"))
	FGameplayAttributeData MaxReservedHealth;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, MaxReservedHealth);

	/** Flat amount of reserved health. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_FlatReservedHealth, Category = "Vital Attribute|Health", meta = (DisplayName = "Flat Reserved Health"))
	FGameplayAttributeData FlatReservedHealth;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, FlatReservedHealth);

	/** Percentage of max health that is reserved. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_PercentageReservedHealth, Category = "Vital Attribute|Health", meta = (DisplayName = "Percentage Reserved Health"))
	FGameplayAttributeData PercentageReservedHealth;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, PercentageReservedHealth);
	
		/* ============================= */
	/* === Stamina Attributes === */
	/* ============================= */

	/** Current stamina used for sprinting, dodging, and special actions. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_Stamina, Category = "Vital Attribute|Stamina", meta = (DisplayName = "Stamina"))
	FGameplayAttributeData Stamina;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, Stamina);

	/** Maximum stamina pool available. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxStamina, Category = "Vital Attribute|Stamina", meta = (DisplayName = "Max Stamina"))
	FGameplayAttributeData MaxStamina;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, MaxStamina);

	/** Effective maximum stamina after considering reserved stamina. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxEffectiveStamina, Category = "Vital Attribute|Stamina", meta = (DisplayName = "Max Effective Stamina"))
	FGameplayAttributeData MaxEffectiveStamina;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, MaxEffectiveStamina);

	/** Rate at which stamina regenerates per second. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_StaminaRegenRate, Category = "Vital Attribute|Stamina", meta = (DisplayName = "Stamina Regen Rate"))
	FGameplayAttributeData StaminaRegenRate;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, StaminaRegenRate);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_StaminaDegenRate, Category = "Vital Attribute|Stamina", meta = (DisplayName = "Stamina Degen Rate"))
	FGameplayAttributeData StaminaDegenRate;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, StaminaDegenRate);

	/** Maximum rate at which stamina can regenerate per second. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxStaminaRegenRate, Category = "Vital Attribute|Stamina", meta = (DisplayName = "Max Stamina Regen Rate"))
	FGameplayAttributeData MaxStaminaRegenRate;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, MaxStaminaRegenRate);

	/** Amount of stamina restored per tick. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_StaminaRegenAmount, Category = "Vital Attribute|Stamina", meta = (DisplayName = "Stamina Regen Amount"))
	FGameplayAttributeData StaminaRegenAmount;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, StaminaRegenAmount);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_StaminaDegenAmount, Category = "Vital Attribute|Stamina", meta = (DisplayName = "Stamina Degen Amount"))
	FGameplayAttributeData StaminaDegenAmount;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, StaminaDegenAmount);

	/** Maximum stamina regeneration per tick. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxStaminaRegenAmount, Category = "Vital Attribute|Stamina", meta = (DisplayName = "Max Stamina Regen Amount"))
	FGameplayAttributeData MaxStaminaRegenAmount;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, MaxStaminaRegenAmount);

	/** Reserved stamina, reducing max effective stamina. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ReservedStamina, Category = "Vital Attribute|Stamina", meta = (DisplayName = "Reserved Stamina"))
	FGameplayAttributeData ReservedStamina;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, ReservedStamina);

	/** Maximum reserved stamina possible. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxReservedStamina, Category = "Vital Attribute|Stamina", meta = (DisplayName = "Max Reserved Stamina"))
	FGameplayAttributeData MaxReservedStamina;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, MaxReservedStamina);

	/** Flat amount of stamina that is reserved. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_FlatReservedStamina, Category = "Vital Attribute|Stamina", meta = (DisplayName = "Flat Reserved Stamina"))
	FGameplayAttributeData FlatReservedStamina;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, FlatReservedStamina);

	/** Percentage of max stamina that is reserved. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_PercentageReservedStamina, Category = "Vital Attribute|Stamina", meta = (DisplayName = "Percentage Reserved Stamina"))
	FGameplayAttributeData PercentageReservedStamina;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, PercentageReservedStamina);


	/* ============================= */
	/* === Mana Attributes === */
	/* ============================= */

	/** Current mana used for casting spells. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_Mana, Category = "Vital Attribute|Mana", meta = (DisplayName = "Mana"))
	FGameplayAttributeData Mana;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, Mana);

	/** Maximum mana pool available. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxMana, Category = "Vital Attribute|Mana", meta = (DisplayName = "Max Mana"))
	FGameplayAttributeData MaxMana;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, MaxMana);

	/** Effective max mana after considering reserved mana. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxEffectiveMana, Category = "Vital Attribute|Mana", meta = (DisplayName = "Max Effective Mana"))
	FGameplayAttributeData MaxEffectiveMana;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, MaxEffectiveMana);

	/** Rate at which a mana regenerates per second. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ManaRegenRate, Category = "Vital Attribute|Mana", meta = (DisplayName = "Mana Regen Rate"))
	FGameplayAttributeData ManaRegenRate;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, ManaRegenRate);

	/** Maximum rate at which a mana can regenerate per second. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxManaRegenRate, Category = "Vital Attribute|Mana", meta = (DisplayName = "Max Mana Regen Rate"))
	FGameplayAttributeData MaxManaRegenRate;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, MaxManaRegenRate);

	/** Amount of mana restored per tick. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ManaRegenAmount, Category = "Vital Attribute|Mana", meta = (DisplayName = "Mana Regen Amount"))
	FGameplayAttributeData ManaRegenAmount;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, ManaRegenAmount);

	/** Maximum mana regeneration per tick. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxManaRegenAmount, Category = "Vital Attribute|Mana", meta = (DisplayName = "Max Mana Regen Amount"))
	FGameplayAttributeData MaxManaRegenAmount;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, MaxManaRegenAmount);

	/** Reserved mana, reducing max effective mana. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ReservedMana, Category = "Vital Attribute|Mana", meta = (DisplayName = "Reserved Mana"))
	FGameplayAttributeData ReservedMana;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, ReservedMana);

	/** Maximum reserved mana possible. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxReservedMana, Category = "Vital Attribute|Mana", meta = (DisplayName = "Max Reserved Mana"))
	FGameplayAttributeData MaxReservedMana;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, MaxReservedMana);

	/** Flat amount of reserved mana. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_FlatReservedMana, Category = "Vital Attribute|Mana", meta = (DisplayName = "Flat Reserved Mana"))
	FGameplayAttributeData FlatReservedMana;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, FlatReservedMana);

	/** Percentage of max mana that is reserved. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_PercentageReservedMana, Category = "Vital Attribute|Mana", meta = (DisplayName = "Percentage Reserved Mana"))
	FGameplayAttributeData PercentageReservedMana;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, PercentageReservedMana);

	/* ============================= */
	/* === Arcane Shield (Energy Shield) Attributes === */
	/* ============================= */

	/** Current arcane shield value (absorbs damage before health is affected). */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ArcaneShield, Category = "Vital Attribute|Arcane Shield", meta = (DisplayName = "Arcane Shield"))
	FGameplayAttributeData ArcaneShield;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, ArcaneShield);

	/** Maximum arcane shield capacity. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxArcaneShield, Category = "Vital Attribute|Arcane Shield", meta = (DisplayName = "Max Arcane Shield"))
	FGameplayAttributeData MaxArcaneShield;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, MaxArcaneShield);

	/** Effective max arcane shield after considering reserved shield. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxEffectiveArcaneShield, Category = "Vital Attribute|Arcane Shield", meta = (DisplayName = "Max Effective Arcane Shield"))
	FGameplayAttributeData MaxEffectiveArcaneShield;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, MaxEffectiveArcaneShield);

	/** Arcane shield regeneration rate per second. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ArcaneShieldRegenRate, Category = "Vital Attribute|Arcane Shield", meta = (DisplayName = "Arcane Shield Regen Rate"))
	FGameplayAttributeData ArcaneShieldRegenRate;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, ArcaneShieldRegenRate);

	/** Maximum rate at which arcane shield can regenerate per second. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxArcaneShieldRegenRate, Category = "Vital Attribute|Arcane Shield", meta = (DisplayName = "Max Arcane Shield Regen Rate"))
	FGameplayAttributeData MaxArcaneShieldRegenRate;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, MaxArcaneShieldRegenRate);

	/** Amount of arcane shield restored per tick. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ArcaneShieldRegenAmount, Category = "Vital Attribute|Arcane Shield", meta = (DisplayName = "Arcane Shield Regen Amount"))
	FGameplayAttributeData ArcaneShieldRegenAmount;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, ArcaneShieldRegenAmount);

	/** Maximum arcane shield regeneration per tick. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxArcaneShieldRegenAmount, Category = "Vital Attribute|Arcane Shield", meta = (DisplayName = "Max Arcane Shield Regen Amount"))
	FGameplayAttributeData MaxArcaneShieldRegenAmount;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, MaxArcaneShieldRegenAmount);

	/** Reserved arcane shield, reducing max effective shield. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ReservedArcaneShield, Category = "Vital Attribute|Arcane Shield", meta = (DisplayName = "Reserved Arcane Shield"))
	FGameplayAttributeData ReservedArcaneShield;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, ReservedArcaneShield);

	/** Maximum reserved arcane shield possible. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxReservedArcaneShield, Category = "Vital Attribute|Arcane Shield", meta = (DisplayName = "Max Reserved Arcane Shield"))
	FGameplayAttributeData MaxReservedArcaneShield;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, MaxReservedArcaneShield);

	/** Flat amount of reserved arcane shield. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_FlatReservedArcaneShield, Category = "Vital Attribute|Arcane Shield", meta = (DisplayName = "Flat Reserved Arcane Shield"))
	FGameplayAttributeData FlatReservedArcaneShield;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, FlatReservedArcaneShield);

	/** Percentage of max arcane shield that is reserved. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_PercentageReservedArcaneShield, Category = "Vital Attribute|Arcane Shield", meta = (DisplayName = "Percentage Reserved Arcane Shield"))
	FGameplayAttributeData PercentageReservedArcaneShield;
	ATTRIBUTE_ACCESSORS(UHunterAttributeSet, PercentageReservedArcaneShield);


protected:

	/* ============================= */
	/* === Health Replication Functions === */
	/* ============================= */

	/** Called when health value changes. */
	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldHealth) const;

	/** Called when max health changes. */
	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth) const;

	/** Called when max effective health (after reserves) changes. */
	UFUNCTION()
	void OnRep_MaxEffectiveHealth(const FGameplayAttributeData& OldAmount) const;

	/** Called when health regeneration rate changes. */
	UFUNCTION()
	void OnRep_HealthRegenRate(const FGameplayAttributeData& OldAmount) const;

	/** Called when max health regeneration rate changes. */
	UFUNCTION()
	void OnRep_MaxHealthRegenRate(const FGameplayAttributeData& OldAmount) const;

	/** Called when health regeneration amount changes. */
	UFUNCTION()
	void OnRep_HealthRegenAmount(const FGameplayAttributeData& OldAmount) const;

	/** Called when max health regeneration amount changes. */
	UFUNCTION()
	void OnRep_MaxHealthRegenAmount(const FGameplayAttributeData& OldAmount) const;

	/** Called when reserved health amount changes. */
	UFUNCTION()
	void OnRep_ReservedHealth(const FGameplayAttributeData& OldAmount) const;

	/** Called when max reserved health changes. */
	UFUNCTION()
	void OnRep_MaxReservedHealth(const FGameplayAttributeData& OldAmount) const;

	/** Called when flat reserved health changes. */
	UFUNCTION()
	void OnRep_FlatReservedHealth(const FGameplayAttributeData& OldAmount) const;

	/** Called when percentage reserved health changes. */
	UFUNCTION()
	void OnRep_PercentageReservedHealth(const FGameplayAttributeData& OldAmount) const;

	/* ============================= */
	/* === Stamina Replication Functions === */
	/* ============================= */

	/** Called when stamina value changes. */
	UFUNCTION()
	void OnRep_Stamina(const FGameplayAttributeData& OldStamina) const;

	/** Called when max stamina changes. */
	UFUNCTION()
	void OnRep_MaxStamina(const FGameplayAttributeData& OldMaxStamina) const;

	/** Called when max effective stamina (after reserves) changes. */
	UFUNCTION()
	void OnRep_MaxEffectiveStamina(const FGameplayAttributeData& OldAmount) const;

	/** Called when stamina regeneration rate changes. */
	UFUNCTION()
	void OnRep_StaminaRegenRate(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_StaminaDegenRate(const FGameplayAttributeData& OldAmount) const;

	/** Called when max stamina regeneration rate changes. */
	UFUNCTION()
	void OnRep_MaxStaminaRegenRate(const FGameplayAttributeData& OldAmount) const;

	/** Called when stamina regeneration amount changes. */
	UFUNCTION()
	void OnRep_StaminaRegenAmount(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_StaminaDegenAmount(const FGameplayAttributeData& OldAmount) const;

	/** Called when max stamina regeneration amount changes. */
	UFUNCTION()
	void OnRep_MaxStaminaRegenAmount(const FGameplayAttributeData& OldAmount) const;

	/** Called when reserved stamina amount changes. */
	UFUNCTION()
	void OnRep_ReservedStamina(const FGameplayAttributeData& OldAmount) const;

	/** Called when max reserved stamina changes. */
	UFUNCTION()
	void OnRep_MaxReservedStamina(const FGameplayAttributeData& OldAmount) const;

	/** Called when flat reserved stamina changes. */
	UFUNCTION()
	void OnRep_FlatReservedStamina(const FGameplayAttributeData& OldAmount) const;

	/** Called when percentage reserved stamina changes. */
	UFUNCTION()
	void OnRep_PercentageReservedStamina(const FGameplayAttributeData& OldAmount) const;

	
	/* ============================= */
	/* === Mana Replication Functions === */
	/* ============================= */

	/** Called when mana value changes. */
	UFUNCTION()
	void OnRep_Mana(const FGameplayAttributeData& OldAmount) const;

	/** Called when max mana changes. */
	UFUNCTION()
	void OnRep_MaxMana(const FGameplayAttributeData& OldAmount) const;

	/** Called when max effective mana (after reserves) changes. */
	UFUNCTION()
	void OnRep_MaxEffectiveMana(const FGameplayAttributeData& OldAmount) const;

	/** Called when mana regeneration rate changes. */
	UFUNCTION()
	void OnRep_ManaRegenRate(const FGameplayAttributeData& OldAmount) const;

	/** Called when max mana regeneration rate changes. */
	UFUNCTION()
	void OnRep_MaxManaRegenRate(const FGameplayAttributeData& OldAmount) const;

	/** Called when mana regeneration amount changes. */
	UFUNCTION()
	void OnRep_ManaRegenAmount(const FGameplayAttributeData& OldAmount) const;

	/** Called when max mana regeneration amount changes. */
	UFUNCTION()
	void OnRep_MaxManaRegenAmount(const FGameplayAttributeData& OldAmount) const;

	/** Called when reserved mana amount changes. */
	UFUNCTION()
	void OnRep_ReservedMana(const FGameplayAttributeData& OldAmount) const;

	/** Called when max reserved mana changes. */
	UFUNCTION()
	void OnRep_MaxReservedMana(const FGameplayAttributeData& OldAmount) const;

	/** Called when flat reserved mana changes. */
	UFUNCTION()
	void OnRep_FlatReservedMana(const FGameplayAttributeData& OldAmount) const;

	/** Called when percentage reserved mana changes. */
	UFUNCTION()
	void OnRep_PercentageReservedMana(const FGameplayAttributeData& OldAmount) const;

	/* ============================= */
	/* === Arcane Shield (Energy Shield) Replication === */
	/* ============================= */

	/** Called when arcane shield value changes. */
	UFUNCTION()
	void OnRep_ArcaneShield(const FGameplayAttributeData& OldAmount) const;

	/** Called when max arcane shield changes. */
	UFUNCTION()
	void OnRep_MaxArcaneShield(const FGameplayAttributeData& OldAmount) const;

	/** Called when max effective arcane shield (after reserves) changes. */
	UFUNCTION()
	void OnRep_MaxEffectiveArcaneShield(const FGameplayAttributeData& OldAmount) const;

	/** Called when arcane shield regeneration rate changes. */
	UFUNCTION()
	void OnRep_ArcaneShieldRegenRate(const FGameplayAttributeData& OldAmount) const;

	/** Called when max arcane shield regeneration rate changes. */
	UFUNCTION()
	void OnRep_MaxArcaneShieldRegenRate(const FGameplayAttributeData& OldAmount) const;

	/** Called when arcane shield regeneration amount changes. */
	UFUNCTION()
	void OnRep_ArcaneShieldRegenAmount(const FGameplayAttributeData& OldAmount) const;

	/** Called when max arcane shield regeneration amount changes. */
	UFUNCTION()
	void OnRep_MaxArcaneShieldRegenAmount(const FGameplayAttributeData& OldAmount) const;

	/** Called when reserved arcane shield amount changes. */
	UFUNCTION()
	void OnRep_ReservedArcaneShield(const FGameplayAttributeData& OldAmount) const;

	/** Called when max reserved arcane shield changes. */
	UFUNCTION()
	void OnRep_MaxReservedArcaneShield(const FGameplayAttributeData& OldAmount) const;

	/** Called when flat reserved arcane shield changes. */
	UFUNCTION()
	void OnRep_FlatReservedArcaneShield(const FGameplayAttributeData& OldAmount) const;

	/** Called when percentage reserved arcane shield changes. */
	UFUNCTION()
	void OnRep_PercentageReservedArcaneShield(const FGameplayAttributeData& OldAmount) const;

	/* ============================= */
	/* === Primary Attributes Replication === */
	/* ============================= */

	/** Called when gems currency changes. */
	UFUNCTION()
	void OnRep_Gems(const FGameplayAttributeData& OldAmount) const;

	/** Called when strength attribute changes. */
	UFUNCTION()
	void OnRep_Strength(const FGameplayAttributeData& OldAmount) const;

	/** Called when intelligence attribute changes. */
	UFUNCTION()
	void OnRep_Intelligence(const FGameplayAttributeData& OldAmount) const;

	/** Called when dexterity attribute changes. */
	UFUNCTION()
	void OnRep_Dexterity(const FGameplayAttributeData& OldAmount) const;

	/** Called when endurance attribute changes. */
	UFUNCTION()
	void OnRep_Endurance(const FGameplayAttributeData& OldAmount) const;

	/** Called when affliction attribute changes. */
	UFUNCTION()
	void OnRep_Affliction(const FGameplayAttributeData& OldAmount) const;

	/** Called when luck attribute changes. */
	UFUNCTION()
	void OnRep_Luck(const FGameplayAttributeData& OldAmount) const;

	/** Called when covenant attribute changes. */
	UFUNCTION()
	void OnRep_Covenant(const FGameplayAttributeData& OldAmount) const;


	/* ==============================================*/
	/* ===  EXPERIENCE GAIN MODIFIERS Functions === */
	/* ============================================ */
	
	UFUNCTION()
	void OnRep_GlobalXPGain(const FGameplayAttributeData& OldValue) const;
	
	UFUNCTION()
	void OnRep_LocalXPGain(const FGameplayAttributeData& OldValue) const;

	
	UFUNCTION()
	void OnRep_XPGainMultiplier(const FGameplayAttributeData& OldValue) const;

	
	UFUNCTION()
	void OnRep_XPPenalty(const FGameplayAttributeData& OldValue) const;


	/* ===================================== */
	/* === Damage Replication Functions === */
	/* =================================== */

	/** Called when global damage Damage changes. */
	UFUNCTION()
	void OnRep_GlobalDamages(const FGameplayAttributeData& OldAmount) const;

	/** Corruption Damage */
	UFUNCTION()
	void OnRep_MinCorruptionDamage(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_MaxCorruptionDamage(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_CorruptionFlatDamage(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_CorruptionPercentDamage(const FGameplayAttributeData& OldAmount) const;

	/** Fire Damage */
	UFUNCTION()
	void OnRep_MinFireDamage(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_MaxFireDamage(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_FireFlatDamage(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_FirePercentDamage(const FGameplayAttributeData& OldAmount) const;

	/** Ice Damage */
	UFUNCTION()
	void OnRep_MinIceDamage(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_MaxIceDamage(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_IceFlatDamage(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_IcePercentDamage(const FGameplayAttributeData& OldAmount) const;

	/** Light Damage */
	UFUNCTION()
	void OnRep_MinLightDamage(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_MaxLightDamage(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_LightFlatDamage(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_LightPercentDamage(const FGameplayAttributeData& OldAmount) const;


	/** Reflection */
	UFUNCTION()
	void OnRep_ReflectPhysical(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_ReflectElemental(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_ReflectChancePhysical(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_ReflectChanceElemental(const FGameplayAttributeData& OldAmount) const;
	

	/** Lightning Damage */
	UFUNCTION()
	void OnRep_MinLightningDamage(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_MaxLightningDamage(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_LightningFlatDamage(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_LightningPercentDamage(const FGameplayAttributeData& OldAmount) const;

	/** Physical Damage */
	UFUNCTION()
	void OnRep_MinPhysicalDamage(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_MaxPhysicalDamage(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_PhysicalFlatDamage(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_PhysicalPercentDamage(const FGameplayAttributeData& OldAmount) const;


	/* ============================= */
/* === Damage Conversion OnRep Functions === */
/* ============================= */

/** Physical Damage Conversion OnRep Functions */
UFUNCTION()
void OnRep_PhysicalToFire(const FGameplayAttributeData& OldAmount) const;

UFUNCTION()
void OnRep_PhysicalToIce(const FGameplayAttributeData& OldAmount) const;

UFUNCTION()
void OnRep_PhysicalToLightning(const FGameplayAttributeData& OldAmount) const;

UFUNCTION()
void OnRep_PhysicalToLight(const FGameplayAttributeData& OldAmount) const;

UFUNCTION()
void OnRep_PhysicalToCorruption(const FGameplayAttributeData& OldAmount) const;

/** Fire Damage Conversion OnRep Functions */
UFUNCTION()
void OnRep_FireToPhysical(const FGameplayAttributeData& OldAmount) const;

UFUNCTION()
void OnRep_FireToIce(const FGameplayAttributeData& OldAmount) const;

UFUNCTION()
void OnRep_FireToLightning(const FGameplayAttributeData& OldAmount) const;

UFUNCTION()
void OnRep_FireToLight(const FGameplayAttributeData& OldAmount) const;

UFUNCTION()
void OnRep_FireToCorruption(const FGameplayAttributeData& OldAmount) const;

/** Ice Damage Conversion OnRep Functions */
UFUNCTION()
void OnRep_IceToPhysical(const FGameplayAttributeData& OldAmount) const;

UFUNCTION()
void OnRep_IceToFire(const FGameplayAttributeData& OldAmount) const;

UFUNCTION()
void OnRep_IceToLightning(const FGameplayAttributeData& OldAmount) const;

UFUNCTION()
void OnRep_IceToLight(const FGameplayAttributeData& OldAmount) const;

UFUNCTION()
void OnRep_IceToCorruption(const FGameplayAttributeData& OldAmount) const;

/** Lightning Damage Conversion OnRep Functions */
UFUNCTION()
void OnRep_LightningToPhysical(const FGameplayAttributeData& OldAmount) const;

UFUNCTION()
void OnRep_LightningToFire(const FGameplayAttributeData& OldAmount) const;

UFUNCTION()
void OnRep_LightningToIce(const FGameplayAttributeData& OldAmount) const;

UFUNCTION()
void OnRep_LightningToLight(const FGameplayAttributeData& OldAmount) const;

UFUNCTION()
void OnRep_LightningToCorruption(const FGameplayAttributeData& OldAmount) const;

/** Light Damage Conversion OnRep Functions */
UFUNCTION()
void OnRep_LightToPhysical(const FGameplayAttributeData& OldAmount) const;

UFUNCTION()
void OnRep_LightToFire(const FGameplayAttributeData& OldAmount) const;

UFUNCTION()
void OnRep_LightToIce(const FGameplayAttributeData& OldAmount) const;

UFUNCTION()
void OnRep_LightToLightning(const FGameplayAttributeData& OldAmount) const;

UFUNCTION()
void OnRep_LightToCorruption(const FGameplayAttributeData& OldAmount) const;

/** Corruption Damage Conversion OnRep Functions */
UFUNCTION()
void OnRep_CorruptionToPhysical(const FGameplayAttributeData& OldAmount) const;

UFUNCTION()
void OnRep_CorruptionToFire(const FGameplayAttributeData& OldAmount) const;

UFUNCTION()
void OnRep_CorruptionToIce(const FGameplayAttributeData& OldAmount) const;

UFUNCTION()
void OnRep_CorruptionToLightning(const FGameplayAttributeData& OldAmount) const;

UFUNCTION()
void OnRep_CorruptionToLight(const FGameplayAttributeData& OldAmount) const;

	/* ============================= */
	/* === Offensive Stats Replication Functions === */
	/* ============================= */

	UFUNCTION()
	void OnRep_AreaDamage(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_AreaOfEffect(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_AttackRange(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_AttackSpeed(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_CastSpeed(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_CritChance(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_CritMultiplier(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_DamageOverTime(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_ElementalDamage(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_SpellsCritChance(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_SpellsCritMultiplier(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_DamageBonusWhileAtFullHP(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_DamageBonusWhileAtLowHP(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_MeleeDamage(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_SpellDamage(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_ProjectileCount(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_ProjectileSpeed(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_RangedDamage(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_ChainCount(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_ForkCount(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_ChainDamage(const FGameplayAttributeData& OldAmount) const;

	/* ============================= */
	/* === Resistance Replication Functions === */
	/* ============================= */

	/** Called when global defenses change. */
	UFUNCTION()
	void OnRep_GlobalDefenses(const FGameplayAttributeData& OldAmount) const;

	/** Armour */
	UFUNCTION()
	void OnRep_Armour(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_ArmourFlatBonus(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_ArmourPercentBonus(const FGameplayAttributeData& OldAmount) const;

	/** Corruption Resistance */
	UFUNCTION()
	void OnRep_CorruptionResistanceFlatBonus(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_CorruptionResistancePercentBonus(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_MaxCorruptionResistance(const FGameplayAttributeData& OldAmount) const;

	/** Fire Resistance */
	UFUNCTION()
	void OnRep_FireResistanceFlatBonus(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_FireResistancePercentBonus(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_MaxFireResistance(const FGameplayAttributeData& OldAmount) const;

	/** Ice Resistance */
	UFUNCTION()
	void OnRep_IceResistanceFlatBonus(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_IceResistancePercentBonus(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_MaxIceResistance(const FGameplayAttributeData& OldAmount) const;

	/** Light Resistance */
	UFUNCTION()
	void OnRep_LightResistanceFlatBonus(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_LightResistancePercentBonus(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_MaxLightResistance(const FGameplayAttributeData& OldAmount) const;

	/** Lightning Resistance */
	UFUNCTION()
	void OnRep_LightningResistanceFlatBonus(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_LightningResistancePercentBonus(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_MaxLightningResistance(const FGameplayAttributeData& OldAmount) const;

	/** Block Strength */
	UFUNCTION()
	void OnRep_BlockStrength(const FGameplayAttributeData& OldAmount) const;

	
	/* ============================= */
	/* === Piercing Replication Functions === */
	/* ============================= */

	UFUNCTION()
	void OnRep_ArmourPiercing(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_FirePiercing(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_LightPiercing(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_LightningPiercing(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_CorruptionPiercing(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_IcePiercing(const FGameplayAttributeData& OldAmount) const;

	/* ============================= */
	/* === Miscellaneous Attributes Replication === */
	/* ============================= */

	UFUNCTION()
	void OnRep_ComboCounter(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_Cooldown(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_LifeLeech(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_ManaLeech(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_MovementSpeed(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_Poise(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_Weight(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_PoiseResistance(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_StunRecovery(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_ManaCostChanges(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_HealthCostChanges(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_LifeOnHit(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_ManaOnHit(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_StaminaOnHit(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_StaminaCostChanges(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_AuraEffect(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_AuraRadius(const FGameplayAttributeData& OldAmount) const;

	/* ============================= */
	/* === Chance To Apply Ailments Replication === */
	/* ============================= */

	UFUNCTION()
	void OnRep_ChanceToBleed(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_ChanceToCorrupt(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_ChanceToFreeze(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_ChanceToIgnite(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_ChanceToKnockBack(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_ChanceToPetrify(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_ChanceToShock(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_ChanceToStun(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_ChanceToPurify(const FGameplayAttributeData& OldAmount) const;

	/* ============================= */
	/* === Status Effect Duration Replication === */
	/* ============================= */

	UFUNCTION()
	void OnRep_BurnDuration(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_BleedDuration(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_FreezeDuration(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_CorruptionDuration(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_ShockDuration(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_PetrifyBuildUpDuration(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_PurifyDuration(const FGameplayAttributeData& OldAmount) const;
	
private:
	// Validation helper functions
	void ClampVitalAttributes(const FGameplayAttribute& Attribute, float& NewValue) const;
	void ClampPrimaryAttributes(const FGameplayAttribute& Attribute, float& NewValue) const;
	void ClampPercentageAttributes(const FGameplayAttribute& Attribute, float& NewValue) const;
	void ClampDamageAttributes(const FGameplayAttribute& Attribute, float& NewValue) const;
	void ClampResistanceAttributes(const FGameplayAttribute& Attribute, float& NewValue) const;
	void ClampRateAndAmountAttributes(const FGameplayAttribute& Attribute, float& NewValue) const;
	void ClampUtilityAttributes(const FGameplayAttribute& Attribute, float& NewValue) const;
	void ClampSpecialAttributes(const FGameplayAttribute& Attribute, float& NewValue) const;
    
	// Helper for min/max damage validation
	void ValidateMinMaxDamage(const FGameplayAttribute& Attribute, float& NewValue) const;
};
