#pragma once

#include "CoreMinimal.h"
#include "Library/ALSCharacterEnumLibrary.h"
#include "PHItemEnumLibrary.h"
#include "Engine/DataTable.h"
#include "GameplayEffect.h"
#include "PHItemStructLibrary.generated.h"


class AItemPickup;
class AEquippableItem;
class APickup;
class AConsumablePickup;
class AEquippedObject;
class AWeaponPickup;





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
struct FConsumableItemData
{
	GENERATED_BODY()

	

	// Properties specific to consumable items
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Consumable")
	TSubclassOf<UGameplayEffect> GameplayEffectClass;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	int Quantity; // The quantity of the item in a stack

	FConsumableItemData()
		:GameplayEffectClass(nullptr),
		Quantity(0)
	{}
};

USTRUCT(BlueprintType)
struct FMinMax
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float Min;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float Max;

	// Default Constructor
	FMinMax()
		: Min(0), Max(0)
	{}

	// Custom Constructor
	FMinMax(float InMin, float InMax)
		: Min(InMin), Max(InMax)
	{}
	
};

USTRUCT(BlueprintType)
struct FWeaponItemData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base")
	TSubclassOf<AWeaponPickup> PickupClass;

	// Properties specific to weapons
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	EWeaponHandle WeaponHandle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	EALSOverlayState OverlayState;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	FCollisionInfo CollisionInfo; // Information related to the item's collision behavior

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	TMap<EDamageTypes, FMinMax> DamageStats;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	TMap<EItemRequiredStatsCategory, float> WeaponRequirementStats;

	FWeaponItemData(): WeaponHandle(EWeaponHandle::WH_None),
	OverlayState(EALSOverlayState::Default),
	CollisionInfo(0.0f,0.0f)
	{}
};


USTRUCT(BlueprintType)
struct FDamageRange
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Min = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Max = 0.0f;

	FDamageRange& operator+=(const FDamageRange& Other)
	{
		Min += Other.Min;
		Max += Other.Max;
		return *this;
	}

	FDamageRange& operator*=(float Scale)
	{
		Min *= Scale;
		Max *= Scale;
		return *this;
	}

	friend FDamageRange operator*(const FDamageRange& Range, float Scale)
	{
		FDamageRange Result = Range;
		Result *= Scale;
		return Result;
	}
};


USTRUCT(BlueprintType)
struct FDefenseStats
{
	GENERATED_BODY()

	/** Flat Resistance Values */
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

	/** Percent Resistance Values */
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

	/** Global Defense and Block Strength */
	UPROPERTY(BlueprintReadWrite, Category = "Defense")
	float GlobalDefenses = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category = "Defense")
	float BlockStrength = 0.0f;

	/** Default Constructor */
	FDefenseStats() = default;
};


USTRUCT(BlueprintType)
struct FItemInformation : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base")
	float BaseGradeValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base")
	TSubclassOf<AItemPickup> PickupClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base|Mesh")
	UStaticMesh* StaticMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base|Mesh")
	USkeletalMesh* SkeletalMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base|Owner")
	FString OwnerID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base")
	FText ItemName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base")
	FName ItemTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base")
	UMaterialInstance* ItemImage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base")
	UMaterialInstance* ItemImageRotated;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base")
	EItemType ItemType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base")
	EItemSubType ItemSubType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base")
	EItemRarity ItemRarity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base")
	int Value;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base")
	float ValueModifier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base")
	bool IsTradeable;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base|Inventory")
	FIntPoint Dimensions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base|Inventory")
	EEquipmentSlot EquipmentSlot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base|Text")
	FText ItemDescription;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base|Inventory")
	bool Stackable;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base|Inventory")
	int Quantity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base|Inventory")
	bool Rotated;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base|Inventory")
	ECurrentItemSlot LastSavedSlot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base|Transform")
	FTransform Transform;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base|Consumable")
	TSubclassOf<UGameplayEffect> GameplayEffectClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base|Consumable")
	bool bHasNameBeenGenerated;

	FItemInformation()
		: BaseGradeValue(0),
		  StaticMesh(nullptr),
		  SkeletalMesh(nullptr),
		  OwnerID(""),
		  ItemName(FText::FromString("")),
		  Description(FText::FromString("")),
		  ItemTag(""),
		  ItemImage(nullptr),
		  ItemImageRotated(nullptr),
		  ItemType(EItemType::IT_None),
		  ItemSubType(EItemSubType::IST_None),
		  ItemRarity(EItemRarity::IR_None),
		  Value(0),
		  ValueModifier(0.0f),
		  IsTradeable(false),
		  Dimensions(FIntPoint::ZeroValue),
		  EquipmentSlot(EEquipmentSlot::ES_None),
		  ItemDescription(FText::FromString("")),
		  Stackable(false),
		  Quantity(0),
		  Rotated(false),
		  LastSavedSlot(ECurrentItemSlot::CIS_None),
		  Transform(),
		  GameplayEffectClass(nullptr),
	      bHasNameBeenGenerated(false)
	{}
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
struct FTile
{
	GENERATED_BODY()

