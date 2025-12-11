#pragma once

#include "CoreMinimal.h"
#include "PHItemEnumLibrary.generated.h"


/* ============================= */
/* ===   ITEM CLASSIFICATION === */
/* ============================= */

UENUM(BlueprintType)
enum class EItemType : uint8
{
	IT_None         UMETA(DisplayName = "None"),

	/* Equipment */
	IT_Weapon       UMETA(DisplayName = "Weapon"),
	IT_Armor        UMETA(DisplayName = "Armor"),
	IT_Shield       UMETA(DisplayName = "Shield"),
	IT_Accessory    UMETA(DisplayName = "Accessory"),

	/* Consumables */
	IT_Consumable   UMETA(DisplayName = "Consumable"),
	IT_Flask        UMETA(DisplayName = "Flask"),

	/* Progression */
	IT_QuestItem    UMETA(DisplayName = "Quest Item"),
	IT_KeyItem      UMETA(DisplayName = "Key Item"),

	/* Crafting & Economy */
	IT_Ingredient   UMETA(DisplayName = "Ingredient"),
	IT_Currency     UMETA(DisplayName = "Currency"),
	IT_Recipe       UMETA(DisplayName = "Recipe"),

	/* Catch-All */
	IT_Other        UMETA(DisplayName = "Other")
};

UENUM(BlueprintType)
enum class EItemSubType : uint8
{
	IST_None UMETA(DisplayName = "None"),

	/* Weapons */
	IST_Sword        UMETA(DisplayName = "Sword"),
	IST_GreatSword   UMETA(DisplayName = "Great Sword"),
	IST_Katana       UMETA(DisplayName = "Katana"),
	IST_Hammer       UMETA(DisplayName = "Warhammer"),
	IST_Axe          UMETA(DisplayName = "Axe"),
	IST_Bow          UMETA(DisplayName = "Bow"),
	IST_Staff        UMETA(DisplayName = "Staff"),
	IST_Wand         UMETA(DisplayName = "Wand"),
	IST_Dagger       UMETA(DisplayName = "Dagger"),
	IST_Fist         UMETA(DisplayName = "Fist Weapon"),

	/* Armor */
	IST_Helmet       UMETA(DisplayName = "Helmet"),
	IST_Chest        UMETA(DisplayName = "Chestplate"),
	IST_Gloves       UMETA(DisplayName = "Gloves"),
	IST_Legs         UMETA(DisplayName = "Leggings"),
	IST_Boots        UMETA(DisplayName = "Boots"),
	IST_Cloak        UMETA(DisplayName = "Cloak"),

	/* Shields */
	IST_Buckler      UMETA(DisplayName = "Buckler"),
	IST_TowerShield  UMETA(DisplayName = "Tower Shield"),

	/* Accessories */
	IST_Ring         UMETA(DisplayName = "Ring"),
	IST_Amulet       UMETA(DisplayName = "Amulet"),
	IST_Belt         UMETA(DisplayName = "Belt"),

	/* Consumables */
	IST_HealthPotion UMETA(DisplayName = "Health Potion"),
	IST_ManaPotion   UMETA(DisplayName = "Mana Potion"),
	IST_StaminaFood  UMETA(DisplayName = "Stamina Food"),
	IST_RefillableFlask UMETA(DisplayName = "Refillable Flask"),

	/* Quest / Key Items */
	IST_Key          UMETA(DisplayName = "Key"),
	IST_QuestScroll  UMETA(DisplayName = "Quest Scroll"),

	/* Crafting */
	IST_Herb         UMETA(DisplayName = "Herb"),
	IST_Metal        UMETA(DisplayName = "Metal"),
	IST_AlchemyBook  UMETA(DisplayName = "Alchemy Recipe"),

	/* Currency */
	IST_Gold         UMETA(DisplayName = "Gold"),
	IST_Silver       UMETA(DisplayName = "Silver"),
	IST_GemShard     UMETA(DisplayName = "Gem Shard"),

