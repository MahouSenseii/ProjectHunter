#pragma once

#include "CoreMinimal.h"
#include "Library/ALSCharacterEnumLibrary.h"
#include "PHItemEnumLibrary.h"
#include "Engine/DataTable.h"
#include "Engine/EngineTypes.h" 
#include "GameplayEffect.h"
#include "PHItemStructLibrary.generated.h"


class UItemDefinitionAsset;
/* ============================= */
/* === Forward Declarations === */
/* ============================= */
class AItemPickup;
class AEquippableItem;
class AEquippedObject;
class APickup;
class AConsumablePickup;
class AWeaponPickup;

/* ============================= */
/* === Utility Structs === */
/* ============================= */
USTRUCT(BlueprintType)
struct FMinMax
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float Min;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float Max;

	FMinMax() : Min(0), Max(0) {}
	FMinMax(float InMin, float InMax) : Min(InMin), Max(InMax) {}
};



USTRUCT(BlueprintType)
struct FCollisionInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision")
	int NumberOfTraces = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision")
	float DamageTraceRadius = 0.0;
};


USTRUCT(BlueprintType)
struct FDamageRange
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Min = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Max = 0.0f;

	FDamageRange operator+(const FDamageRange& Other) const { return FDamageRange{ Min + Other.Min, Max + Other.Max }; }
	FDamageRange operator*(const float Scale) const { return FDamageRange{ Min * Scale, Max * Scale }; }
    
	float GetAverage() const { return (Min + Max) * 0.5f; }
	float GetRange() const { return Max - Min; }
};

/* ============================= */
/* === Tile System Structs === */
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
FORCEINLINE uint32 GetTypeHash(const FTile& Tile) { return HashCombine(GetTypeHash(Tile.X), GetTypeHash(Tile.Y)); }

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
/* === Core Stat Structs === */
/* ============================= */
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
};

/* ============================= */
/* === Attribute & Effect Structs === */
/* ============================= */
USTRUCT(BlueprintType)
struct FItemPassiveEffectInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UGameplayEffect> EffectClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Level = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bPersistent = true;
};

USTRUCT(BlueprintType)
struct FItemStatRequirement
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Requirements") float RequiredLevel = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Requirements") float RequiredStrength = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Requirements") float RequiredIntelligence = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Requirements") float RequiredDexterity = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Requirements") float RequiredEndurance = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Requirements") float RequiredAffliction = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Requirements") float RequiredLuck = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Requirements") float RequiredCovenant = 0.0f;
};

/* ============================= */
/* === Affix & Modifier Structs === */
/* ============================= */


USTRUCT(BlueprintType)
struct FInventoryWeightInfo
{
	GENERATED_BODY()
    
	UPROPERTY(BlueprintReadOnly)
	float CurrentWeight = 0.0f;
    
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxWeight = 100.0f;
    
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float OverweightSpeedPenalty = 0.5f;  // 50% speed reduction when overweight
    
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float OverweightThreshold = 0.9f;  // 90% capacity triggers "heavy" status
    
	float GetWeightPercent() const 
	{ 
		return MaxWeight > 0 ? CurrentWeight / MaxWeight : 0.0f; 
	}
    
	bool IsOverweight() const 
	{ 
		return CurrentWeight > MaxWeight; 
	}
    
	bool IsHeavy() const 
	{ 
		return GetWeightPercent() >= OverweightThreshold; 
	}
    
	float GetRemainingCapacity() const 
	{ 
		return FMath::Max(0.0f, MaxWeight - CurrentWeight); 
	}
    
	bool CanAddWeight(float AdditionalWeight) const 
	{ 
		return (CurrentWeight + AdditionalWeight) <= MaxWeight; 
	}
};

USTRUCT(BlueprintType)
struct FPHAttributeData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName AttributeName;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EPrefixSuffix PrefixSuffix;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayAttribute ModifiedAttribute;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FGameplayTag> AffectedTags;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MinStatChanged;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxStatChanged;
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Transient)
	float RolledStatValue = 0.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float PercentBonus = 0.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bDisplayAsRange;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsIdentified;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EAttributeDisplayFormat DisplayFormat;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ERankPoints RankPoints;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EAffixOrigin AffixOrigin;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 AffixTier = 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bAffectsBaseWeaponStatsDirectly = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ModifierID;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MaxStacks = 1;
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	int32 StackCount = 1;
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	FGuid ModifierUID;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Weight = 1.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bShowInTooltip = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bRollsAsInteger = false;
};

