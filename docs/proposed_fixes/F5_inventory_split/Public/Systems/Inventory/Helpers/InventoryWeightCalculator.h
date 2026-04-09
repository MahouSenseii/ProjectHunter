// Public/Systems/Inventory/Helpers/InventoryWeightCalculator.h
#pragma once

#include "CoreMinimal.h"

class UInventoryManager;
class UItemInstance;

/**
 * InventoryWeightCalculator
 *
 * Owns weight math for the Inventory system. Stateless. The Inventory owner
 * stores MaxWeight; this helper computes everything derived from it.
 *
 * Per Quentin UE Coding Skill: weight is one of the clearly separable
 * behavioral categories Rule E asks us to split out of the manager.
 */
class ALS_PROJECTHUNTER_API FInventoryWeightCalculator
{
public:
	/** Sum of all valid item weights in the inventory. */
	static float GetTotalWeight(const UInventoryManager* Inventory);

	/** MaxWeight - GetTotalWeight, clamped to >= 0. */
	static float GetRemainingWeight(const UInventoryManager* Inventory);

	/** GetTotalWeight / MaxWeight, clamped to [0, 1]. Returns 0 if MaxWeight <= 0. */
	static float GetWeightPercent(const UInventoryManager* Inventory);

	/** True if total weight exceeds MaxWeight. */
	static bool IsOverweight(const UInventoryManager* Inventory);

	/** True if adding Item would push total over MaxWeight. */
	static bool WouldExceedWeight(const UInventoryManager* Inventory, const UItemInstance* Item);

	/**
	 * Computes the new max weight from a Hunter strength stat and pushes it
	 * to the inventory via SetMaxWeight (which broadcasts).
	 */
	static void UpdateMaxWeightFromStrength(UInventoryManager* Inventory, int32 Strength);

	/**
	 * Sets MaxWeight on the inventory and triggers a weight broadcast.
	 * Centralizes the broadcast so callers cannot forget it.
	 */
	static void SetMaxWeight(UInventoryManager* Inventory, float NewMaxWeight);
};
