#pragma once

#include "CoreMinimal.h"
#include "PHItemEnumLibrary.generated.h"

UENUM(BlueprintType)
enum class EItemType : uint8
{
	IS_None UMETA(DisplayName = "None"),
	IS_Armor UMETA(DisplayName = "Armor"),
	IS_Weapon UMETA(DisplayName = "Weapon"),
	IS_Shield UMETA(DisplayName = "Shield"),
	IS_Consumable UMETA(DisplayName = "Consumable"),
	IS_QuestItem UMETA(DisplayName = "Quest Item"),
	IS_Ingredient UMETA(DisplayName = "Ingredient"),
	IS_Currency UMETA(DisplayName = "Currency"),
	IS_OtherRecipe UMETA(DisplayName = "Other Recipe"),
	IS_Other UMETA(DisplayName = "Other")

};


UENUM(BlueprintType)
enum class EItemSubType : uint8
{
	IST_None UMETA(DisplayName = "None"),
	IST_Other UMETA(DisplayName = "None"),
	IST_LongSword UMETA(DisplayName = "Long Sword"),
	IST_WarHammer UMETA(DisplayName = "WarHammer"),
	IST_Katana UMETA(DisplayName = "Katana"),
	IST_GreatSword UMETA(DisplayName = "Great Sword"),
	IST_Spear UMETA(DisplayName = "Spear"),
	IST_Claws UMETA(DisplayName = "Claws"),
	IST_Fist UMETA(DisplayName = "Fist"),
	IST_Bow UMETA(DisplayName = "Bow"),
	IST_Staff UMETA(DisplayName = "Staff"),
	IST_Rapier UMETA(DisplayName = "Rapier"),
	IST_Potion  UMETA(DisplayName = "Potion"),
};


UENUM(BlueprintType)
enum class EEquipmentSlot : uint8
{
	ES_None UMETA(DisplayName = "None"),
	ES_Head UMETA(DisplayName = "Head"),
	ES_Gloves UMETA(DisplayName = "Gloves"),
	ES_Neck UMETA(DisplayName = "Neck"),
	ES_Chestplate UMETA(DisplayName = "Chestplate"),
	ES_Legs UMETA(DisplayName = "Legs"),
	ES_Boots UMETA(DisplayName = "Boots"),
	ES_MainHand UMETA(DisplayName = "MainHandSocket"),
	ES_OffHand UMETA(DisplayName = "OffHandSocket"),
	ES_Ring UMETA(DisplayName = "Ring"),
	ES_Flask UMETA(DisplayName = "Flask"),
	ES_Belt  UMETA(DisplayName = "Belt"),
	ES_Cloak UMETA(DisplayName = "Cloak"),
};

UENUM(BlueprintType)
enum class EItemRarity : uint8
{
	IR_None UMETA(DisplayName = "None"),
	IR_GradeF UMETA(DisplayName = "Grade F"),
	IR_GradeD UMETA(DisplayName = "Grade D"),
	IR_GradeC UMETA(DisplayName = "Grade C"),
	IR_GradeB UMETA(DisplayName = "Grade B"),
	IR_GradeA UMETA(DisplayName = "Grade A"),
	IR_GradeS UMETA(DisplayName = "Grade S"),
	IR_Unkown UMETA(DisplayName = "Unkown"),
	IR_Corrupted UMETA(DisplayName = "$#@$%!$%")

};


UENUM(BlueprintType)
enum class EWeaponHandle : uint8
{
	WH_None UMETA(DisplayName = "None"),
	WH_OneHanded UMETA(DisplayName = "One-Hand"),
	WH_TwoHanded UMETA(DisplayName = "Two-Hand"),

};


UENUM(BlueprintType)
enum class ECurrentItemSlot : uint8
{
	CIS_None UMETA(DisplayName = "None"),
	CIS_Inventory UMETA(DisplayName = "Inventory"),
	CIS_MainHand  UMETA(DisplayName = "Main-Hand"),
	CIS_OffHand UMETA(DisplayName = "Off-Hand"),
	CIS_Head UMETA(DisplayName = "Head"),
	CIS_Gloves UMETA(DisplayName = "Gloves"),
	CIS_Neck UMETA(DisplayName = "Neck"),
	CIS_Chestplate UMETA(DisplayName = "Chestplate"),
	CIS_Legs UMETA(DisplayName = "Legs"),
	CIS_Boots UMETA(DisplayName = "Boots"),
	CIS_Ring1 UMETA(DisplayName = "Ring 1"),
	CIS_Ring2 UMETA(DisplayName = "Ring 2"),
	CIS_Ring3 UMETA(DisplayName = "Ring 3"),
	CIS_Ring4 UMETA(DisplayName = "Ring 4"),
	CIS_Ring5 UMETA(DisplayName = "Ring 5"),
	CIS_Ring6 UMETA(DisplayName = "Ring 6"),
	CIS_Ring7 UMETA(DisplayName = "Ring 7"),
	CIS_Ring8 UMETA(DisplayName = "Ring 8"),
	CIS_Ring9 UMETA(DisplayName = "Ring 9"),
	CIS_Ring10 UMETA(DisplayName = "Ring 10"),
	CIS_Flask UMETA(DisplayName = "Flask"),
	CIS_Belt  UMETA(DisplayName = "Belt")
};