USTRUCT(BlueprintType)
struct FPHItemStats  
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	bool bAffixesGenerated = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	TArray<FPHAttributeData> Prefixes;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	TArray<FPHAttributeData> Suffixes;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	TArray<FPHAttributeData> Implicits;
	
        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
        TArray<FPHAttributeData> Crafted;



	FORCEINLINE TArray<FPHAttributeData> GetAllStats() const
        {
        	TArray<FPHAttributeData> Out;
        	const int32 TotalSize = Prefixes.Num() + Suffixes.Num() + Implicits.Num() + Crafted.Num();
        	Out.Reserve(TotalSize); 
        	Out.Append(Prefixes);
        	Out.Append(Suffixes);
        	Out.Append(Implicits);
        	Out.Append(Crafted);
        	return Out;
        }


	int32 GetTotalAffixCount() const
        {
        	return Prefixes.Num() + Suffixes.Num() + Implicits.Num() + Crafted.Num();
        }
    
	bool HasAnyAffixes() const
        {
        	return GetTotalAffixCount() > 0;
        }
    
	void ClearAllAffixes()
        {
        	Prefixes.Empty();
        	Suffixes.Empty();
        	Implicits.Empty();
        	Crafted.Empty();
        	bAffixesGenerated = false;
        }
};

/* ============================= */
/* === Equippable & Item Data Structs === */
/* ============================= */


USTRUCT(BlueprintType)
struct FItemDurability
{
	GENERATED_BODY()
    
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Durability")
	float MaxDurability = 100.0f;
    
	UPROPERTY(BlueprintReadWrite, Category = "Durability")
	float CurrentDurability = 100.0f;
    
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Durability")
	float DurabilityLossRate = 1.0f;
    
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Durability")
	bool bCanBreak = true;
    
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Durability")
	float BrokenWeightMultiplier = 1.5f;  // Broken items might be heavier
    
	float GetDurabilityPercent() const 
	{ 
		return MaxDurability > 0 ? CurrentDurability / MaxDurability : 0.0f; 
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
struct FPrefixSuffixEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Generation")
	EPrefixSuffix Kind = EPrefixSuffix::Prefix;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Generation")
	FPHAttributeData Data;
};

USTRUCT(BlueprintType)
struct FEquippableItemData
{
	GENERATED_BODY()
	

    // --- Common Equipment Properties ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    TSubclassOf<AEquippedObject> EquipClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    EEquipmentSlot EquipSlot;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    FName ItemName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Requirements")
    FItemStatRequirement StatRequirements;

    // --- Base Stats ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base Stats")
    FBaseWeaponStats WeaponBaseStats;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base Stats")
    FBaseArmorStats ArmorBaseStats;

    // --- Passive Effects --- //
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Passive Effects")
    TArray<FItemPassiveEffectInfo> PassiveEffects;

    // --- Affixes --- //
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affixes")
    FPHItemStats Affixes;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
    EWeaponHandle WeaponHandle = EWeaponHandle::WH_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
    EALSOverlayState OverlayState = EALSOverlayState::Default;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
    FCollisionInfo CollisionInfo;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Requirements")
    TMap<EItemRequiredStatsCategory, float> RequirementStats;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Durability")
	FItemDurability Durability;

    FEquippableItemData()
        : EquipSlot(EEquipmentSlot::ES_None)
	{}

	FORCEINLINE bool IsValid() const
    {
    	const bool bHasValidAffixes = Affixes.bAffixesGenerated;
    	const bool bHasPassiveEffects = !PassiveEffects.IsEmpty();
    	const bool bHasWeaponType = WeaponHandle != EWeaponHandle::WH_None;

    	return  bHasValidAffixes || bHasPassiveEffects || bHasWeaponType;
    }
	
	// on-demand, transient cache for quick lookups
	mutable TOptional<TMap<EItemRequiredStatsCategory, float>> CachedReqMap;

