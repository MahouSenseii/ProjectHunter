// Item/Library/ItemStructs.h

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Item/Library/ItemEnums.h"
#include "Item/Library/AffixEnums.h"
#include "AttributeSet.h"
#include "ItemStructs.generated.h"

// Forward declarations
class UStaticMesh;
class USkeletalMesh;
class UMaterialInstance;
class UGameplayEffect;

// ═══════════════════════════════════════════════════════════════════════
// ATTACHMENT RULES
// ═══════════════════════════════════════════════════════════════════════

/**
 * Item attachment rules (how item attaches to character)
 */
USTRUCT(BlueprintType)
struct FItemAttachmentRules
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attachment")
	EPHAttachmentRule LocationRule = EPHAttachmentRule::AR_SnapToTarget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attachment")
	EPHAttachmentRule RotationRule = EPHAttachmentRule::AR_SnapToTarget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attachment")
	EPHAttachmentRule ScaleRule = EPHAttachmentRule::AR_KeepRelative;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attachment")
	bool bWeldSimulatedBodies = false;

	FItemAttachmentRules() = default;
};

// ═══════════════════════════════════════════════════════════════════════
// BASE WEAPON STATS
// ═══════════════════════════════════════════════════════════════════════

/**
 * Base weapon damage stats 
 */
USTRUCT(BlueprintType)
struct FBaseWeaponStats
{
    GENERATED_BODY()

    // Physical Damage
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Damage", meta = (ClampMin = "0.0"))
    float MinPhysicalDamage = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Damage", meta = (ClampMin = "0.0"))
    float MaxPhysicalDamage = 0.0f;

    // Elemental Damage
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Damage", meta = (ClampMin = "0.0"))
    float MinFireDamage = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Damage", meta = (ClampMin = "0.0"))
    float MaxFireDamage = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Damage", meta = (ClampMin = "0.0"))
    float MinIceDamage = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Damage", meta = (ClampMin = "0.0"))
    float MaxIceDamage = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Damage", meta = (ClampMin = "0.0"))
    float MinLightningDamage = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Damage", meta = (ClampMin = "0.0"))
    float MaxLightningDamage = 0.0f;

    // Special Damage
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Damage", meta = (ClampMin = "0.0"))
    float MinLightDamage = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Damage", meta = (ClampMin = "0.0"))
    float MaxLightDamage = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Damage", meta = (ClampMin = "0.0"))
    float MinCorruptionDamage = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Damage", meta = (ClampMin = "0.0"))
    float MaxCorruptionDamage = 0.0f;

    // Attack Stats
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Attack", meta = (ClampMin = "0.1", ClampMax = "10.0"))
    float AttackSpeed = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Attack", meta = (ClampMin = "0.0", ClampMax = "100.0"))
    float CriticalStrikeChance = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Attack", meta = (ClampMin = "0.1"))
    float Range = 1.0f;

    FBaseWeaponStats() = default;

    // ═══════════════════════════════════════════════
    // VALIDATION (Editor Only)
    // ═══════════════════════════════════════════════

#if WITH_EDITOR
    /**
     * Called when property is edited in DataTable
     * Automatically clamps Max to be >= Min
     */
    void OnDataTableChanged(const UDataTable* InDataTable, const FName InRowName)
    {
        ValidateMinMaxRanges();
    }

    void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
    {
        ValidateMinMaxRanges();
    }

private:
    void ValidateMinMaxRanges()
    {
        // Clamp Max values to be >= Min values
        if (MaxPhysicalDamage < MinPhysicalDamage)
            MaxPhysicalDamage = MinPhysicalDamage;
        
        if (MaxFireDamage < MinFireDamage)
            MaxFireDamage = MinFireDamage;
        
        if (MaxIceDamage < MinIceDamage)
            MaxIceDamage = MinIceDamage;
        
        if (MaxLightningDamage < MinLightningDamage)
            MaxLightningDamage = MinLightningDamage;
        
        if (MaxLightDamage < MinLightDamage)
            MaxLightDamage = MinLightDamage;
        
        if (MaxCorruptionDamage < MinCorruptionDamage)
            MaxCorruptionDamage = MinCorruptionDamage;
    }
#endif
};

// ═══════════════════════════════════════════════════════════════════════
// BASE ARMOR STATS
// ═══════════════════════════════════════════════════════════════════════

/**
 * Base armor/defense stats
 */
