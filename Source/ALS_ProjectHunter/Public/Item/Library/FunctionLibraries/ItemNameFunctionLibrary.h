// Item/Library/FunctionLibraries/ItemNameFunctionLibrary.h

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Item/Library/Structs/ItemAttributeStructs.h"
#include "Item/Library/Structs/ItemStatsStructs.h"
#include "Item/Library/Structs/ItemStructs.h"
#include "ItemNameFunctionLibrary.generated.h"

UCLASS()
class ALS_PROJECTHUNTER_API UItemNameFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Item|Names", meta = (
		DisplayName = "Generate Item Name",
		Keywords = "name generate affix hunter"))
	static FText GenerateItemName(
		const FPHItemStats& ItemStats,
		const FItemBase& ItemBase,
		EItemRarity Rarity);

	UFUNCTION(BlueprintPure, Category = "Item|Names", meta = (
		DisplayName = "Generate Legendary Name",
		Keywords = "name generate legendary rare unique random"))
	static FText GenerateLegendaryName(int32 Seed);

	UFUNCTION(BlueprintPure, Category = "Item|Names", meta = (
		DisplayName = "Get Prefix Name",
		Keywords = "prefix name affix"))
	static FText GetPrefixName(const FPHAttributeData& Affix);

	UFUNCTION(BlueprintPure, Category = "Item|Names", meta = (
		DisplayName = "Get Suffix Name",
		Keywords = "suffix name affix"))
	static FText GetSuffixName(const FPHAttributeData& Affix);
};