UENUM(BlueprintType)
enum class EItemStats : uint8
{
	IS_None UMETA(DisplayName = "None"),
	IS_LevelRequirement UMETA(DisplayName = "Level Requirement"),// Level Required to use may not be used 
	IS_AttackSpeed UMETA(DisplayName = "Attack Speed"), // Attack Speed of weapon 
	IS_CritChance UMETA(DispalayName = "Critical Chance"), // item base critical chance 
	IS_DamagePerSecond UMETA (DisplayName = "Damage Per Second"), 
	IS_ElementDamagePerSecond UMETA(DisplayName = "Element Damage Per Second"),
	IS_Armor UMETA(DisplayName = "Armor"),
	IS_ManaCost UMETA(DisplayName = "Mana Cost"),
	IS_CastTime UMETA(DisplayName = "CastTime"),
	IS_WeaponRange UMETA(DisplayName = "Weapon Range"),
	IS_StaminaCost UMETA(DisplayName = "Stamina Cost"),
};


UENUM(BlueprintType)
enum class EItemRequiredStatsCategory : uint8
{
	ISC_None UMETA(DisplayName = "None"),
	ISC_RequiredStrength UMETA(DisplayName = "Required Strength"),
	ISC_RequiredDexterity UMETA(DisplayName = "Required Dexterity"),
	ISC_RequiredIntelligence UMETA(DisplayName = "Required Intelligence"),
	ISC_RequiredEndurance UMETA(DisplayName = "Required Endurance"),
	ISC_RequiredAffliction UMETA(DisplayName = "Required Affliction"),
	ISC_RequiredLuck UMETA(DisplayName = "Required Luck"),
	ISC_RequiredCovenant UMETA(DisplayName = "Required Covenant"),

};

UENUM(BlueprintType)
enum class EDefenseTypes : uint8
{
	DT_None UMETA(Display = "None"),
	DT_Armor UMETA(Display = "Armor"),
	DT_ManaShield UMETA(Display = "Mana Shield"),
	DT_BlockReduction UMETA(Display = "Block Reduction"),

	// Elemental Resistances
	DT_DarkResistance UMETA(DisplayName = "Dark Resistance"),
	DT_EarthResistance UMETA(DisplayName = "Earth Resistance"),
	DT_FireResistance UMETA(DisplayName = "Fire Resistance"),
	DT_IceResistance UMETA(DisplayName = "Ice Resistance"),
	DT_LightResistance UMETA(DisplayName = "Light Resistance"),
	DT_LightningResistance UMETA(DisplayName = "Lightning Resistance"),
	DT_PoisonResistance UMETA(DisplayName = "Poison Resistance"),
	DT_WindResistance UMETA(DisplayName = "Wind Resistance"),

};


UENUM(BlueprintType)
enum class EDamageTypes : uint8
{
	DT_None UMETA(DisplayName = "None"),
	DT_Dark UMETA(DisplayName = "Dark"),
	DT_Earth UMETA(DisplayName = "Earth"),
	DT_Fire UMETA(DisplayName = "Fire"),
	DT_Ice UMETA(DisplayName = "Ice"),
	DT_Light UMETA(DisplayName = "Light"),
	DT_Lightning UMETA(DisplayName = "Lightning"),
	DT_Poison UMETA(DisplayName = "Poison"),
	DT_Wind UMETA(DisplayName = "Wind"),
	DT_Physical UMETA(DisplayName = "Physical")
};

UENUM(BlueprintType)
enum class EAttributeDisplayFormat : uint8
{
	//add more as needed 
	Additive          UMETA(DisplayName = "+{0} TO ATTRIBUTE"),        // "+10 TO STRENGTH"
	Percent          UMETA(DisplayName = "+{0}% TO ATTRIBUTE"),       // "+20% TO FIRE RESISTANCE"
	MinMax          UMETA(DisplayName = "ADD {0} TO {1} DAMAGE"),     // "ADD 5 TO 10 FIRE DAMAGE"
};

UENUM(BlueprintType)
enum class EPrefixSuffix
{
	Prefix        UMETA(DisplayName = "Preffix"),
	Suffix        UMETA(DisplayName = "Suffix"), 
};

UENUM(BlueprintType)
enum class ERankPoints : uint8
{
	RP_5  UMETA(DisplayName = "5"),
	RP_10 UMETA(DisplayName = "10"),
	RP_15 UMETA(DisplayName = "15"),
	RP_20 UMETA(DisplayName = "20"),
	RP_25 UMETA(DisplayName = "25"),
	RP_30 UMETA(DisplayName = "30")
};


