#pragma once

#include "CoreMinimal.h"

class UInventoryManager;
class UItemInstance;

class ALS_PROJECTHUNTER_API FInventoryStackHandler
{
public:
	static bool TryStackItem(UInventoryManager& Manager, UItemInstance* Item);
	static bool StackItems(UInventoryManager& Manager, UItemInstance* SourceItem, UItemInstance* TargetItem);
	static UItemInstance* SplitStack(UInventoryManager& Manager, UItemInstance* Item, int32 Amount);
};

