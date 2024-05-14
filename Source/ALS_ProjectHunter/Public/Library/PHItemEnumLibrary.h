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