	const TMap<EItemRequiredStatsCategory, float>& GetRequirementMap() const
	{
		if (!CachedReqMap.IsSet())
		{
			TMap<EItemRequiredStatsCategory, float> M;
			M.Add(EItemRequiredStatsCategory::ISC_RequiredLevel,        StatRequirements.RequiredLevel);
			M.Add(EItemRequiredStatsCategory::ISC_RequiredStrength,     StatRequirements.RequiredStrength);
			M.Add(EItemRequiredStatsCategory::ISC_RequiredDexterity,    StatRequirements.RequiredDexterity);
			M.Add(EItemRequiredStatsCategory::ISC_RequiredIntelligence, StatRequirements.RequiredIntelligence);
			M.Add(EItemRequiredStatsCategory::ISC_RequiredEndurance,    StatRequirements.RequiredEndurance);
			M.Add(EItemRequiredStatsCategory::ISC_RequiredAffliction,   StatRequirements.RequiredAffliction);
			M.Add(EItemRequiredStatsCategory::ISC_RequiredLuck,         StatRequirements.RequiredLuck);
			M.Add(EItemRequiredStatsCategory::ISC_RequiredCovenant,     StatRequirements.RequiredCovenant);
			CachedReqMap = MoveTemp(M);
		}
		return CachedReqMap.GetValue();
	}
	
	bool MeetsRequirements(const FItemStatRequirement& PlayerStats) const
    {
    	return PlayerStats.RequiredLevel >= StatRequirements.RequiredLevel &&
			   PlayerStats.RequiredStrength >= StatRequirements.RequiredStrength &&
			   PlayerStats.RequiredIntelligence >= StatRequirements.RequiredIntelligence &&
			   PlayerStats.RequiredDexterity >= StatRequirements.RequiredDexterity &&
			   PlayerStats.RequiredEndurance >= StatRequirements.RequiredEndurance &&
			   PlayerStats.RequiredAffliction >= StatRequirements.RequiredAffliction &&
			   PlayerStats.RequiredLuck >= StatRequirements.RequiredLuck &&
			   PlayerStats.RequiredCovenant >= StatRequirements.RequiredCovenant;
    }

};

USTRUCT(BlueprintType)
struct FConsumableItemData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Consumable")
	TSubclassOf<UGameplayEffect> GameplayEffectClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	int Quantity = 0;
};


/* ============================= */
/* === Equippable & Item Data Structs === */
/* ============================= */

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


UENUM(BlueprintType)
enum class EPHAttachmentRule : uint8
{
	KeepRelative  UMETA(DisplayName="Keep Relative"),
	KeepWorld     UMETA(DisplayName="Keep World"),
	SnapToTarget  UMETA(DisplayName="Snap To Target"),
};

FORCEINLINE EAttachmentRule ToEngineRule(EPHAttachmentRule R)
{
	switch (R)
	{
	case EPHAttachmentRule::KeepRelative: return EAttachmentRule::KeepRelative;
	case EPHAttachmentRule::KeepWorld:    return EAttachmentRule::KeepWorld;
	case EPHAttachmentRule::SnapToTarget: return EAttachmentRule::SnapToTarget;
	default:                               return EAttachmentRule::KeepRelative;
	}
}


USTRUCT(BlueprintType)
struct FItemAttachmentRules
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attachment")
    EPHAttachmentRule LocationRule = EPHAttachmentRule::SnapToTarget;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attachment")
    EPHAttachmentRule RotationRule = EPHAttachmentRule::SnapToTarget;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attachment")
    EPHAttachmentRule ScaleRule    = EPHAttachmentRule::KeepRelative;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attachment")
    bool bWeldSimulatedBodies = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attachment")
    FTransform AttachmentOffset;

    // Convert to engine type
    FAttachmentTransformRules ToAttachmentRules() const
    {
        return FAttachmentTransformRules(
            ToEngineRule(LocationRule),
            ToEngineRule(RotationRule),
            ToEngineRule(ScaleRule),
            bWeldSimulatedBodies
        );
    }

    // Presets
    static FItemAttachmentRules SnapToTarget()
    {
        FItemAttachmentRules R;
        R.LocationRule = EPHAttachmentRule::SnapToTarget;
        R.RotationRule = EPHAttachmentRule::SnapToTarget;
        R.ScaleRule    = EPHAttachmentRule::SnapToTarget;
        return R;
    }

    static FItemAttachmentRules KeepRelative()
    {
        FItemAttachmentRules R;
        R.LocationRule = EPHAttachmentRule::KeepRelative;
        R.RotationRule = EPHAttachmentRule::KeepRelative;
        R.ScaleRule    = EPHAttachmentRule::KeepRelative;
        return R;
    }

    static FItemAttachmentRules KeepWorld()
    {
        FItemAttachmentRules R;
        R.LocationRule = EPHAttachmentRule::KeepWorld;
        R.RotationRule = EPHAttachmentRule::KeepWorld;
        R.ScaleRule    = EPHAttachmentRule::KeepWorld;
        return R;
    }

    static FItemAttachmentRules WeaponDefault()
    {
        FItemAttachmentRules R;
        R.LocationRule = EPHAttachmentRule::SnapToTarget;
        R.RotationRule = EPHAttachmentRule::SnapToTarget;
        R.ScaleRule    = EPHAttachmentRule::KeepRelative;
        R.bWeldSimulatedBodies = true;
        return R;
    }
};




