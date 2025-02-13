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
 * 
 */
UCLASS()
class ALS_PROJECTHUNTER_API UPHAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	
	UPHAttributeSet();
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	float GetAttributeValue(const FGameplayAttribute& Attribute) const;
	
	float GetAttributeValue(FGameplayAttribute& Attribute) const;

	UFUNCTION(BlueprintCallable, Category = "Combat Alignment")
	ECombatAlignment GetCombatAlignmentBP() const;

	UFUNCTION(BlueprintCallable, Category = "Combat Alignment")
	void SetCombatAlignmentBP(ECombatAlignment NewAlignment);

	UPROPERTY(BlueprintAssignable, Category = "Combat Alignment")
	FOnAttributeChangeSignature OnCombatAlignmentChange;
	
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;


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
	 *Combat Indecatorss 
	 */

	UPROPERTY(BlueprintReadOnly, Category = "Combat Alignment", ReplicatedUsing = OnRep_CombatAlignment)
	FGameplayAttributeData CombatAlignment;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, CombatAlignment)
 
	/*
	 * Primary Attributes 
	 */
	// stats may change 
	
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_Strength, Category = "Primary Attribute")
	FGameplayAttributeData Strength;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, Strength); //+5 max health, 2% physical damage.

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_Intelligence, Category = "Primary Attribute")
	FGameplayAttributeData Intelligence;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, Intelligence); // +5 mana , 1.3% elemental damage.

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_Dexterity, Category = "Primary Attribute")
	FGameplayAttributeData Dexterity;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, Dexterity); // +.5 Crit multiplier , +.5 Attack/Cast Speed

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_Endurance, Category = "Primary Attribute")
	FGameplayAttributeData Endurance;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, Endurance); // +10 Stamina  +.01 Resistance;

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_Affliction, Category = "Primary Attribute")
	FGameplayAttributeData Affliction;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, Affliction); // +Damage overtime .05 Duration 0.01

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_Luck, Category = "Primary Attribute")
	FGameplayAttributeData Luck;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, Luck); // Chance to apply .01 drop chance .02

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_Covenant, Category = "Primary Attribute")
	FGameplayAttributeData Covenant;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, Covenant); // minion damage +.02  minion health +.01
	
	/*
	* Secondary Attributes
	*/

	/*
	 * Global
	 */

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_GlobalDamages, Category = "Secondary Attribute|Damage")
	FGameplayAttributeData GlobalDamages;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, GlobalDamages); // increases all damages

	/*
	* Damage - Secondary Attributes
	*/

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_CorruptionDamage, Category = "Secondary Attribute|Damage")
	FGameplayAttributeData CorruptionDamage;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, CorruptionDamage);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_CorruptionFlatBonus, Category = "Secondary Attribute|Damage")
	FGameplayAttributeData CorruptionFlatBonus;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, CorruptionFlatBonus);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_CorruptionPercentBonus, Category = "Secondary Attribute|Damage")
	FGameplayAttributeData CorruptionPercentBonus;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, CorruptionPercentBonus);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_EarthDamage, Category = "Secondary Attribute|Damage")
	FGameplayAttributeData EarthDamage;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, EarthDamage);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_EarthFlatBonus, Category = "Secondary Attribute|Damage")
	FGameplayAttributeData EarthFlatBonus;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, EarthFlatBonus);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_EarthPercentBonus, Category = "Secondary Attribute|Damage")
	FGameplayAttributeData EarthPercentBonus;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, EarthPercentBonus);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_FireDamage, Category = "Secondary Attribute|Damage")
	FGameplayAttributeData FireDamage;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, FireDamage);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_FireFlatBonus, Category = "Secondary Attribute|Damage")
	FGameplayAttributeData FireFlatBonus;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, FireFlatBonus);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_FirePercentBonus, Category = "Secondary Attribute|Damage")
	FGameplayAttributeData FirePercentBonus;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, FirePercentBonus);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_IceDamage, Category = "Secondary Attribute|Damage")
	FGameplayAttributeData IceDamage;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, IceDamage);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_IceFlatBonus, Category = "Secondary Attribute|Damage")
	FGameplayAttributeData IceFlatBonus;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, IceFlatBonus);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_IcePercentBonus, Category = "Secondary Attribute|Damage")
	FGameplayAttributeData IcePercentBonus;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, IcePercentBonus);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_LightDamage, Category = "Secondary Attribute|Damage")
	FGameplayAttributeData LightDamage;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, LightDamage);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_LightFlatBonus, Category = "Secondary Attribute|Damage")
	FGameplayAttributeData LightFlatBonus;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, LightFlatBonus);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_LightPercentBonus, Category = "Secondary Attribute|Damage")
	FGameplayAttributeData LightPercentBonus;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, LightPercentBonus);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_LightningDamage, Category = "Secondary Attribute|Damage")
	FGameplayAttributeData LightningDamage;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, LightningDamage);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_LightningFlatBonus, Category = "Secondary Attribute|Damage")
	FGameplayAttributeData LightningFlatBonus;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, LightningFlatBonus);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_LightningPercentBonus, Category = "Secondary Attribute|Damage")
	FGameplayAttributeData LightningPercentBonus;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, LightningPercentBonus);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_PhysicalDamage, Category = "Secondary Attribute|Damage")
	FGameplayAttributeData PhysicalDamage;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, PhysicalDamage);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_PhysicalFlatBonus, Category = "Secondary Attribute|Damage")
	FGameplayAttributeData PhysicalFlatBonus;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, PhysicalFlatBonus);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_PhysicalPercentBonus, Category = "Secondary Attribute|Damage")
	FGameplayAttributeData PhysicalPercentBonus;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, PhysicalPercentBonus);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_WindDamage, Category = "Secondary Attribute|Damage")
	FGameplayAttributeData WindDamage;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, WindDamage);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_WindFlatBonus, Category = "Secondary Attribute|Damage")
	FGameplayAttributeData WindFlatBonus;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, WindFlatBonus);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_WindPercentBonus, Category = "Secondary Attribute|Damage")
	FGameplayAttributeData WindPercentBonus;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, WindPercentBonus);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_DamageBonusWhileAtFullHP, Category = "Secondary Attribute|Damage")
	FGameplayAttributeData DamageBonusWhileAtFullHP;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, DamageBonusWhileAtFullHP);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_DamageBonusWhileAtLowHP, Category = "Secondary Attribute|Damage")
	FGameplayAttributeData DamageBonusWhileAtLowHP;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, DamageBonusWhileAtLowHP);


	/*
	* Other Offensive Stats - Secondary Attributes
	*/

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_AreaDamage, Category = "Secondary Attribute|Offensive Stats")
	FGameplayAttributeData AreaDamage;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, AreaDamage);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_AreaOfEffect, Category = "Secondary Attribute|Offensive Stats")
	FGameplayAttributeData AreaOfEffect;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, AreaOfEffect);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_AttackRange, Category = "Secondary Attribute|Offensive Stats")
	FGameplayAttributeData AttackRange;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, AttackRange);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_AttackSpeed, Category = "Secondary Attribute|Offensive Stats")
	FGameplayAttributeData AttackSpeed;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, AttackSpeed);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_CastSpeed, Category = "Secondary Attribute|Offensive Stats")
	FGameplayAttributeData CastSpeed;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, CastSpeed);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_CritChance, Category = "Secondary Attribute|Offensive Stats")
	FGameplayAttributeData CritChance;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, CritChance);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_CritMultiplier, Category = "Secondary Attribute|Offensive Stats")
	FGameplayAttributeData CritMultiplier;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, CritMultiplier);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_DamageOverTime, Category = "Secondary Attribute|Offensive Stats")
	FGameplayAttributeData DamageOverTime;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, DamageOverTime);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ElementalDamage, Category = "Secondary Attribute|Offensive Stats")
	FGameplayAttributeData ElementalDamage;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, ElementalDamage);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MeleeDamage, Category = "Secondary Attribute|Offensive Stats")
	FGameplayAttributeData MeleeDamage;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MeleeDamage)

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ProjectileCount, Category = "Secondary Attribute|Offensive Stats")
	FGameplayAttributeData ProjectileCount;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, ProjectileCount)

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ProjectileSpeed, Category = "Secondary Attribute|Offensive Stats")
	FGameplayAttributeData ProjectileSpeed;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, ProjectileSpeed)

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_RangedDamage, Category = "Secondary Attribute|Offensive Stats")
	FGameplayAttributeData RangedDamage;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, RangedDamage)
	
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_SpellsCritChance, Category = "Secondary Attribute|Offensive Stats")
	FGameplayAttributeData SpellsCritChance;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, SpellsCritChance);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_SpellsCritMultiplier, Category = "Secondary Attribute|Offensive Stats")
	FGameplayAttributeData SpellsCritMultiplier;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, SpellsCritMultiplier);

	/*
	 * Chance To Apply - Secondary Attributes
	*/

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ChanceToBleed, Category = "Secondary Attribute|Ailments")
	FGameplayAttributeData ChanceToBleed;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, ChanceToBleed);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ChanceToCorrupt, Category = "Secondary Attribute|Ailments")
	FGameplayAttributeData ChanceToCorrupt;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, ChanceToCorrupt);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ChanceToFreeze, Category = "Secondary Attribute|Ailments")
	FGameplayAttributeData ChanceToFreeze;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, ChanceToFreeze);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ChanceToPurify, Category = "Secondary Attribute|Ailments")
	FGameplayAttributeData ChanceToPurify;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, ChanceToPurify);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ChanceToIgnite, Category = "Secondary Attribute|Ailments")
	FGameplayAttributeData ChanceToIgnite;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, ChanceToIgnite);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ChanceToKnockBack, Category = "Secondary Attribute|Ailments")
	FGameplayAttributeData ChanceToKnockBack;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, ChanceToKnockBack);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ChanceToPetrify, Category = "Secondary Attribute|Ailments")
	FGameplayAttributeData ChanceToPetrify;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, ChanceToPetrify);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ChanceToShock, Category = "Secondary Attribute|Ailments")
	FGameplayAttributeData ChanceToShock;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, ChanceToShock);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ChanceToStun, Category = "Secondary Attribute|Ailments")
	FGameplayAttributeData ChanceToStun;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, ChanceToStun);

	//Duration

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_BurnDuration , Category = "Vital Attribute|Duration")
	FGameplayAttributeData BurnDuration ;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, BurnDuration );

	
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_BleedDuration , Category = "Vital Attribute|Duration")
	FGameplayAttributeData BleedDuration ;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet,BleedDuration );

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_FreezeDuration , Category = "Vital Attribute|Duration")
	FGameplayAttributeData FreezeDuration ;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, FreezeDuration);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_CorruptionDuration, Category = "Vital Attribute|Duration")
	FGameplayAttributeData CorruptionDuration ;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, CorruptionDuration);
	
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ShockDuration, Category = "Vital Attribute|Duration")
	FGameplayAttributeData ShockDuration ;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, ShockDuration);
	
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_PetrifyBuildUpDuration, Category = "Vital Attribute|Duration")
	FGameplayAttributeData PetrifyBuildUpDuration ;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, PetrifyBuildUpDuration);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_PurifyDuration, Category = "Vital Attribute|Duration")
	FGameplayAttributeData PurifyDuration ;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, PurifyDuration);


	/*
	* Resistances - Secondary Attributes
	*/

	/*
	 * Global 
	 */

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_GlobalDefenses, Category = "Secondary Attribute|Resistances")
	FGameplayAttributeData GlobalDefenses;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, GlobalDefenses); // increases all defenses

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_BlockStrength, Category = "Secondary Attribute|Resistances")
	FGameplayAttributeData BlockStrength;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, BlockStrength);

	/*
	* Armour
	* Reduces damage taken from physical attacks.
	* Helps resist stun effects.
	*/

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_Armour, Category = "Secondary Attribute|Resistances")
	FGameplayAttributeData Armour;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, Armour);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ArmourFlatBonus, Category = "Secondary Attribute|Resistances")
	FGameplayAttributeData ArmourFlatBonus;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, ArmourFlatBonus);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ArmourPercentBonus, Category = "Secondary Attribute|Resistances")
	FGameplayAttributeData ArmourPercentBonus;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, ArmourPercentBonus);

	/*
	* Corruption Resistance
	* Reduces damage taken from corruption-based attacks and curses.
	*/

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_CorruptionResistance, Category = "Secondary Attribute|Resistances")
	FGameplayAttributeData CorruptionResistance;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, CorruptionResistance);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxCorruptionResistance, Category = "Secondary Attribute|Resistances")
	FGameplayAttributeData MaxCorruptionResistance;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MaxCorruptionResistance);
	/*
	* Earth Resistance
	* Reduces damage taken from earth-based attacks.
	* Helps resist petrification or root effects.
	 */

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_EarthResistance, Category = "Secondary Attribute|Resistances")
	FGameplayAttributeData EarthResistance;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, EarthResistance);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxEarthResistance, Category = "Secondary Attribute|Resistances")
	FGameplayAttributeData MaxEarthResistance;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MaxEarthResistance);
	
	/*
	* Fire Resistance
	* Reduces damage taken from fire-based attacks.
	* Helps resist burn effects.
	*/

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_FireResistance, Category = "Secondary Attribute|Resistances")
	FGameplayAttributeData FireResistance;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, FireResistance);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxFireResistance, Category = "Secondary Attribute|Resistances")
	FGameplayAttributeData MaxFireResistance;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MaxFireResistance);


	/*
	* Ice Resistance
	* Reduces damage taken from ice-based attacks.
	* Helps resist freezing effects.
	*/

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_IceResistance, Category = "Secondary Attribute|Resistances")
	FGameplayAttributeData IceResistance;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, IceResistance);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxIceResistance, Category = "Secondary Attribute|Resistances")
	FGameplayAttributeData MaxIceResistance;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MaxIceResistance);
	
	/*
	* Light Resistance
	* Reduces damage taken from light-based attacks.
	* Helps resist holy energy effects.
	*/

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_LightResistance, Category = "Secondary Attribute|Resistances")
	FGameplayAttributeData LightResistance;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, LightResistance);
	
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxLightResistance, Category = "Secondary Attribute|Resistances")
	FGameplayAttributeData MaxLightResistance;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MaxLightResistance);
	
	/*
	 * Lightning Resistance	
	* Reduces damage taken from lightning-based attacks.
	* Helps resist shock effects.
	*/

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_LightningResistance, Category = "Secondary Attribute|Resistances")
	FGameplayAttributeData LightningResistance;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, LightningResistance);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxLightningResistance, Category = "Secondary Attribute|Resistances")
	FGameplayAttributeData MaxLightningResistance;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MaxLightningResistance);
	
	/*
	* Wind Resistance
	* Reduces damage taken from wind-based attacks.
	* Helps resist knockback effects.
	*/
	
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_WindResistance, Category = "Secondary Attribute|Resistances")
	FGameplayAttributeData WindResistance;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, WindResistance);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxWindResistance, Category = "Secondary Attribute|Resistances")
	FGameplayAttributeData MaxWindResistance;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MaxWindResistance);

	/*
	 * Piercing 
	 */

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ArmourPiercing, Category = "Secondary Attribute|Piercing")
	FGameplayAttributeData ArmourPiercing;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, ArmourPiercing);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_FirePiercing, Category = "Secondary Attribute|Piercing")
	FGameplayAttributeData FirePiercing;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, FirePiercing);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_EarthPiercing, Category = "Secondary Attribute|Piercing")
	FGameplayAttributeData EarthPiercing;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, EarthPiercing);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_LightPiercing, Category = "Secondary Attribute|Piercing")
	FGameplayAttributeData LightPiercing;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, LightPiercing);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_LightningPiercing, Category = "Secondary Attribute|Piercing")
	FGameplayAttributeData LightningPiercing;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, LightningPiercing);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_CorruptionPiercing, Category = "Secondary Attribute|Piercing")
	FGameplayAttributeData CorruptionPiercing;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, CorruptionPiercing);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_IcePiercing, Category = "Secondary Attribute|Piercing")
	FGameplayAttributeData IcePiercing;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, IcePiercing);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_WindPiercing, Category = "Secondary Attribute|Piercing")
	FGameplayAttributeData WindPiercing;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet,WindPiercing);

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

	
	
	

	/*
	 * Vital Attributes 
	 */
	
	// Health
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_Health, Category = "Vital Attribute|Health")
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, Health);
	
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxHealth, Category = "Vital Attribute|Health")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MaxHealth);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxEffectiveHealth, Category = "Vital Attribute|Health")
	FGameplayAttributeData  MaxEffectiveHealth;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MaxEffectiveHealth);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_HealthRegenRate, Category = "Vital Attribute|Health")
	FGameplayAttributeData HealthRegenRate;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, HealthRegenRate);
	
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxHealthRegenRate, Category = "Vital Attribute|Health")
	FGameplayAttributeData MaxHealthRegenRate;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MaxHealthRegenRate);
	
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_HealthRegenAmount, Category = "Vital Attribute|Health")
	FGameplayAttributeData HealthRegenAmount;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, HealthRegenAmount);
	
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxHealthRegenAmount, Category = "Vital Attribute|Health")
	FGameplayAttributeData MaxHealthRegenAmount;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MaxHealthRegenAmount);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ReservedHealth, Category = "Vital Attribute|Health")
	FGameplayAttributeData ReservedHealth;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, ReservedHealth);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxReservedHealth, Category = "Vital Attribute|Health")
	FGameplayAttributeData MaxReservedHealth;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MaxReservedHealth);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_FlatReservedHealth, Category = "Vital Attribute|Health")
	FGameplayAttributeData FlatReservedHealth;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, FlatReservedHealth);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing =  OnRep_PercentageReservedHealth, Category = "Vital Attribute|Health")
	FGameplayAttributeData PercentageReservedHealth;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, PercentageReservedHealth);
	
	//Stamina
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_Stamina, Category = "Vital Attribute|Stamina")
	FGameplayAttributeData Stamina;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, Stamina);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxStamina, Category = "Vital Attribute|Stamina")
	FGameplayAttributeData MaxStamina;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MaxStamina);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxEffectiveStamina, Category = "Vital Attribute|Health")
	FGameplayAttributeData  MaxEffectiveStamina;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MaxEffectiveStamina);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_StaminaRegenRate, Category = "Vital Attribute|Stamina")
	FGameplayAttributeData StaminaRegenRate;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, StaminaRegenRate);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxStaminaRegenRate, Category = "Vital Attribute|Stamina")
	FGameplayAttributeData MaxStaminaRegenRate;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MaxStaminaRegenRate);
	
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_StaminaRegenAmount, Category = "Vital Attribute|Stamina")
	FGameplayAttributeData StaminaRegenAmount;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, StaminaRegenAmount);
	
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxStaminaRegenAmount, Category = "Vital Attribute|Stamina")
	FGameplayAttributeData MaxStaminaRegenAmount;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MaxStaminaRegenAmount);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ReservedStamina, Category = "Vital Attribute|Stamina")
	FGameplayAttributeData ReservedStamina;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, ReservedStamina);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxReservedStamina, Category = "Vital Attribute|Stamina")
	FGameplayAttributeData MaxReservedStamina;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MaxReservedStamina);
	
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_FlatReservedStamina, Category = "Vital Attribute|Stamina")
	FGameplayAttributeData FlatReservedStamina;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, FlatReservedStamina);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing =  OnRep_PercentageReservedStamina, Category = "Vital Attribute|Stamina")
	FGameplayAttributeData PercentageReservedStamina;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, PercentageReservedStamina);

	//Mana
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_Mana, Category = "Vital Attribute|Mana")
	FGameplayAttributeData Mana;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, Mana);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxMana, Category = "Vital Attribute|Mana")
	FGameplayAttributeData MaxMana;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MaxMana);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxEffectiveHealth, Category = "Vital Attribute|Health")
	FGameplayAttributeData  MaxEffectiveMana;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MaxEffectiveMana);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ManaRegenRate, Category = "Vital Attribute|Mana")
	FGameplayAttributeData ManaRegenRate;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, ManaRegenRate);
	
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxManaRegenRate, Category = "Vital Attribute|Mana")
	FGameplayAttributeData MaxManaRegenRate;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MaxManaRegenRate);
	
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ManaRegenAmount, Category = "Vital Attribute|Mana")
	FGameplayAttributeData ManaRegenAmount;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, ManaRegenAmount);
	
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxManaRegenAmount, Category = "Vital Attribute|Mana")
	FGameplayAttributeData MaxManaRegenAmount;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MaxManaRegenAmount);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ReservedMana, Category = "Vital Attribute|Mana")
	FGameplayAttributeData ReservedMana;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, ReservedMana);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxReservedMana, Category = "Vital Attribute|Mana")
	FGameplayAttributeData MaxReservedMana;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MaxReservedMana);
	
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_FlatReservedMana, Category = "Vital Attribute|Mana")
	FGameplayAttributeData FlatReservedMana;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, FlatReservedMana);


	UPROPERTY(BlueprintReadWrite, ReplicatedUsing =  OnRep_PercentageReservedMana, Category = "Vital Attribute|Mana")
	FGameplayAttributeData PercentageReservedMana;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, PercentageReservedMana);


	//EnergyShield

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_Mana, Category = "Vital Attribute|Arcane Shield")
	FGameplayAttributeData ArcaneShield;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, ArcaneShield);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxMana, Category = "Vital Attribute|Arcane Shield")
	FGameplayAttributeData MaxArcaneShield;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MaxArcaneShield);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxEffectiveHealth, Category = "Vital Attribute|Arcane Shield")
	FGameplayAttributeData  MaxEffectiveArcaneShield;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MaxEffectiveArcaneShield);
	
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ArcaneShieldRegenRate, Category = "Vital Attribute|Arcane Shield")
	FGameplayAttributeData ArcaneShieldRegenRate;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, ArcaneShieldRegenRate);
	
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ArcaneShieldRegenRate, Category = "Vital Attribute|Arcane Shield")
	FGameplayAttributeData MaxArcaneShieldRegenRate;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MaxArcaneShieldRegenRate);
	
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ArcaneShieldRegenAmount, Category = "Vital Attribute|Arcane Shield")
	FGameplayAttributeData ArcaneShieldRegenAmount;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, ArcaneShieldRegenAmount);
	
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxArcaneShieldRegenAmount, Category = "Vital Attribute|Arcane Shield")
	FGameplayAttributeData MaxArcaneShieldRegenAmount;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MaxArcaneShieldRegenAmount);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ReservedArcaneShield, Category = "Vital Attribute|Arcane Shield")
	FGameplayAttributeData ReservedArcaneShield;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, ReservedArcaneShield);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxReservedArcaneShield, Category = "Vital Attribute|Arcane Shield")
	FGameplayAttributeData MaxReservedArcaneShield;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MaxReservedArcaneShield);
	
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_FlatReservedArcaneShield, Category = "Vital Attribute|Arcane Shield")
	FGameplayAttributeData FlatReservedArcaneShield;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, FlatReservedArcaneShield);


	UPROPERTY(BlueprintReadWrite, ReplicatedUsing =  OnRep_PercentageReservedArcaneShield, Category = "Vital Attribute|Arcane Shield")
	FGameplayAttributeData PercentageReservedArcaneShield;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, PercentageReservedArcaneShield);

