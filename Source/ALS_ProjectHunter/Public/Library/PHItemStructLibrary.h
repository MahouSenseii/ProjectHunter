// PHItemStructLibrary.h
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameplayEffectTypes.h"
#include "AttributeSet.h"
#include "Engine/DataTable.h"
#include "PHItemEnumLibrary.h"
#include "PHItemStructLibrary.generated.h"

/* ============================= */
/* ===   FORWARD DECLARATIONS === */
/* ============================= */

enum class EALSOverlayState : uint8;
class UGameplayEffect;
class UItemDefinitionAsset;
class UStaticMesh;
class USkeletalMesh;
class UMaterialInstance;
class AItemPickup;
class AEquippedObject;

struct FGameplayAttribute;


/* ============================= */
/* ===   TILE SYSTEM         === */
/* ============================= */

USTRUCT(BlueprintType)
struct FTile
{
	GENERATED_BODY()
	
	FTile() : X(0), Y(0) {}
	FTile(const int32 InX, const int32 InY) : X(InX), Y(InY) {}

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 X;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Y;
	
	bool operator==(const FTile& Other) const { return X == Other.X && Y == Other.Y; }
};

FORCEINLINE uint32 GetTypeHash(const FTile& Tile) 
{ 
	return HashCombine(GetTypeHash(Tile.X), GetTypeHash(Tile.Y)); 
}

USTRUCT(BlueprintType)
struct FTileLoopInfo
{
	GENERATED_BODY()
	
	FTileLoopInfo() : X(0), Y(0), bIsTileAvailable(false) {}

	UPROPERTY(BlueprintReadWrite)
	int32 X;
	
	UPROPERTY(BlueprintReadWrite)
	int32 Y;
	
	UPROPERTY(BlueprintReadWrite)
	bool bIsTileAvailable;
};

/* ============================= */
/* ===   CORE STRUCTS        === */
/* ============================= */

USTRUCT(BlueprintType)
struct FDamageRange
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Min = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Max = 0.0f;

	FDamageRange() = default;
	FDamageRange(float InMin, float InMax) : Min(InMin), Max(InMax) {}
};

USTRUCT(BlueprintType)
struct FItemDurability
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	float MaxDurability = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	float CurrentDurability = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bCanBreak = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bSelfRepairing = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float RepairCostMultiplier = 1.0f;

	float GetDurabilityPercent() const 
	{ 
		return MaxDurability > 0.0f ? CurrentDurability / MaxDurability : 0.0f; 
	}

	bool IsBroken() const 
	{ 
		return bCanBreak && CurrentDurability <= 0.0f; 
	}

	FItemDurability()
	{
		CurrentDurability = MaxDurability;
	}
};

USTRUCT(BlueprintType)
struct FItemStatRequirement
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float RequiredStrength = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float RequiredDexterity = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float RequiredIntelligence = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float RequiredEndurance = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float RequiredAffliction = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float RequiredLuck = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float RequiredCovenant = 0.0f;

	FItemStatRequirement() = default;
};

USTRUCT(BlueprintType)
struct FBaseWeaponStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AttackSpeed = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CritChance = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<EDamageTypes, FDamageRange> BaseDamage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float WeaponRange = 100.0f;

	FBaseWeaponStats() = default;
};

USTRUCT(BlueprintType)
struct FBaseArmorStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Armor = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Poise = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<EDefenseTypes, float> Resistances;

	FBaseArmorStats() = default;
};

USTRUCT(BlueprintType)
struct FDefenseStats
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Defense")
	float ArmorFlat = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category = "Defense")
	float FireResistanceFlat = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category = "Defense")
	float IceResistanceFlat = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category = "Defense")
	float LightningResistanceFlat = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category = "Defense")
	float LightResistanceFlat = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category = "Defense")
	float CorruptionResistanceFlat = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category = "Defense")
	float ArmorPercent = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category = "Defense")
	float FireResistancePercent = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category = "Defense")
	float IceResistancePercent = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category = "Defense")
	float LightningResistancePercent = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category = "Defense")
	float LightResistancePercent = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category = "Defense")
	float CorruptionResistancePercent = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category = "Defense")
	float GlobalDefenses = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category = "Defense")
	float BlockStrength = 0.0f;

	FDefenseStats() = default;
};

/* ============================= */
/* ===   ATTACHMENT SYSTEM   === */
/* ============================= */

