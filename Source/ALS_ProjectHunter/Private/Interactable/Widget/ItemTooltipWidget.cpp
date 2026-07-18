#include "Interactable/Widget/ItemTooltipWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Item/Library/FunctionLibraries/ItemTooltipFunctionLibrary.h"
#include "Item/Library/Structs/ItemStructs.h"

namespace ItemTooltipWidgetPrivate
{
	/** "MaxFireDamage" -> "Max Fire Damage". */
	FString PrettyAttributeName(const FName AttributeName)
	{
		return FName::NameToDisplayString(AttributeName.ToString(), /*bIsBool*/ false);
	}

	/** Trim trailing zeros: 10 -> "10", 7.5 -> "7.5". */
	FString FormatStatValue(const float Value)
	{
		if (FMath::IsNearlyEqual(Value, FMath::RoundToFloat(Value), 0.01f))
		{
			return FString::Printf(TEXT("%.0f"), Value);
		}
		return FString::Printf(TEXT("%.1f"), Value);
	}

	/** Compose one affix line from its display format. */
	FString FormatAffixLine(const FPHAttributeData& Stat)
	{
		// Designer-supplied override wins. Supports an optional {0} value slot.
		if (!Stat.DisplayText.IsEmpty())
		{
			return FText::Format(
				FTextFormat(Stat.DisplayText),
				FText::FromString(FormatStatValue(Stat.RolledStatValue))).ToString();
		}

		const FString Name  = PrettyAttributeName(Stat.AttributeName);
		const FString Value = FormatStatValue(Stat.RolledStatValue);

		switch (Stat.DisplayFormat)
		{
		case EAttributeDisplayFormat::ADF_Additive:     return FString::Printf(TEXT("+%s %s"), *Value, *Name);
		case EAttributeDisplayFormat::ADF_FlatNegative: return FString::Printf(TEXT("-%s %s"), *Value, *Name);
		case EAttributeDisplayFormat::ADF_Percent:      return FString::Printf(TEXT("+%s%% %s"), *Value, *Name);
		case EAttributeDisplayFormat::ADF_Increase:     return FString::Printf(TEXT("%s%% increased %s"), *Value, *Name);
		case EAttributeDisplayFormat::ADF_More:         return FString::Printf(TEXT("%s%% more %s"), *Value, *Name);
		case EAttributeDisplayFormat::ADF_Less:         return FString::Printf(TEXT("%s%% less %s"), *Value, *Name);
		case EAttributeDisplayFormat::ADF_Chance:       return FString::Printf(TEXT("%s%% chance to %s"), *Value, *Name);
		case EAttributeDisplayFormat::ADF_Duration:     return FString::Printf(TEXT("+%ss %s"), *Value, *Name);
		case EAttributeDisplayFormat::ADF_MinMax:       return FString::Printf(TEXT("Adds %s %s"), *Value, *Name);
		default:                                        return FString::Printf(TEXT("%s: %s"), *Name, *Value);
		}
	}

	/** Append a "Label: Min-Max" damage row when the range is non-zero. */
	void AppendDamageRange(TArray<FString>& OutRows, const TCHAR* Label, const float Min, const float Max)
	{
		if (Max <= 0.0f && Min <= 0.0f)
		{
			return;
		}
		OutRows.Add(FString::Printf(TEXT("%s: %s-%s"),
			Label, *FormatStatValue(Min), *FormatStatValue(Max)));
	}

	bool IsAffixSection(const EItemTooltipSectionType SectionType)
	{
		switch (SectionType)
		{
		case EItemTooltipSectionType::Implicits:
		case EItemTooltipSectionType::Prefixes:
		case EItemTooltipSectionType::Suffixes:
		case EItemTooltipSectionType::Crafted:
		case EItemTooltipSectionType::Enchants:
		case EItemTooltipSectionType::Unique:
		case EItemTooltipSectionType::Corruption:
			return true;
		default:
			return false;
		}
	}