	/* Catch-All */
	IST_Other        UMETA(DisplayName = "Other")
};

UENUM(BlueprintType)
enum class EItemRarity : uint8
{
	IR_None      UMETA(DisplayName = "None"),
	IR_GradeF    UMETA(DisplayName = "Grade F"),
	IR_GradeD    UMETA(DisplayName = "Grade D"),
	IR_GradeC    UMETA(DisplayName = "Grade C"),
	IR_GradeB    UMETA(DisplayName = "Grade B"),
	IR_GradeA    UMETA(DisplayName = "Grade A"),
	IR_GradeS    UMETA(DisplayName = "Grade S"),
	IR_Unknown   UMETA(DisplayName = "Unknown"),
	IR_Corrupted UMETA(DisplayName = "Corrupted"),
};

/* ============================= */
/* ===   EQUIPMENT / SLOTS   === */
/* ============================= */

UENUM(BlueprintType)
enum class EEquipmentSlot : uint8
{
	ES_None        UMETA(DisplayName = "None"),
	ES_Head        UMETA(DisplayName = "Head"),
	ES_Gloves      UMETA(DisplayName = "Gloves"),
	ES_Neck        UMETA(DisplayName = "Neck"),
	ES_Chestplate  UMETA(DisplayName = "Chestplate"),
	ES_Legs        UMETA(DisplayName = "Legs"),
	ES_Boots       UMETA(DisplayName = "Boots"),
	ES_MainHand    UMETA(DisplayName = "Main Hand"),
	ES_OffHand     UMETA(DisplayName = "Off Hand"),
	ES_Ring        UMETA(DisplayName = "Ring"),
	ES_Flask       UMETA(DisplayName = "Flask"),
	ES_Belt        UMETA(DisplayName = "Belt"),
	ES_Cloak       UMETA(DisplayName = "Cloak"),
};

UENUM(BlueprintType)
enum class EWeaponHandle : uint8
{
	WH_None       UMETA(DisplayName = "None"),
	WH_OneHanded  UMETA(DisplayName = "One-Hand"),
	WH_TwoHanded  UMETA(DisplayName = "Two-Hand"),
};

UENUM(BlueprintType)
enum class ECurrentItemSlot : uint8
{
	CIS_None        UMETA(DisplayName = "None"),
	CIS_Inventory   UMETA(DisplayName = "Inventory"),

	/* Equipment */
	CIS_MainHand    UMETA(DisplayName = "Main-Hand"),
	CIS_OffHand     UMETA(DisplayName = "Off-Hand"),
	CIS_Head        UMETA(DisplayName = "Head"),
	CIS_Gloves      UMETA(DisplayName = "Gloves"),
	CIS_Neck        UMETA(DisplayName = "Neck"),
	CIS_Chestplate  UMETA(DisplayName = "Chestplate"),
	CIS_Legs        UMETA(DisplayName = "Legs"),
	CIS_Boots       UMETA(DisplayName = "Boots"),
	CIS_Belt        UMETA(DisplayName = "Belt"),
	CIS_Cloak       UMETA(DisplayName = "Cloak"),

	/* Rings (dynamic, indexed externally) */
	CIS_Ring        UMETA(DisplayName = "Ring"),

	/* Utility */
	CIS_Flask       UMETA(DisplayName = "Flask"),
};

/* ============================= */
/* ===   ITEM META           === */
/* ============================= */

UENUM(BlueprintType)
enum class EItemStats : uint8
{
	IS_None                UMETA(DisplayName = "None"),
	IS_LevelRequirement    UMETA(DisplayName = "Level Requirement"),
	IS_AttackSpeed         UMETA(DisplayName = "Attack Speed"),
	IS_CritChance          UMETA(DisplayName = "Critical Chance"),
	IS_DamagePerSecond     UMETA(DisplayName = "Damage Per Second"),
	IS_ElementDamagePerSec UMETA(DisplayName = "Element Damage Per Second"),
	IS_Armor               UMETA(DisplayName = "Armor"),
	IS_ManaCost            UMETA(DisplayName = "Mana Cost"),
	IS_CastTime            UMETA(DisplayName = "Cast Time"),
	IS_WeaponRange         UMETA(DisplayName = "Weapon Range"),
	IS_StaminaCost         UMETA(DisplayName = "Stamina Cost"),
};