USTRUCT(BlueprintType)
struct FItemAttachmentRules
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EPHAttachmentRule LocationRule = EPHAttachmentRule::SnapToTarget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EPHAttachmentRule RotationRule = EPHAttachmentRule::SnapToTarget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EPHAttachmentRule ScaleRule = EPHAttachmentRule::KeepRelative;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bWeldSimulatedBodies = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attachment")
	FTransform AttachmentOffset = FTransform::Identity;

	FAttachmentTransformRules ToAttachmentRules() const
	{
		FAttachmentTransformRules R(
			ToEngineRule(LocationRule),
			ToEngineRule(RotationRule),
			ToEngineRule(ScaleRule),
			bWeldSimulatedBodies
		);
		return R;
	}

	static FItemAttachmentRules DefaultWeaponRules()
	{
		FItemAttachmentRules R;
		R.LocationRule = EPHAttachmentRule::SnapToTarget;
		R.RotationRule = EPHAttachmentRule::SnapToTarget;
		R.ScaleRule    = EPHAttachmentRule::KeepRelative;
		R.bWeldSimulatedBodies = true;
		return R;
	}
};

/* ============================= */
/* ===   AFFIX SYSTEM        === */
/* ============================= */

// PHItemStructLibrary.h

USTRUCT(BlueprintType)
struct FPHAttributeData : public FTableRowBase
{
    GENERATED_BODY()

    // === Core Attribute ===
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute")
    FGameplayAttribute ModifiedAttribute;

    // === Rolled Value ===
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Value")
    float RolledStatValue = 0.0f;

    // === Reroll System ===
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reroll")
    FGuid ModifierUID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Value")
    bool bRollsAsInteger = true;

    // === Roll Range ===
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Value")
    FVector2D StatRollRange = FVector2D(0.0f, 0.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Value")
    float MinStatChanged = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Value")
    float MaxStatChanged = 0.0f;

    // === Display ===
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display")
    FText StatDisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display")
    FName AttributeName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display")
    EAttributeDisplayFormat DisplayFormat = EAttributeDisplayFormat::Additive;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display")
    bool bDisplayAsRange = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display")
    FText CustomDisplayFormatText;

    // === Identification ===
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identification", SaveGame)
    bool bIsIdentified = true;

    // === Affix Classification ===
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix")
    EAffixes PrefixSuffix = EAffixes::Prefix;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix")
    FName AffixID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix")
    ERankPoints RankPoints = ERankPoints::RP_0;

    // === Gameplay Tags ===
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tags")
    FGameplayTagContainer AffectedTags;

    // === Application Rules ===
    
    /** 
     * If true, this stat modifies the weapon's base damage/stats directly
     * (e.g., "Adds 10-20 Physical Damage" to weapon)
     * If false, it's applied to the character's stats
     * (e.g., "+15% Increased Physical Damage" to character)
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Application")
    bool bAffectsBaseWeaponStatsDirectly = false;

    /** 
     * If true, this stat applies to local weapon only
     * If false, it's a global character stat
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Application")
    bool bIsLocalToWeapon = true;

    // === Helper Functions ===
    
    void UpdateMinMaxFromRange()
    {
        if (MinStatChanged == 0.0f && MaxStatChanged == 0.0f)
        {
            MinStatChanged = StatRollRange.X;
            MaxStatChanged = StatRollRange.Y;
        }
    }

    float RollValue()
    {
        if (StatRollRange.X == StatRollRange.Y)
        {
            RolledStatValue = StatRollRange.X;
        }
        else
        {
            RolledStatValue = FMath::FRandRange(StatRollRange.X, StatRollRange.Y);
        }
        
        UpdateMinMaxFromRange();
        return RolledStatValue;
    }

    /**
     * Checks if this attribute data is valid and properly configured
     * @return true if the data is valid, false otherwise
     */
    bool IsValid() const
    {
        // Check if the core attribute is valid
        if (!ModifiedAttribute.IsValid())
        {
            return false;
        }

        // Check if stat roll range is properly configured (min <= max)
        if (StatRollRange.X > StatRollRange.Y)
        {
            return false;
        }

        // Check if attribute name is set
        if (AttributeName.IsNone())
        {
            return false;
        }

        // Check if display name is set (optional but recommended)
        // Uncomment if you want to enforce display names
        // if (StatDisplayName.IsEmpty())
        // {
        //     return false;
        // }

        return true;
    }

    /**
     * Checks if this attribute data has been properly initialized
     * More lenient than IsValid() - just checks minimum requirements
     * @return true if initialized, false otherwise
     */
    bool IsInitialized() const
    {
        return ModifiedAttribute.IsValid() && !AttributeName.IsNone();
    }

    FPHAttributeData() = default;
};

// PHItemStructLibrary.h

USTRUCT(BlueprintType)
struct FPHItemStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FPHAttributeData> Prefixes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FPHAttributeData> Suffixes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FPHAttributeData> Implicits;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FPHAttributeData> Crafted;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bAffixesGenerated = false;

