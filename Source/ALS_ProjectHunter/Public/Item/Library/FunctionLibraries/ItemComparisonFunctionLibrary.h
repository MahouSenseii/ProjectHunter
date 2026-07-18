// Item/Library/FunctionLibraries/ItemComparisonFunctionLibrary.h

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Item/Library/Structs/ItemStructs.h"
#include "ItemComparisonFunctionLibrary.generated.h"

class UItemInstance;

UCLASS()
class ALS_PROJECTHUNTER_API UItemComparisonFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Item|Comparison", meta = (
		DisplayName = "Compare Item Damage",
		Keywords = "compare damage item"))
	static int32 CompareItemDamage(const FItemBase& ItemA, const FItemBase& ItemB);

	UFUNCTION(BlueprintPure, Category = "Item|Comparison", meta = (
		DisplayName = "Compare Item Value",
		Keywords = "compare value price item"))
	static int32 CompareItemValue(const FItemBase& ItemA, const FItemBase& ItemB);

	UFUNCTION(BlueprintPure, Category = "Item|Comparison", meta = (
		DisplayName = "Compare Item Instance Value",
		Keywords = "compare value instance"))
	static int32 CompareItemInstanceValue(const UItemInstance* ItemA, const UItemInstance* ItemB);

	UFUNCTION(BlueprintPure, Category = "Item|Comparison", meta = (
		DisplayName = "Compare Item Instance Rarity",
		Keywords = "compare rarity instance grade"))
	static int32 CompareItemInstanceRarity(const UItemInstance* ItemA, const UItemInstance* ItemB);

	UFUNCTION(BlueprintPure, Category = "Item|Comparison", meta = (
		DisplayName = "Compare Item Instance Weight",
		Keywords = "compare weight instance"))
	static int32 CompareItemInstanceWeight(const UItemInstance* ItemA, const UItemInstance* ItemB);
};
