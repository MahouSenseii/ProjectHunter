// Item/Library/Structs/AffixStructs.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "Item/Library/Enums/ItemEnums.h"
#include "Item/Library/Enums/AffixEnums.h"
#include "Item/Library/FunctionLibraries/ItemEnumFunctionLibrary.h"
#include "AttributeSet.h"
#include "AffixStructs.generated.h"

class UGameplayEffect;

/**
 * Affix Tier - Different power levels of same affix
 *
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

	/** Minimum roll for the upper endpoint of an Add Range affix. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tier|Range",
		meta = (EditCondition = "ModifyType == EModifyType::MT_AddRange || ModifyType == EModifyType::MT_MultiplyRange"))
	float MinSecondaryValue = 0.0f;

	/** Maximum roll for the upper endpoint of an Add Range affix. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tier|Range",
		meta = (EditCondition = "ModifyType == EModifyType::MT_AddRange || ModifyType == EModifyType::MT_MultiplyRange"))
	float MaxSecondaryValue = 0.0f;

	/** Roll a distinct upper endpoint. Disabled preserves legacy single-value range rows. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tier|Range",
		meta = (EditCondition = "ModifyType == EModifyType::MT_AddRange || ModifyType == EModifyType::MT_MultiplyRange"))
	bool bRollSecondaryValue = false;

	/** Item-power score contributed by this tier to the finished item's grade. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tier", meta = (ClampMin = "-1000.0"))
	float PowerValue = 10.0f;

	/** Optional tier-specific multiplier for the parent affix's spawn weight. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tier", meta = (ClampMin = "0.0"))
	float WeightMultiplier = 1.0f;

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


	/** Unique affix ID (e.g., "IncreasedPhysicalDamage") */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
	FName AffixID;

	/** Display name (e.g., "Increased Physical Damage") */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
	FText AffixName;

	/** Stable runtime/stat lookup name. Defaults to AffixID when left empty. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
	FName AttributeName = NAME_None;

	/** Affix type (Prefix, Suffix, Implicit, etc.) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
	EAffixes AffixType = EAffixes::AF_Prefix;

	/**
	 * Generation weight (higher = more common).
	 *   -1 = derive from AffixRarity
	 *    0 = never spawns
	 *   >0 = explicit weight
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic", meta = (ClampMin = "-1"))
	int32 Weight = 100;

	/** Affix rarity tier (affects default weight) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
	EAffixRarity AffixRarity = EAffixRarity::AR_Common;


	/** Primary tag (for grouping and exclusion) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Categories")
	EAffixTag PrimaryTag = EAffixTag::AT_None;

	/** Secondary tags (for filtering) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Categories")
	TArray<EAffixTag> SecondaryTags;

	/** Tag group (for mutual exclusion - only one affix per group) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Categories")
	FName TagGroup;


	/** Allowed item types (empty = all types) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Restrictions")
	TArray<EItemType> AllowedItemTypes;

	/** Allowed subtypes (empty = all subtypes) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Restrictions")
	TArray<EItemSubType> AllowedSubTypes;

	/** Excluded item types (takes precedence) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Restrictions")
	TArray<EItemType> ExcludedItemTypes;


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


	/** Condition for this affix to apply */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conditional")
	EAffixCondition Condition = EAffixCondition::AC_None;

	/** Condition description (for tooltip) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conditional")
	FText ConditionDescription;

	/** Optional Gameplay Effect for conditional or complex behavior. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conditional")
	TSubclassOf<UGameplayEffect> GameplayEffect;

	/** Every source/skill tag in this container must be present for the affix to apply. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conditional|Tags")
	FGameplayTagContainer RequiredSourceTags;

	/** The affix does not apply when any source/skill tag in this container is present. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conditional|Tags")
	FGameplayTagContainer BlockedSourceTags;

	/** Every target tag in this container must be present for the affix to apply. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conditional|Tags")
	FGameplayTagContainer RequiredTargetTags;

	/** The affix does not apply when any target tag in this container is present. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conditional|Tags")
	FGameplayTagContainer BlockedTargetTags;


	/** Affix tiers (different power levels by item level) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tiers")
	TArray<FAffixTier> Tiers;


	/** Display format (e.g., "{0}% Increased Physical Damage") */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display")
	FText DisplayFormat;

	/** Display format type */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display")
	EAffixDisplayFormat FormatType = EAffixDisplayFormat::ADF_CustomFormat;

	/** Icon for this affix (optional) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display")
	UTexture2D* AffixIcon = nullptr;

	// DAMAGE CONVERSION (For "Convert X% to Y" affixes)

	/** Source damage type (for conversion affixes) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conversion", meta = (EditCondition = "ModifyType == EModifyType::MT_ConvertTo"))
	EDamageType FromDamageType = EDamageType::DT_Physical;

	/** Target damage type (for conversion affixes) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conversion", meta = (EditCondition = "ModifyType == EModifyType::MT_ConvertTo"))
	EDamageType ToDamageType = EDamageType::DT_Fire;

	/** Copy the converted amount without removing the source damage. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conversion", meta = (EditCondition = "ModifyType == EModifyType::MT_ConvertTo"))
	bool bGainAsExtra = false;


	/** Get effective weight. Zero is an explicit "never"; only -1 derives from rarity. */
	FORCEINLINE int32 GetEffectiveWeight() const
	{
		return Weight >= 0 ? Weight : UItemEnumFunctionLibrary::GetAffixRarityWeight(AffixRarity);
	}

	/** Check if this affix can spawn on item type */
	FORCEINLINE bool CanSpawnOnItemType(EItemType ItemType) const
	{
		if (ExcludedItemTypes.Contains(ItemType))
		{
			return false;
		}

		// If no restrictions, allow all
		if (AllowedItemTypes.Num() == 0)
		{
			return true;
		}

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

	/**
	 * Weight for this affix in this pool only.
	 *   -1 = use the affix's own weight
	 *    0 = excluded from this pool
	 *   >0 = explicit weight
	 *
	 * Zero is how a sub-type opts out of an affix it inherited through an include.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pool", meta = (ClampMin = "-1"))
	int32 WeightOverride = -1;

	/** Force specific tier (0 = use item level) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pool")
	int32 ForceTier = 0;

	/** Required item level override */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pool")
	int32 MinItemLevelOverride = 0;
};