UENUM(BlueprintType)
enum class EItemRequiredStatsCategory : uint8
{
	ISC_None              UMETA(DisplayName = "None"),
	ISC_RequiredLevel     UMETA(DisplayName = "Required Level"),
	ISC_RequiredStrength  UMETA(DisplayName = "Required Strength"),
	ISC_RequiredDexterity UMETA(DisplayName = "Required Dexterity"),
	ISC_RequiredIntelligence UMETA(DisplayName = "Required Intelligence"),
	ISC_RequiredEndurance UMETA(DisplayName = "Required Endurance"),
	ISC_RequiredAffliction UMETA(DisplayName = "Required Affliction"),
	ISC_RequiredLuck      UMETA(DisplayName = "Required Luck"),
	ISC_RequiredCovenant  UMETA(DisplayName = "Required Covenant"),
};

/* ============================= */
/* ===   COMBAT MECHANICS    === */
/* ============================= */

UENUM(BlueprintType)
enum class EDefenseTypes : uint8
{
	DFT_None          UMETA(DisplayName = "None"),
	DFT_Armor         UMETA(DisplayName = "Armor"),
	DFT_ManaShield    UMETA(DisplayName = "Mana Shield"),
	DFT_BlockReduction UMETA(DisplayName = "Block Reduction"),

	/* Elemental Resistances */
	DFT_FireResistance      UMETA(DisplayName = "Fire Resistance"),
	DFT_IceResistance       UMETA(DisplayName = "Ice Resistance"),
	DFT_LightResistance     UMETA(DisplayName = "Light Resistance"),
	DFT_LightningResistance UMETA(DisplayName = "Lightning Resistance"),
	DFT_PhysicalResistance  UMETA(DisplayName = "Physical Resistance"),
	DFT_CorruptionResistance    UMETA(DisplayName = "Corruption Resistance"),
};

UENUM(BlueprintType)
enum class EDamageTypes : uint8
{
	DT_None      UMETA(DisplayName = "None"),
	DT_Fire      UMETA(DisplayName = "Fire"),
	DT_Ice       UMETA(DisplayName = "Ice"),
	DT_Light     UMETA(DisplayName = "Light"),
	DT_Lightning UMETA(DisplayName = "Lightning"),
	DT_Physical  UMETA(DisplayName = "Physical"),
	DT_Poison    UMETA(DisplayName = "Poison"),
	DT_Corruption UMETA(DisplayName = "Corruption"),
};

/* ============================= */
/* ===   AFFIXES             === */
/* ============================= */

UENUM(BlueprintType)
enum class EAffixes : uint8
{
	Prefix UMETA(DisplayName = "Prefix"),
	Suffix UMETA(DisplayName = "Suffix"),
	Implicit UMETA(DisplayName = "Implicit"),
};

/* ============================= */
/* ===   COMPARISON          === */
/* ============================= */

UENUM(BlueprintType)
enum class EItemComparisonResult : uint8
{
	Better        UMETA(DisplayName = "Better"),
	Worse         UMETA(DisplayName = "Worse"),
	Equal         UMETA(DisplayName = "Equal"),
	Incomparable  UMETA(DisplayName = "Incomparable"),
};

/* ============================= */
/* ===   ATTACHMENT          === */
/* ============================= */

UENUM(BlueprintType)
enum class EPHAttachmentRule : uint8
{
	KeepRelative  UMETA(DisplayName="Keep Relative"),
	KeepWorld     UMETA(DisplayName="Keep World"),
	SnapToTarget  UMETA(DisplayName="Snap To Target"),
};

/* ============================= */
/* ===   EFFECT TRACKING     === */
/* ============================= */

