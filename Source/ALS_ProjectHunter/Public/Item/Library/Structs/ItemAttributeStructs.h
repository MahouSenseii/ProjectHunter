// Item/Library/Structs/ItemAttributeStructs.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "AttributeSet.h"
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

	/** Explicit generation weight. Zero preserves legacy RankPoints-based weighting. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute|Generation", meta = (ClampMin = "0"))
	int32 SpawnWeight = 0;

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


	/** Minimum value for this affix */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute|Value")
	float MinValue = 0.0f;

	/** Maximum value for this affix */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute|Value")
	float MaxValue = 0.0f;

	/** Rolled stat value (generated on item creation) */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Attribute|Value")
	float RolledStatValue = 0.0f;


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
	}

	/**
	 * Seeded overload so affix generation is fully deterministic.
	 * Call this when a FRandomStream is available (e.g. inside FAffixGenerator)
	 * to keep the generation seed self-contained and replay-safe.
	 */
	void RollValue(FRandomStream& RandStream)
	{
		RolledStatValue = RandStream.RandRange(MinValue, MaxValue);
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
		return ModifiedLocation == EAffixScope::AS_Local;
	}

	bool IsGlobal() const
	{
		return ModifiedLocation == EAffixScope::AS_Global;
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
		if (SpawnWeight > 0)
		{
			return SpawnWeight;
		}

		const int32 RankValue = UItemEnumFunctionLibrary::GetRankPointsValue(RankPoints);
		const int32 TierDistance = FMath::Abs(RankValue);
		return FMath::Clamp(1000 / (1 + TierDistance), 1, 1000);
	}

	bool IsCorruptedAffix() const { return AffixType == EAffixes::AF_Corrupted; }

	FName GetStableAffixID() const { return AffixID.IsNone() ? AttributeName : AffixID; }

	// Use dedicated MinItemLevel/MaxItemLevel instead of MinValue/MaxValue.
	bool IsValidForItemLevel(int32 Level) const { return MinItemLevel <= Level && Level <= MaxItemLevel; }
};
