// Item/Library/Structs/ItemAttributeStructs.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "AttributeSet.h"
#include "GameplayTagContainer.h"
#include "Item/Library/Enums/AffixEnums.h"
#include "Item/Library/Enums/ItemEnums.h"
#include "Item/Library/FunctionLibraries/ItemEnumFunctionLibrary.h"
#include "ItemAttributeStructs.generated.h"

class UGameplayEffect;


/**
 * Single attribute modification (used for affixes)
 */
USTRUCT(BlueprintType)
struct FPHAttributeData : public FTableRowBase
{
	GENERATED_BODY()


	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Attribute")
	FGuid AttributeUID;

	/** Stable ID of the authored affix definition. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Attribute|Affix")
	FName AffixID = NAME_None;


	/** Affix type: Prefix, Suffix, Implicit, etc. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute|Affix")
	EAffixes AffixType = EAffixes::AF_Prefix;


	/** Affix name for item naming (e.g., "Dragon's", "of the Fang") */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute|Affix")
	FText AffixName;

	/**
	 * Mutual-exclusion group.  Only one affix per group may appear on an item.
	 * Example: all added fire damage affixes can share group "FireDamage" so a second
	 * fire-damage affix cannot roll alongside the first.
	 * Leave as NAME_None to skip group enforcement for this affix.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute|Affix")
	FName AffixGroup = NAME_None;

	/** Rank points for quality (-10 to +10) - Also determines weight (higher tier = rarer) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute|Affix")
	ERankPoints RankPoints = ERankPoints::RP_0;

	/** Authored tier number retained on the rolled modifier for UI and crafting. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Attribute|Affix", meta = (ClampMin = "0"))
	int32 TierNumber = 0;

	/** Contribution of this rolled affix to the item's derived power score. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Attribute|Power")
	float PowerValue = 0.0f;

	/**
	 * Generation weight.
	 *   -1 = derive from RankPoints (the default)
	 *    0 = never spawns
	 *   >0 = explicit weight
	 *
	 * Zero used to mean "unset" and fell through to the RankPoints formula, which
	 * returns up to 1000 - so the obvious way to disable an affix made it as
	 * common as possible instead.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute|Generation", meta = (ClampMin = "-1"))
	int32 SpawnWeight = -1;

	/** Primary category used by future targeted crafting. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Attribute|Generation")
	EAffixTag PrimaryTag = EAffixTag::AT_None;

	/** Additional categories used by future targeted crafting. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Attribute|Generation")
	TArray<EAffixTag> SecondaryTags;


	/** Allowed item types (empty = all types allowed) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute|Filtering")
	TArray<EItemType> AllowedItemTypes;

	/** Allowed item subtypes (empty = all subtypes allowed) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute|Filtering")
	TArray<EItemSubType> AllowedSubTypes;

	/** Explicit exclusions take precedence over AllowedItemTypes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute|Filtering")
	TArray<EItemType> ExcludedItemTypes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute|Filtering",
		meta = (ClampMin = "1", ClampMax = "100"))
	int32 MinItemLevel = 1;

	/** Maximum item level at which this affix can be rolled (inclusive, 100 = any) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute|Filtering",
		meta = (ClampMin = "1", ClampMax = "100"))
	int32 MaxItemLevel = 100;


	/** What attribute does this modify? (Strength, PhysicalDamage, etc.) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute|Modification")
	FGameplayAttribute ModifiedAttribute;

	/** Internal name for the attribute (for lookups) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute|Modification")
	FName AttributeName = NAME_None;

	/** How does it modify? (Add, Multiply, More, etc.) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute|Modification")
	EModifyType ModifyType = EModifyType::MT_Add;

	/** Where does it apply? (Local weapon, Global character, Skill, etc.) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute|Modification")
	EAffixScope ModifiedLocation = EAffixScope::AS_Global;

	/** Condition for this affix to apply (optional) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute|Modification")
	EAffixCondition Condition = EAffixCondition::AC_None;

	/** Human-readable condition retained from the affix definition for tooltips. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Attribute|Modification")
	FText ConditionDescription;

	/** Every source/skill tag in this container must be present for this modifier to apply. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Attribute|Modification|Tags")
	FGameplayTagContainer RequiredSourceTags;

	/** This modifier is blocked when any source/skill tag in this container is present. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Attribute|Modification|Tags")
	FGameplayTagContainer BlockedSourceTags;

	/** Every target tag in this container must be present for this modifier to apply. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Attribute|Modification|Tags")
	FGameplayTagContainer RequiredTargetTags;

	/** This modifier is blocked when any target tag in this container is present. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Attribute|Modification|Tags")
	FGameplayTagContainer BlockedTargetTags;

	/** Source damage type for conversion modifiers. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Attribute|Modification|Conversion")
	EDamageType FromDamageType = EDamageType::DT_None;

	/** Destination damage type for conversion modifiers. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Attribute|Modification|Conversion")
	EDamageType ToDamageType = EDamageType::DT_None;

	/** Gain-as-extra copies damage instead of removing it from the source type. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Attribute|Modification|Conversion")
	bool bGainAsExtra = false;


	/** Minimum value for this affix */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute|Value")
	float MinValue = 0.0f;

