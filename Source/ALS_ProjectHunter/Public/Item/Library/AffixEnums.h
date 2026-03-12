// Item/Library/AffixEnums.h
#pragma once

#include "CoreMinimal.h"
#include "AffixEnums.generated.h"

// ═══════════════════════════════════════════════════════════════════════
// AFFIX SYSTEM ENUMS - Hunter Manga / PoE2 Hybrid Style
// ═══════════════════════════════════════════════════════════════════════

/**
 * Affix Type (Prefix or Suffix)
 * Hunter Manga Style: Items can have skills and stats from affixes
 */
UENUM(BlueprintType)
enum class EAffixes : uint8
{
	AF_None       UMETA(DisplayName = "None"),
	AF_Prefix     UMETA(DisplayName = "Prefix"),
	AF_Suffix     UMETA(DisplayName = "Suffix"),
	AF_Implicit   UMETA(DisplayName = "Implicit"),   // Built into item
	AF_Crafted    UMETA(DisplayName = "Crafted"),    // Hunter-crafted (Runes)
	AF_Corrupted  UMETA(DisplayName = "Corrupted"),  // Chaos/corruption mods
	AF_Enchant    UMETA(DisplayName = "Enchant"),    // Special enchantments
	AF_Skill      UMETA(DisplayName = "Skill")       // Items with active skills
};

/**
 * Modify Type - How the affix modifies the attribute
 * Supports PoE2-style math + Hunter Manga flat bonuses
 */
UENUM(BlueprintType)
enum class EModifyType : uint8
{
	MT_None           UMETA(DisplayName = "None"),
	
	// Basic operations
	MT_Add            UMETA(DisplayName = "Add"),           // Flat: +50 Strength
	MT_Multiply       UMETA(DisplayName = "Multiply"),      // Percent: +75%
	MT_Override       UMETA(DisplayName = "Override"),      // Replace value
	
	// PoE2-style operations
	MT_More           UMETA(DisplayName = "More"),          // Multiplicative: × (1 + Value/100)
	MT_Increased      UMETA(DisplayName = "Increased"),     // Additive: Sum all Increased
	MT_Reduced        UMETA(DisplayName = "Reduced"),       // Negative Increased
	MT_Less           UMETA(DisplayName = "Less"),          // Negative More
	
	// Special operations
	MT_ConvertTo      UMETA(DisplayName = "Convert To"),    // Damage conversion
	MT_AddRange       UMETA(DisplayName = "Add Range"),     // Min-max range (damage)
	MT_MultiplyRange  UMETA(DisplayName = "Multiply Range"),// Range multiplier
	
	// Hunter Manga specific
	MT_GrantSkill     UMETA(DisplayName = "Grant Skill"),   // Grants active/passive skill
	MT_SetRank        UMETA(DisplayName = "Set Rank")       // Sets skill rank level
};

/**
 * Rank Points for affix quality - Solo Leveling Style
 * Positive = beneficial, Negative = cursed/corrupted
 * Used to calculate item grade contribution
 */
UENUM(BlueprintType)
enum class ERankPoints : uint8
{
	// Detrimental (Cursed/Corrupted Affixes)
	RP_Minus10  UMETA(DisplayName = "-10 Points (Severely Cursed)"),
	RP_Minus9   UMETA(DisplayName = "-9 Points"),
	RP_Minus8   UMETA(DisplayName = "-8 Points"),
	RP_Minus7   UMETA(DisplayName = "-7 Points"),
	RP_Minus6   UMETA(DisplayName = "-6 Points"),
	RP_Minus5   UMETA(DisplayName = "-5 Points (Cursed)"),
	RP_Minus4   UMETA(DisplayName = "-4 Points"),
	RP_Minus3   UMETA(DisplayName = "-3 Points"),
	RP_Minus2   UMETA(DisplayName = "-2 Points"),
	RP_Minus1   UMETA(DisplayName = "-1 Point"),
	