USTRUCT(BlueprintType)
struct FBaseArmorStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Armor")
	float Armor = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Armor|Resistances")
	float FireResistance = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Armor|Resistances")
	float IceResistance = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Armor|Resistances")
	float LightningResistance = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Armor|Resistances")
	float LightResistance = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Armor|Resistances")
	float CorruptionResistance = 0.0f;

	FBaseArmorStats() = default;
};

// ═══════════════════════════════════════════════════════════════════════
// STAT REQUIREMENTS
// ═══════════════════════════════════════════════════════════════════════

/**
 * Hunter stat requirements to equip item
 */
USTRUCT(BlueprintType)
struct FItemStatRequirement
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Requirements")
	int32 RequiredLevel = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Requirements")
	int32 RequiredStrength = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Requirements")
	int32 RequiredDexterity = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Requirements")
	int32 RequiredIntelligence = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Requirements")
	int32 RequiredEndurance = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Requirements")
	int32 RequiredAffliction = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Requirements")
	int32 RequiredLuck = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Requirements")
	int32 RequiredCovenant = 0;

	FItemStatRequirement() = default;

	/** Check if hunter meets all requirements */
	bool MeetsRequirements(
		int32 Level,
		int32 Strength,
		int32 Dexterity,
		int32 Intelligence,
		int32 Endurance,
		int32 Affliction,
		int32 Luck,
		int32 Covenant) const
	{
		return Level >= RequiredLevel
			&& Strength >= RequiredStrength
			&& Dexterity >= RequiredDexterity
			&& Intelligence >= RequiredIntelligence
			&& Endurance >= RequiredEndurance
			&& Affliction >= RequiredAffliction
			&& Luck >= RequiredLuck
			&& Covenant >= RequiredCovenant;
	}
};

// ═══════════════════════════════════════════════════════════════════════
// DURABILITY (Equipment)
// ═══════════════════════════════════════════════════════════════════════

/**
 * Durability system for equipment
 */
USTRUCT(BlueprintType)
struct FItemDurability
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Durability")
	float CurrentDurability = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Durability")
	float MaxDurability = 100.0f;

	FItemDurability() = default;

	void SetMaxDurability(float NewMax)
	{
		MaxDurability = NewMax;
		CurrentDurability = MaxDurability;
	}

	void Reduce(float Amount)
	{
		CurrentDurability = FMath::Max(0.0f, CurrentDurability - Amount);
	}

	void Repair(float Amount)
	{
		CurrentDurability = FMath::Min(MaxDurability, CurrentDurability + Amount);
	}

	void RepairFull()
	{
		CurrentDurability = MaxDurability;
	}

	bool IsBroken() const
	{
		return CurrentDurability <= 0.0f;
	}

	float GetDurabilityPercent() const
	{
		return MaxDurability > 0.0f ? (CurrentDurability / MaxDurability) : 0.0f;
	}
};

// ═══════════════════════════════════════════════════════════════════════
// RUNE CRAFTING
// ═══════════════════════════════════════════════════════════════════════

/**
 * Rune socket data 
 */
USTRUCT(BlueprintType)
struct FRuneSocket
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Rune")
	bool bIsSocketed = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Rune")
	FName RuneID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Rune")
	int32 RuneLevel = 0;

	FRuneSocket() = default;
};

/**
 * Rune crafting system 
 */
USTRUCT(BlueprintType)
struct FRuneCraftingData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Rune")
	TArray<FRuneSocket> RuneSockets;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Rune")
	int32 EnhancementLevel = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Rune")
	int32 MaxEnhancementLevel = 15;

	FRuneCraftingData() = default;

	int32 GetSocketCount() const { return RuneSockets.Num(); }
	int32 GetSocketedRuneCount() const
	{
		return RuneSockets.FilterByPredicate([](const FRuneSocket& Socket) {
			return Socket.bIsSocketed;
		}).Num();
	}
};

// ═══════════════════════════════════════════════════════════════════════
// CONSUMABLE DATA
// ═══════════════════════════════════════════════════════════════════════

/**
 * Consumable effect data (Potions, Food, Scrolls)
 */
USTRUCT(BlueprintType)
struct FConsumableData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Consumable")
	int32 MaxUses = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Consumable")
	float Cooldown = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Consumable")
	TArray<TSubclassOf<UGameplayEffect>> EffectsToApply;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Consumable")
	bool bConsumedOnUse = true;

	FConsumableData() = default;
};