	// === Helper Functions ===

	/** Get all stats combined */
	TArray<FPHAttributeData> GetAllStats() const
	{
		TArray<FPHAttributeData> All;
		All.Reserve(Prefixes.Num() + Suffixes.Num() + Implicits.Num() + Crafted.Num());
		All.Append(Prefixes);
		All.Append(Suffixes);
		All.Append(Implicits);
		All.Append(Crafted);
		return All;
	}

	/** Get total affix count */
	int32 GetTotalAffixCount() const
	{
		return Prefixes.Num() + Suffixes.Num();
	}

	/** Get total stat count including implicits and crafted */
	int32 GetTotalStatCount() const
	{
		return Prefixes.Num() + Suffixes.Num() + Implicits.Num() + Crafted.Num();
	}

	/** Get prefix count */
	int32 GetPrefixCount() const { return Prefixes.Num(); }

	/** Get suffix count */
	int32 GetSuffixCount() const { return Suffixes.Num(); }

	/** Get implicit count */
	int32 GetImplicitCount() const { return Implicits.Num(); }

	/** Get crafted mod count */
	int32 GetCraftedCount() const { return Crafted.Num(); }

	/** Check if any stats are unidentified */
	bool HasUnidentifiedStats() const
	{
		auto HasUnidentified = [](const TArray<FPHAttributeData>& Stats) -> bool
		{
			for (const FPHAttributeData& Stat : Stats)
			{
				if (!Stat.bIsIdentified)
					return true;
			}
			return false;
		};

		return HasUnidentified(Prefixes) || 
		       HasUnidentified(Suffixes) || 
		       HasUnidentified(Implicits) || 
		       HasUnidentified(Crafted);
	}

	/** Get count of unidentified stats */
	int32 GetUnidentifiedCount() const
	{
		auto CountUnidentified = [](const TArray<FPHAttributeData>& Stats) -> int32
		{
			int32 Count = 0;
			for (const FPHAttributeData& Stat : Stats)
			{
				if (!Stat.bIsIdentified)
					Count++;
			}
			return Count;
		};

		return CountUnidentified(Prefixes) + 
		       CountUnidentified(Suffixes) + 
		       CountUnidentified(Implicits) + 
		       CountUnidentified(Crafted);
	}

	/** Check if item has max affixes (3 prefix, 3 suffix) */
	bool HasMaxAffixes() const
	{
		return Prefixes.Num() >= 3 && Suffixes.Num() >= 3;
	}

	/** Check if can add prefix */
	bool CanAddPrefix() const { return Prefixes.Num() < 3; }

	/** Check if can add suffix */
	bool CanAddSuffix() const { return Suffixes.Num() < 3; }

	/** Get all stats matching a specific tag */
	TArray<FPHAttributeData> GetStatsByTag(const FGameplayTag& Tag) const
	{
		TArray<FPHAttributeData> MatchingStats;
		
		for (const FPHAttributeData& Stat : GetAllStats())
		{
			if (Stat.AffectedTags.HasTag(Tag))
			{
				MatchingStats.Add(Stat);
			}
		}
		
		return MatchingStats;
	}

	/** Get all stats affecting a specific attribute */
	TArray<FPHAttributeData> GetStatsByAttribute(const FGameplayAttribute& Attribute) const
	{
		TArray<FPHAttributeData> MatchingStats;
		
		for (const FPHAttributeData& Stat : GetAllStats())
		{
			if (Stat.ModifiedAttribute == Attribute)
			{
				MatchingStats.Add(Stat);
			}
		}
		
		return MatchingStats;
	}


	/** Clear all stats */
	void ClearAllStats()
	{
		Prefixes.Empty();
		Suffixes.Empty();
		Implicits.Empty();
		Crafted.Empty();
		bAffixesGenerated = false;
	}

	/** Clear only affixes (keep implicits) */
	void ClearAffixes()
	{
		Prefixes.Empty();
		Suffixes.Empty();
		bAffixesGenerated = false;
	}

	/** Check if item has any stats */
	bool HasAnyStats() const
	{
		return Prefixes.Num() > 0 || 
		       Suffixes.Num() > 0 || 
		       Implicits.Num() > 0 || 
		       Crafted.Num() > 0;
	}

	/** Check if valid for use */
	bool IsValid() const
	{
		return bAffixesGenerated && HasAnyStats();
	}

	FPHItemStats() = default;
};

USTRUCT(BlueprintType)
struct FAffixesEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Generation")
	EAffixes Kind = EAffixes::Prefix;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Generation")
	FPHAttributeData Data;
};

