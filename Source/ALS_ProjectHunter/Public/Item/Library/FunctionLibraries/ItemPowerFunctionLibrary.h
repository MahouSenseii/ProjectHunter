#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Item/Library/Enums/ItemEnums.h"
#include "Item/Library/Structs/ItemStatsStructs.h"
#include "ItemPowerFunctionLibrary.generated.h"

class UItemInstance;

UCLASS()
class ALS_PROJECTHUNTER_API UItemPowerFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Base item power + every implicit, prefix, suffix, crafted modifier and enchant. */
	UFUNCTION(BlueprintPure, Category = "Item|Power")
	static float CalculateItemPower(float BasePowerValue, const FPHItemStats& Stats);

	/** Converts a total score using Project Settings > Item Power Grades. */
	UFUNCTION(BlueprintPure, Category = "Item|Power")
	static EItemRarity GetGradeForPower(float ItemPower);

	/** Recalculates both ItemPowerScore and Rarity after generation or crafting. */
	UFUNCTION(BlueprintCallable, Category = "Item|Power")
	static bool RecalculateItemGrade(UItemInstance* Item);
};
