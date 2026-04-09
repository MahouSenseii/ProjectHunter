#pragma once

#include "CoreMinimal.h"

class UItemInstance;

class ALS_PROJECTHUNTER_API FItemStackingHandler
{
public:
	static void UpdateTotalWeight(UItemInstance& Item);
	static bool IsStackable(const UItemInstance& Item);
	static bool CanStackWith(const UItemInstance& Item, const UItemInstance* Other);
	static bool IsConsumed(const UItemInstance& Item);
	static int32 AddToStack(UItemInstance& Item, int32 Amount);
	static int32 RemoveFromStack(UItemInstance& Item, int32 Amount);
	static UItemInstance* SplitStack(UItemInstance& Item, int32 Amount);
	static int32 GetRemainingStackSpace(const UItemInstance& Item);
};