	bool IsDescriptionSection(const EItemTooltipSectionType SectionType)
	{
		return SectionType == EItemTooltipSectionType::Description;
	}

	FString MakeTooltipLineString(const FItemTooltipLine& Line)
	{
		if (Line.bUseValueColumn && !Line.Value.IsEmpty())
		{
			return FString::Printf(TEXT("%s: %s"), *Line.Label.ToString(), *Line.Value.ToString());
		}

		return Line.Label.ToString();
	}
}

void UItemTooltipWidget::UpdateTooltip(UItemInstance* Item)
{
	if (!Item)
	{
		ClearTooltip();
		return;
	}

	if (!UItemTooltipFunctionLibrary::BuildItemTooltipData(Item, TooltipData))
	{
		ClearTooltip();
		return;
	}

	if (ItemNameText)
	{
		ItemNameText->SetText(TooltipData.DisplayName);
	}

	if (ItemTypeText)
	{
		ItemTypeText->SetText(FText::Format(
			FText::FromString(TEXT("{0} - {1}")),
			TooltipData.RarityName,
			TooltipData.ItemSubTypeName.IsEmpty() ? TooltipData.ItemTypeName : TooltipData.ItemSubTypeName));
	}

	if (ItemIconImage && TooltipData.IconMaterial)
	{
		ItemIconImage->SetBrushFromMaterial(TooltipData.IconMaterial);
	}

	SetGradeVisuals(TooltipData.Rarity);
	PopulateBaseStats(Item);
	PopulateAffixes(Item);
	PopulateLore(Item);
	OnTooltipDataUpdated(TooltipData);

	// Blueprint extension point - runs after the base population pass.
	OnTooltipUpdated(Item);
}

