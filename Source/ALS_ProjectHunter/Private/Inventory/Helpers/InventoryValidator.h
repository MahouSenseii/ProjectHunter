#pragma once

#include "CoreMinimal.h"

class UInventoryManager;
class UItemInstance;

class ALS_PROJECTHUNTER_API FInventoryValidator
{
public:
	static bool IsFull(const UInventoryManager& Manager);
	static bool IsOverweight(const UInventoryManager& Manager);
	static bool CanAddItem(const UInventoryManager& Manager, UItemInstance* Item);
	static bool IsSlotEmpty(const UInventoryManager& Manager, int32 SlotIndex);
};