/**
 * Affix Set - the list of affixes that can roll on one item sub-type.
 *
 * Entries reference affixes by ID, so the tier ladder still lives in exactly one
 * row of the affix table no matter how many sub-types can roll it.
 *
 * AUTHORING WORKFLOW:
 * 1. Open DT_AffixPools.
 * 2. Add a row per item sub-type and set SubType on it.
 * 3. Put the affixes every slot shares into rows left at SubType = None, and
 *    pull them in through IncludedSets rather than re-listing them.
 * 4. List only the sub-type's own affixes in Prefixes/Suffixes/Implicits.
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

	/**
	 * Item sub-type this pool is the affix list for.
	 *
	 * None marks a shared building block that is only ever pulled in through
	 * another set's IncludedSets - it is never selected for an item directly.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Set")
	EItemSubType SubType = EItemSubType::IST_None;

	/**
	 * Sets merged into this one before its own entries.
	 *
	 * Resolution is depth-first and de-duplicates by AffixID with the LAST entry
	 * winning, so re-listing an included affix here overrides its weight or tier
	 * for this sub-type only. Cycles are detected and ignored.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Set")
	TArray<FDataTableRowHandle> IncludedSets;

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

/** One FAffixSet with its IncludedSets flattened in and duplicates collapsed. */
USTRUCT(BlueprintType)
struct FResolvedAffixPool
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Pool")
	TArray<FAffixPoolEntry> Prefixes;

	UPROPERTY(BlueprintReadOnly, Category = "Pool")
	TArray<FAffixPoolEntry> Suffixes;

	UPROPERTY(BlueprintReadOnly, Category = "Pool")
	TArray<FAffixPoolEntry> Implicits;

	bool IsEmpty() const
	{
		return Prefixes.IsEmpty() && Suffixes.IsEmpty() && Implicits.IsEmpty();
	}
};