/* ============================= */
/* ===   PASSIVE EFFECTS     === */
/* ============================= */

USTRUCT(BlueprintType)
struct FItemPassiveEffectInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName PassiveID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText PassiveName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UGameplayEffect> EffectClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Level = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsActive = true;

	FItemPassiveEffectInfo() = default;
};

USTRUCT()
struct FPassiveEffectHandleList
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FActiveGameplayEffectHandle> Handles;
};

/* ============================= */
/* ===   RUNE SYSTEM         === */
/* ============================= */

USTRUCT(BlueprintType)
struct FAppliedRune
{
	GENERATED_BODY()

	UPROPERTY(SaveGame, BlueprintReadOnly)
	FName RuneID;

	UPROPERTY(SaveGame, BlueprintReadOnly)
	EItemRarity RuneRarity;

	UPROPERTY(SaveGame, BlueprintReadOnly)
	FGameplayTag RuneEffect;

	UPROPERTY(SaveGame, BlueprintReadOnly)
	int32 RuneLevel = 1;

	UPROPERTY(SaveGame, BlueprintReadOnly)
	float EffectMagnitude = 1.0f;

	UPROPERTY(SaveGame, BlueprintReadOnly)
	FDateTime AppliedTimestamp;

	FAppliedRune() = default;
};

USTRUCT(BlueprintType)
struct FRuneCraftingData
{
	GENERATED_BODY()

	// Applied runes on this item
	UPROPERTY(SaveGame, BlueprintReadOnly)
	TArray<FAppliedRune> AppliedRunes;

	// Rune slots based on rarity (set at initialization)
	UPROPERTY(SaveGame, BlueprintReadOnly)
	int32 MaxRuneSlots = 1;

	// Crafting quality bonus from rarity (0.0 - 1.0)
	UPROPERTY(SaveGame, BlueprintReadOnly)
	float RarityQualityBonus = 0.0f;

	// Track crafting attempts for increasing difficulty
	UPROPERTY(SaveGame, BlueprintReadOnly)
	int32 TotalCraftingAttempts = 0;

	FRuneCraftingData() = default;
};

USTRUCT(BlueprintType)
struct FRuneDefinition : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName RuneID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText RuneName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText RuneDescription;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EItemRarity RuneRarity;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag EffectTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<UGameplayEffect> RuneEffect;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BaseSuccessChance = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float DurabilityCostPerAttempt = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EItemRarity MinimumItemRarity = EItemRarity::IR_GradeF;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector2D MagnitudeRange = FVector2D(1.0f, 1.5f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UTexture2D* RuneIcon;
};

/* ============================= */
/* ===   EFFECT TRACKING     === */
/* ============================= */

USTRUCT(BlueprintType)
struct FTrackedEffect
{
	GENERATED_BODY()

	UPROPERTY()
	FActiveGameplayEffectHandle Handle;

	UPROPERTY()
	EEffectSource Source;

	UPROPERTY()
	FName SourceID;

	UPROPERTY()
	TWeakObjectPtr<UObject> SourceObject;

	UPROPERTY()
	float AppliedTimestamp;

	UPROPERTY()
	bool bIsPermanent = true;

	FTrackedEffect() : Source(), AppliedTimestamp(0.0f)
	{
	}
};

USTRUCT()
struct FEffectRegistry
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FTrackedEffect> ActiveEffects;

	// Quick lookups (not serialized, rebuilt on load)
	TMap<FName, TArray<int32>> EffectsBySourceID;
	TMap<EEffectSource, TArray<int32>> EffectsByType;
	TMap<FActiveGameplayEffectHandle, int32> HandleToIndex;

	void AddEffect(const FTrackedEffect& Effect)
	{
		int32 Index = ActiveEffects.Add(Effect);
		HandleToIndex.Add(Effect.Handle, Index);
		EffectsBySourceID.FindOrAdd(Effect.SourceID).Add(Index);
		EffectsByType.FindOrAdd(Effect.Source).Add(Index);
	}

	void RemoveEffect(const FActiveGameplayEffectHandle& Handle)
	{
		if (int32* IndexPtr = HandleToIndex.Find(Handle))
		{
			int32 Index = *IndexPtr;
			if (ActiveEffects.IsValidIndex(Index))
			{
				const FTrackedEffect& Effect = ActiveEffects[Index];

				if (TArray<int32>* Arr = EffectsBySourceID.Find(Effect.SourceID))
				{
					Arr->Remove(Index);
				}
				if (TArray<int32>* Arr = EffectsByType.Find(Effect.Source))
				{
					Arr->Remove(Index);
				}

				ActiveEffects.RemoveAt(Index);
				HandleToIndex.Remove(Handle);
				RebuildIndices();
			}
		}
	}

	TArray<FTrackedEffect> GetEffectsBySource(EEffectSource Source) const
	{
		TArray<FTrackedEffect> Results;
		if (const TArray<int32>* Indices = EffectsByType.Find(Source))
		{
			for (int32 Idx : *Indices)
			{
				if (ActiveEffects.IsValidIndex(Idx))
				{
					Results.Add(ActiveEffects[Idx]);
				}
			}
		}
		return Results;
	}

	TArray<FTrackedEffect> GetEffectsBySourceID(FName SourceID) const
	{
		TArray<FTrackedEffect> Results;
		if (const TArray<int32>* Indices = EffectsBySourceID.Find(SourceID))
		{
			for (int32 Idx : *Indices)
			{
				if (ActiveEffects.IsValidIndex(Idx))
				{
					Results.Add(ActiveEffects[Idx]);
				}
			}
		}
		return Results;
	}