// ═══════════════════════════════════════════════════════════════════════
// ATTRIBUTE DATA (For Affixes)
// ═══════════════════════════════════════════════════════════════════════

/**
 * Single attribute modification (used for affixes)
 * PoE2-Style: Each affix has ONE name (either prefix or suffix)
 * 
 * Can be used as DataTable row for DT_Affixes
 */
USTRUCT(BlueprintType)
struct FPHAttributeData : public FTableRowBase
{
	GENERATED_BODY()

	// ═══════════════════════════════════════════════
	// UNIQUE IDENTIFIER
	// ═══════════════════════════════════════════════

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Attribute")
	FGuid AttributeUID;

	// ═══════════════════════════════════════════════
	// AFFIX TYPE & NAMING 
	// ═══════════════════════════════════════════════

	/** Affix type: Prefix, Suffix, Implicit, etc. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute|Affix")
	EAffixes AffixType = EAffixes::AF_Prefix;
	

	/** Affix name for item naming (e.g., "Dragon's", "of the Fang") */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute|Affix")
	FText AffixName;

	/** Rank points for quality (-10 to +10) - Also determines weight (higher tier = rarer) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute|Affix")
	ERankPoints RankPoints = ERankPoints::RP_0;

	

	// ═══════════════════════════════════════════════
	// ITEM TYPE FILTERING 
	// ═══════════════════════════════════════════════

	/** Allowed item types (empty = all types allowed) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute|Filtering")
	TArray<EItemType> AllowedItemTypes;

	/** Allowed item subtypes (empty = all subtypes allowed) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute|Filtering")
	TArray<EItemSubType> AllowedSubTypes;

	// ═══════════════════════════════════════════════
	// MODIFICATION TARGET & SCOPE
	// ═══════════════════════════════════════════════

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

	// ═══════════════════════════════════════════════
	// VALUE RANGE & ROLLED VALUE
	// ═══════════════════════════════════════════════

	/** Minimum value for this affix */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute|Value")
	float MinValue = 0.0f;

	/** Maximum value for this affix */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute|Value")
	float MaxValue = 0.0f;

	/** Rolled stat value (generated on item creation) */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Attribute|Value")
	float RolledStatValue = 0.0f;

	// ═══════════════════════════════════════════════
	// DISPLAY
	// ═══════════════════════════════════════════════

	/** How to format this value for tooltip display */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute|Display")
	EAttributeDisplayFormat DisplayFormat = EAttributeDisplayFormat::ADF_Additive;

	/** Custom display text (optional override) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute|Display")
	FText DisplayText;

	/** Has this affix been identified? (for unidentified items) */
	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Attribute|Display")
	bool bIsIdentified = true;

	// ═══════════════════════════════════════════════
	// GAMEPLAY EFFECT 
	// ═══════════════════════════════════════════════

	/** Gameplay Effect to apply (optional, for complex effects) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute|GameplayEffect")
	TSubclassOf<UGameplayEffect> GameplayEffect;

	// ═══════════════════════════════════════════════
	// LEGACY COMPATIBILITY (Deprecated)
	// ═══════════════════════════════════════════════

	/** @deprecated Use ModifiedLocation instead */
	UPROPERTY()
	bool bIsLocalToWeapon = false;

	/** @deprecated Use ModifiedLocation instead */
	UPROPERTY()
	bool bAffectsBaseWeaponStatsDirectly = false;

	// ═══════════════════════════════════════════════
	// HELPER FUNCTIONS
	// ═══════════════════════════════════════════════

	FPHAttributeData() = default;

	void GenerateUID()
	{
		AttributeUID = FGuid::NewGuid();
	}

	void RollValue()
	{
		RolledStatValue = FMath::RandRange(MinValue, MaxValue);
	}