	// Beneficial (Normal Affixes)
	RP_0        UMETA(DisplayName = "0 Points"),
	RP_1        UMETA(DisplayName = "1 Point"),
	RP_2        UMETA(DisplayName = "2 Points"),
	RP_3        UMETA(DisplayName = "3 Points"),
	RP_4        UMETA(DisplayName = "4 Points"),
	RP_5        UMETA(DisplayName = "5 Points"),
	RP_6        UMETA(DisplayName = "6 Points"),
	RP_7        UMETA(DisplayName = "7 Points"),
	RP_8        UMETA(DisplayName = "8 Points"),
	RP_9        UMETA(DisplayName = "9 Points"),
	RP_10       UMETA(DisplayName = "10 Points (Perfect Roll)"),
};

/**
 * Affix Scope - Where the affix applies
 */
UENUM(BlueprintType)
enum class EAffixScope : uint8
{
	AS_Local        UMETA(DisplayName = "Local"),       // Only affects this item
	AS_Global       UMETA(DisplayName = "Global"),      // Affects hunter/character
	AS_Conditional  UMETA(DisplayName = "Conditional"), // Conditional bonus
	AS_Skill        UMETA(DisplayName = "Skill")        // Grants/modifies skill
};

/**
 * Affix Condition Type - For conditional affixes
 */
UENUM(BlueprintType)
enum class EAffixCondition : uint8
{
	AC_None                 UMETA(DisplayName = "None"),
	AC_WhileDualWielding    UMETA(DisplayName = "While Dual Wielding"),
	AC_WhileUnarmed         UMETA(DisplayName = "While Unarmed"),
	AC_WhileShieldEquipped  UMETA(DisplayName = "While Shield Equipped"),
	AC_OnFullLife           UMETA(DisplayName = "On Full Life"),
	AC_OnLowLife            UMETA(DisplayName = "On Low Life (Below 35%)"),
	AC_RecentlyHit          UMETA(DisplayName = "Recently Hit (4s)"),
	AC_RecentlyKilled       UMETA(DisplayName = "Recently Killed (4s)"),
	AC_AgainstBoss          UMETA(DisplayName = "Against Boss Monsters"),
	AC_AgainstElite         UMETA(DisplayName = "Against Elite Monsters"),
	AC_DuringFlaskEffect    UMETA(DisplayName = "During Flask Effect"),
	AC_WhileMoving          UMETA(DisplayName = "While Moving"),
	AC_WhileStationary      UMETA(DisplayName = "While Stationary"),
	AC_InDungeon            UMETA(DisplayName = "In Dungeon"),
	AC_AgainstCorrupted     UMETA(DisplayName = "Against Corrupted Enemies")
};

/**
 * Affix Rarity - How rare an affix is (spawn weight)
 */
UENUM(BlueprintType)
enum class EAffixRarity : uint8
{
	AR_Common       UMETA(DisplayName = "Common"),        // Weight: 100-150
	AR_Uncommon     UMETA(DisplayName = "Uncommon"),      // Weight: 50-100
	AR_Rare         UMETA(DisplayName = "Rare"),          // Weight: 20-50
	AR_VeryRare     UMETA(DisplayName = "Very Rare"),     // Weight: 5-20
	AR_Unique       UMETA(DisplayName = "Unique"),        // Weight: 1-5
	AR_Mythic       UMETA(DisplayName = "Mythic")         // Weight: <1 (extremely rare)
};

/**
 * Affix Tag - Categorization for affix groups
 */
UENUM(BlueprintType)
enum class EAffixTag : uint8
{
	AT_None              UMETA(DisplayName = "None"),
	
