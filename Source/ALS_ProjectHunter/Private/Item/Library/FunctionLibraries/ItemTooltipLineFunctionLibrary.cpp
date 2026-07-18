#include "Item/Library/FunctionLibraries/ItemTooltipLineFunctionLibrary.h"

namespace ItemTooltipLineColors
{
	const FLinearColor MutedText(0.72f, 0.72f, 0.72f, 1.0f);
	const FLinearColor StatText(0.78f, 0.90f, 1.0f, 1.0f);
	const FLinearColor AffixText(0.72f, 0.88f, 1.0f, 1.0f);
	const FLinearColor PositiveText(0.42f, 0.95f, 0.48f, 1.0f);
	const FLinearColor NegativeText(1.0f, 0.28f, 0.25f, 1.0f);
	const FLinearColor WarningText(1.0f, 0.78f, 0.28f, 1.0f);
	const FLinearColor CorruptedText(0.78f, 0.20f, 1.0f, 1.0f);
	const FLinearColor DescriptionText(0.82f, 0.82f, 0.82f, 1.0f);
}

FString UItemTooltipLineFunctionLibrary::FormatTooltipNumber(const float Value, const int32 MaxDecimals)
{
	if (FMath::IsNearlyEqual(Value, FMath::RoundToFloat(Value), 0.01f))
	{
		return FString::Printf(TEXT("%d"), FMath::RoundToInt(Value));
	}

	switch (FMath::Clamp(MaxDecimals, 0, 3))
	{
	case 0:
		return FString::Printf(TEXT("%.0f"), Value);
	case 2:
		return FString::Printf(TEXT("%.2f"), Value);
	case 3:
		return FString::Printf(TEXT("%.3f"), Value);
	case 1:
	default:
		return FString::Printf(TEXT("%.1f"), Value);
	}
}

FText UItemTooltipLineFunctionLibrary::FormatTooltipNumberText(const float Value, const int32 MaxDecimals)
{
	return FText::FromString(FormatTooltipNumber(Value, MaxDecimals));
}

FText UItemTooltipLineFunctionLibrary::FormatTooltipRangeText(const float MinValue, const float MaxValue)
{
	return FText::FromString(FString::Printf(TEXT("%s-%s"), *FormatTooltipNumber(MinValue), *FormatTooltipNumber(MaxValue)));
}

FText UItemTooltipLineFunctionLibrary::FormatTooltipPercentText(const float Value)
{
	return FText::FromString(FString::Printf(TEXT("%s%%"), *FormatTooltipNumber(Value)));
}

FText UItemTooltipLineFunctionLibrary::FormatTooltipWeightText(const float Value)
{
	return FText::FromString(FormatTooltipNumber(Value, 2));
}

FItemTooltipLine UItemTooltipLineFunctionLibrary::MakeTooltipLine(
	const FText Label,
	const FText Value,
	const FLinearColor Color,
	const EItemTooltipLineStyle Style,
	const bool bUseValueColumn,
	const bool bEmphasized)
{
	FItemTooltipLine Line;
	Line.Label = Label;
	Line.Value = Value;
	Line.TextColor = Color;
	Line.Style = Style;
	Line.bUseValueColumn = bUseValueColumn;
	Line.bEmphasized = bEmphasized;
	return Line;
}

FItemTooltipLine UItemTooltipLineFunctionLibrary::MakeTooltipTextLine(
	const FText Text,
	const FLinearColor Color,
	const EItemTooltipLineStyle Style,
	const bool bEmphasized)
{
	return MakeTooltipLine(Text, FText::GetEmpty(), Color, Style, false, bEmphasized);
}

void UItemTooltipLineFunctionLibrary::AddTooltipValueLine(
	TArray<FItemTooltipLine>& Lines,
	const TCHAR* Label,
	const FText& Value,
	const FLinearColor& Color,
	const EItemTooltipLineStyle Style)
{
	if (!Value.IsEmpty())
	{
		Lines.Add(MakeTooltipLine(FText::FromString(Label), Value, Color, Style));
	}
}

void UItemTooltipLineFunctionLibrary::AddTooltipNonZeroLine(
	TArray<FItemTooltipLine>& Lines,
	const TCHAR* Label,
	const float Value,
	const bool bPercent,
	const FLinearColor& Color)
{
	if (FMath::IsNearlyZero(Value))
	{
		return;
	}

	AddTooltipValueLine(
		Lines,
		Label,
		bPercent ? FormatTooltipPercentText(Value) : FormatTooltipNumberText(Value),
		Color,
		EItemTooltipLineStyle::Stat);
}

void UItemTooltipLineFunctionLibrary::AddTooltipPositiveLine(
	TArray<FItemTooltipLine>& Lines,
	const TCHAR* Label,
	const int32 Value,
	const FLinearColor& Color)
{
	if (Value > 0)
	{
		AddTooltipValueLine(Lines, Label, FText::AsNumber(Value), Color);
	}
}

FLinearColor UItemTooltipLineFunctionLibrary::GetMutedTextColor()
{
	return ItemTooltipLineColors::MutedText;
}

FLinearColor UItemTooltipLineFunctionLibrary::GetStatTextColor()
{
	return ItemTooltipLineColors::StatText;
}

FLinearColor UItemTooltipLineFunctionLibrary::GetAffixTextColor()
{
	return ItemTooltipLineColors::AffixText;
}

FLinearColor UItemTooltipLineFunctionLibrary::GetPositiveTextColor()
{
	return ItemTooltipLineColors::PositiveText;
}

FLinearColor UItemTooltipLineFunctionLibrary::GetNegativeTextColor()
{
	return ItemTooltipLineColors::NegativeText;
}

FLinearColor UItemTooltipLineFunctionLibrary::GetWarningTextColor()
{
	return ItemTooltipLineColors::WarningText;
}

FLinearColor UItemTooltipLineFunctionLibrary::GetCorruptedTextColor()
{
	return ItemTooltipLineColors::CorruptedText;
}

FLinearColor UItemTooltipLineFunctionLibrary::GetDescriptionTextColor()
{
	return ItemTooltipLineColors::DescriptionText;
}