	int32 GetRankPointValue() const
	{
		return GetRankPointsValue(RankPoints);
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
	 * Get weight for random selection (inverse of RankPoints)
	 * Higher tier = rarer = lower weight
	 */
	int32 GetWeight() const
	{
		int32 RankValue = GetRankPointsValue(RankPoints);
		
		if (RankValue <= 0)
		{
			return 1;
		}
		
		int32 Weight = 1000 / FMath::Max(1, RankValue);
		return FMath::Clamp(Weight, 1, 1000);
	}
	
	bool IsCorruptedAffix() const { return AffixType == EAffixes::AF_Corrupted; }
	bool IsValidForItemLevel(int32 Level) const {return MinValue <= Level && Level <= MaxValue; }
};

// ═══════════════════════════════════════════════════════════════════════
// ITEM STATS (Collection of Affixes) - OPTIMIZED
// ═══════════════════════════════════════════════════════════════════════

/**
 * All stats/affixes on an item
 * 
 * OPTIMIZATIONS:
 * - GetAllStats() pre-allocates with Reserve()
 * - ForEachStat() for zero-allocation iteration
 * - Early-exit in HasUnidentifiedStats()
 */
USTRUCT(BlueprintType)
struct FPHItemStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Stats")
	TArray<FPHAttributeData> Prefixes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Stats")
	TArray<FPHAttributeData> Suffixes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Stats")
	TArray<FPHAttributeData> Implicits;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Stats")
	TArray<FPHAttributeData> Crafted;

	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Stats")
	bool bAffixesGenerated = false;

	FPHItemStats() = default;

	/** Get total count of all stats (zero allocation) */
	FORCEINLINE int32 GetTotalStatCount() const
	{
		return Implicits.Num() + Prefixes.Num() + Suffixes.Num() + Crafted.Num();
	}

	/** Get all stats combined - OPTIMIZED with Reserve() */
	TArray<FPHAttributeData> GetAllStats() const
	{
		TArray<FPHAttributeData> All;
		All.Reserve(GetTotalStatCount());
		All.Append(Implicits);
		All.Append(Prefixes);
		All.Append(Suffixes);
		All.Append(Crafted);
		return All;
	}

	/** Zero-allocation iteration over all stats */
	template<typename Func>
	void ForEachStat(Func&& Callback) const
	{
		for (const FPHAttributeData& Stat : Implicits) { Callback(Stat); }
		for (const FPHAttributeData& Stat : Prefixes) { Callback(Stat); }
		for (const FPHAttributeData& Stat : Suffixes) { Callback(Stat); }
		for (const FPHAttributeData& Stat : Crafted) { Callback(Stat); }
	}

	/** Zero-allocation iteration with index */
	template<typename Func>
	void ForEachStatIndexed(Func&& Callback) const
	{
		int32 Index = 0;
		for (const FPHAttributeData& Stat : Implicits) { Callback(Stat, Index++); }
		for (const FPHAttributeData& Stat : Prefixes) { Callback(Stat, Index++); }
		for (const FPHAttributeData& Stat : Suffixes) { Callback(Stat, Index++); }
		for (const FPHAttributeData& Stat : Crafted) { Callback(Stat, Index++); }
	}

	/** Zero-allocation find with predicate */
	template<typename Predicate>
	const FPHAttributeData* FindStat(Predicate&& Pred) const
	{
		for (const FPHAttributeData& Stat : Implicits) { if (Pred(Stat)) return &Stat; }
		for (const FPHAttributeData& Stat : Prefixes) { if (Pred(Stat)) return &Stat; }
		for (const FPHAttributeData& Stat : Suffixes) { if (Pred(Stat)) return &Stat; }
		for (const FPHAttributeData& Stat : Crafted) { if (Pred(Stat)) return &Stat; }
		return nullptr;
	}

	/** Find stat by name */
	const FPHAttributeData* FindStatByName(FName AttributeName) const
	{
		return FindStat([AttributeName](const FPHAttributeData& Stat) {
			return Stat.AttributeName == AttributeName;
		});
	}

	/** Get affix count (prefixes + suffixes only) */
	int32 GetTotalAffixCount() const
	{
		return Prefixes.Num() + Suffixes.Num();
	}

	/** Check for unidentified stats - OPTIMIZED with early exit */
	bool HasUnidentifiedStats() const
	{
		for (const FPHAttributeData& Stat : Implicits) { if (!Stat.bIsIdentified) return true; }
		for (const FPHAttributeData& Stat : Prefixes) { if (!Stat.bIsIdentified) return true; }
		for (const FPHAttributeData& Stat : Suffixes) { if (!Stat.bIsIdentified) return true; }
		for (const FPHAttributeData& Stat : Crafted) { if (!Stat.bIsIdentified) return true; }
		return false;
	}

	/** Total rank point value - OPTIMIZED with ForEachStat */
	float GetTotalAffixValue() const
	{
		float Total = 0.0f;
		ForEachStat([&Total](const FPHAttributeData& Stat) {
			Total += Stat.GetRankPointValue();
		});
		return Total;
	}

