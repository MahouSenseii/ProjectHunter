#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Item/Library/Structs/ItemTooltipStructs.h"
#include "ItemTooltipFunctionLibrary.generated.h"

class UItemInstance;

UCLASS()
class ALS_PROJECTHUNTER_API UItemTooltipFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Item|Tooltip")
	static bool BuildItemTooltipData(UItemInstance* Item, FItemTooltipData& OutTooltipData);

	UFUNCTION(BlueprintPure, Category = "Item|Tooltip")
	static FItemTooltipData GetItemTooltipData(UItemInstance* Item);
};