	/** Maximum value for this affix */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute|Value")
	float MaxValue = 0.0f;

	/** Roll bounds for the upper endpoint of range modifiers. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute|Value|Range")
	float MinSecondaryValue = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute|Value|Range")
	float MaxSecondaryValue = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute|Value|Range")
	bool bRollSecondaryValue = false;

	/** Rolled stat value (generated on item creation) */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Attribute|Value")
	float RolledStatValue = 0.0f;

	/** Rolled upper endpoint for Add Range and Multiply Range modifiers. */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Attribute|Value")
	float RolledSecondaryStatValue = 0.0f;

	/** True when this modifier folds into the base values of its owning item. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Attribute|Modification")
	bool bAffectsBaseItemStats = false;


	/** How to format this value for tooltip display */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute|Display")
	EAttributeDisplayFormat DisplayFormat = EAttributeDisplayFormat::ADF_Additive;

	/** Custom display text (optional override) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute|Display")
	FText DisplayText;

	/** Has this affix been identified? (for unidentified items) */
	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Attribute|Display")
	bool bIsIdentified = true;


	/** Gameplay Effect to apply (optional, for complex effects) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute|GameplayEffect")
	TSubclassOf<UGameplayEffect> GameplayEffect;


	/** @deprecated Use ModifiedLocation instead */
	UPROPERTY()
	bool bIsLocalToWeapon = false;

	/** @deprecated Use ModifiedLocation instead */
	UPROPERTY()
	bool bAffectsBaseWeaponStatsDirectly = false;


	FPHAttributeData() = default;

	void GenerateUID()
	{
		AttributeUID = FGuid::NewGuid();
	}

	/** Deterministic identity for seeded loot generation and replay tests. */
	void GenerateUID(FRandomStream& RandStream)
	{
		AttributeUID = FGuid(
			static_cast<uint32>(RandStream.RandHelper(MAX_int32)),
			static_cast<uint32>(RandStream.RandHelper(MAX_int32)),
			static_cast<uint32>(RandStream.RandHelper(MAX_int32)),
			static_cast<uint32>(RandStream.RandHelper(MAX_int32)));
	}

	void RollValue()
	{
		RolledStatValue = FMath::RandRange(MinValue, MaxValue);
		RolledSecondaryStatValue = UsesValueRange() && bRollSecondaryValue
			? FMath::RandRange(MinSecondaryValue, MaxSecondaryValue)
			: RolledStatValue;
		NormalizeRolledRange();
	}

	/**
	 * Seeded overload so affix generation is fully deterministic.
	 * Call this when a FRandomStream is available (e.g. inside FAffixGenerator)
	 * to keep the generation seed self-contained and replay-safe.
	 */
	void RollValue(FRandomStream& RandStream)
	{
		RolledStatValue = RandStream.RandRange(MinValue, MaxValue);
		RolledSecondaryStatValue = UsesValueRange() && bRollSecondaryValue
			? RandStream.RandRange(MinSecondaryValue, MaxSecondaryValue)
			: RolledStatValue;
		NormalizeRolledRange();
	}

