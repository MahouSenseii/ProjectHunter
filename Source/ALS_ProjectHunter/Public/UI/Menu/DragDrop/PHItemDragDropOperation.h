// Author: Quentin Davis

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "Equipment/Library/Enums/EquipmentEnums.h"
#include "PHItemDragDropOperation.generated.h"

class UItemInstance;
class UUserWidget;

/** Where a dragged item was picked up from. */
UENUM(BlueprintType)
enum class EPHItemDragSource : uint8
{
	IDS_None		UMETA(DisplayName = "None"),
	IDS_Inventory	UMETA(DisplayName = "Inventory"),
	IDS_Equipment	UMETA(DisplayName = "Equipment")
};

/**
 * Payload carried while dragging an item around the menu.
 *
 * Drop targets read SourceType to decide what the drop means:
 *   Inventory -> Inventory  = move / swap
 *   Inventory -> Equipment  = equip
 *   Equipment -> Inventory  = unequip
 *   Equipment -> Equipment  = re-slot (unequip to bag, then equip)
 *   dropped outside the menu = drop to ground / unequip
 */
UCLASS(BlueprintType)
class ALS_PROJECTHUNTER_API UPHItemDragDropOperation : public UDragDropOperation
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "Item Drag")
	EPHItemDragSource SourceType = EPHItemDragSource::IDS_None;

	/** The item being dragged. Never null for a valid operation. */
	UPROPERTY(BlueprintReadWrite, Category = "Item Drag")
	TObjectPtr<UItemInstance> Item = nullptr;

	/** Valid when SourceType is IDS_Inventory. */
	UPROPERTY(BlueprintReadWrite, Category = "Item Drag")
	int32 SourceInventorySlotIndex = INDEX_NONE;

	/** Valid when SourceType is IDS_Equipment. */
	UPROPERTY(BlueprintReadWrite, Category = "Item Drag")
	EEquipmentSlot SourceEquipmentSlot = EEquipmentSlot::ES_None;

	/** Widget the drag started from. Weak so a rebuilt grid can't dangle. */
	TWeakObjectPtr<UUserWidget> SourceWidget = nullptr;

	UFUNCTION(BlueprintPure, Category = "Item Drag")
	UUserWidget* GetSourceWidget() const { return SourceWidget.Get(); }

	UFUNCTION(BlueprintPure, Category = "Item Drag")
	bool IsValidDrag() const { return Item != nullptr && SourceType != EPHItemDragSource::IDS_None; }

	UFUNCTION(BlueprintPure, Category = "Item Drag")
	bool IsFromInventory() const { return SourceType == EPHItemDragSource::IDS_Inventory; }

	UFUNCTION(BlueprintPure, Category = "Item Drag")
	bool IsFromEquipment() const { return SourceType == EPHItemDragSource::IDS_Equipment; }

	/** True when the drag started from this exact inventory slot. */
	UFUNCTION(BlueprintPure, Category = "Item Drag")
	bool IsSameInventorySlot(int32 SlotIndex) const
	{
		return IsFromInventory() && SlotIndex != INDEX_NONE && SourceInventorySlotIndex == SlotIndex;
	}

	/** True when the drag started from this exact equipment slot. */
	UFUNCTION(BlueprintPure, Category = "Item Drag")
	bool IsSameEquipmentSlot(EEquipmentSlot Slot) const
	{
		return IsFromEquipment() && Slot != EEquipmentSlot::ES_None && SourceEquipmentSlot == Slot;
	}
};
