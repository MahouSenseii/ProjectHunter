#pragma once

#include "CoreMinimal.h"

class UItemInstance;

class ALS_PROJECTHUNTER_API FItemNameBuilder
{
public:
	static FText GetDisplayName(UItemInstance& Item);
	static void RegenerateDisplayName(UItemInstance& Item);
	static FText GenerateRareName(const UItemInstance& Item);
};
