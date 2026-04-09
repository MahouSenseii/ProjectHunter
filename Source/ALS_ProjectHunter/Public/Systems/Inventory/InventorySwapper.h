#pragma once

#include "CoreMinimal.h"

class UInventoryManager;

class ALS_PROJECTHUNTER_API FInventorySwapper
{
public:
	static bool SwapItems(UInventoryManager& Manager, int32 SlotA, int32 SlotB);
};