USTRUCT(BlueprintType)
struct FItemBase: public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base")
	float BaseGradeValue;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<AItemPickup> PickupClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	mutable UStaticMesh* StaticMesh;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USkeletalMesh* SkeletalMesh;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString OwnerID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ItemID;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText ItemName;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Description;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ItemTag;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UMaterialInstance* ItemImage;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UMaterialInstance* ItemImageRotated;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EItemType ItemType;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EItemSubType ItemSubType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText BaseTypeName;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EItemRarity ItemRarity;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite,  Category = " Trade")
	int Value;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = " Trade")	
	float ValueModifier;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = " Trade")
	bool IsTradeable;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FIntPoint Dimensions;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EEquipmentSlot EquipmentSlot;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText ItemDescription;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool Stackable;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Quantity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MaxStackSize;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool Rotated;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ECurrentItemSlot LastSavedSlot;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FTransform Transform;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UGameplayEffect> GameplayEffectClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bHasNameBeenGenerated;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Attachment")
	FName AttachmentSocket;
    
	// Add contextual sockets for different states
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Attachment")
	TMap<FName, FName> ContextualSockets;  // e.g., "Combat" -> "WeaponSocket"
    
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Attachment")
	FItemAttachmentRules AttachmentRules;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Weight")
	float BaseWeight; 
    
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Weight")
	bool bScaleWeightWithQuantity;

	FItemBase()
		: BaseGradeValue(0.0f)
		, StaticMesh(nullptr)
		, SkeletalMesh(nullptr)
		, OwnerID("")
		, ItemID(NAME_None)
		, ItemName(FText::GetEmpty())
		, Description(FText::GetEmpty())
		, ItemTag(NAME_None)
		, ItemImage(nullptr)
		, ItemImageRotated(nullptr)
		, ItemType(EItemType::IT_None)
		, ItemSubType(EItemSubType::IST_None)
		, ItemRarity(EItemRarity::IR_None)
		, Value(0)
		, ValueModifier(0.0f)
		, IsTradeable(false)
		, Dimensions(FIntPoint::ZeroValue)
		, EquipmentSlot(EEquipmentSlot::ES_None)
		, ItemDescription(FText::GetEmpty())
		, Stackable(false)
		, Quantity(1)
		, MaxStackSize(1)
		, Rotated(false)
		, LastSavedSlot(ECurrentItemSlot::CIS_None)
		, Transform(FTransform::Identity)
		, GameplayEffectClass(nullptr)
		, bHasNameBeenGenerated(false)
		, AttachmentSocket(NAME_None)
		, BaseWeight(0.1f)
		, bScaleWeightWithQuantity(true)
	{
	}

	FORCEINLINE bool IsValid() const
	{
		const bool bHasAnyMesh = (StaticMesh != nullptr) || (SkeletalMesh != nullptr);
		return !ItemName.IsEmpty() && ItemType != EItemType::IT_None && bHasAnyMesh;
	}

	bool HasValidAttachment() const
	{
		return !AttachmentSocket.IsNone() || ContextualSockets.Num() > 0;
	}

	float GetTotalWeight() const
	{
		if (Stackable && bScaleWeightWithQuantity)
		{
			return BaseWeight * FMath::Max(1, Quantity);
		}
		return BaseWeight;
	}
    
	float GetWeightPerUnit() const { return BaseWeight; }


	float GetCalculatedValue() const
	{
		float BaseValue = static_cast<float>(Value);
        
		// Apply value modifier
		BaseValue *= (1.0f + ValueModifier);
		
		float RarityMultiplier = 1.0f;
		switch (ItemRarity)
		{
		case EItemRarity::IR_GradeS: RarityMultiplier = 10.0f; break;
		case EItemRarity::IR_GradeA: RarityMultiplier = 5.0f; break;
		case EItemRarity::IR_GradeB: RarityMultiplier = 4.5f; break;
		case EItemRarity::IR_GradeC: RarityMultiplier = 3.5f; break;
		case EItemRarity::IR_GradeD: RarityMultiplier = 2.0f; break;
		case EItemRarity::IR_GradeF: RarityMultiplier = 1.0f; break;
		default: ;
		}
		BaseValue *= RarityMultiplier;
        
		// Stack value for quantity
		if (Stackable)
		{
			BaseValue *= FMath::Max(1, Quantity);
		}
        
		return FMath::Max(0.0f, BaseValue);
	}


	
	bool IsValidForInventory() const
	{
		if (!IsValid()) return false;
    
		// Check dimensions are reasonable
		if (Dimensions.X <= 0 || Dimensions.Y <= 0 || 
			Dimensions.X > 10 || Dimensions.Y > 10)
		{
			return false;
		}
    
		// Check quantity logic
		if (Stackable && MaxStackSize <= 0)
		{
			return false;
		}
    
		if (!Stackable && Quantity > 1)
		{
			return false;
		}
    
		// Check weight is reasonable
		if (BaseWeight < 0.0f || BaseWeight > 10000.0f)
		{
			return false;
		}
    
		return true;
	}

	static int32 GetRarityScore(EItemRarity R)
	{
		switch (R) {
		case EItemRarity::IR_GradeS: return 6;
		case EItemRarity::IR_GradeA: return 5;
		case EItemRarity::IR_GradeB: return 4;
		case EItemRarity::IR_GradeC: return 3;
		case EItemRarity::IR_GradeD: return 2;
		case EItemRarity::IR_GradeF: return 1;
		default: return 0; // Unknown/Corrupted handled elsewhere
		}
	}

	EItemComparisonResult CompareTo(const FItemBase& Other) const
	{
		// Can't compare different item types
		if (ItemType != Other.ItemType)
		{
			return EItemComparisonResult::Incomparable;
		}
    
		// Compare by rarity first
		if (GetRarityScore != Other.GetRarityScore)
		{
			return GetRarityScore > Other.GetRarityScore ? 
				   EItemComparisonResult::Better : 
				   EItemComparisonResult::Worse;
		}
    
		// Compare by value
		float ThisValue = GetCalculatedValue();
		float OtherValue = Other.GetCalculatedValue();
    
		if (FMath::IsNearlyEqual(ThisValue, OtherValue))
		{
			return EItemComparisonResult::Equal;
		}
    
		return ThisValue > OtherValue ? 
			   EItemComparisonResult::Better : 
			   EItemComparisonResult::Worse;
	}
};



