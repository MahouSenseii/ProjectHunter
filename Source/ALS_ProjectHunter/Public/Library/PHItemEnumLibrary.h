#pragma once

#include "CoreMinimal.h"
#include "PHItemEnumLibrary.generated.h"

UENUM(BlueprintType)
enum class EItemType : uint8
{
	IT_None         UMETA(DisplayName = "None"),

	/* === Equipment === */
	IT_Weapon       UMETA(DisplayName = "Weapon"),
	IT_Armor        UMETA(DisplayName = "Armor"),
	IT_Shield       UMETA(DisplayName = "Shield"),
	IT_Accessory    UMETA(DisplayName = "Accessory"), // Rings, Amulets, Belts

	/* === Consumables === */
	IT_Consumable   UMETA(DisplayName = "Consumable"), // Potions, Food
	IT_Flask        UMETA(DisplayName = "Flask"),      // Refillable effects

	/* === Progression === */
	IT_QuestItem    UMETA(DisplayName = "Quest Item"),
	IT_KeyItem      UMETA(DisplayName = "Key Item"),

	/* === Crafting & Economy === */
	IT_Ingredient   UMETA(DisplayName = "Ingredient"),
	IT_Currency     UMETA(DisplayName = "Currency"),
	IT_Recipe       UMETA(DisplayName = "Recipe"),

	IT_Other        UMETA(DisplayName = "Other")
};



UENUM(BlueprintType)
enum class EItemSubType : uint8
{
	IST_None UMETA(DisplayName = "None"),

	/* === Weapon Subtypes (for IS_Weapon) === */
	IST_Sword       UMETA(DisplayName = "Sword"),
	IST_GreatSword  UMETA(DisplayName = "Great Sword"),
	IST_Katana      UMETA(DisplayName = "Katana"),
	IST_Hammer      UMETA(DisplayName = "Warhammer"),
	IST_Axe         UMETA(DisplayName = "Axe"),
	IST_Bow         UMETA(DisplayName = "Bow"),
	IST_Staff       UMETA(DisplayName = "Staff"),
	IST_Wand        UMETA(DisplayName = "Wand"),
	IST_Dagger      UMETA(DisplayName = "Dagger"),
	IST_Fist        UMETA(DisplayName = "Fist Weapon"),

	/* === Armor Subtypes (for IS_Armor) === */
	IST_Helmet      UMETA(DisplayName = "Helmet"),
	IST_Chest       UMETA(DisplayName = "Chestplate"),
	IST_Gloves      UMETA(DisplayName = "Gloves"),
	IST_Legs        UMETA(DisplayName = "Leggings"),
	IST_Boots       UMETA(DisplayName = "Boots"),
	IST_Cloak       UMETA(DisplayName = "Cloak"),

	/* === Shield Subtypes (for IS_Shield) === */
	IST_Buckler     UMETA(DisplayName = "Buckler"),
	IST_TowerShield UMETA(DisplayName = "Tower Shield"),

	/* === Accessory Subtypes (for IS_Accessory) === */
	IST_Ring        UMETA(DisplayName = "Ring"),
	IST_Amulet      UMETA(DisplayName = "Amulet"),
	IST_Belt        UMETA(DisplayName = "Belt"),

	/* === Consumable Subtypes (for IS_Consumable/IS_Flask) === */
	IST_HealthPotion UMETA(DisplayName = "Health Potion"),
	IST_ManaPotion   UMETA(DisplayName = "Mana Potion"),
	IST_StaminaFood  UMETA(DisplayName = "Stamina Food"),
	IST_RefillableFlask UMETA(DisplayName = "Refillable Flask"),

	/* === Quest/Key Subtypes (for IS_QuestItem, IS_KeyItem) === */
	IST_Key         UMETA(DisplayName = "Key"),
	IST_QuestScroll UMETA(DisplayName = "Quest Scroll"),

	/* === Ingredient/Crafting Subtypes (for IS_Ingredient, IS_Recipe) === */
	IST_Herb        UMETA(DisplayName = "Herb"),
	IST_Metal       UMETA(DisplayName = "Metal"),
	IST_AlchemyBook UMETA(DisplayName = "Alchemy Recipe"),