UENUM(BlueprintType)
enum class EEffectSource : uint8
{
	None            UMETA(DisplayName = "None"),
	ItemPassive     UMETA(DisplayName = "Item Passive"),
	ItemRune        UMETA(DisplayName = "Item Rune"),
	ItemEnchant     UMETA(DisplayName = "Item Enchantment"),
	CharPassive     UMETA(DisplayName = "Character Passive"),
	TempBuff        UMETA(DisplayName = "Temporary Buff"),
	StatusEffect    UMETA(DisplayName = "Status Effect"),
};

/* ============================= */
/* ===   HELPER FUNCTIONS    === */
/* ============================= */

FORCEINLINE EAttachmentRule ToEngineRule(EPHAttachmentRule R)
{
	switch (R)
	{
	case EPHAttachmentRule::KeepRelative: return EAttachmentRule::KeepRelative;
	case EPHAttachmentRule::KeepWorld:     return EAttachmentRule::KeepWorld;
	case EPHAttachmentRule::SnapToTarget:  return EAttachmentRule::SnapToTarget;
	default: return EAttachmentRule::KeepRelative;
	}
}

/* ============================= */
/* ===   ATTRIBUTE DISPLAY   === */
/* ============================= */

UENUM(BlueprintType)
enum class EAttributeDisplayFormat : uint8
{
	Additive       UMETA(DisplayName = "Additive (+X Stat)"),
	FlatNegative   UMETA(DisplayName = "Flat Negative (-X Stat)"),
	Percent        UMETA(DisplayName = "Percent (+X% Stat)"),
	MinMax         UMETA(DisplayName = "Min-Max Range (Adds X-Y Stat)"),
	Increase       UMETA(DisplayName = "Increase (X% increased Stat)"),
	More           UMETA(DisplayName = "More (X% more Stat)"),
	Less           UMETA(DisplayName = "Less (X% less Stat)"),
	Chance         UMETA(DisplayName = "Chance (X% chance to Stat)"),
	Duration       UMETA(DisplayName = "Duration (Xs duration to Stat)"),
	Cooldown       UMETA(DisplayName = "Cooldown (Xs cooldown on Stat)"),
	CustomText     UMETA(DisplayName = "Custom Text"),
};

/* ============================= */
/* ===   RANK POINTS         === */
/* ============================= */

UENUM(BlueprintType)
enum class ERankPoints : uint8
{
	RP_Neg30   UMETA(DisplayName = "-30"),
	RP_Neg25   UMETA(DisplayName = "-25"),
	RP_Neg20   UMETA(DisplayName = "-20"),
	RP_Neg15   UMETA(DisplayName = "-15"),
	RP_Neg10   UMETA(DisplayName = "-10"),
	RP_Neg5    UMETA(DisplayName = "-5"),
	RP_0       UMETA(DisplayName = "0"),
	RP_5       UMETA(DisplayName = "+5"),
	RP_10      UMETA(DisplayName = "+10"),
	RP_15      UMETA(DisplayName = "+15"),
	RP_20      UMETA(DisplayName = "+20"),
	RP_25      UMETA(DisplayName = "+25"),
	RP_30      UMETA(DisplayName = "+30"),
	RP_35       UMETA(DisplayName = "+35"),
	RP_40       UMETA(DisplayName = "+40"),
	RP_45     UMETA(DisplayName = "+45"),
	RP_50      UMETA(DisplayName = "+50"),
	RP_55      UMETA(DisplayName = "+55"),
	RP_60      UMETA(DisplayName = "+60"),
	RP_65      UMETA(DisplayName = "+65"),
	RP_70      UMETA(DisplayName = "+70"),
	RP_75      UMETA(DisplayName = "+75"),
	RP_80     UMETA(DisplayName = "+80"),
	RP_85      UMETA(DisplayName = "+85"),
	RP_90      UMETA(DisplayName = "+90"),
	RP_95       UMETA(DisplayName = "+95"),
	RP_100       UMETA(DisplayName = "+100"),
};