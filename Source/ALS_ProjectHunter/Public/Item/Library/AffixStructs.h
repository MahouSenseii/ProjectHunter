// Item/Library/AffixStructs.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Item/Library/ItemEnums.h"
#include "Item/Library/AffixEnums.h"
#include "AttributeSet.h"
#include "AffixStructs.generated.h"

/**
 * Affix Tier - Different power levels of same affix
 * PoE2 Style: Higher tier = better stats, higher item level requirement
 * 
 * Example:
 * "Increased Physical Damage"
 *   Tier 1 (iLvl 1-10):   10-20%
 *   Tier 2 (iLvl 11-25):  21-35%
 *   Tier 3 (iLvl 26-40):  36-50%
 *   Tier 4 (iLvl 41-60):  51-75%
 *   Tier 5 (iLvl 61+):    76-100%
 */
USTRUCT(BlueprintType)
struct FAffixTier
{
	GENERATED_BODY()

	/** Tier number (1 = lowest, 5 = highest) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tier")
	int32 TierNumber = 1;

	/** Minimum item level required */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tier")
	int32 MinItemLevel = 1;

	/** Maximum item level for this tier */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tier")
	int32 MaxItemLevel = 100;

	/** Minimum stat value */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tier")
	float MinValue = 0.0f;

	/** Maximum stat value */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tier")
	float MaxValue = 0.0f;

	/** Attribute to modify */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tier")
	FGameplayAttribute ModifiedAttribute;

	/** How to modify (Add, Multiply, More, etc.) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tier")
	EModifyType ModifyType = EModifyType::MT_Add;

	/** Application order for this stat */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tier")
	EStatApplicationOrder ApplicationOrder = EStatApplicationOrder::SAO_Base;

	/** Visual tier color */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tier")
	EAffixColorTier ColorTier = EAffixColorTier::ACT_Normal;
};

/**
 * Affix Data - Base affix definition (one row in DataTable)
 * This is what you create in the editor to define affixes
 * 
 * EASY CREATION WORKFLOW:
 * 1. Open DT_Affixes DataTable
 * 2. Add new row
 * 3. Fill in name, type, weight
 * 4. Add tiers with different power levels
 * 5. Done!
 */
USTRUCT(BlueprintType)
struct FAffixData : public FTableRowBase
{
	GENERATED_BODY()

	// ═══════════════════════════════════════════════
	// BASIC INFO
	// ═══════════════════════════════════════════════

	/** Unique affix ID (e.g., "IncreasedPhysicalDamage") */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
	FName AffixID;

	/** Display name (e.g., "Increased Physical Damage") */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
	FText AffixName;

	/** Affix type (Prefix, Suffix, Implicit, etc.) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
	EAffixes AffixType = EAffixes::AF_Prefix;

	/** Generation weight (higher = more common) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
	int32 Weight = 100;

	/** Affix rarity tier (affects default weight) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
	EAffixRarity AffixRarity = EAffixRarity::AR_Common;

	// ═══════════════════════════════════════════════
	// CATEGORIZATION
	// ═══════════════════════════════════════════════

	/** Primary tag (for grouping and exclusion) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Categories")
	EAffixTag PrimaryTag = EAffixTag::AT_None;

	/** Secondary tags (for filtering) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Categories")
	TArray<EAffixTag> SecondaryTags;

	/** Tag group (for mutual exclusion - only one affix per group) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Categories")
	FName TagGroup;

	// ═══════════════════════════════════════════════
	// ITEM TYPE RESTRICTIONS
	// ═══════════════════════════════════════════════

	/** Allowed item types (empty = all types) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Restrictions")
	TArray<EItemType> AllowedItemTypes;

	/** Allowed subtypes (empty = all subtypes) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Restrictions")
	TArray<EItemSubType> AllowedSubTypes;

	/** Excluded item types (takes precedence) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Restrictions")
	TArray<EItemType> ExcludedItemTypes;

	// ═══════════════════════════════════════════════
	// MODIFIER PROPERTIES
	// ═══════════════════════════════════════════════

	/** Affix scope (Local, Global, Conditional) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Properties")
	EAffixScope Scope = EAffixScope::AS_Global;

	/** Is this a local modifier? (only affects this weapon) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Properties")
	bool bIsLocal = false;

	/** Affects base weapon stats directly? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Properties")
	bool bAffectsBaseStats = false;

	/** Can appear on corrupted items? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Properties")
	bool bCanBeCorrupted = true;

	/** Can be rerolled/crafted? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Properties")
	bool bCanBeRerolled = true;

	// ═══════════════════════════════════════════════
	// CONDITIONAL MODIFIERS (PoE2 Style)
	// ═══════════════════════════════════════════════

	/** Condition for this affix to apply */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conditional")
	EAffixCondition Condition = EAffixCondition::AC_None;

