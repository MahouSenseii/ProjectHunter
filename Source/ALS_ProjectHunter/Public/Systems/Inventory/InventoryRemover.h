#pragma once

#include "CoreMinimal.h"

class UInventoryManager;
class UItemInstance;

class ALS_PROJECTHUNTER_API FInventoryRemover
{
public:
	static bool RemoveItem(UInventoryManager& Manager, UItemInstance* Item);
	static UItemInstance* RemoveItemAtSlot(UInventoryManager& Manager, int32 SlotIndex);
	static bool RemoveQuantity(UInventoryManager& Manager, UItemInstance* Item, int32 Quantity);
	static void DropItem(UInventoryManager& Manager, UItemInstance* Item, FVector DropLocation);
	static void DropItemAtSlot(UInventoryManager& Manager, int32 SlotIndex, FVector DropLocation);
};

