// Author: Quentin Davis

#pragma once

#include "CoreMinimal.h"
#include "Equipment/Library/Enums/EquipmentEnums.h"
#include "Templates/SubclassOf.h"

class UItemInstance;
class UPHItemDragDropOperation;
class UUserWidget;
class UWidget;

/**
 * Shared drag-and-drop plumbing for the menu slot widgets.
 *
 * Keeps the operation construction and the drag-visual fallback in one place so
 * the inventory and equipment slot widgets stay thin.
 */
class FMenuItemDragDropHelper
{
public:
	/** Builds an inventory-sourced operation. Returns null when Item is null. */
	static UPHItemDragDropOperation* MakeInventoryDrag(
		UUserWidget& SourceWidget,
		UItemInstance* Item,
		int32 SourceSlotIndex,
		TSubclassOf<UUserWidget> DragVisualClass,
		FVector2D DragVisualSize);

	/** Builds an equipment-sourced operation. Returns null when Item is null. */
	static UPHItemDragDropOperation* MakeEquipmentDrag(
		UUserWidget& SourceWidget,
		UItemInstance* Item,
		EEquipmentSlot SourceSlot,
		TSubclassOf<UUserWidget> DragVisualClass,
		FVector2D DragVisualSize);

	/**
	 * Drag visual for the cursor.
	 *
	 * Uses DragVisualClass when one is configured, otherwise builds a bare
	 * UImage from the item icon so drag-and-drop works with no Blueprint setup.
	 */
	static UWidget* MakeDragVisual(
		UUserWidget& SourceWidget,
		UItemInstance* Item,
		TSubclassOf<UUserWidget> DragVisualClass,
		FVector2D DragVisualSize);

private:
	static UPHItemDragDropOperation* MakeOperation(
		UUserWidget& SourceWidget,
		UItemInstance* Item,
		TSubclassOf<UUserWidget> DragVisualClass,
		FVector2D DragVisualSize);
};