	bool UsesValueRange() const
	{
		return ModifyType == EModifyType::MT_AddRange
			|| ModifyType == EModifyType::MT_MultiplyRange;
	}

	void NormalizeRolledRange()
	{
		if (UsesValueRange() && RolledSecondaryStatValue < RolledStatValue)
		{
			Swap(RolledStatValue, RolledSecondaryStatValue);
		}
	}

	int32 GetRankPointValue() const
	{
		return UItemEnumFunctionLibrary::GetRankPointsValue(RankPoints);
	}

	bool IsPrefix() const
	{
		return AffixType == EAffixes::AF_Prefix;
	}

	bool IsSuffix() const
	{
		return AffixType == EAffixes::AF_Suffix;
	}

	bool IsImplicit() const
	{
		return AffixType == EAffixes::AF_Implicit;
	}

	bool IsLocal() const
	{
		return ModifiedLocation == EAffixScope::AS_Local
			|| bAffectsBaseItemStats
			|| bIsLocalToWeapon
			|| bAffectsBaseWeaponStatsDirectly;
	}

	/**
	 * Strict complement of IsLocal.
	 *
	 * These two decide whether a modifier folds into the owning item's own stats
	 * or applies to the character, so anything they both accept would be applied
	 * twice. IsLocal is the wider test - AS_Global plus bAffectsBaseItemStats is
	 * a reachable authoring state - so this must be derived from it rather than
	 * testing AS_Global on its own.
	 */
	bool IsGlobal() const
	{
		return !IsLocal();
	}

	/**
	 * Check if this affix is allowed on given item type
	 */
	bool IsAllowedOnItemType(EItemType ItemType) const
	{
		if (ExcludedItemTypes.Contains(ItemType))
		{
			return false;
		}

		if (AllowedItemTypes.Num() == 0)
		{
			return true;
		}
		return AllowedItemTypes.Contains(ItemType);
	}

	/**
	 * Check if this affix is allowed on given item subtype
	 */
	bool IsAllowedOnSubType(EItemSubType ItemSubType) const
	{
		if (AllowedSubTypes.Num() == 0)
		{
			return true;
		}
		return AllowedSubTypes.Contains(ItemSubType);
	}

	/**
	 * Get weight for random selection (inverse of RankPoints tier distance).
	 * RP_0 = most common (weight 1000); each tier away from 0 is rarer -
	 * both higher quality tiers AND deeper corruption tiers.
	 *
	 * the old formula returned weight 1 for RankValue <= 0, making RP_0
	 * affixes 1000x rarer than RP_1 - a cliff no table author would expect.
	 * Relative rarity between positive tiers is preserved
	 * (1000/(1+r) vs 1000/r - near-identical ratios).
	 */
	int32 GetWeight() const
	{
		// Zero is an explicit "never", not a missing value; only the -1 sentinel
		// falls through to the RankPoints formula.
		if (SpawnWeight >= 0)
		{
			return SpawnWeight;
		}

		const int32 RankValue = UItemEnumFunctionLibrary::GetRankPointsValue(RankPoints);
		const int32 TierDistance = FMath::Abs(RankValue);
		return FMath::Clamp(1000 / (1 + TierDistance), 1, 1000);
	}

	/** False for affixes disabled by a zero weight. */
	bool CanEverSpawn() const { return GetWeight() > 0; }

	bool IsCorruptedAffix() const { return AffixType == EAffixes::AF_Corrupted; }

	FName GetStableAffixID() const { return AffixID.IsNone() ? AttributeName : AffixID; }

	// Use dedicated MinItemLevel/MaxItemLevel instead of MinValue/MaxValue.
	bool IsValidForItemLevel(int32 Level) const { return MinItemLevel <= Level && Level <= MaxItemLevel; }
};
