#pragma once

#include "CoreMinimal.h"
#include "Item/Library/Enums/ItemEnums.h"
#include "Menu/Library/Enums/MenuEnums.h"
#include "MenuStructs.generated.h"

class UItemInstance;
class UPHMenuPageWidgetBase;
class UTexture2D;

USTRUCT(BlueprintType)
struct FMenuEntry
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "Menu")
	EMenuType MenuType = EMenuType::MT_None;

	UPROPERTY(EditDefaultsOnly, Category = "Menu")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, Category = "Menu")
	TObjectPtr<UTexture2D> Icon = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Menu")
	TSubclassOf<UPHMenuPageWidgetBase> WidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UPHMenuPageWidgetBase> CachedInstance = nullptr;
};

USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FEquipmentMenuSlotViewData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Equipment Slot")
	EEquipmentSlot Slot = EEquipmentSlot::ES_None;

	UPROPERTY(BlueprintReadOnly, Category = "Equipment Slot")
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "Equipment Slot")
	TObjectPtr<UItemInstance> Item = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Equipment Slot")
	bool bOccupied = false;
};

USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FEquipmentMenuInventorySlotViewData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Inventory Menu")
	int32 SlotIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory Menu")
	TObjectPtr<UItemInstance> Item = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory Menu")
	bool bOccupied = false;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory Menu")
	bool bCanEquip = false;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory Menu")
	EEquipmentSlot SuggestedEquipmentSlot = EEquipmentSlot::ES_None;
};