	// Damage Tags
	AT_PhysicalDamage    UMETA(DisplayName = "Physical Damage"),
	AT_FireDamage        UMETA(DisplayName = "Fire Damage"),
	AT_IceDamage         UMETA(DisplayName = "Ice Damage"),
	AT_LightningDamage   UMETA(DisplayName = "Lightning Damage"),
	AT_LightDamage       UMETA(DisplayName = "Light Damage"),
	AT_CorruptionDamage  UMETA(DisplayName = "Corruption Damage"),
	AT_ElementalDamage   UMETA(DisplayName = "Elemental Damage"),
	
	// Defense Tags
	AT_Armor             UMETA(DisplayName = "Armor"),
	AT_FireResistance    UMETA(DisplayName = "Fire Resistance"),
	AT_IceResistance     UMETA(DisplayName = "Ice Resistance"),
	AT_LightningResistance UMETA(DisplayName = "Lightning Resistance"),
	AT_LightResistance   UMETA(DisplayName = "Light Resistance"),
	AT_CorruptionResistance UMETA(DisplayName = "Corruption Resistance"),
	AT_AllResistances    UMETA(DisplayName = "All Resistances"),
	
	// Attribute Tags (Your custom stats)
	AT_Strength          UMETA(DisplayName = "Strength"),
	AT_Dexterity         UMETA(DisplayName = "Dexterity"),
	AT_Intelligence      UMETA(DisplayName = "Intelligence"),
	AT_Endurance         UMETA(DisplayName = "Endurance"),
	AT_Affliction        UMETA(DisplayName = "Affliction"),
	AT_Luck              UMETA(DisplayName = "Luck"),
	AT_Covenant          UMETA(DisplayName = "Covenant"),
	AT_AllAttributes     UMETA(DisplayName = "All Attributes"),
	
	// Life/Mana Tags
	AT_Life              UMETA(DisplayName = "Life"),
	AT_Mana              UMETA(DisplayName = "Mana"),
	AT_LifeRegen         UMETA(DisplayName = "Life Regeneration"),
	AT_ManaRegen         UMETA(DisplayName = "Mana Regeneration"),
	AT_LifeLeech         UMETA(DisplayName = "Life Leech"),
	AT_ManaLeech         UMETA(DisplayName = "Mana Leech"),
	
	// Speed Tags
	AT_AttackSpeed       UMETA(DisplayName = "Attack Speed"),
	AT_CastSpeed         UMETA(DisplayName = "Cast Speed"),
	AT_MovementSpeed     UMETA(DisplayName = "Movement Speed"),
	
	// Critical Tags
	AT_CriticalChance    UMETA(DisplayName = "Critical Strike Chance"),
	AT_CriticalMultiplier UMETA(DisplayName = "Critical Strike Multiplier"),
	
	// Special Tags
	AT_Accuracy          UMETA(DisplayName = "Accuracy"),
	AT_Quality           UMETA(DisplayName = "Quality"),
	AT_Skill             UMETA(DisplayName = "Skill"),
	AT_Unique            UMETA(DisplayName = "Unique"),
	AT_Corrupted         UMETA(DisplayName = "Corrupted")
};

/**
 * Stat Application Order - For calculation priority (PoE2 style)
 */
UENUM(BlueprintType)
enum class EStatApplicationOrder : uint8
{
	SAO_Base        UMETA(DisplayName = "Base"),         // Base stats
	SAO_Flat        UMETA(DisplayName = "Flat"),         // Flat additions
	SAO_Increased   UMETA(DisplayName = "Increased"),    // Additive increases
	SAO_More        UMETA(DisplayName = "More"),         // Multiplicative increases
	SAO_Final       UMETA(DisplayName = "Final"),        // Final adjustments
	SAO_Override    UMETA(DisplayName = "Override")      // Overrides everything
};

/**
 * Affix Color Tier - Visual rarity indicator
 */