	/** Condition description (for tooltip) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conditional")
	FText ConditionDescription;

	// ═══════════════════════════════════════════════
	// TIERS (Power Levels)
	// ═══════════════════════════════════════════════

	/** Affix tiers (different power levels by item level) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tiers")
	TArray<FAffixTier> Tiers;

	// ═══════════════════════════════════════════════
	// DISPLAY
	// ═══════════════════════════════════════════════

	/** Display format (e.g., "{0}% Increased Physical Damage") */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display")
	FText DisplayFormat;

	/** Display format type */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display")
	EAffixDisplayFormat FormatType = EAffixDisplayFormat::ADF_CustomFormat;

	/** Icon for this affix (optional) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display")
	UTexture2D* AffixIcon;

	// ═══════════════════════════════════════════════
	// DAMAGE CONVERSION (For "Convert X% to Y" affixes)
	// ═══════════════════════════════════════════════

	/** Source damage type (for conversion affixes) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conversion", meta = (EditCondition = "ModifyType == EModifyType::MT_ConvertTo"))
	EDamageType FromDamageType = EDamageType::DT_Physical;

	/** Target damage type (for conversion affixes) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conversion", meta = (EditCondition = "ModifyType == EModifyType::MT_ConvertTo"))
	EDamageType ToDamageType = EDamageType::DT_Fire;

	// ═══════════════════════════════════════════════
	// HELPER FUNCTIONS
	// ═══════════════════════════════════════════════

	/** Get effective weight (base weight × rarity weight) */
	FORCEINLINE int32 GetEffectiveWeight() const
	{
		// If custom weight is set, use it; otherwise use rarity-based weight
		return Weight > 0 ? Weight : GetAffixRarityWeight(AffixRarity);
	}

	/** Check if this affix can spawn on item type */
	FORCEINLINE bool CanSpawnOnItemType(EItemType ItemType) const
	{
		// Check exclusions first
		if (ExcludedItemTypes.Contains(ItemType))
		{
			return false;
		}
		
		// If no restrictions, allow all
		if (AllowedItemTypes.Num() == 0)
		{
			return true;
		}
		
		// Check if in allowed list
		return AllowedItemTypes.Contains(ItemType);
	}

	/** Check if this affix can spawn on item subtype */
	FORCEINLINE bool CanSpawnOnSubType(EItemSubType SubType) const
	{
		// If no restrictions, allow all
		if (AllowedSubTypes.Num() == 0)
		{
			return true;
		}
		
		// Check if in allowed list
		return AllowedSubTypes.Contains(SubType);
	}

	/** Check if affix has valid tier for item level */
	FORCEINLINE bool HasValidTierForLevel(int32 ItemLevel) const
	{
		for (const FAffixTier& Tier : Tiers)
		{
			if (ItemLevel >= Tier.MinItemLevel && ItemLevel <= Tier.MaxItemLevel)
			{
				return true;
			}
		}
		return false;
	}
};

/**
 * Unique Affix - Fixed affix for unique items
 * Uniques have predetermined affixes (not random)
 */
USTRUCT(BlueprintType)
struct FUniqueAffix
{
	GENERATED_BODY()

	/** Affix ID from affix DataTable */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix")
	FName AffixID;

	/** Fixed value (not random) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix")
	float FixedValue = 0.0f;

	/** Modified attribute */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix")
	FGameplayAttribute ModifiedAttribute;

	/** Modify type */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix")
	EModifyType ModifyType = EModifyType::MT_Add;

	/** Display text override (optional) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix")
	FText CustomDisplayText;
};

/**
 * Affix Pool Entry - Weighted entry in an affix pool
 * Used for curated affix selection
 */
USTRUCT(BlueprintType)
struct FAffixPoolEntry
{
	GENERATED_BODY()

	/** Affix ID */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pool")
	FName AffixID;

	/** Custom weight override (0 = use affix's default weight) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pool")
	int32 WeightOverride = 0;

	/** Force specific tier (0 = use item level) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pool")
	int32 ForceTier = 0;

	/** Required item level override */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pool")
	int32 MinItemLevelOverride = 0;
};

/**
 * Affix Set - Predefined set of affixes (for crafting, uniques, etc.)
 */
USTRUCT(BlueprintType)
struct FAffixSet : public FTableRowBase
{
	GENERATED_BODY()

	/** Set name */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Set")
	FText SetName;

	/** Set description */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Set")
	FText SetDescription;

	/** Prefixes in this set */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Set")
	TArray<FAffixPoolEntry> Prefixes;

	/** Suffixes in this set */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Set")
	TArray<FAffixPoolEntry> Suffixes;

	/** Implicits in this set */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Set")
	TArray<FAffixPoolEntry> Implicits;
	
};