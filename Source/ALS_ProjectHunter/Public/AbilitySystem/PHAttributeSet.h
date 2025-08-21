// Copyright@2024 Quentin Davis

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectExtension.h"
#include "Library/AttributeStructsLibrary.h"
#include "Library/PHCharacterEnumLibrary.h"
#include "PHAttributeSet.generated.h"


	#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAttributeChangeSignature, float, NewValue);

template<class T>
using TStaticFuncPtr = TBaseStaticDelegateInstance<FGameplayAttribute(), FDefaultTSDelegateUserPolicy>::FFuncPtr;

/**
 * Represents an attribute set defining various character traits and attributes.
 * This includes primary attributes (e.g., Strength, Dexterity) and secondary attributes (e.g., specific damage types).
 */
UCLASS()
class ALS_PROJECTHUNTER_API UPHAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	
	UPHAttributeSet();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/* ============================= */
	/* === Attribute Utility Functions === */
	/* ============================= */

        float GetAttributeValue(const FGameplayAttribute& Attribute) const;

	UFUNCTION(BlueprintCallable, Category = "Combat Alignment")
	ECombatAlignment GetCombatAlignmentBP() const;

	UFUNCTION(BlueprintCallable, Category = "Combat Alignment")
	void SetCombatAlignmentBP(ECombatAlignment NewAlignment);

	UPROPERTY(BlueprintAssignable, Category = "Combat Alignment")
	FOnAttributeChangeSignature OnCombatAlignmentChange;
	
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	static void ClampResource(const TFunctionRef<float()>& Getter, const TFunctionRef<float()>& MaxGetter, const TFunctionRef<void(float)>& Setter);
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
	static bool ShouldUpdateThresholdTags(const FGameplayAttribute& Attribute);


	TMap<FGameplayTag, TStaticFuncPtr<FGameplayAttribute()>> TagsToAttributes;

	UPROPERTY(BlueprintReadOnly)
	TMap<FGameplayTag, FGameplayTag> TagsMinMax;

	UPROPERTY(BlueprintReadOnly)
	TMap<FString, FGameplayAttribute> BaseDamageAttributesMap;
	
	UPROPERTY(BlueprintReadOnly)
	TMap<FString, FGameplayAttribute> FlatDamageAttributesMap;
	
	UPROPERTY(BlueprintReadOnly)
	TMap<FString, FGameplayAttribute> PercentDamageAttributesMap;

	UPROPERTY(BlueprintReadOnly)
	TMap<FString, FGameplayAttribute> AllAttributesMap;

    /*
    *Combat Indicators
    */

	UPROPERTY(BlueprintReadOnly, Category = "Combat Alignment", ReplicatedUsing = OnRep_CombatAlignment)
	FGameplayAttributeData CombatAlignment;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, CombatAlignment)
 
	/* ============================= */
	/* === Primary Attributes === */
	/* ============================= */

	/** Determines the character’s strength, increasing max health and physical damage. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_Strength, Category = "Primary Attribute")
	FGameplayAttributeData Strength;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, Strength); // +5 Max Health, +2% Physical Damage

	/** Determines the character’s intelligence, increasing mana and elemental damage. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_Intelligence, Category = "Primary Attribute")
	FGameplayAttributeData Intelligence;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, Intelligence); // +5 Mana, +1.3% Elemental Damage

	/** Determines agility and precision, increasing crit multiplier and attack/cast speed. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_Dexterity, Category = "Primary Attribute")
	FGameplayAttributeData Dexterity;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, Dexterity); // +0.5 Crit Multiplier, +0.5 Attack/Cast Speed

	/** Determines stamina and resilience, increasing stamina and resistances. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_Endurance, Category = "Primary Attribute")
	FGameplayAttributeData Endurance;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, Endurance); // +10 Stamina, +0.01 Resistance

	/** Determines affliction-based effectiveness, increasing DOT and duration. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_Affliction, Category = "Primary Attribute")
	FGameplayAttributeData Affliction;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, Affliction); // +Damage Over Time, +0.05 Duration, +0.01

	/** Determines luck, affecting drop rates and status effect applications. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_Luck, Category = "Primary Attribute")
	FGameplayAttributeData Luck;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, Luck); // +0.01 Chance to Apply, +0.02 Drop Chance

	/** Determines covenant affinity, increasing minion strength and durability. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_Covenant, Category = "Primary Attribute")
	FGameplayAttributeData Covenant;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, Covenant); // +0.02 Minion Damage, +0.01 Minion Health
	
	/* ============================= */
	/* === Secondary Attributes === */
	/* ============================= */

	/** 
	 * Increases all forms of damage globally. 
	 * Applied as a multiplier to all damage types.
	 */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_GlobalDamages, Category = "Secondary Attribute|Damage")
	FGameplayAttributeData GlobalDamages;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, GlobalDamages);

	/* ========================= */
	/* === Damage Attributes === */
	/* ========================= */

	/** Physical Damage */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MinPhysicalDamage, Category = "Secondary Attribute|Damage")
	FGameplayAttributeData MinPhysicalDamage;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MinPhysicalDamage);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxPhysicalDamage, Category = "Secondary Attribute|Damage")
	FGameplayAttributeData MaxPhysicalDamage;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MaxPhysicalDamage);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_PhysicalFlatBonus, Category = "Secondary Attribute|Damage")
	FGameplayAttributeData PhysicalFlatBonus;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, PhysicalFlatBonus);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_PhysicalPercentBonus, Category = "Secondary Attribute|Damage")
	FGameplayAttributeData PhysicalPercentBonus;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, PhysicalPercentBonus);

	/** Fire Damage */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MinFireDamage, Category = "Secondary Attribute|Damage")
	FGameplayAttributeData MinFireDamage;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MinFireDamage);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxFireDamage, Category = "Secondary Attribute|Damage")
	FGameplayAttributeData MaxFireDamage;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MaxFireDamage);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_FireFlatBonus, Category = "Secondary Attribute|Damage")
	FGameplayAttributeData FireFlatBonus;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, FireFlatBonus);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_FirePercentBonus, Category = "Secondary Attribute|Damage")
	FGameplayAttributeData FirePercentBonus;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, FirePercentBonus);

	/** Ice Damage */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MinIceDamage, Category = "Secondary Attribute|Damage")
	FGameplayAttributeData MinIceDamage;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MinIceDamage);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxIceDamage, Category = "Secondary Attribute|Damage")
	FGameplayAttributeData MaxIceDamage;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MaxIceDamage);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_IceFlatBonus, Category = "Secondary Attribute|Damage")
	FGameplayAttributeData IceFlatBonus;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, IceFlatBonus);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_IcePercentBonus, Category = "Secondary Attribute|Damage")
	FGameplayAttributeData IcePercentBonus;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, IcePercentBonus);

	/** Light Damage */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MinLightDamage, Category = "Secondary Attribute|Damage")
	FGameplayAttributeData MinLightDamage;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MinLightDamage);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxLightDamage, Category = "Secondary Attribute|Damage")
	FGameplayAttributeData MaxLightDamage;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MaxLightDamage);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_LightFlatBonus, Category = "Secondary Attribute|Damage")
	FGameplayAttributeData LightFlatBonus;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, LightFlatBonus);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_LightPercentBonus, Category = "Secondary Attribute|Damage")
	FGameplayAttributeData LightPercentBonus;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, LightPercentBonus);

	/** Lightning Damage */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MinLightningDamage, Category = "Secondary Attribute|Damage")
	FGameplayAttributeData MinLightningDamage;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MinLightningDamage);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxLightningDamage, Category = "Secondary Attribute|Damage")
	FGameplayAttributeData MaxLightningDamage;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MaxLightningDamage);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_LightningFlatBonus, Category = "Secondary Attribute|Damage")
	FGameplayAttributeData LightningFlatBonus;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, LightningFlatBonus);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_LightningPercentBonus, Category = "Secondary Attribute|Damage")
	FGameplayAttributeData LightningPercentBonus;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, LightningPercentBonus);

	/** Corruption Damage */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MinCorruptionDamage, Category = "Secondary Attribute|Damage")
	FGameplayAttributeData MinCorruptionDamage;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MinCorruptionDamage);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxCorruptionDamage, Category = "Secondary Attribute|Damage")
	FGameplayAttributeData MaxCorruptionDamage;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MaxCorruptionDamage);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_CorruptionFlatBonus, Category = "Secondary Attribute|Damage")
	FGameplayAttributeData CorruptionFlatBonus;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, CorruptionFlatBonus);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_CorruptionPercentBonus, Category = "Secondary Attribute|Damage")
	FGameplayAttributeData CorruptionPercentBonus;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, CorruptionPercentBonus);

	/** Special Damage Modifiers */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_DamageBonusWhileAtFullHP, Category = "Secondary Attribute|Damage")
	FGameplayAttributeData DamageBonusWhileAtFullHP;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, DamageBonusWhileAtFullHP);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_DamageBonusWhileAtLowHP, Category = "Secondary Attribute|Damage")
	FGameplayAttributeData DamageBonusWhileAtLowHP;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, DamageBonusWhileAtLowHP);



		/* ============================= */
	/* === Other Offensive Stats === */
	/* ============================= */

	/** Increases area-based damage output. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_AreaDamage, Category = "Secondary Attribute|Offensive Stats")
	FGameplayAttributeData AreaDamage;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, AreaDamage);

	/** Increases the radius of area-based effects (AoE). */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_AreaOfEffect, Category = "Secondary Attribute|Offensive Stats")
	FGameplayAttributeData AreaOfEffect;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, AreaOfEffect);

	/** Increases melee and ranged attack reach. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_AttackRange, Category = "Secondary Attribute|Offensive Stats")
	FGameplayAttributeData AttackRange;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, AttackRange);

	/** Increases the speed of physical melee attacks. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_AttackSpeed, Category = "Secondary Attribute|Offensive Stats")
	FGameplayAttributeData AttackSpeed;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, AttackSpeed);

	/** Increases the speed of spell casting. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_CastSpeed, Category = "Secondary Attribute|Offensive Stats")
	FGameplayAttributeData CastSpeed;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, CastSpeed);

	/** Increases chance for critical hits. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_CritChance, Category = "Secondary Attribute|Offensive Stats")
	FGameplayAttributeData CritChance;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, CritChance);

	/** Increases the multiplier for critical hits. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_CritMultiplier, Category = "Secondary Attribute|Offensive Stats")
	FGameplayAttributeData CritMultiplier;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, CritMultiplier);

	/** Increases damage-over-time effectiveness. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_DamageOverTime, Category = "Secondary Attribute|Offensive Stats")
	FGameplayAttributeData DamageOverTime;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, DamageOverTime);

	/** Increases overall elemental damage. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ElementalDamage, Category = "Secondary Attribute|Offensive Stats")
	FGameplayAttributeData ElementalDamage;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, ElementalDamage);

	/** Increases melee attack damage. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MeleeDamage, Category = "Secondary Attribute|Offensive Stats")
	FGameplayAttributeData MeleeDamage;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MeleeDamage);

	/** Increases Spell attack damage. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_SpellDamage, Category = "Secondary Attribute|Offensive Stats")
	FGameplayAttributeData SpellDamage;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, SpellDamage);

	/** Increases the number of projectiles fired at once. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ProjectileCount, Category = "Secondary Attribute|Offensive Stats")
	FGameplayAttributeData ProjectileCount;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, ProjectileCount);

	/** Increases projectile speed for ranged attacks and spells. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ProjectileSpeed, Category = "Secondary Attribute|Offensive Stats")
	FGameplayAttributeData ProjectileSpeed;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, ProjectileSpeed);

	/** Increases damage for all ranged attacks. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_RangedDamage, Category = "Secondary Attribute|Offensive Stats")
	FGameplayAttributeData RangedDamage;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, RangedDamage);

	/** Increases spell critical chance. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_SpellsCritChance, Category = "Secondary Attribute|Offensive Stats")
	FGameplayAttributeData SpellsCritChance;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, SpellsCritChance);

	/** Increases spell critical multiplier. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_SpellsCritMultiplier, Category = "Secondary Attribute|Offensive Stats")
	FGameplayAttributeData SpellsCritMultiplier;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, SpellsCritMultiplier);

	/* =================================== */
	/* === Chance to Apply Ailments === */
	/* =================================== */

	/** Increases the chance to apply bleed effects. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ChanceToBleed, Category = "Secondary Attribute|Ailments")
	FGameplayAttributeData ChanceToBleed;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, ChanceToBleed);

	/** Increases the chance to apply corruption effects. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ChanceToCorrupt, Category = "Secondary Attribute|Ailments")
	FGameplayAttributeData ChanceToCorrupt;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, ChanceToCorrupt);

	/** Increases the chance to apply freeze effects. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ChanceToFreeze, Category = "Secondary Attribute|Ailments")
	FGameplayAttributeData ChanceToFreeze;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, ChanceToFreeze);

	/** Increases the chance to apply purification effects. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ChanceToPurify, Category = "Secondary Attribute|Ailments")
	FGameplayAttributeData ChanceToPurify;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, ChanceToPurify);

	/** Increases the chance to apply ignite effects (burning). */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ChanceToIgnite, Category = "Secondary Attribute|Ailments")
	FGameplayAttributeData ChanceToIgnite;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, ChanceToIgnite);

	/** Increases the chance to knock back an enemy. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ChanceToKnockBack, Category = "Secondary Attribute|Ailments")
	FGameplayAttributeData ChanceToKnockBack;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, ChanceToKnockBack);

	/** Increases the chance to apply petrify effects. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ChanceToPetrify, Category = "Secondary Attribute|Ailments")
	FGameplayAttributeData ChanceToPetrify;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, ChanceToPetrify);

	/** Increases the chance to apply shock effects. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ChanceToShock, Category = "Secondary Attribute|Ailments")
	FGameplayAttributeData ChanceToShock;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, ChanceToShock);

	/** Increases the chance to apply stun effects. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ChanceToStun, Category = "Secondary Attribute|Ailments")
	FGameplayAttributeData ChanceToStun;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, ChanceToStun);


		/* ============================= */
	/* === Duration Attributes === */
	/* ============================= */

	/** Duration of burn effects (fire damage over time). */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_BurnDuration, Category = "Secondary Attribute|Duration")
	FGameplayAttributeData BurnDuration;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, BurnDuration);

	/** Duration of bleed effects (physical damage over time). */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_BleedDuration, Category = "Secondary Attribute|Duration")
	FGameplayAttributeData BleedDuration;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, BleedDuration);

	/** Duration of freeze effects (ice immobilization). */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_FreezeDuration, Category = "Secondary Attribute|Duration")
	FGameplayAttributeData FreezeDuration;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, FreezeDuration);

	/** Duration of corruption effects (dark magic damage over time). */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_CorruptionDuration, Category = "Secondary Attribute|Duration")
	FGameplayAttributeData CorruptionDuration;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, CorruptionDuration);

	/** Duration of shock effects (lightning stun and damage over time). */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ShockDuration, Category = "Secondary Attribute|Duration")
	FGameplayAttributeData ShockDuration;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, ShockDuration);

	/** Build-up duration for petrification effects. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_PetrifyBuildUpDuration, Category = "Secondary Attribute|Duration")
	FGameplayAttributeData PetrifyBuildUpDuration;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, PetrifyBuildUpDuration);

	/** Duration of purification effects (removal of debuffs). */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_PurifyDuration, Category = "Secondary Attribute|Duration")
	FGameplayAttributeData PurifyDuration;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, PurifyDuration);

	/* ============================= */
	/* === Resistance Attributes === */
	/* ============================= */

	/** Increases all defensive resistances. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_GlobalDefenses, Category = "Secondary Attribute|Resistances")
	FGameplayAttributeData GlobalDefenses;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, GlobalDefenses);

	/** Strength of blocking incoming attacks. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_BlockStrength, Category = "Secondary Attribute|Resistances")
	FGameplayAttributeData BlockStrength;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, BlockStrength);

	/** Armor rating for physical damage mitigation. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_Armour, Category = "Secondary Attribute|Resistances")
	FGameplayAttributeData Armour;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, Armour);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ArmourFlatBonus, Category = "Secondary Attribute|Resistances")
	FGameplayAttributeData ArmourFlatBonus;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, ArmourFlatBonus);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ArmourPercentBonus, Category = "Secondary Attribute|Resistances")
	FGameplayAttributeData ArmourPercentBonus;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, ArmourPercentBonus);

	/** Corruption Resistance - Reduces corruption-based damage and curses. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_CorruptionResistanceFlatBonus, Category = "Secondary Attribute|Resistances")
	FGameplayAttributeData CorruptionResistanceFlatBonus;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, CorruptionResistanceFlatBonus);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_CorruptionResistancePercentBonus, Category = "Secondary Attribute|Resistances")
	FGameplayAttributeData CorruptionResistancePercentBonus;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, CorruptionResistancePercentBonus);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxCorruptionResistance, Category = "Secondary Attribute|Resistances")
	FGameplayAttributeData MaxCorruptionResistance;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MaxCorruptionResistance);

	/** Fire Resistance - Reduces fire damage and burn effects. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_FireResistanceFlatBonus, Category = "Secondary Attribute|Resistances")
	FGameplayAttributeData FireResistanceFlatBonus;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, FireResistanceFlatBonus);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_FireResistancePercentBonus, Category = "Secondary Attribute|Resistances")
	FGameplayAttributeData FireResistancePercentBonus;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, FireResistancePercentBonus);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxFireResistance, Category = "Secondary Attribute|Resistances")
	FGameplayAttributeData MaxFireResistance;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MaxFireResistance);

	/** Ice Resistance - Reduces ice damage and freeze effects. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_IceResistanceFlatBonus, Category = "Secondary Attribute|Resistances")
	FGameplayAttributeData IceResistanceFlatBonus;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, IceResistanceFlatBonus);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_IceResistancePercentBonus, Category = "Secondary Attribute|Resistances")
	FGameplayAttributeData IceResistancePercentBonus;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, IceResistancePercentBonus);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxIceResistance, Category = "Secondary Attribute|Resistances")
	FGameplayAttributeData MaxIceResistance;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MaxIceResistance);

	/** Light Resistance - Reduces light damage and holy-based effects. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_LightResistanceFlatBonus, Category = "Secondary Attribute|Resistances")
	FGameplayAttributeData LightResistanceFlatBonus;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, LightResistanceFlatBonus);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_LightResistancePercentBonus, Category = "Secondary Attribute|Resistances")
	FGameplayAttributeData LightResistancePercentBonus;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, LightResistancePercentBonus);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxLightResistance, Category = "Secondary Attribute|Resistances")
	FGameplayAttributeData MaxLightResistance;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MaxLightResistance);

	/** Lightning Resistance - Reduces lightning damage and shock effects. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_LightningResistanceFlatBonus, Category = "Secondary Attribute|Resistances")
	FGameplayAttributeData LightningResistanceFlatBonus;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, LightningResistanceFlatBonus);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_LightningResistancePercentBonus, Category = "Secondary Attribute|Resistances")
	FGameplayAttributeData LightningResistancePercentBonus;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, LightningResistancePercentBonus);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxLightningResistance, Category = "Secondary Attribute|Resistances")
	FGameplayAttributeData MaxLightningResistance;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MaxLightningResistance);

	/* ============================= */
	/* === Piercing Attributes === */
	/* ============================= */

	/** Ignores a percentage of the target’s armor. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ArmourPiercing, Category = "Secondary Attribute|Piercing")
	FGameplayAttributeData ArmourPiercing;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, ArmourPiercing);

	/** Ignores a percentage of the target’s fire resistance. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_FirePiercing, Category = "Secondary Attribute|Piercing")
	FGameplayAttributeData FirePiercing;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, FirePiercing);

	/** Ignores a percentage of the target’s light resistance. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_LightPiercing, Category = "Secondary Attribute|Piercing")
	FGameplayAttributeData LightPiercing;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, LightPiercing);

	/** Ignores a percentage of the target’s lightning resistance. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_LightningPiercing, Category = "Secondary Attribute|Piercing")
	FGameplayAttributeData LightningPiercing;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, LightningPiercing);

	/** Ignores a percentage of the target’s corruption resistance. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_CorruptionPiercing, Category = "Secondary Attribute|Piercing")
	FGameplayAttributeData CorruptionPiercing;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, CorruptionPiercing);

	/** Ignores a percentage of the target’s ice resistance. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_IcePiercing, Category = "Secondary Attribute|Piercing")
	FGameplayAttributeData IcePiercing;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, IcePiercing);


	/*
	* Misc Attributes
	*/

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ComboCounter, Category = "Vital Attribute|Misc")
	FGameplayAttributeData ComboCounter;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, ComboCounter);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_CoolDown, Category = "Vital Attribute|Misc")
	FGameplayAttributeData CoolDown;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, CoolDown);
	
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_Gems, Category = "Vital Attribute|Misc|Gems")
	FGameplayAttributeData Gems;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, Gems);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_LifeLeech, Category = "Vital Attribute|Misc")
	FGameplayAttributeData LifeLeech;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, LifeLeech);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ManaLeech, Category = "Vital Attribute|Misc")
	FGameplayAttributeData ManaLeech;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, ManaLeech);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MovementSpeed, Category = "Vital Attribute|Misc")
	FGameplayAttributeData MovementSpeed;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MovementSpeed);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_Poise, Category = "Vital Attribute|Misc")
	FGameplayAttributeData Poise;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, Poise);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_Weight, Category = "Vital Attribute|Misc")
	FGameplayAttributeData Weight;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, Weight);
	
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_PoiseResistance, Category = "Vital Attribute|Misc")
	FGameplayAttributeData PoiseResistance;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, PoiseResistance);
	
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_StunRecovery, Category = "Vital Attribute|Misc")
	FGameplayAttributeData StunRecovery;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, StunRecovery);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ManaCostChanges, Category = "Vital Attribute|Misc")
	FGameplayAttributeData ManaCostChanges;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, ManaCostChanges);
	
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_LifeOnHit, Category = "Vital Attribute|Misc")
	FGameplayAttributeData LifeOnHit;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, LifeOnHit);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ManaOnHit, Category = "Vital Attribute|Misc")
	FGameplayAttributeData ManaOnHit;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, ManaOnHit);
	
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_StaminaOnHit, Category = "Vital Attribute|Misc")
	FGameplayAttributeData StaminaOnHit;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, StaminaOnHit);


	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_StaminaCostChanges, Category = "Vital Attribute|Misc")
	FGameplayAttributeData StaminaCostChanges;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, StaminaCostChanges);

	
	
	

	/* ============================= */
	/* === Vital Attributes === */
	/* ============================= */

	/* ============================= */
	/* === Health Attributes === */
	/* ============================= */

