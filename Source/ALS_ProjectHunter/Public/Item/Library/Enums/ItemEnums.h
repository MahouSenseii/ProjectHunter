// Item/Library/Enums/ItemEnums.h
#pragma once

#include "CoreMinimal.h"
#include "Equipment/Library/Enums/EquipmentEnums.h"
#include "ItemEnums.generated.h"


/**
 * Item Type Categories
 */
UENUM(BlueprintType)
enum class EItemType : uint8
{
	IT_None         UMETA(DisplayName = "None"),
	IT_Weapon       UMETA(DisplayName = "Weapon"),
	IT_Armor        UMETA(DisplayName = "Armor"),
	IT_Accessory    UMETA(DisplayName = "Accessory"),
	IT_Consumable   UMETA(DisplayName = "Consumable"),
	IT_Material     UMETA(DisplayName = "Material"),
	IT_Currency     UMETA(DisplayName = "Currency"),
	IT_Quest        UMETA(DisplayName = "Quest"),
	IT_Key          UMETA(DisplayName = "Key"),
};

/**
 * Item Sub-Types (Weapons, Armor pieces, etc.)
 */
UENUM(BlueprintType)
enum class EItemSubType : uint8
{
	IST_None            UMETA(DisplayName = "None"),

	IST_Sword           UMETA(DisplayName = "Sword"),
	IST_Katana          UMETA(DisplayName = "Katana"),
	IST_Greatsword      UMETA(DisplayName = "Greatsword"),
	IST_Dagger          UMETA(DisplayName = "Dagger"),
	IST_Axe             UMETA(DisplayName = "Axe"),
	IST_Mace            UMETA(DisplayName = "Mace"),
	IST_Spear           UMETA(DisplayName = "Spear"),
	IST_Bow             UMETA(DisplayName = "Bow"),
	IST_Crossbow        UMETA(DisplayName = "Crossbow"),
	IST_Staff           UMETA(DisplayName = "Staff"),
	IST_Wand            UMETA(DisplayName = "Wand"),
	IST_Shield          UMETA(DisplayName = "Shield"),

	IST_Helmet          UMETA(DisplayName = "Helmet"),
	IST_Chest           UMETA(DisplayName = "Chest"),
	IST_Gloves          UMETA(DisplayName = "Gloves"),
	IST_Boots           UMETA(DisplayName = "Boots"),
	IST_Legs            UMETA(DisplayName = "Legs"),

	IST_Ring            UMETA(DisplayName = "Ring"),
	IST_Amulet          UMETA(DisplayName = "Amulet"),
	IST_Belt            UMETA(DisplayName = "Belt"),

	IST_Potion          UMETA(DisplayName = "Potion"),
	IST_Scroll          UMETA(DisplayName = "Scroll"),
	IST_Food            UMETA(DisplayName = "Food"),

};

/** Item rarity grades and their default affix-count bands. */
UENUM(BlueprintType)
enum class EItemRarity : uint8
{
	IR_None         UMETA(DisplayName = "None"),
	IR_GradeF       UMETA(DisplayName = "Grade F (Common)"),
	IR_GradeE       UMETA(DisplayName = "Grade E (Uncommon)"),
	IR_GradeD       UMETA(DisplayName = "Grade D (Rare)"),
	IR_GradeC       UMETA(DisplayName = "Grade C (Elite)"),
	IR_GradeB       UMETA(DisplayName = "Grade B (Named)"),
	IR_GradeA       UMETA(DisplayName = "Grade A (Legendary)"),
	IR_GradeS       UMETA(DisplayName = "Grade S (Mythic)"),
	IR_GradeSS      UMETA(DisplayName = "Grade SS (EX-Rank)"),
	IR_Unknown      UMETA(DisplayName = "Unknown (Unidentified)"),
	IR_Corrupted    UMETA(DisplayName = "Corrupted (Chaos)"),
};

/**
 * Current slot where item is stored
 */
UENUM(BlueprintType)
enum class ECurrentItemSlot : uint8
{
	CIS_None        UMETA(DisplayName = "None"),
	CIS_Inventory   UMETA(DisplayName = "Inventory"),
	CIS_Equipment   UMETA(DisplayName = "Equipment"),
	CIS_Stash       UMETA(DisplayName = "Stash"),
	CIS_Vendor      UMETA(DisplayName = "Vendor"),
	CIS_Ground      UMETA(DisplayName = "Ground"),
};

/**
 * Weapon Handle Type
 */
UENUM(BlueprintType)
enum class EWeaponHandle : uint8
{
	WH_None         UMETA(DisplayName = "None"),
	WH_OneHanded    UMETA(DisplayName = "One-Handed"),
	WH_TwoHanded    UMETA(DisplayName = "Two-Handed"),
	WH_DualWield    UMETA(DisplayName = "Dual Wield"),
};

/** Physical, elemental, and special damage types. */
UENUM(BlueprintType)
enum class EDamageType : uint8
{
	DT_None             UMETA(DisplayName = "None"),
	DT_Physical         UMETA(DisplayName = "Physical"),
	DT_Fire             UMETA(DisplayName = "Fire"),
	DT_Ice              UMETA(DisplayName = "Ice"),
	DT_Lightning        UMETA(DisplayName = "Lightning"),
	DT_Light            UMETA(DisplayName = "Light"),           // Holy/Divine damage
	DT_Corruption       UMETA(DisplayName = "Corruption"),      // Chaos/Shadow damage
	DT_True             UMETA(DisplayName = "True Damage")      // Ignores all resistance
};

/**
 * Defense/Resistance Types
 */
UENUM(BlueprintType)
enum class EDefenseType : uint8
{
	DFT_None                    UMETA(DisplayName = "None"),
	DFT_Armor                   UMETA(DisplayName = "Armor"),
	DFT_FireResistance          UMETA(DisplayName = "Fire Resistance"),
	DFT_IceResistance           UMETA(DisplayName = "Ice Resistance"),
	DFT_LightningResistance     UMETA(DisplayName = "Lightning Resistance"),
	DFT_LightResistance         UMETA(DisplayName = "Light Resistance"),
	DFT_CorruptionResistance    UMETA(DisplayName = "Corruption Resistance"),
};

/** Item stat requirement category. */
UENUM(BlueprintType)
enum class EItemRequiredStatsCategory : uint8
{
	IRSC_Strength       UMETA(DisplayName = "Strength"),
	IRSC_Dexterity      UMETA(DisplayName = "Dexterity"),
	IRSC_Intelligence   UMETA(DisplayName = "Intelligence"),
	IRSC_Endurance      UMETA(DisplayName = "Endurance"),
	IRSC_Affliction     UMETA(DisplayName = "Affliction"),
	IRSC_Luck           UMETA(DisplayName = "Luck"),
	IRSC_Covenant       UMETA(DisplayName = "Covenant"),
};

/**
 * Item comparison result
 */
UENUM(BlueprintType)
enum class EItemComparisonResult : uint8
{
	ICR_Better          UMETA(DisplayName = "Better"),
	ICR_Equal           UMETA(DisplayName = "Equal"),
	ICR_Worse           UMETA(DisplayName = "Worse"),
	ICR_Incomparable    UMETA(DisplayName = "Incomparable"),
};

/**
 * Attachment Rule for item equipment
 */
UENUM(BlueprintType)
enum class EPHAttachmentRule : uint8
{
	AR_KeepRelative     UMETA(DisplayName = "Keep Relative"),
	AR_KeepWorld        UMETA(DisplayName = "Keep World"),
	AR_SnapToTarget     UMETA(DisplayName = "Snap To Target"),
};