	// Default constructor
	FTile() : X(0), Y(0) {}

	// Constructor with parameters
	FTile(const int32 InX, const int32 InY) : X(InX), Y(InY) {}

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 X;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Y;
};

USTRUCT(BlueprintType)
struct FTileLoopInfo
{
	GENERATED_BODY()

	FTileLoopInfo() : X(0), Y(0), bIsTileAvailable(false) {}

	UPROPERTY(BlueprintReadWrite, Category = "TileLoopInfo")
	int32 X;

	UPROPERTY(BlueprintReadWrite, Category = "TileLoopInfo")
	int32 Y;

	UPROPERTY(BlueprintReadWrite, Category = "TileLoopInfo")
	bool bIsTileAvailable;
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

USTRUCT(BlueprintType)
struct FPHAttributeData : public FTableRowBase
{
	GENERATED_BODY()

public:
	/** Name of the Attribute (e.g., "Strength", "Fire Damage", etc.) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName AttributeName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EPrefixSuffix PrefixSuffix;

	/** The Gameplay Attribute being modified */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayAttribute StatChanged;

	/** Minimum change applied to the attribute */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MinStatChanged;

	/** Maximum change applied to the attribute */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxStatChanged;

	/** Determines if the attribute is displayed as a range (e.g., "2 TO 3 FIRE DAMAGE") */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsRange;

	/** Determines if the stat has been identified if not it will show up as ?????*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsIdentified;

	/** The format string that defines how the attribute should be displayed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EAttributeDisplayFormat DisplayFormat;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ERankPoints RankPoints;
};

USTRUCT(BlueprintType)
struct FPHItemStats
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	bool bHasGenerated = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	TArray<FPHAttributeData> PrefixStats;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	TArray<FPHAttributeData> SuffixStats;
};

/** Struct that defines stat requirements for equipping an item */
USTRUCT(BlueprintType)
struct FItemStatRequirement
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Requirements")
	float RequiredLevel = 0.0f;
	
	/** Minimum Strength required */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Requirements")
	float RequiredStrength = 0.0f;

	/** Minimum Intelligence required */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Requirements")
	float RequiredIntelligence = 0.0f;

	/** Minimum Dexterity required */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Requirements")
	float RequiredDexterity = 0.0f;

	/** Minimum Endurance required */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Requirements")
	float RequiredEndurance = 0.0f;

	/** Minimum Affliction required */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Requirements")
	float RequiredAffliction = 0.0f;

	/** Minimum Luck required */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Requirements")
	float RequiredLuck = 0.0f;

	/** Minimum Covenant required */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Requirements")
	float RequiredCovenant = 0.0f;
};

USTRUCT(BlueprintType)
struct FItemPassiveEffectInfo
{
	GENERATED_BODY()

	/** GameplayEffect class to apply */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UGameplayEffect> EffectClass;

	/** Level of the effect (for scaling) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Level = 1.0f;

	/** Should this effect be applied permanently or only under conditions? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bPersistent = true;
};

USTRUCT(BlueprintType)
struct FEquippableItemData
{
	GENERATED_BODY()

	// Properties specific to equippable items
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	TSubclassOf<AEquippedObject> EquipClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	EEquipmentSlot  EquipSlot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FName ItemName;

	/** Requirements to equip this item */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Requirements")
	FItemStatRequirement StatRequirements;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	TArray<FPHAttributeData> ArmorAttributes;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Passive Effects")
	TArray<FItemPassiveEffectInfo> PassiveEffects;
	
	FEquippableItemData(): EquipSlot()
	{
	}
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