/** Current health of the character. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_Health, Category = "Vital Attribute|Health")
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, Health);

	/** Maximum health capacity. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxHealth, Category = "Vital Attribute|Health")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MaxHealth);

	/** Effective max health after considering reserved health. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxEffectiveHealth, Category = "Vital Attribute|Health")
	FGameplayAttributeData MaxEffectiveHealth;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MaxEffectiveHealth);

	/** Health regeneration rate per second. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_HealthRegenRate, Category = "Vital Attribute|Health")
	FGameplayAttributeData HealthRegenRate;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, HealthRegenRate);

	/** Maximum rate at which health can regenerate per second. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxHealthRegenRate, Category = "Vital Attribute|Health")
	FGameplayAttributeData MaxHealthRegenRate;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MaxHealthRegenRate);

	/** Amount of health restored per tick. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_HealthRegenAmount, Category = "Vital Attribute|Health")
	FGameplayAttributeData HealthRegenAmount;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, HealthRegenAmount);

	/** Maximum health regeneration per tick. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxHealthRegenAmount, Category = "Vital Attribute|Health")
	FGameplayAttributeData MaxHealthRegenAmount;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MaxHealthRegenAmount);

	/** Reserved health, reducing max effective health. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ReservedHealth, Category = "Vital Attribute|Health")
	FGameplayAttributeData ReservedHealth;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, ReservedHealth);

	/** Maximum reserved health possible. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxReservedHealth, Category = "Vital Attribute|Health")
	FGameplayAttributeData MaxReservedHealth;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MaxReservedHealth);

	/** Flat amount of reserved health. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_FlatReservedHealth, Category = "Vital Attribute|Health")
	FGameplayAttributeData FlatReservedHealth;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, FlatReservedHealth);

	/** Percentage of max health that is reserved. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_PercentageReservedHealth, Category = "Vital Attribute|Health")
	FGameplayAttributeData PercentageReservedHealth;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, PercentageReservedHealth);
	
		/* ============================= */
	/* === Stamina Attributes === */
	/* ============================= */

	/** Current stamina used for sprinting, dodging, and special actions. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_Stamina, Category = "Vital Attribute|Stamina")
	FGameplayAttributeData Stamina;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, Stamina);

	/** Maximum stamina pool available. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxStamina, Category = "Vital Attribute|Stamina")
	FGameplayAttributeData MaxStamina;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MaxStamina);

	/** Effective maximum stamina after considering reserved stamina. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxEffectiveStamina, Category = "Vital Attribute|Stamina")
	FGameplayAttributeData MaxEffectiveStamina;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MaxEffectiveStamina);

	/** Rate at which stamina regenerates per second. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_StaminaRegenRate, Category = "Vital Attribute|Stamina")
	FGameplayAttributeData StaminaRegenRate;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, StaminaRegenRate);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_StaminaDegenRate, Category = "Vital Attribute|Stamina")
	FGameplayAttributeData StaminaDegenRate;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, StaminaDegenRate);

	/** Maximum rate at which stamina can regenerate per second. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxStaminaRegenRate, Category = "Vital Attribute|Stamina")
	FGameplayAttributeData MaxStaminaRegenRate;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MaxStaminaRegenRate);

	/** Amount of stamina restored per tick. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_StaminaRegenAmount, Category = "Vital Attribute|Stamina")
	FGameplayAttributeData StaminaRegenAmount;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, StaminaRegenAmount);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_StaminaDegenAmount, Category = "Vital Attribute|Stamina")
	FGameplayAttributeData StaminaDegenAmount;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, StaminaDegenAmount);

	/** Maximum stamina regeneration per tick. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxStaminaRegenAmount, Category = "Vital Attribute|Stamina")
	FGameplayAttributeData MaxStaminaRegenAmount;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MaxStaminaRegenAmount);

	/** Reserved stamina, reducing max effective stamina. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ReservedStamina, Category = "Vital Attribute|Stamina")
	FGameplayAttributeData ReservedStamina;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, ReservedStamina);

	/** Maximum reserved stamina possible. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxReservedStamina, Category = "Vital Attribute|Stamina")
	FGameplayAttributeData MaxReservedStamina;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MaxReservedStamina);

	/** Flat amount of stamina that is reserved. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_FlatReservedStamina, Category = "Vital Attribute|Stamina")
	FGameplayAttributeData FlatReservedStamina;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, FlatReservedStamina);

	/** Percentage of max stamina that is reserved. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_PercentageReservedStamina, Category = "Vital Attribute|Stamina")
	FGameplayAttributeData PercentageReservedStamina;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, PercentageReservedStamina);


	/* ============================= */
	/* === Mana Attributes === */
	/* ============================= */

	/** Current mana used for casting spells. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_Mana, Category = "Vital Attribute|Mana")
	FGameplayAttributeData Mana;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, Mana);

	/** Maximum mana pool available. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxMana, Category = "Vital Attribute|Mana")
	FGameplayAttributeData MaxMana;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MaxMana);

	/** Effective max mana after considering reserved mana. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxEffectiveMana, Category = "Vital Attribute|Mana")
	FGameplayAttributeData MaxEffectiveMana;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MaxEffectiveMana);

	/** Rate at which mana regenerates per second. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ManaRegenRate, Category = "Vital Attribute|Mana")
	FGameplayAttributeData ManaRegenRate;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, ManaRegenRate);

	/** Maximum rate at which mana can regenerate per second. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxManaRegenRate, Category = "Vital Attribute|Mana")
	FGameplayAttributeData MaxManaRegenRate;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MaxManaRegenRate);

	/** Amount of mana restored per tick. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ManaRegenAmount, Category = "Vital Attribute|Mana")
	FGameplayAttributeData ManaRegenAmount;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, ManaRegenAmount);

	/** Maximum mana regeneration per tick. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxManaRegenAmount, Category = "Vital Attribute|Mana")
	FGameplayAttributeData MaxManaRegenAmount;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MaxManaRegenAmount);

	/** Reserved mana, reducing max effective mana. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ReservedMana, Category = "Vital Attribute|Mana")
	FGameplayAttributeData ReservedMana;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, ReservedMana);

	/** Maximum reserved mana possible. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxReservedMana, Category = "Vital Attribute|Mana")
	FGameplayAttributeData MaxReservedMana;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MaxReservedMana);

	/** Flat amount of reserved mana. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_FlatReservedMana, Category = "Vital Attribute|Mana")
	FGameplayAttributeData FlatReservedMana;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, FlatReservedMana);

	/** Percentage of max mana that is reserved. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_PercentageReservedMana, Category = "Vital Attribute|Mana")
	FGameplayAttributeData PercentageReservedMana;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, PercentageReservedMana);

	/* ============================= */
	/* === Arcane Shield (Energy Shield) Attributes === */
	/* ============================= */

	/** Current arcane shield value (absorbs damage before health is affected). */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ArcaneShield, Category = "Vital Attribute|Arcane Shield")
	FGameplayAttributeData ArcaneShield;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, ArcaneShield);

	/** Maximum arcane shield capacity. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxArcaneShield, Category = "Vital Attribute|Arcane Shield")
	FGameplayAttributeData MaxArcaneShield;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MaxArcaneShield);

	/** Effective max arcane shield after considering reserved shield. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxEffectiveArcaneShield, Category = "Vital Attribute|Arcane Shield")
	FGameplayAttributeData MaxEffectiveArcaneShield;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MaxEffectiveArcaneShield);

	/** Arcane shield regeneration rate per second. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ArcaneShieldRegenRate, Category = "Vital Attribute|Arcane Shield")
	FGameplayAttributeData ArcaneShieldRegenRate;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, ArcaneShieldRegenRate);

	/** Maximum rate at which arcane shield can regenerate per second. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxArcaneShieldRegenRate, Category = "Vital Attribute|Arcane Shield")
	FGameplayAttributeData MaxArcaneShieldRegenRate;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MaxArcaneShieldRegenRate);

	/** Amount of arcane shield restored per tick. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ArcaneShieldRegenAmount, Category = "Vital Attribute|Arcane Shield")
	FGameplayAttributeData ArcaneShieldRegenAmount;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, ArcaneShieldRegenAmount);

	/** Maximum arcane shield regeneration per tick. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxArcaneShieldRegenAmount, Category = "Vital Attribute|Arcane Shield")
	FGameplayAttributeData MaxArcaneShieldRegenAmount;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MaxArcaneShieldRegenAmount);

	/** Reserved arcane shield, reducing max effective shield. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ReservedArcaneShield, Category = "Vital Attribute|Arcane Shield")
	FGameplayAttributeData ReservedArcaneShield;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, ReservedArcaneShield);

	/** Maximum reserved arcane shield possible. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxReservedArcaneShield, Category = "Vital Attribute|Arcane Shield")
	FGameplayAttributeData MaxReservedArcaneShield;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MaxReservedArcaneShield);

	/** Flat amount of reserved arcane shield. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_FlatReservedArcaneShield, Category = "Vital Attribute|Arcane Shield")
	FGameplayAttributeData FlatReservedArcaneShield;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, FlatReservedArcaneShield);

	/** Percentage of max arcane shield that is reserved. */
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_PercentageReservedArcaneShield, Category = "Vital Attribute|Arcane Shield")
	FGameplayAttributeData PercentageReservedArcaneShield;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, PercentageReservedArcaneShield);


