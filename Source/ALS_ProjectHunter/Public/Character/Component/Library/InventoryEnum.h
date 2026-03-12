
// Item/Library/ItemEnums.h
#pragma once

#include "CoreMinimal.h"
#include "InventoryEnum.generated.h"


/**
* Sort mode for inventory organization
 */
UENUM(BlueprintType)
enum class ESortMode : uint8
{
	SM_None         UMETA(DisplayName = "None"),
	SM_Type         UMETA(DisplayName = "By Type"),
	SM_Rarity       UMETA(DisplayName = "By Rarity"),
	SM_Name         UMETA(DisplayName = "By Name"),
	SM_Weight       UMETA(DisplayName = "By Weight"),
	SM_Value        UMETA(DisplayName = "By Value")
};