private:
	void RebuildIndices()
	{
		HandleToIndex.Empty();
		EffectsBySourceID.Empty();
		EffectsByType.Empty();

		for (int32 i = 0; i < ActiveEffects.Num(); ++i)
		{
			const FTrackedEffect& Effect = ActiveEffects[i];
			HandleToIndex.Add(Effect.Handle, i);
			EffectsBySourceID.FindOrAdd(Effect.SourceID).Add(i);
			EffectsByType.FindOrAdd(Effect.Source).Add(i);
		}
	}
};

/* ============================= */
/* ===   ITEM DEFINITION     === */
/* ===   (Static/Shared)     === */
/* ============================= */

USTRUCT(BlueprintType)
struct FItemBase : public FTableRowBase
{
	GENERATED_BODY()

	// Pointers (8 bytes each on 64-bit)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<AItemPickup> PickupClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	mutable UStaticMesh* StaticMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USkeletalMesh* SkeletalMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UMaterialInstance* ItemImage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UMaterialInstance* ItemImageRotated;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UGameplayEffect> GameplayEffectClass;

	// 4-byte types
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trade")
	int32 Value;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MaxStackSize;

	// Floats (4 bytes)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base")
	float BaseGradeValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trade")
	float ValueModifier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Weight")
	float BaseWeight;

	// Enums (typically 1 byte)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EItemType ItemType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EItemSubType ItemSubType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EItemRarity ItemRarity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EEquipmentSlot EquipmentSlot;

	// Bools (1 byte each but padded)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trade")
	bool IsTradeable;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool Stackable;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Weight")
	bool bScaleWeightWithQuantity;

	// Larger types last
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FIntPoint Dimensions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ItemID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ItemTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Attachment")
	FName AttachmentSocket;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText ItemName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText BaseTypeName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText ItemDescription;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Attachment")
	TMap<FName, FName> ContextualSockets;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Attachment")
	FItemAttachmentRules AttachmentRules;

	FItemBase()
		: PickupClass(nullptr)
		, StaticMesh(nullptr)
		, SkeletalMesh(nullptr)
		, ItemImage(nullptr)
		, ItemImageRotated(nullptr)
		, GameplayEffectClass(nullptr)
		, Value(0)
		, MaxStackSize(1)
		, BaseGradeValue(0.0f)
		, ValueModifier(0.0f)
		, BaseWeight(0.1f)
		, ItemType(EItemType::IT_None)
		, ItemSubType(EItemSubType::IST_None)
		, ItemRarity(EItemRarity::IR_None)
		, EquipmentSlot(EEquipmentSlot::ES_None)
		, IsTradeable(false)
		, Stackable(false)
		, bScaleWeightWithQuantity(true)
		, Dimensions(FIntPoint::ZeroValue)
		, ItemID(NAME_None)
		, ItemTag(NAME_None)
		, AttachmentSocket(NAME_None)
		, ItemName(FText::GetEmpty())
		, Description(FText::GetEmpty())
		, BaseTypeName(FText::GetEmpty())
		, ItemDescription(FText::GetEmpty())
	{}

	FORCEINLINE bool IsValid() const
	{
		const bool bHasAnyMesh = (StaticMesh != nullptr) || (SkeletalMesh != nullptr);
		return !ItemName.IsEmpty() && ItemType != EItemType::IT_None && bHasAnyMesh;
	}

	bool HasValidAttachment() const
	{
		return !AttachmentSocket.IsNone() || ContextualSockets.Num() > 0;
	}

	float GetTotalWeight(int32 Quantity = 1) const
	{
		if (Stackable && bScaleWeightWithQuantity)
		{
			return BaseWeight * FMath::Max(1, Quantity);
		}
		return BaseWeight;
	}