public:
	
	/* ============================= */
	/* === Combat Indicators === */
	/* ============================= */

	/** Called when combat alignment changes. */
	UFUNCTION()
	void OnRep_CombatAlignment(const FGameplayAttributeData& OldCombatAlignment) const;

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


		/* ============================= */
	/* === Damage Replication Functions === */
	/* ============================= */

	/** Called when global damage bonus changes. */
	UFUNCTION()
	void OnRep_GlobalDamages(const FGameplayAttributeData& OldAmount) const;

	/** Corruption Damage */
	UFUNCTION()
	void OnRep_MinCorruptionDamage(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_MaxCorruptionDamage(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_CorruptionFlatBonus(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_CorruptionPercentBonus(const FGameplayAttributeData& OldAmount) const;

	/** Fire Damage */
	UFUNCTION()
	void OnRep_MinFireDamage(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_MaxFireDamage(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_FireFlatBonus(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_FirePercentBonus(const FGameplayAttributeData& OldAmount) const;

	/** Ice Damage */
	UFUNCTION()
	void OnRep_MinIceDamage(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_MaxIceDamage(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_IceFlatBonus(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_IcePercentBonus(const FGameplayAttributeData& OldAmount) const;

	/** Light Damage */
	UFUNCTION()
	void OnRep_MinLightDamage(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_MaxLightDamage(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_LightFlatBonus(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_LightPercentBonus(const FGameplayAttributeData& OldAmount) const;

	/** Lightning Damage */
	UFUNCTION()
	void OnRep_MinLightningDamage(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_MaxLightningDamage(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_LightningFlatBonus(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_LightningPercentBonus(const FGameplayAttributeData& OldAmount) const;

	/** Physical Damage */
	UFUNCTION()
	void OnRep_MinPhysicalDamage(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_MaxPhysicalDamage(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_PhysicalFlatBonus(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_PhysicalPercentBonus(const FGameplayAttributeData& OldAmount) const;

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
	void OnRep_CoolDown(const FGameplayAttributeData& OldAmount) const;
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
	void OnRep_LifeOnHit(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_ManaOnHit(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_StaminaOnHit(const FGameplayAttributeData& OldAmount) const;
	UFUNCTION()
	void OnRep_StaminaCostChanges(const FGameplayAttributeData& OldAmount) const;

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

	/** Utility function for setting effect properties */
	static void SetEffectProperties(const FGameplayEffectModCallbackData& Data, FEffectProperties& Props);
	
	/** Utility function for setting attribute values */
	void SetAttributeValue(const FGameplayAttribute& Attribute, float NewValue);

};
