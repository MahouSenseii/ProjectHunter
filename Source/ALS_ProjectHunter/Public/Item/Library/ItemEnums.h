// Item/Library/ItemEnums.h
#pragma once

#include "CoreMinimal.h"
#include "ItemEnums.generated.h"

// ═══════════════════════════════════════════════════════════════════════
// ITEM CORE ENUMS - Solo Leveling / ORV Hunter Manga Style
// ═══════════════════════════════════════════════════════════════════════

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

FORCEINLINE bool IsValidItemType(const EItemType Type)
{
	return Type != EItemType::IT_None;
}

/**
 * Item Sub-Types (Weapons, Armor pieces, etc.)
 */
UENUM(BlueprintType)
enum class EItemSubType : uint8
{
	IST_None            UMETA(DisplayName = "None"),
	
	// Weapons
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
	
	// Armor
	IST_Helmet          UMETA(DisplayName = "Helmet"),
	IST_Chest           UMETA(DisplayName = "Chest"),
	IST_Gloves          UMETA(DisplayName = "Gloves"),
	IST_Boots           UMETA(DisplayName = "Boots"),
	IST_Legs            UMETA(DisplayName = "Legs"),
	
	// Accessories
	IST_Ring            UMETA(DisplayName = "Ring"),
	IST_Amulet          UMETA(DisplayName = "Amulet"),
	IST_Belt            UMETA(DisplayName = "Belt"),
	
	// Consumables
	IST_Potion          UMETA(DisplayName = "Potion"),
	IST_Scroll          UMETA(DisplayName = "Scroll"),
	IST_Food            UMETA(DisplayName = "Food"),
	
};

FORCEINLINE bool IsValidItemSubType(const EItemSubType SubType)
{
	return SubType != EItemSubType::IST_None;
}

/**
 * Item Rarity Grades - Solo Leveling / ORV Style (F-SS Grade System)
 * Hunter Manga progression system
 * 
 * Grade F: Common Hunter gear (0 affixes)
 * Grade E: Uncommon drops (1-2 affixes)
 * Grade D: Rare drops (2-3 affixes)
 * Grade C: Elite equipment (3-4 affixes)
 * Grade B: Named items (4-5 affixes)
 * Grade A: Legendary artifacts (5-6 affixes)
 * Grade S: Mythic/National Treasure (6 affixes max)
 * Grade SS: EX-Rank items (fixed unique affixes)
 */
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
 * Equipment Slots - Hunter Manga Style
 * Multiple ring slots for accessory focus
 */
UENUM(BlueprintType)
enum class EEquipmentSlot : uint8
{
	ES_None         UMETA(DisplayName = "None"),
	ES_MainHand     UMETA(DisplayName = "Main Hand"),
	ES_OffHand      UMETA(DisplayName = "Off Hand"),
	ES_TwoHand      UMETA(DisplayName = "Two Hand"),
	ES_Head         UMETA(DisplayName = "Head"),
	ES_Chest        UMETA(DisplayName = "Chest"),
	ES_Hands        UMETA(DisplayName = "Hands"),
	ES_Legs         UMETA(DisplayName = "Legs"),
	ES_Feet         UMETA(DisplayName = "Feet"),
	ES_Ring1        UMETA(DisplayName = "Ring 1"),
	ES_Ring2        UMETA(DisplayName = "Ring 2"),
	ES_Ring3        UMETA(DisplayName = "Ring 3"),
	ES_Ring4        UMETA(DisplayName = "Ring 4"),
	ES_Ring5        UMETA(DisplayName = "Ring 5"),
	ES_Ring6        UMETA(DisplayName = "Ring 6"),
	ES_Ring7        UMETA(DisplayName = "Ring 7"),
	ES_Ring8        UMETA(DisplayName = "Ring 8"),
	ES_Ring9        UMETA(DisplayName = "Ring 9"),
	ES_Ring10       UMETA(DisplayName = "Ring 10"),
	ES_Amulet       UMETA(DisplayName = "Amulet"),
	ES_Belt         UMETA(DisplayName = "Belt"),
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

/**
 * Damage Types - Hunter Manga Style
 * Physical + Elemental (Fire, Ice, Lightning) + Special (Light, Corruption)
 */
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

/**
 * Item stat requirement category - Hunter Stats
 */
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

// ═══════════════════════════════════════════════════════════════════════
// HELPER FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════

/** Get color for item rarity (Solo Leveling style) */
FORCEINLINE FLinearColor GetItemRarityColor(EItemRarity Rarity)
{
	switch (Rarity)
	{
		case EItemRarity::IR_GradeF:      return FLinearColor(0.5f, 0.5f, 0.5f);  // Gray
		case EItemRarity::IR_GradeE:      return FLinearColor::White;              // White
		case EItemRarity::IR_GradeD:      return FLinearColor(0.3f, 0.9f, 0.3f);  // Green
		case EItemRarity::IR_GradeC:      return FLinearColor(0.4f, 0.6f, 1.0f);  // Blue
		case EItemRarity::IR_GradeB:      return FLinearColor(0.7f, 0.3f, 0.9f);  // Purple
		case EItemRarity::IR_GradeA:      return FLinearColor(1.0f, 0.7f, 0.0f);  // Gold
		case EItemRarity::IR_GradeS:      return FLinearColor(1.0f, 0.3f, 0.0f);  // Orange/Red
		case EItemRarity::IR_GradeSS:     return FLinearColor(1.0f, 0.0f, 0.0f);  // Bright Red
		case EItemRarity::IR_Unknown:     return FLinearColor(0.3f, 0.3f, 0.3f);  // Dark Gray
		case EItemRarity::IR_Corrupted:   return FLinearColor(0.2f, 0.0f, 0.2f);  // Dark Purple
		default:                           return FLinearColor::White;
	}
}

/** Helper function to convert attachment rule */
FORCEINLINE EAttachmentRule ToEngineRule(EPHAttachmentRule Rule)
{
	switch (Rule)
	{
		case EPHAttachmentRule::AR_KeepRelative: return EAttachmentRule::KeepRelative;
		case EPHAttachmentRule::AR_KeepWorld:    return EAttachmentRule::KeepWorld;
		case EPHAttachmentRule::AR_SnapToTarget: return EAttachmentRule::SnapToTarget;
		default: return EAttachmentRule::KeepRelative;
	}
}