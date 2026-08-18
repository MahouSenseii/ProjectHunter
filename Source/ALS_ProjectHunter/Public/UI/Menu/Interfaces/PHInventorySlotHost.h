// Author: Quentin Davis

#pragma once

#include "CoreMinimal.h"
#include "Equipment/Library/Enums/EquipmentEnums.h"
#include "UObject/Interface.h"
#include "PHInventorySlotHost.generated.h"

class UPHItemDragDropOperation;

UINTERFACE(MinimalAPI)
class UPHInventorySlotHost : public UInterface
{
	GENERATED_BODY()
};

/**
 * Anything that can own a grid of UPHInventorySlotWidget cells.
 *
 * Implemented by both UPHInventoryMenuPanelWidget (a dedicated panel inside the
 * page) and UPHEquipmentMenuPageWidget (the page hosting the grid directly, with
 * no separate panel widget). The slot cell only ever talks to this interface, so
 * either layout works without the cells knowing the difference.
 */
class ALS_PROJECTHUNTER_API IPHInventorySlotHost
{
	GENERATED_BODY()

public:
	/** Selection request from a cell click. */
	virtual void SelectInventorySlot(int32 SlotIndex) = 0;

	virtual bool CanEquipInventorySlotToSlot(int32 SlotIndex, EEquipmentSlot TargetSlot) const = 0;

	virtual bool RequestEquipInventorySlot(int32 SlotIndex, EEquipmentSlot TargetSlot) = 0;

	/** True when Operation may be dropped onto TargetSlotIndex. */
	virtual bool CanAcceptDroppedItem(UPHItemDragDropOperation* Operation, int32 TargetSlotIndex) const = 0;

	/** Applies a drop onto an inventory cell. */
	virtual bool HandleItemDroppedOnSlot(UPHItemDragDropOperation* Operation, int32 TargetSlotIndex) = 0;

	/** Drops a cell's item into the world. */
	virtual bool RequestDropInventorySlotToGround(int32 SlotIndex) = 0;
};
