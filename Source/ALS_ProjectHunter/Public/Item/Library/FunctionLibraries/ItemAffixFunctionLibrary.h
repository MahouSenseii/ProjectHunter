// Item/Library/FunctionLibraries/ItemAffixFunctionLibrary.h

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Item/Library/Enums/AffixEnums.h"
#include "Item/Library/Enums/ItemEnums.h"
#include "Item/Library/Structs/ItemAttributeStructs.h"
#include "ItemAffixFunctionLibrary.generated.h"

UCLASS()
class ALS_PROJECTHUNTER_API UItemAffixFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Item|Display", meta = (
		DisplayName = "Get Affix Count Text",
		Keywords = "affix count rarity display"))
	static FText GetAffixCountText(EItemRarity Rarity);

	UFUNCTION(BlueprintPure, Category = "Item|Formatting", meta = (
		DisplayName = "Format Affix Value",
		Keywords = "format affix tooltip display"))
	static FString FormatAffixValue(
		float Value,
		EAttributeDisplayFormat Format,
		FName AttributeName,
		float MinValue = 0.0f,
		float MaxValue = 0.0f,
		const FText& CustomText = FText::GetEmpty());

	UFUNCTION(BlueprintPure, Category = "Item|Formatting", meta = (
		DisplayName = "Format Affix Text",
		Keywords = "format affix tooltip"))
	static FString FormatAffixText(const FPHAttributeData& Affix);

	UFUNCTION(BlueprintPure, Category = "Item|Formatting", meta = (
		DisplayName = "Get Modify Type Symbol",
		Keywords = "modify type symbol format"))
	static FString GetModifyTypeSymbol(EModifyType ModifyType);

	UFUNCTION(BlueprintPure, Category = "Item|RankPoints", meta = (
		DisplayName = "Get Rank Points Value",
		Keywords = "rank points tier value"))
	static int32 GetRankPointsValue(ERankPoints Points);

	UFUNCTION(BlueprintPure, Category = "Item|RankPoints", meta = (
		DisplayName = "Get Tier Name",
		Keywords = "tier name rank"))
	static FText GetTierName(ERankPoints Points);

	UFUNCTION(BlueprintPure, Category = "Item|RankPoints", meta = (
		DisplayName = "Compare Affix Rank",
		Keywords = "compare rank tier affix"))
	static bool CompareAffixRank(const FPHAttributeData& AffixA, const FPHAttributeData& AffixB);

	UFUNCTION(BlueprintPure, Category = "Item|Generation", meta = (
		DisplayName = "Get Affix Count By Rarity",
		Keywords = "affix count rarity grade hunter"))
	static void GetAffixCountByRarity(
		EItemRarity Rarity,
		int32& OutMinPrefixes,
		int32& OutMaxPrefixes,
		int32& OutMinSuffixes,
		int32& OutMaxSuffixes);
};