UENUM(BlueprintType)
enum class EAffixColorTier : uint8
{
	ACT_Normal      UMETA(DisplayName = "Normal"),       // White
	ACT_Uncommon    UMETA(DisplayName = "Uncommon"),     // Green
	ACT_Rare        UMETA(DisplayName = "Rare"),         // Blue
	ACT_Elite       UMETA(DisplayName = "Elite"),        // Purple
	ACT_Legendary   UMETA(DisplayName = "Legendary"),    // Gold
	ACT_Mythic      UMETA(DisplayName = "Mythic"),       // Red
	ACT_Corrupted   UMETA(DisplayName = "Corrupted")     // Dark Red/Black
};

/**
 * Affix Display Format - How to format the affix text
 */
UENUM(BlueprintType)
enum class EAffixDisplayFormat : uint8
{
	ADF_None            UMETA(DisplayName = "None"),
	ADF_Percentage      UMETA(DisplayName = "Percentage"),      // "{0}% Increased Damage"
	ADF_FlatValue       UMETA(DisplayName = "Flat Value"),      // "+{0} to Strength"
	ADF_Range           UMETA(DisplayName = "Range"),           // "Adds {0}-{1} Fire Damage"
	ADF_PercentRange    UMETA(DisplayName = "Percent Range"),   // "{0}%-{1}% Increased"
	ADF_Skill           UMETA(DisplayName = "Skill"),           // "Grants [Skill] Level {0}"
	ADF_CustomFormat    UMETA(DisplayName = "Custom Format")    // Use DisplayFormat string
};

/**
 * Affix Generation Flags - Control affix rolling behavior
 */
UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EAffixGenerationFlags : uint8
{
	AGF_None              = 0 UMETA(DisplayName = "None"),
	AGF_AllowDuplicates   = 1 << 0 UMETA(DisplayName = "Allow Duplicates"),
	AGF_ForceMaxTier      = 1 << 1 UMETA(DisplayName = "Force Max Tier"),
	AGF_PreferOffensive   = 1 << 2 UMETA(DisplayName = "Prefer Offensive"),
	AGF_PreferDefensive   = 1 << 3 UMETA(DisplayName = "Prefer Defensive"),
	AGF_NoImplicits       = 1 << 4 UMETA(DisplayName = "No Implicits"),
	AGF_GuaranteedPrefix  = 1 << 5 UMETA(DisplayName = "Guaranteed Prefix"),
	AGF_GuaranteedSuffix  = 1 << 6 UMETA(DisplayName = "Guaranteed Suffix")
};
ENUM_CLASS_FLAGS(EAffixGenerationFlags);

/**
 * Attribute display format in tooltip
 */
UENUM(BlueprintType)
enum class EAttributeDisplayFormat : uint8
{
	ADF_Additive       UMETA(DisplayName = "Additive (+X Stat)"),
	ADF_FlatNegative   UMETA(DisplayName = "Flat Negative (-X Stat)"),
	ADF_Percent        UMETA(DisplayName = "Percent (+X% Stat)"),
	ADF_MinMax         UMETA(DisplayName = "Min-Max Range (Adds X-Y Stat)"),
	ADF_Increase       UMETA(DisplayName = "Increase (X% increased Stat)"),
	ADF_More           UMETA(DisplayName = "More (X% more Stat)"),
	ADF_Less           UMETA(DisplayName = "Less (X% less Stat)"),
	ADF_Chance         UMETA(DisplayName = "Chance (X% chance to Stat)"),
	ADF_Duration       UMETA(DisplayName = "Duration (Xs duration to Stat)"),
	ADF_Cooldown       UMETA(DisplayName = "Cooldown (Xs cooldown on Stat)"),
	ADF_SkillGrant     UMETA(DisplayName = "Skill Grant (Grants [Skill] Level X)"),
	ADF_CustomText     UMETA(DisplayName = "Custom Text"),
};

// ═══════════════════════════════════════════════════════════════════════
// HELPER FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════

