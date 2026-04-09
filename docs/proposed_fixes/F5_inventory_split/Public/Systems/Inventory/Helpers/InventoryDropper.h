// Public/Systems/Inventory/Helpers/InventoryDropper.h
#pragma once

#include "CoreMinimal.h"

class UInventoryManager;
class UItemInstance;
class UWorld;

/**
 * InventoryDropper
 *
 * Owns the drop-to-ground workflow. Replaces the old FInventoryGroundDropHelper
 * which had a vague "Helper" name (Rule B violation) and only handled the
 * ground subsystem call. This helper takes ownership of the full drop flow:
 * remove from inventory → spawn ground item → broadcast.
 *
 * Stateless. The Inventory owner forwards its public DropItem* API here.
 */
class ALS_PROJECTHUNTER_API FInventoryDropper
{
public:
	/**
	 * Removes Item from Inventory and places it in the world via the
	 * GroundItemSubsystem.
	 */
	static void DropItem(UInventoryManager* Inventory, UItemInstance* Item, const FVector& DropLocation);

	/** Convenience: drop whatever lives at SlotIndex. */
	static void DropItemAtSlot(UInventoryManager* Inventory, int32 SlotIndex, const FVector& DropLocation);

private:
	/**
	 * Low-level helper kept private. Spawns the ground item via the
	 * GroundItemSubsystem; returns false if the world or subsystem is missing.
	 */
	static bool SpawnGroundItem(UWorld* World, UItemInstance* Item, const FVector& DropLocation);
};