	float GetWeightPerUnit() const { return BaseWeight; }

	float GetCalculatedValue(int32 Quantity = 1, EItemRarity InstanceRarity = EItemRarity::IR_None) const
	{
		float BaseValue = static_cast<float>(Value);
		BaseValue *= (1.0f + ValueModifier);

		EItemRarity RarityToUse = (InstanceRarity != EItemRarity::IR_None) ? InstanceRarity : ItemRarity;

		float RarityMultiplier = 1.0f;
		switch (RarityToUse)
		{
		case EItemRarity::IR_GradeS: RarityMultiplier = 10.0f; break;
		case EItemRarity::IR_GradeA: RarityMultiplier = 5.0f; break;
		case EItemRarity::IR_GradeB: RarityMultiplier = 4.5f; break;
		case EItemRarity::IR_GradeC: RarityMultiplier = 3.5f; break;
		case EItemRarity::IR_GradeD: RarityMultiplier = 2.0f; break;
		case EItemRarity::IR_GradeF: RarityMultiplier = 1.0f; break;
		default: break;
		}
		BaseValue *= RarityMultiplier;

		if (Stackable)
		{
			BaseValue *= FMath::Max(1, Quantity);
		}

		return FMath::Max(0.0f, BaseValue);
	}

	bool IsValidForInventory() const
	{
		if (!IsValid()) return false;

		if (Dimensions.X <= 0 || Dimensions.Y <= 0 || 
			Dimensions.X > 10 || Dimensions.Y > 10)
		{
			return false;
		}

		if (Stackable && MaxStackSize <= 0)
		{
			return false;
		}

		if (!Stackable && MaxStackSize > 1)
		{
			return false;
		}

		if (BaseWeight < 0.0f || BaseWeight > 10000.0f)
		{
			return false;
		}

		return true;
	}

	static int32 GetRarityScore(EItemRarity R)
	{
		switch (R)
		{
		case EItemRarity::IR_GradeS: return 6;
		case EItemRarity::IR_GradeA: return 5;
		case EItemRarity::IR_GradeB: return 4;
		case EItemRarity::IR_GradeC: return 3;
		case EItemRarity::IR_GradeD: return 2;
		case EItemRarity::IR_GradeF: return 1;
		default: return 0;
		}
	}

	EItemComparisonResult CompareTo(const FItemBase& Other, EItemRarity ThisInstanceRarity = EItemRarity::IR_None, EItemRarity OtherInstanceRarity = EItemRarity::IR_None) const
	{
		if (ItemType != Other.ItemType)
		{
			return EItemComparisonResult::Incomparable;
		}

		EItemRarity ThisRarity = (ThisInstanceRarity != EItemRarity::IR_None) ? ThisInstanceRarity : ItemRarity;
		EItemRarity OtherRarity = (OtherInstanceRarity != EItemRarity::IR_None) ? OtherInstanceRarity : Other.ItemRarity;

		int32 ThisRarityScore = GetRarityScore(ThisRarity);
		int32 OtherRarityScore = GetRarityScore(OtherRarity);

		if (ThisRarityScore != OtherRarityScore)
		{
			return ThisRarityScore > OtherRarityScore ? 
				   EItemComparisonResult::Better : 
				   EItemComparisonResult::Worse;
		}

		float ThisValue = GetCalculatedValue(1, ThisRarity);
		float OtherValue = Other.GetCalculatedValue(1, OtherRarity);

		if (FMath::IsNearlyEqual(ThisValue, OtherValue))
		{
			return EItemComparisonResult::Equal;
		}

		return ThisValue > OtherValue ? 
			   EItemComparisonResult::Better : 
			   EItemComparisonResult::Worse;
	}
};

/* ============================= */
/* ===   ITEM INSTANCE DATA  === */
/* ===   (Runtime/Per-Item)  === */
/* ============================= */

USTRUCT(BlueprintType)
struct FItemInstanceData
{
	GENERATED_BODY()

	// === Identity ===
	UPROPERTY(SaveGame, BlueprintReadOnly)
	FString UniqueID;

	UPROPERTY(SaveGame, BlueprintReadOnly)
	FString OwnerID;

	// === Inventory State ===
	UPROPERTY(SaveGame, BlueprintReadWrite)
	int32 Quantity;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	bool bRotated;

	UPROPERTY(SaveGame, BlueprintReadOnly)
	ECurrentItemSlot LastSavedSlot;

	UPROPERTY(SaveGame)
	FTransform WorldTransform;

	// === Procedural Properties ===
	UPROPERTY(SaveGame, BlueprintReadWrite)
	int32 ItemLevel;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	EItemRarity Rarity;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	bool bIdentified;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	int32 Seed;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	FText DisplayName;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	bool bHasNameBeenGenerated;