public:
	
	/*
	 *Combat Indicators
	 */

	UFUNCTION()
	void OnRep_CombatAlignment(const FGameplayAttributeData& OldCombatAlignment) const;

	
	// Health 
	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldHealth)const ;

	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData&  OldMaxHealth)const;

	UFUNCTION()
	void OnRep_MaxEffectiveHealth(const FGameplayAttributeData& OldAmount)const;

	UFUNCTION()
	void OnRep_HealthRegenRate(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_MaxHealthRegenRate(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_HealthRegenAmount(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_MaxHealthRegenAmount(const FGameplayAttributeData& OldAmount) const;
	
	UFUNCTION()
	void OnRep_ReservedHealth(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_MaxReservedHealth(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_FlatReservedHealth(const FGameplayAttributeData& OldAmount) const;
	
	UFUNCTION()
	void OnRep_PercentageReservedHealth(const FGameplayAttributeData& OldAmount) const;
	
	// Stamina
	UFUNCTION()
	void OnRep_Stamina(const FGameplayAttributeData& OldStamina)const ;

	UFUNCTION()
	void OnRep_MaxStamina(const FGameplayAttributeData& OldMaxStamina)const ;

	UFUNCTION()
	void OnRep_MaxEffectiveStamina(const FGameplayAttributeData& OldAmount)const;

	UFUNCTION()
	void OnRep_StaminaRegenRate(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_MaxStaminaRegenRate(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_StaminaRegenAmount(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_MaxStaminaRegenAmount(const FGameplayAttributeData& OldAmount) const;
	
	UFUNCTION()
	void OnRep_ReservedStamina(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_MaxReservedStamina(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_FlatReservedStamina(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_PercentageReservedStamina(const FGameplayAttributeData& OldAmount) const;
	

	//Mana
	UFUNCTION()
	void OnRep_Mana(const FGameplayAttributeData& OldMana)const ;

	UFUNCTION()
	void OnRep_MaxMana(const FGameplayAttributeData& OldMaxMana)const ;

	UFUNCTION()
	void OnRep_MaxEffectiveMana(const FGameplayAttributeData& OldAmount)const;

	UFUNCTION()
	void OnRep_ManaRegenRate(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_MaxManaRegenRate(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_ManaRegenAmount(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_MaxManaRegenAmount(const FGameplayAttributeData& OldAmount) const;
	
	UFUNCTION()
	void OnRep_ReservedMana(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_MaxReservedMana(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_FlatReservedMana(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_PercentageReservedMana(const FGameplayAttributeData& OldAmount) const;


	//Arcane Shield
	UFUNCTION()
	void OnRep_ArcaneShield(const FGameplayAttributeData& OldAmount)const ;

	UFUNCTION()
	void OnRep_MaxArcaneShield(const FGameplayAttributeData& OldAmount)const ;

	UFUNCTION()
	void OnRep_MaxEffectiveArcaneShield(const FGameplayAttributeData& OldAmount)const;

	UFUNCTION()
	void OnRep_ArcaneShieldRegenRate(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_MaxArcaneShieldRegenRate(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_ArcaneShieldRegenAmount(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_MaxArcaneShieldRegenAmount(const FGameplayAttributeData& OldAmount) const;
	
	UFUNCTION()
	void OnRep_ReservedArcaneShield(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_MaxReservedArcaneShield(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_FlatReservedArcaneShield(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_PercentageReservedArcaneShield(const FGameplayAttributeData& OldAmount) const;
	
	
	UFUNCTION()
	void OnRep_Gems(const FGameplayAttributeData& OldGems)const ;
	
	UFUNCTION()
	void OnRep_Strength(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_Intelligence(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_Dexterity(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_Endurance(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_Affliction(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_Luck(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_Covenant(const FGameplayAttributeData& OldAmount) const;


	/*
	* Replication Functions for Secondary Attributes
	*/

	// Damage Attributes

	UFUNCTION()
	void OnRep_GlobalDamages(const FGameplayAttributeData& OldAmount) const;
	
	UFUNCTION()
	void OnRep_CorruptionDamage(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_CorruptionFlatBonus(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_CorruptionPercentBonus(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_EarthDamage(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_EarthFlatBonus(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_EarthPercentBonus(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_FireDamage(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_FireFlatBonus(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_FirePercentBonus(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_IceDamage(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_IceFlatBonus(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_IcePercentBonus(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_LightDamage(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_LightFlatBonus(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_LightPercentBonus(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_LightningDamage(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_LightningFlatBonus(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_LightningPercentBonus(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_PhysicalDamage(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_PhysicalFlatBonus(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_PhysicalPercentBonus(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_WindDamage(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_WindFlatBonus(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_WindPercentBonus(const FGameplayAttributeData& OldAmount) const;

	// Offensive Stats
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
	void OnRep_ProjectileCount(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_ProjectileSpeed(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_RangedDamage(const FGameplayAttributeData& OldAmount) const;


	// Resistances

	UFUNCTION()
	void OnRep_GlobalDefenses(const FGameplayAttributeData& OldAmount) const;
	
	UFUNCTION()
	void OnRep_Armour(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_ArmourFlatBonus(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_ArmourPercentBonus(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_CorruptionResistance(const FGameplayAttributeData& OldAmount) const;
	
	UFUNCTION()
	void OnRep_EarthResistance(const FGameplayAttributeData& OldAmount) const;
	
	UFUNCTION()
	void OnRep_FireResistance(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_IceResistance(const FGameplayAttributeData& OldAmount) const;
	
	UFUNCTION()
	void OnRep_LightResistance(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_LightningResistance(const FGameplayAttributeData& OldAmount) const;
	
	UFUNCTION()
	void OnRep_WindResistance(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_BlockStrength(const FGameplayAttributeData& OldAmount) const;


	UFUNCTION()
	void OnRep_MaxCorruptionResistance(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_MaxEarthResistance(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_MaxFireResistance(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_MaxIceResistance(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_MaxLightResistance(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_MaxLightningResistance(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_MaxWindResistance(const FGameplayAttributeData& OldAmount) const;

	
	//Piercing
	UFUNCTION()
	void OnRep_ArmourPiercing(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_FirePiercing(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_EarthPiercing(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_LightPiercing(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_LightningPiercing(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_CorruptionPiercing(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_IcePiercing(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_WindPiercing(const FGameplayAttributeData& OldAmount) const;


	
	
	// Misc Attributes
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


	// Chance To Apply Ailments
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


	//Duration
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

	static void SetEffectProperties(const FGameplayEffectModCallbackData& Data, FEffectProperties& Props);
	
	void SetAttributeValue(const FGameplayAttribute& Attribute, float NewValue);
};
