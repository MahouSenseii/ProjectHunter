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
	bool Stackable; // Indicates whether the item can be stacked in the inventory

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	int Quantity; // The quantity of the item in a stack

	FConsumableItemData()
		:GameplayEffectClass(nullptr),
		Stackable(false),
		Quantity(0)
	{}
};

USTRUCT(BlueprintType)
struct FWeaponItemData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base")
	TSubclassOf<AConsumablePickup> PickupClass;

	// Properties specific to weapons
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	EWeaponHandle WeaponHandle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	EALSOverlayState OverlayState;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	FCollisionInfo CollisionInfo; // Information related to the item's collision behavior

	FWeaponItemData(): WeaponHandle(EWeaponHandle::WH_None),
	OverlayState(EALSOverlayState::Default),
	CollisionInfo(0.0f,0.0f)
	{}
};

USTRUCT(BlueprintType)
struct FEquippableItemData
{
	GENERATED_BODY()

	// Properties specific to equippable items
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	TSubclassOf<AEquippedObject> EquipClass;
	
	FEquippableItemData()
	{}
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
	UStaticMesh* StaticMesh; // The static mesh representation of the item

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base|Mesh")
	USkeletalMesh* SkeletalMesh; // The skeletal mesh representation of the item, if it has moving parts

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base|Owner")
	FString OwnerID; // The unique identifier for the owner of the item

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base")
	FText ItemName; // The displayed name of the item

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base")
	FText Description; // A textual description of the item

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base")
	FName ItemTag; // A tag for categorizing or filtering the item

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base")
	UMaterialInstance* ItemImage; // The image icon representing the item in the inventory or UI

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base")
	UMaterialInstance* ItemImageRotated; // The rotated image icon of the item

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base")
	EItemType ItemType; // The general type of the item (e.g., weapon, armor)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base")
	EItemSubType ItemSubType; // The specific subtype within the main type
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base")
	EItemRarity ItemRarity; // The rarity or grade of the item

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base")
	int Value; // The monetary value of the item

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base")
	float ValueModifier; // A modifier that might affect the item's value

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base")
	bool IsTradeable; // Indicates whether the item can be traded between players
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base|Inventory")
	FIntPoint Dimensions; // The dimensions of the item, e.g., in a grid-based inventory system
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base|Text")
	FText ItemDescription;

	// Properties specific to consumable items
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Consumable")
	TSubclassOf<UGameplayEffect> GameplayEffectClass;
	
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
		  ItemType(EItemType::IS_None),
		  ItemSubType(EItemSubType::IST_None),
		  ItemRarity(EItemRarity::IR_None),
		  Value(0),
		  ValueModifier(0.0f),
		  IsTradeable(false),
		  Dimensions(FIntPoint::ZeroValue),
		  ItemDescription(FText::FromString("")),
		  GameplayEffectClass(nullptr)
	{}
};


USTRUCT(BlueprintType)
struct FUnifiedItemData : public FTableRowBase
{
	GENERATED_BODY()

	// Base Item Info
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base")
	FItemInformation ItemInfo;

	// Weapon-specific data
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	FWeaponItemData WeaponInfo;

	// Consumable-specific data
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Consumable")
	FConsumableItemData ConsumableInfo;

	// Equippable-specific data
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equippable")
	FEquippableItemData EquippableInfo;
	
};


USTRUCT(BlueprintType)
struct FMinMax
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float Min;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	float Max;

	FMinMax()
		:Min(0),
		Max(0)
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
