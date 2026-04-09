// Public/Systems/Inventory/Helpers/InventoryRemover.h
#pragma once

#include "CoreMinimal.h"

class UInventoryManager;
class UItemInstance;

/**
 * InventoryRemover
 *
 * Owns the remove-side workflow of the Inventory system. Stateless. Mirrors
 * the layout of FInventoryAdder so a developer can predict where remove
 * bugs live (Rule D: folder/class layout reflects debug flow).
 */
class ALS_PROJECTHUNTER_API FInventoryRemover
{
public:
	/** Removes an item by reference. Returns true if found and removed. */
	static bool RemoveItem(UInventoryManager* Inventory, UItemInstance* Item);

	/** Removes whichever item lives at SlotIndex. Returns the removed item. */
	static UItemInstance* RemoveItemAtSlot(UInventoryManager* Inventory, int32 SlotIndex);

	/**
	 * Removes Quantity from Item's stack. If the stack hits zero, the item
	 * is removed from the inventory entirely.
	 */
	static bool RemoveQuantity(UInventoryManager* Inventory, UItemInstance* Item, int32 Quantity);

	/** Removes all items and broadcasts a single change event. */
	static void ClearAll(UInventoryManager* Inventory);

	/**
	 * Sweep the array and null out any items whose base data has gone bad.
	 * Used by Inventory health checks; does not broadcast.
	 */
	static void CleanupInvalidItems(UInventoryManager* Inventory);
};