	/* === Currency Subtypes (for IS_Currency) === */
	IST_Gold        UMETA(DisplayName = "Gold"),
	IST_Silver      UMETA(DisplayName = "Silver"),
	IST_GemShard    UMETA(DisplayName = "Gem Shard"),

	/* === Catch-All === */
	IST_Other       UMETA(DisplayName = "Other")
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
	ISC_RequiredLevel UMETA(DisplayName = "Required Level"),
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
	DT_FireResistance          UMETA(DisplayName = "FireResistance"),
	DT_IceResistance           UMETA(DisplayName = "IceResistance"),
	DT_LightResistance         UMETA(DisplayName = "LightResistance"),
	DT_LightningResistance     UMETA(DisplayName = "LightningResistance"),
	DT_PhysicalResistance      UMETA(DisplayName = "PhysicalResistance"),
	DT_PoisonResistance        UMETA(DisplayName = "CorruptionResistance"),
};


UENUM(BlueprintType)
enum class EDamageTypes : uint8
{
	DT_None         UMETA(DisplayName = "None"),

	DT_Fire         UMETA(DisplayName = "Fire"),
	DT_Ice          UMETA(DisplayName = "Ice"),
	DT_Light        UMETA(DisplayName = "Light"),
	DT_Lightning    UMETA(DisplayName = "Lightning"),
	DT_Physical     UMETA(DisplayName = "Physical"),
	DT_Corruption   UMETA(DisplayName = "Corruption"),

};


UENUM(BlueprintType)
enum class EAttributeDisplayFormat : uint8
{
	//add more as needed 
	Additive UMETA(DisplayName = "+{0} TO ATTRIBUTE"),        // "+10 TO STRENGTH"
	FlatNegative UMETA(DisplayName = "-{0} {1}"),			 //"-10% Movement Speed"	
	Percent UMETA(DisplayName = "+{0}% TO ATTRIBUTE"),       // "+20% TO FIRE RESISTANCE"
	MinMax UMETA(DisplayName = "ADD {0} TO {1} DAMAGE"),     // "ADD 5 TO 10 FIRE DAMAGE"
	Increase UMETA(DisplayName = "{0}% increased {1}"),		//" 30% increased Attack Speed "
	More UMETA(DisplayName = "{0}% more {1}"),				//"40% more Projectile Damage"
	Less UMETA(DisplayName = "{0}% less {1}"),				//"25% less Area of Effect"
	Chance UMETA(DisplayName = "{0}% chance to {1}"),		//"15% chance to Gain Frenzy Charge on Crit"
	Duration UMETA(DisplayName = "{0}s duration to {1}"),	// "5s duration to Freeze"
	Cooldown UMETA(DisplayName = "{0}s cooldown on {1}"),	//"1.5s cooldown on Lightning Warp"
	CustomText UMETA(DisplayName = "Custom Format"),		//Custom no examples 
};

UENUM(BlueprintType)
enum class EPrefixSuffix
{
	Prefix,
	Suffix,
	Implicit,
	Enchant,
	Corrupted 
};

UENUM(BlueprintType)
enum class ERankPoints : uint8
{
	RP_Neg30	UMETA(DisplayName = "-30"),
	RP_Neg25	UMETA(DisplayName = "-25"),
	RP_Neg20	UMETA(DisplayName = "-20"),
	RP_Neg15	UMETA(DisplayName = "-15"),
	RP_Neg10	UMETA(DisplayName = "-10"),
	RP_Neg5		UMETA(DisplayName = "-5"),
	RP_0		UMETA(DisplayName = "0"),
	RP_5		UMETA(DisplayName = "5"),
	RP_10		UMETA(DisplayName = "10"),
	RP_15		UMETA(DisplayName = "15"),
	RP_20		UMETA(DisplayName = "20"),
	RP_25		UMETA(DisplayName = "25"),
	RP_30		UMETA(DisplayName = "30")
};


UENUM(BlueprintType)
enum class EAffixOrigin : uint8
{
	Generated,
	Crafted,
	Implicit,
	Corrupted,
};