/** Convert ERankPoints to integer value for calculations */
FORCEINLINE int32 GetRankPointsValue(ERankPoints Points)
{
	switch (Points)
	{
		case ERankPoints::RP_Minus10: return -10;
		case ERankPoints::RP_Minus9:  return -9;
		case ERankPoints::RP_Minus8:  return -8;
		case ERankPoints::RP_Minus7:  return -7;
		case ERankPoints::RP_Minus6:  return -6;
		case ERankPoints::RP_Minus5:  return -5;
		case ERankPoints::RP_Minus4:  return -4;
		case ERankPoints::RP_Minus3:  return -3;
		case ERankPoints::RP_Minus2:  return -2;
		case ERankPoints::RP_Minus1:  return -1;
		case ERankPoints::RP_0:       return 0;
		case ERankPoints::RP_1:       return 1;
		case ERankPoints::RP_2:       return 2;
		case ERankPoints::RP_3:       return 3;
		case ERankPoints::RP_4:       return 4;
		case ERankPoints::RP_5:       return 5;
		case ERankPoints::RP_6:       return 6;
		case ERankPoints::RP_7:       return 7;
		case ERankPoints::RP_8:       return 8;
		case ERankPoints::RP_9:       return 9;
		case ERankPoints::RP_10:      return 10;
		default: return 0;
	}
}

/** Get affix rarity weight */
FORCEINLINE int32 GetAffixRarityWeight(EAffixRarity Rarity)
{
	switch (Rarity)
	{
		case EAffixRarity::AR_Common:    return 125;
		case EAffixRarity::AR_Uncommon:  return 75;
		case EAffixRarity::AR_Rare:      return 35;
		case EAffixRarity::AR_VeryRare:  return 12;
		case EAffixRarity::AR_Unique:    return 3;
		case EAffixRarity::AR_Mythic:    return 1;
		default:                          return 100;
	}
}

/** Get color for affix tier */
FORCEINLINE FLinearColor GetAffixTierColor(EAffixColorTier Tier)
{
	switch (Tier)
	{
		case EAffixColorTier::ACT_Normal:    return FLinearColor::White;
		case EAffixColorTier::ACT_Uncommon:  return FLinearColor(0.3f, 0.9f, 0.3f);  // Green
		case EAffixColorTier::ACT_Rare:      return FLinearColor(0.4f, 0.4f, 1.0f);  // Blue
		case EAffixColorTier::ACT_Elite:     return FLinearColor(0.7f, 0.3f, 0.9f);  // Purple
		case EAffixColorTier::ACT_Legendary: return FLinearColor(1.0f, 0.85f, 0.0f); // Gold
		case EAffixColorTier::ACT_Mythic:    return FLinearColor(1.0f, 0.2f, 0.2f);  // Red
		case EAffixColorTier::ACT_Corrupted: return FLinearColor(0.2f, 0.0f, 0.2f);  // Dark
		default:                              return FLinearColor::White;
	}
}

/** Get modify type symbol */
FORCEINLINE FString GetModifyTypeSymbol(EModifyType ModifyType)
{
	switch (ModifyType)
	{
		case EModifyType::MT_Add:            return TEXT("+");
		case EModifyType::MT_Multiply:       return TEXT("+% ");
		case EModifyType::MT_Override:       return TEXT("= ");
		case EModifyType::MT_More:           return TEXT("% More ");
		case EModifyType::MT_Increased:      return TEXT("% Increased ");
		case EModifyType::MT_Reduced:        return TEXT("% Reduced ");
		case EModifyType::MT_Less:           return TEXT("% Less ");
		case EModifyType::MT_ConvertTo:      return TEXT("% Converted to ");
		case EModifyType::MT_AddRange:       return TEXT("Adds ");
		case EModifyType::MT_MultiplyRange:  return TEXT("% Increased ");
		case EModifyType::MT_GrantSkill:     return TEXT("Grants ");
		case EModifyType::MT_SetRank:        return TEXT("Level ");
		default:                              return TEXT("");
	}
}