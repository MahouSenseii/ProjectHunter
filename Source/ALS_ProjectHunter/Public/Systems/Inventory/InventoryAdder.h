#pragma once

#include "CoreMinimal.h"

class UInventoryManager;
class UItemInstance;

class ALS_PROJECTHUNTER_API FInventoryAdder
{
public:
	static bool AddItem(UInventoryManager& Manager, UItemInstance* Item);
	static bool AddItemToSlot(UInventoryManager& Manager, UItemInstance* Item, int32 SlotIndex);
};