USTRUCT(BlueprintType)
struct FDropTable : public FTableRowBase
{

	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName RowName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	float DropRating = 0;
};




USTRUCT(BlueprintType)
struct FPHItemText
{
	GENERATED_BODY()
public:	
	FString Text;
	float Min;
	float Max;
};


USTRUCT()
struct FAppliedStats
{
        GENERATED_BODY()

        UPROPERTY()
        TArray<FPHAttributeData> Stats;

        UPROPERTY()
        TArray<float> RolledValues;

        /** Attributes and values added from base weapon damage. */
        UPROPERTY()
        TArray<FGameplayAttribute> BaseDamageAttributes;

        UPROPERTY()
        TArray<float> BaseDamageValues;
};



USTRUCT()
struct FPassiveEffectHandleList
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FActiveGameplayEffectHandle> Handles;
};

// PHItemStructLibrary.h (replace the old version)
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

	// Runtime roll data
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 ItemLevel = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) EItemRarity Rarity = EItemRarity::IR_GradeF;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bIdentified = true;

	// Rolled affixes (same type you already use for affix rows)
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FPHAttributeData> Prefixes;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FPHAttributeData> Suffixes;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FPHAttributeData> Crafted;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FPHAttributeData> Implicits;

	// Durability per instance
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FItemDurability Durability;

	// Cosmetic / runtime only
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Seed = 0; // for deterministic rerolls
};


USTRUCT(BlueprintType)
struct FSpawnableItemRow_DA : public FTableRowBase
{
	GENERATED_BODY()

	// The base item asset (archetype)
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UItemDefinitionAsset> BaseDef;

	// (Optional) per-entry notes
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FString Notes;
};