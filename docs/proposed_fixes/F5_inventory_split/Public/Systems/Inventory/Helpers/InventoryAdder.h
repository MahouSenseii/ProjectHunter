// Public/Systems/Inventory/Helpers/InventoryAdder.h
#pragma once

#include "CoreMinimal.h"

class UInventoryManager;
class UItemInstance;

/**
 * InventoryAdder
 *
 * Owns the add-side workflow of the Inventory system: adding to the first
 * empty slot, adding to a specific slot, stacking onto existing items, and
 * splitting stacks. Stateless.
 *
 * Per Quentin UE Coding Skill: this is the Adder helper. The owner
 * (UInventoryManager) keeps the state and the public Blueprint API; this
 * class performs the actual mutation work and emits the warning logs when
 * an add fails.
 */
class ALS_PROJECTHUNTER_API FInventoryAdder
{
public:
	/** Tries to stack first (if enabled), then adds to the first empty slot. */
	static bool AddItem(UInventoryManager* Inventory, UItemInstance* Item);

	/** Adds the item directly to a specific slot index. */
	static bool AddItemToSlot(UInventoryManager* Inventory, UItemInstance* Item, int32 SlotIndex);

	/** Tries to stack the incoming item onto an existing identical item. */
	static bool TryStackItem(UInventoryManager* Inventory, UItemInstance* Item);

	/** Stacks SourceItem onto TargetItem, removing SourceItem if fully consumed. */
	static bool StackItems(UInventoryManager* Inventory, UItemInstance* SourceItem, UItemInstance* TargetItem);

	/** Splits Amount off Item into a new instance and adds it to the inventory. */
	static UItemInstance* SplitStack(UInventoryManager* Inventory, UItemInstance* Item, int32 Amount);
};
