#pragma once

#include "CoreMinimal.h"

class UItemInstance;
class UWorld;

class ALS_PROJECTHUNTER_API FInventoryGroundDropResolver
{
public:
	static bool DropItem(UWorld* World, UItemInstance* Item, const FVector& DropLocation);
};