	// === Affixes ===
	UPROPERTY(SaveGame, BlueprintReadWrite)
	TArray<FPHAttributeData> Prefixes;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	TArray<FPHAttributeData> Suffixes;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	TArray<FPHAttributeData> Crafted;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	TArray<FPHAttributeData> Implicits;

	// === Durability ===
	UPROPERTY(SaveGame, BlueprintReadWrite)
	FItemDurability Durability;

	// === Rune Crafting ===
	UPROPERTY(SaveGame, BlueprintReadWrite)
	FRuneCraftingData RuneCraftingData;

	// === Economy (instance-specific) ===
	UPROPERTY(SaveGame, BlueprintReadWrite)
	int32 CurrentValue;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	float ValueModifier;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	bool bIsTradeable;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	bool bIsSoulbound;

	// === Runtime Effects (NOT saved, transient) ===
	UPROPERTY(Transient)
	TArray<FActiveGameplayEffectHandle> AppliedEffectHandles;

	UPROPERTY(Transient)
	bool bEffectsActive;

	// === Cached State (NOT saved, transient) ===
	UPROPERTY(Transient)
	bool bCacheDirty;

	// PROPER CONSTRUCTOR
	FItemInstanceData()
		: UniqueID(TEXT(""))
		, OwnerID(TEXT(""))
		, Quantity(0)
		, bRotated(false)
		, LastSavedSlot(ECurrentItemSlot::CIS_None)
		, WorldTransform(FTransform::Identity)
		, ItemLevel(1)
		, Rarity(EItemRarity::IR_GradeF)
		, bIdentified(true)
		, Seed(0)
		, DisplayName(FText::GetEmpty())
		, bHasNameBeenGenerated(false)
		, Durability()
		, RuneCraftingData()
		, CurrentValue(0)
		, ValueModifier(1.0f)
		, bIsTradeable(true)
		, bIsSoulbound(false)
		, bEffectsActive(false)
		, bCacheDirty(true)
	{
	
	}

	/** Quick guard to ensure invariants after load / spawn */
	void Sanitize()
	{
		if (!WorldTransform.IsValid()) 
		{ 
			WorldTransform = FTransform::Identity; 
		}
		
		Quantity = FMath::Max(0, Quantity);
		ItemLevel = FMath::Max(1, ItemLevel);
		ValueModifier = FMath::IsFinite(ValueModifier) ? ValueModifier : 1.0f;
		
		Prefixes.RemoveAll([](const FPHAttributeData& D){ return !D.IsValid(); });
		Suffixes.RemoveAll([](const FPHAttributeData& D){ return !D.IsValid(); });
		Crafted.RemoveAll([](const FPHAttributeData& D){ return !D.IsValid(); });
		Implicits.RemoveAll([](const FPHAttributeData& D){ return !D.IsValid(); });
		
		if (!StaticEnum<ECurrentItemSlot>()->IsValidEnumValue(static_cast<int64>(LastSavedSlot)))
		{
			LastSavedSlot = ECurrentItemSlot::CIS_None;
		}
	}
};

/**
 * Drop table entry for loot spawning
 */
USTRUCT(BlueprintType)
struct FDropTable : public FTableRowBase
{
	GENERATED_BODY()

	/** The weight/rating for this item to be selected (higher = more likely) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drop")
	int32 DropRating = 1;

	/** Reference to the item definition asset */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drop")
	TSoftObjectPtr<UItemDefinitionAsset> ItemDefinition;

	/** Minimum quantity to drop */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drop")
	int32 MinQuantity = 1;

	/** Maximum quantity to drop */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drop")
	int32 MaxQuantity = 1;

	FDropTable()
		: DropRating(1)
		, MinQuantity(1)
		, MaxQuantity(1)
	{}
};

/* ============================= */
/* ===   EQUIPMENT DATA      === */
/* ============================= */

USTRUCT(BlueprintType)
struct FCollisionInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName CollisionSocketStart = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName CollisionSocketEnd = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CollisionRadius = 10.0f;

	FCollisionInfo() = default;
};