	/** Sum all bonuses for a specific attribute */
	float GetTotalValueForAttribute(FName AttributeName) const
	{
		float Total = 0.0f;
		ForEachStat([&Total, AttributeName](const FPHAttributeData& Stat) {
			if (Stat.AttributeName == AttributeName && Stat.bIsIdentified)
			{
				Total += Stat.RolledStatValue;
			}
		});
		return Total;
	}

	/** Check if empty */
	bool IsEmpty() const
	{
		return GetTotalStatCount() == 0;
	}

	/** Clear all stats */
	void Clear()
	{
		Implicits.Empty();
		Prefixes.Empty();
		Suffixes.Empty();
		Crafted.Empty();
		bAffixesGenerated = false;
	}
};

// ═══════════════════════════════════════════════════════════════════════
// ITEM BASE (DataTable Row)
// ═══════════════════════════════════════════════════════════════════════

/**
 * Base item definition (DataTable row)
 * Contains all static/shared data for an item type
 */
USTRUCT(BlueprintType)
struct FItemBase : public FTableRowBase
{
	GENERATED_BODY()

	// ═══════════════════════════════════════════════
	// IDENTITY
	// ═══════════════════════════════════════════════

	/** Is this a unique item? (If false, name is auto-generated from affixes) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Identity")
	bool bIsUnique = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item|Identity")
	FName ItemID = NAME_None;

	/** Only set name for unique items (Grade SS / EX-Rank) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Display", 
		meta = (EditCondition = "bIsUnique", EditConditionHides))
	FText ItemName = FText::GetEmpty();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Display", meta = (MultiLine = "true"))
	FText ItemDescription = FText::GetEmpty();
	
	// ═══════════════════════════════════════════════
	// CLASSIFICATION
	// ═══════════════════════════════════════════════

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Class")
	EItemType ItemType = EItemType::IT_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Class")
	EItemSubType ItemSubType = EItemSubType::IST_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Class")
	EItemRarity ItemRarity = EItemRarity::IR_GradeF;

	/** Only show for equipment types */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Class", 
		meta = (EditCondition = "ItemType == EItemType::IT_Weapon || ItemType == EItemType::IT_Armor || ItemType == EItemType::IT_Accessory", EditConditionHides))
	EEquipmentSlot EquipmentSlot = EEquipmentSlot::ES_None;

	/** Only show for weapons */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Class", 
		meta = (EditCondition = "ItemType == EItemType::IT_Weapon", EditConditionHides))
	EWeaponHandle WeaponHandle = EWeaponHandle::WH_None;

	// ═══════════════════════════════════════════════
	// VISUALS
	// ═══════════════════════════════════════════════

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Visual")
	TObjectPtr<UStaticMesh> StaticMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Visual")
	TObjectPtr<USkeletalMesh> SkeletalMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Visual")
	UMaterialInstance* ItemImage = nullptr;
	
	/** Only for weapons with actor representation */
	UPROPERTY(EditAnywhere, Category = "Item|Visual", 
		meta = (EditCondition = "ItemType == EItemType::IT_Weapon", EditConditionHides))
	bool bUseWeaponActor = false;

	UPROPERTY(EditAnywhere, Category = "Item|Visual", 
		meta = (EditCondition = "ItemType == EItemType::IT_Weapon && bUseWeaponActor", EditConditionHides))
	TSubclassOf<AActor> WeaponActorClass = nullptr;

	// ═══════════════════════════════════════════════
	// WEIGHT & STACKING
	// ═══════════════════════════════════════════════

	/** Base weight for single item  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Weight", meta = (ClampMin = "0.0"))
	float BaseWeight = 0.1f;

	/** 
	 * Can this item stack? 
	 * NOTE: Weapons, Armor, Accessories are NEVER stackable (unique instances)
	 * Only Consumables, Materials, Currency can stack
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Stacking",
		meta = (EditCondition = "ItemType != EItemType::IT_Weapon && ItemType != EItemType::IT_Armor && ItemType != EItemType::IT_Accessory", EditConditionHides))
	bool bStackable = false;

	/** Maximum stack size (only for stackable items) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Stacking", 
		meta = (EditCondition = "bStackable && ItemType != EItemType::IT_Weapon && ItemType != EItemType::IT_Armor && ItemType != EItemType::IT_Accessory", EditConditionHides, ClampMin = "1"))
	int32 MaxStackSize = 1;

	/** Scale weight with quantity? (only relevant for stackables) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Weight",
		meta = (EditCondition = "bStackable", EditConditionHides))
	bool bScaleWeightWithQuantity = true;

	// ═══════════════════════════════════════════════
	// VALUE & ECONOMY
	// ═══════════════════════════════════════════════

	/** Base gold value */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Economy", meta = (ClampMin = "0"))
	int32 Value = 0;

	/** Value modifier percentage */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Economy")
	float ValueModifier = 0.0f;

	/** Can be traded with other hunters? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Economy")
	bool bIsTradeable = true;

	// ═══════════════════════════════════════════════
	// FLAGS
	// ═══════════════════════════════════════════════

	/** Does this item need to be identified? (Only equipment) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Flags",
		meta = (EditCondition = "ItemType == EItemType::IT_Weapon || ItemType == EItemType::IT_Armor || ItemType == EItemType::IT_Accessory", EditConditionHides))
	bool bCanBeIdentified = true;

	// ═══════════════════════════════════════════════
	// ATTACHMENT (Equipment Only)
	// ═══════════════════════════════════════════════

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Attachment",
		meta = (EditCondition = "ItemType == EItemType::IT_Weapon || ItemType == EItemType::IT_Armor || ItemType == EItemType::IT_Accessory", EditConditionHides))
	FName AttachmentSocket = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Attachment",
		meta = (EditCondition = "ItemType == EItemType::IT_Weapon || ItemType == EItemType::IT_Armor || ItemType == EItemType::IT_Accessory", EditConditionHides))
	TMap<FName, FName> ContextualSockets;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Attachment",
		meta = (EditCondition = "ItemType == EItemType::IT_Weapon || ItemType == EItemType::IT_Armor || ItemType == EItemType::IT_Accessory", EditConditionHides))
	FItemAttachmentRules AttachmentRules;

	// ═══════════════════════════════════════════════
	// BASE STATS (Equipment Only)
	// ═══════════════════════════════════════════════

	/** Only show for weapons */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Stats",
		meta = (EditCondition = "ItemType == EItemType::IT_Weapon", EditConditionHides))
	FBaseWeaponStats WeaponStats;

	/** Only show for armor */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Stats",
		meta = (EditCondition = "ItemType == EItemType::IT_Armor", EditConditionHides))
	FBaseArmorStats ArmorStats;

	/** Only show for equipment */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Stats",
		meta = (EditCondition = "ItemType == EItemType::IT_Weapon || ItemType == EItemType::IT_Armor || ItemType == EItemType::IT_Accessory", EditConditionHides))
	FItemStatRequirement StatRequirements;

	// ═══════════════════════════════════════════════
	// DURABILITY (Equipment Only)
	// ═══════════════════════════════════════════════

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Durability",
		meta = (EditCondition = "ItemType == EItemType::IT_Weapon || ItemType == EItemType::IT_Armor || ItemType == EItemType::IT_Accessory", EditConditionHides))
	float MaxDurability = 100.0f;

	// ═══════════════════════════════════════════════
	// IMPLICITS (Equipment Only)
	// ═══════════════════════════════════════════════

	/** Always-present affixes (e.g., "+10 Fire Resistance" on Fire Cloak) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Affixes",
		meta = (EditCondition = "ItemType == EItemType::IT_Weapon || ItemType == EItemType::IT_Armor || ItemType == EItemType::IT_Accessory", EditConditionHides))
	TArray<FPHAttributeData> ImplicitMods;

	// ═══════════════════════════════════════════════
	// UNIQUE ITEMS (Grade SS / EX-Rank)
	// ═══════════════════════════════════════════════

	/** Fixed affixes for unique items (not randomly generated) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Unique", 
		meta = (EditCondition = "bIsUnique", EditConditionHides))
	TArray<FPHAttributeData> UniqueAffixes;

	// ═══════════════════════════════════════════════
	// CONSUMABLE DATA 
	// ═══════════════════════════════════════════════

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Consumable",
		meta = (EditCondition = "ItemType == EItemType::IT_Consumable", EditConditionHides))
	FConsumableData ConsumableData;

	/** Shortcut accessors for consumable data */
	int32 GetMaxUses() const { return ConsumableData.MaxUses; }
	float GetCooldown() const { return ConsumableData.Cooldown; }

	// ═══════════════════════════════════════════════
	// RUNE CRAFTING 
	// ═══════════════════════════════════════════════

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Runes",
		meta = (EditCondition = "ItemType == EItemType::IT_Weapon || ItemType == EItemType::IT_Armor || ItemType == EItemType::IT_Accessory", EditConditionHides))
	int32 MaxRuneSockets = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Runes",
		meta = (EditCondition = "ItemType == EItemType::IT_Weapon || ItemType == EItemType::IT_Armor || ItemType == EItemType::IT_Accessory", EditConditionHides))
	int32 MaxEnhancementLevel = 15;

	// ═══════════════════════════════════════════════
	// HELPER FUNCTIONS
	// ═══════════════════════════════════════════════

	/** Check if base data is valid */
	bool IsValid() const
	{
		const bool bHasAnyMesh = (StaticMesh != nullptr) || (SkeletalMesh != nullptr);
		return !ItemName.IsEmpty() && ItemType != EItemType::IT_None && bHasAnyMesh;
	}

	/** Check if valid for inventory */
	bool IsValidForInventory() const
	{
		if (!IsValid()) return false;
		if (BaseWeight < 0.0f) return false;
		
		// Equipment should never be stackable
		if (IsEquippable() && bStackable)
		{
			UE_LOG(LogTemp, Error, TEXT("ItemBase: Equipment '%s' has bStackable=true! This is invalid."), *ItemID.ToString());
			return false;
		}
		
		// Stackable validation
		if (bStackable && MaxStackSize <= 0) return false;
		if (!bStackable && MaxStackSize > 1) return false;
		
		return true;
	}

	/** Check if this is a weapon */
	bool IsWeapon() const { return ItemType == EItemType::IT_Weapon; }

	/** Check if this is armor */
	bool IsArmor() const { return ItemType == EItemType::IT_Armor; }

	/** Check if this is an accessory */
	bool IsAccessory() const { return ItemType == EItemType::IT_Accessory; }

	/** Check if this can be equipped */
	bool IsEquippable() const
	{
		return ItemType == EItemType::IT_Weapon
			|| ItemType == EItemType::IT_Armor
			|| ItemType == EItemType::IT_Accessory;
	}

	/** Check if this is a consumable */
	bool IsConsumable() const { return ItemType == EItemType::IT_Consumable; }

	/** Check if this is a material */
	bool IsMaterial() const { return ItemType == EItemType::IT_Material; }

	/** Check if this is currency */
	bool IsCurrency() const { return ItemType == EItemType::IT_Currency; }

	/** Get socket name for context */
	FName GetSocketForContext(FName Context) const
	{
		if (const FName* ContextSocket = ContextualSockets.Find(Context))
		{
			return *ContextSocket;
		}
		return AttachmentSocket;
	}

	/**
	 * Calculate value based on rarity and quantity
	 */
	float GetCalculatedValue(int32 Quantity = 1, EItemRarity InstanceRarity = EItemRarity::IR_None) const
	{
		float CalcValue = static_cast<float>(Value) * (1.0f + ValueModifier);

		const EItemRarity RarityToUse = (InstanceRarity != EItemRarity::IR_None) ? InstanceRarity : ItemRarity;

		float RarityMult = 1.0f;
		switch (RarityToUse)
		{
			case EItemRarity::IR_GradeF:  RarityMult = 1.0f; break;
			case EItemRarity::IR_GradeE:  RarityMult = 1.5f; break;
			case EItemRarity::IR_GradeD:  RarityMult = 2.5f; break;
			case EItemRarity::IR_GradeC:  RarityMult = 5.0f; break;
			case EItemRarity::IR_GradeB:  RarityMult = 10.0f; break;
			case EItemRarity::IR_GradeA:  RarityMult = 25.0f; break;
			case EItemRarity::IR_GradeS:  RarityMult = 100.0f; break;
			case EItemRarity::IR_GradeSS: RarityMult = 1000.0f; break;
			default: break;
		}
		CalcValue *= RarityMult;

		if (bStackable)
		{
			CalcValue *= FMath::Max(1, Quantity);
		}

		return FMath::Max(0.0f, CalcValue);
	}

	/**
	 * Get weight for quantity
	 */
	float GetTotalWeight(int32 Quantity = 1) const
	{
		if (bStackable && bScaleWeightWithQuantity)
		{
			return BaseWeight * FMath::Max(1, Quantity);
		}
		return BaseWeight;
	}

	FItemBase() = default;
};