// Public/Systems/Inventory/Helpers/InventoryOrganizer.h
#pragma once

#include "CoreMinimal.h"
#include "Systems/Inventory/Library/InventoryEnums.h"

class UInventoryManager;

/**
 * InventoryOrganizer
 *
 * Owns slot-layout operations: swap, sort, compact. These are the
 * "rearrange items in place" actions; they don't add, remove, or change
 * weight, but they do broadcast OnInventoryChanged.
 *
 * Stateless. Sort/Compact already delegate the heavy lifting to
 * UInventoryFunctionLibrary; this helper exists so the manager doesn't
 * directly call the library.
 */
class ALS_PROJECTHUNTER_API FInventoryOrganizer
{
public:
	/** Swap two slots. Returns true on success. */
	static bool SwapItems(UInventoryManager* Inventory, int32 SlotA, int32 SlotB);

	/** Sort the items array using the function library, then broadcast. */
	static void SortInventory(UInventoryManager* Inventory, ESortMode SortMode);

	/** Compact the items array using the function library, then broadcast. */
	static void CompactInventory(UInventoryManager* Inventory);
};