void UItemTooltipWidget::ClearTooltip()
{
	TooltipData = FItemTooltipData();

	if (BaseStatsContainer)
	{
		BaseStatsContainer->ClearChildren();
	}

	if (AffixesContainer)
	{
		AffixesContainer->ClearChildren();
	}

	if (LoreText)
	{
		LoreText->SetText(FText::GetEmpty());
	}

	if (BaseStatsBox)
	{
		BaseStatsBox->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (AffixesBox)
	{
		AffixesBox->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (LoreBox)
	{
		LoreBox->SetVisibility(ESlateVisibility::Collapsed);
	}

	OnTooltipCleared();
}

void UItemTooltipWidget::SetGradeVisuals(EItemRarity Grade)
{
	const FLinearColor GradeColor = GetGradeColor(Grade);

	if (HeaderBorder)
	{
		HeaderBorder->SetBrushColor(GradeColor);
	}

	if (ItemNameText)
	{
		ItemNameText->SetColorAndOpacity(FSlateColor(GradeColor));
	}
}

FLinearColor UItemTooltipWidget::GetGradeColor(EItemRarity Grade) const
{
	switch (Grade)
	{
	case EItemRarity::IR_GradeF:    return Color_GradeF;
	case EItemRarity::IR_GradeE:    return Color_GradeE;
	case EItemRarity::IR_GradeD:    return Color_GradeD;
	case EItemRarity::IR_GradeC:    return Color_GradeC;
	case EItemRarity::IR_GradeB:    return Color_GradeB;
	case EItemRarity::IR_GradeA:    return Color_GradeA;
	case EItemRarity::IR_GradeS:    return Color_GradeS;
	case EItemRarity::IR_GradeSS:   return Color_GradeSS;
	case EItemRarity::IR_Unknown:   return Color_GradeUnkown;
	case EItemRarity::IR_Corrupted: return Color_GradeCorrupted;
	default:                        return FLinearColor::White;
	}
}

void UItemTooltipWidget::PopulateBaseStats(UItemInstance*)
{
	using namespace ItemTooltipWidgetPrivate;

	if (!BaseStatsContainer)
	{
		return;
	}

	BaseStatsContainer->ClearChildren();

	int32 RowCount = 0;

	for (const FItemTooltipSection& Section : TooltipData.Sections)
	{
		if (IsAffixSection(Section.SectionType) || IsDescriptionSection(Section.SectionType))
		{
			continue;
		}

		if (Section.bShowHeading && !Section.Heading.IsEmpty())
		{
			if (UTextBlock* HeadingBlock = CreateStatTextBlock(Section.Heading.ToString(), TooltipData.HeaderColor))
			{
				BaseStatsContainer->AddChildToVerticalBox(HeadingBlock);
				++RowCount;
			}
		}

		for (const FItemTooltipLine& Line : Section.Lines)
		{
			const FString Row = MakeTooltipLineString(Line);
			if (Row.IsEmpty())
			{
				continue;
			}

			if (UTextBlock* TextBlock = CreateStatTextBlock(Row, Line.TextColor))
			{
				BaseStatsContainer->AddChildToVerticalBox(TextBlock);
				++RowCount;
			}
		}
	}

	if (BaseStatsBox)
	{
		BaseStatsBox->SetVisibility(RowCount > 0
			? ESlateVisibility::Visible
			: ESlateVisibility::Collapsed);
	}
}

void UItemTooltipWidget::PopulateAffixes(UItemInstance*)
{
	using namespace ItemTooltipWidgetPrivate;

	if (!AffixesContainer)
	{
		return;
	}

	AffixesContainer->ClearChildren();

	int32 RowCount = 0;

	for (const FItemTooltipSection& Section : TooltipData.Sections)
	{
		if (!IsAffixSection(Section.SectionType))
		{
			continue;
		}

		if (Section.bShowHeading && !Section.Heading.IsEmpty())
		{
			if (UTextBlock* HeadingBlock = CreateStatTextBlock(Section.Heading.ToString(), TooltipData.HeaderColor))
			{
				AffixesContainer->AddChildToVerticalBox(HeadingBlock);
				++RowCount;
			}
		}

		for (const FItemTooltipLine& Line : Section.Lines)
		{
			const FString Row = MakeTooltipLineString(Line);
			if (Row.IsEmpty())
			{
				continue;
			}

			if (UTextBlock* TextBlock = CreateStatTextBlock(Row, Line.TextColor))
			{
				AffixesContainer->AddChildToVerticalBox(TextBlock);
				++RowCount;
			}
		}
	}

	if (AffixesBox)
	{
		AffixesBox->SetVisibility(RowCount > 0
			? ESlateVisibility::Visible
			: ESlateVisibility::Collapsed);
	}
}

void UItemTooltipWidget::PopulateLore(UItemInstance*)
{
	FText DescriptionValue = FText::GetEmpty();

	for (const FItemTooltipSection& Section : TooltipData.Sections)
	{
		if (Section.SectionType != EItemTooltipSectionType::Description || Section.Lines.Num() == 0)
		{
			continue;
		}

		DescriptionValue = Section.Lines[0].Label;
		break;
	}

	const bool bHasLore = !DescriptionValue.IsEmpty();

	if (LoreText)
	{
		LoreText->SetText(bHasLore ? DescriptionValue : FText::GetEmpty());
		LoreText->SetColorAndOpacity(FSlateColor(LoreColor));
	}

	if (LoreBox)
	{
		LoreBox->SetVisibility(bHasLore
			? ESlateVisibility::Visible
			: ESlateVisibility::Collapsed);
	}
}

UTextBlock* UItemTooltipWidget::CreateStatTextBlock(
	const FString& Text,
	FLinearColor Color)
{
	if (!WidgetTree)
	{
		return nullptr;
	}

	UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());

	if (!TextBlock)
	{
		return nullptr;
	}

	TextBlock->SetText(FText::FromString(Text));
	TextBlock->SetColorAndOpacity(FSlateColor(Color));

	return TextBlock;
}
