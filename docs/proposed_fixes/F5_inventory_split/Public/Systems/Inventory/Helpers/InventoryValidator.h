// Public/Systems/Inventory/Helpers/InventoryValidator.h
#pragma once

#include "CoreMinimal.h"

class UInventoryManager;
class UItemInstance;

/**
 * InventoryValidator
 *
 * Stateless validation helper for InventoryManager. Owns "can this happen?"
 * checks so the manager can stay focused on state transitions and broadcasts.
 *
 * Per Quentin UE Coding Skill: this is the Validator helper for the Inventory
 * owner. It must not mutate state. All functions should be cheap and pure.
 */
class ALS_PROJECTHUNTER_API FInventoryValidator
{
public:
	/** Returns true if Item is non-null and has valid base data. */
	static bool IsItemValid(const UItemInstance* Item);

	/** Returns true if SlotIndex is in [0, MaxSlots). */
	static bool IsSlotIndexInRange(int32 SlotIndex, int32 MaxSlots);

	/**
	 * Full add-precheck. Mirrors the old UInventoryManager::CanAddItem logic
	 * (weight + stackable + slot availability) but lives here so add-side
	 * code paths share one validator.
	 */
	static bool CanAddItem(const UInventoryManager* Inventory, UItemInstance* Item);
};