USTRUCT(BlueprintType)
struct FEquippableItemData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	TSubclassOf<AEquippedObject> EquipClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	EEquipmentSlot EquipSlot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	EALSOverlayState OverlayState;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FName ItemName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Requirements")
	FItemStatRequirement StatRequirements;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base Stats")
	FBaseWeaponStats WeaponBaseStats;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base Stats")
	FBaseArmorStats ArmorBaseStats;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Passive Effects")
	TArray<FItemPassiveEffectInfo> PassiveEffects;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affixes")
	FPHItemStats Affixes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	EWeaponHandle WeaponHandle = EWeaponHandle::WH_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	FCollisionInfo CollisionInfo;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Requirements")
	TMap<EItemRequiredStatsCategory, float> RequirementStats;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Durability")
	FItemDurability Durability;

	FEquippableItemData()
		: EquipSlot(EEquipmentSlot::ES_None), OverlayState()
	{
	}

	FORCEINLINE bool IsValid() const
	{
		const bool bHasValidAffixes = Affixes.bAffixesGenerated;
		const bool bHasPassiveEffects = !PassiveEffects.IsEmpty();
		const bool bHasWeaponType = WeaponHandle != EWeaponHandle::WH_None;

		return bHasValidAffixes || bHasPassiveEffects || bHasWeaponType;
	}

	mutable TOptional<TMap<EItemRequiredStatsCategory, float>> CachedReqMap;

	const TMap<EItemRequiredStatsCategory, float>& GetRequirementMap() const
	{
		if (!CachedReqMap.IsSet())
		{
			TMap<EItemRequiredStatsCategory, float> M;
			M.Reserve(8);
			M.Emplace(EItemRequiredStatsCategory::ISC_RequiredStrength,     StatRequirements.RequiredStrength);
			M.Emplace(EItemRequiredStatsCategory::ISC_RequiredDexterity,    StatRequirements.RequiredDexterity);
			M.Emplace(EItemRequiredStatsCategory::ISC_RequiredIntelligence, StatRequirements.RequiredIntelligence);
			M.Emplace(EItemRequiredStatsCategory::ISC_RequiredEndurance,    StatRequirements.RequiredEndurance);
			M.Emplace(EItemRequiredStatsCategory::ISC_RequiredAffliction,   StatRequirements.RequiredAffliction);
			M.Emplace(EItemRequiredStatsCategory::ISC_RequiredLuck,         StatRequirements.RequiredLuck);
			M.Emplace(EItemRequiredStatsCategory::ISC_RequiredCovenant,     StatRequirements.RequiredCovenant);
			CachedReqMap = MoveTemp(M);
		}
		return CachedReqMap.GetValue();
	}

	bool MeetsRequirements(const FItemStatRequirement& PlayerStats) const
	{
		return PlayerStats.RequiredStrength >= StatRequirements.RequiredStrength &&
			   PlayerStats.RequiredIntelligence >= StatRequirements.RequiredIntelligence &&
			   PlayerStats.RequiredDexterity >= StatRequirements.RequiredDexterity &&
			   PlayerStats.RequiredEndurance >= StatRequirements.RequiredEndurance &&
			   PlayerStats.RequiredAffliction >= StatRequirements.RequiredAffliction &&
			   PlayerStats.RequiredLuck >= StatRequirements.RequiredLuck &&
			   PlayerStats.RequiredCovenant >= StatRequirements.RequiredCovenant;
	}
};

/* ============================= */
/* ===   CONSUMABLE DATA     === */
/* ============================= */

USTRUCT(BlueprintType)
struct FConsumableItemData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Consumable")
	TSubclassOf<UGameplayEffect> GameplayEffectClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	int32 Quantity = 0;

	FConsumableItemData() = default;
};

/* ============================= */
/* ===   APPLIED STATS       === */
/* ============================= */

USTRUCT()
struct FAppliedItemStats
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FGameplayAttribute> Stats;

	UPROPERTY()
	TArray<float> Values;

	UPROPERTY()
	TArray<FGameplayAttribute> BaseDamageAttributes;

	UPROPERTY()
	TArray<float> BaseDamageValues;
};

/* ============================= */
/* ===   SPAWNABLE ITEMS     === */
/* ============================= */

USTRUCT(BlueprintType)
struct FSpawnableItemRow_DA : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UItemDefinitionAsset> BaseDef;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drop")
	int32 DropRating = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FString Notes;
};

/* ============================= */
/* ===   LEGACY STRUCTS      === */
/* ===   (For Compatibility) === */
/* ============================= */

USTRUCT(BlueprintType)
struct FItemInformation
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base")
	FItemBase ItemInfo;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base")
	FEquippableItemData ItemData;

	FORCEINLINE bool IsValid() const
	{
		return ItemInfo.IsValid() || ItemData.IsValid();
	}
};

USTRUCT(BlueprintType)
struct FItemInstance
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 ItemLevel = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EItemRarity Rarity = EItemRarity::IR_GradeF;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIdentified = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FPHAttributeData> Prefixes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FPHAttributeData> Suffixes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FPHAttributeData> Crafted;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FPHAttributeData> Implicits;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FItemDurability Durability;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Seed = 0;
};