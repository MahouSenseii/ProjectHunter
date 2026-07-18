#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Item/Library/Structs/ItemTooltipStructs.h"
#include "ItemTooltipLineFunctionLibrary.generated.h"

UCLASS()
class ALS_PROJECTHUNTER_API UItemTooltipLineFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Item|Tooltip|Formatting")
	static FString FormatTooltipNumber(float Value, int32 MaxDecimals = 1);

	UFUNCTION(BlueprintPure, Category = "Item|Tooltip|Formatting")
	static FText FormatTooltipNumberText(float Value, int32 MaxDecimals = 1);

	UFUNCTION(BlueprintPure, Category = "Item|Tooltip|Formatting")
	static FText FormatTooltipRangeText(float MinValue, float MaxValue);

	UFUNCTION(BlueprintPure, Category = "Item|Tooltip|Formatting")
	static FText FormatTooltipPercentText(float Value);

	UFUNCTION(BlueprintPure, Category = "Item|Tooltip|Formatting")
	static FText FormatTooltipWeightText(float Value);

	UFUNCTION(BlueprintPure, Category = "Item|Tooltip|Line")
	static FItemTooltipLine MakeTooltipLine(
		FText Label,
		FText Value,
		FLinearColor Color,
		EItemTooltipLineStyle Style,
		bool bUseValueColumn = true,
		bool bEmphasized = false);

	UFUNCTION(BlueprintPure, Category = "Item|Tooltip|Line")
	static FItemTooltipLine MakeTooltipTextLine(
		FText Text,
		FLinearColor Color,
		EItemTooltipLineStyle Style,
		bool bEmphasized = false);

	static FLinearColor GetMutedTextColor();
	static FLinearColor GetStatTextColor();
	static FLinearColor GetAffixTextColor();
	static FLinearColor GetPositiveTextColor();
	static FLinearColor GetNegativeTextColor();
	static FLinearColor GetWarningTextColor();
	static FLinearColor GetCorruptedTextColor();
	static FLinearColor GetDescriptionTextColor();

	static void AddTooltipValueLine(
		TArray<FItemTooltipLine>& Lines,
		const TCHAR* Label,
		const FText& Value,
		const FLinearColor& Color,
		EItemTooltipLineStyle Style = EItemTooltipLineStyle::Property);

	static void AddTooltipNonZeroLine(
		TArray<FItemTooltipLine>& Lines,
		const TCHAR* Label,
		float Value,
		bool bPercent = false,
		const FLinearColor& Color = GetStatTextColor());

	static void AddTooltipPositiveLine(
		TArray<FItemTooltipLine>& Lines,
		const TCHAR* Label,
		int32 Value,
		const FLinearColor& Color = GetMutedTextColor());
};
