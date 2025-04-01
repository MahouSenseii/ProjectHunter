#pragma once

#include "CoreMinimal.h"
#include "Library/ALSCharacterEnumLibrary.h"
#include "PHItemEnumLibrary.h"
#include "Engine/DataTable.h"
#include "GameplayEffect.h"
#include "PHItemStructLibrary.generated.h"


/* ============================= */
/* === Forward Declarations === */
/* ============================= */
class AItemPickup;
class AEquippableItem;
class APickup;
class AConsumablePickup;
class AEquippedObject;
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

	FDamageRange& operator+=(const FDamageRange& Other) { Min += Other.Min; Max += Other.Max; return *this; }
	FDamageRange& operator*=(float Scale) { Min *= Scale; Max *= Scale; return *this; }
	friend FDamageRange operator*(const FDamageRange& Range, float Scale) { return FDamageRange{ Range.Min * Scale, Range.Max * Scale }; }
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
struct FPHItemStats // Affixes 
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

	TArray<FPHAttributeData> GetAllStats() const
	{
		TArray<FPHAttributeData> Out;
		Out.Append(Prefixes);
		Out.Append(Suffixes);
		return Out;
	}
};

/* ============================= */
/* === Equippable & Item Data Structs === */
/* ============================= */
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

    // --- Weapon-Specific Fields (only used if this item is a weapon) --- //
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
    TSubclassOf<AWeaponPickup> PickupClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
    EWeaponHandle WeaponHandle = EWeaponHandle::WH_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
    EALSOverlayState OverlayState = EALSOverlayState::Default;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
    FCollisionInfo CollisionInfo;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Requirements")
    TMap<EItemRequiredStatsCategory, float> RequirementStats;

    FEquippableItemData()
        : EquipSlot(EEquipmentSlot::ES_None)
	{}
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

USTRUCT(BlueprintType)
struct FItemInformation : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base")
	float BaseGradeValue;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<AItemPickup> PickupClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UStaticMesh* StaticMesh;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USkeletalMesh* SkeletalMesh;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString OwnerID;
	
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
	EItemRarity ItemRarity;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int Value;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ValueModifier;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
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
	int Quantity;
	
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

	FItemInformation()
		: BaseGradeValue(0), StaticMesh(nullptr), SkeletalMesh(nullptr), OwnerID(""),
		  ItemTag(""), ItemImage(nullptr), ItemImageRotated(nullptr), ItemType(EItemType::IT_None),
		  ItemSubType(EItemSubType::IST_None),
		  ItemRarity(EItemRarity::IR_None), Value(0), ValueModifier(0.0f), IsTradeable(false),
		  Dimensions(FIntPoint::ZeroValue), EquipmentSlot(EEquipmentSlot::ES_None),
		  Stackable(false), Quantity(0), Rotated(false), LastSavedSlot(ECurrentItemSlot::CIS_None),
		  Transform(), GameplayEffectClass(nullptr), bHasNameBeenGenerated(false)
	{
	}
};


USTRUCT(BlueprintType)
struct FDropTable : public FTableRowBase
{

	GENERATED_BODY()

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
};



USTRUCT()
struct FPassiveEffectHandleList
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FActiveGameplayEffectHandle> Handles;
};