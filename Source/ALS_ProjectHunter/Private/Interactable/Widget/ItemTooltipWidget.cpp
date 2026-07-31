#include "Interactable/Widget/ItemTooltipWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Interactable/Widget/ItemTooltipSectionWidget.h"
#include "Item/Library/FunctionLibraries/ItemTooltipFunctionLibrary.h"

namespace ItemTooltipWidgetPrivate
{
	const FSlateColor PureWhiteText(FLinearColor::White);

	FText MakeFallbackLineText(const FItemTooltipLine& Line)
	{
		if (Line.bUseValueColumn && !Line.Value.IsEmpty())
		{
			return FText::Format(
				FText::FromString(TEXT("{0}: {1}")),
				Line.Label,
				Line.Value);
		}

		return Line.Label;
	}
}

void UItemTooltipWidget::UpdateTooltip(UItemInstance* Item)
{
	if (!Item)
	{
		ClearTooltip();
		return;
	}

	if (DisplayedItem.Get() == Item && TooltipData.bHasItem)
	{
		return;
	}

	if (!UItemTooltipFunctionLibrary::BuildItemTooltipData(Item, TooltipData))
	{
		ClearTooltip();
		return;
	}

	DisplayedItem = Item;

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
		ItemTypeText->SetColorAndOpacity(ItemTooltipWidgetPrivate::PureWhiteText);
	}

	if (ItemIconImage && TooltipData.IconMaterial)
	{
		ItemIconImage->SetBrushFromMaterial(TooltipData.IconMaterial);
	}

	SetGradeVisuals(TooltipData.Rarity);
	PopulateSections();
	OnTooltipDataUpdated(TooltipData);
	OnTooltipUpdated(Item);
}

void UItemTooltipWidget::ClearTooltip()
{
	DisplayedItem.Reset();
	TooltipData = FItemTooltipData();

	if (SectionsContainer)
	{
		SectionsContainer->ClearChildren();
	}

	if (ItemNameText)
	{
		ItemNameText->SetText(FText::GetEmpty());
	}

	if (ItemTypeText)
	{
		ItemTypeText->SetText(FText::GetEmpty());
	}

	OnTooltipCleared();
}

void UItemTooltipWidget::SetGradeVisuals(const EItemRarity Grade)
{
	const FLinearColor GradeColor = GetGradeColor(Grade);

	if (HeaderBorder)
	{
		HeaderBorder->SetBrushColor(GradeColor);
	}

	if (ItemNameText)
	{
		ItemNameText->SetColorAndOpacity(ItemTooltipWidgetPrivate::PureWhiteText);
	}
}

FLinearColor UItemTooltipWidget::GetGradeColor(const EItemRarity Grade) const
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
	case EItemRarity::IR_Unknown:   return Color_GradeUnkown;
	case EItemRarity::IR_GradeSS:   return Color_GradeSS;
	case EItemRarity::IR_Corrupted: return Color_GradeCorrupted;
	default:                        return FLinearColor::White;
	}
}

void UItemTooltipWidget::PopulateSections()
{
	if (!SectionsContainer)
	{
		return;
	}

	SectionsContainer->ClearChildren();

	for (const FItemTooltipSection& Section : TooltipData.Sections)
	{
		if (!Section.HasDisplayableLines())
		{
			continue;
		}

		if (SectionWidgetClass && GetWorld())
		{
			if (UItemTooltipSectionWidget* SectionWidget =
				CreateWidget<UItemTooltipSectionWidget>(GetWorld(), SectionWidgetClass))
			{
				SectionWidget->SetSectionData(Section, FLinearColor::White);
				SectionsContainer->AddChildToVerticalBox(SectionWidget);
				continue;
			}
		}

		AddFallbackSection(Section);
	}
}

void UItemTooltipWidget::AddFallbackSection(const FItemTooltipSection& Section)
{
	if (!SectionsContainer)
	{
		return;
	}

	if (Section.bShowHeading && !Section.Heading.IsEmpty())
	{
		if (UTextBlock* HeadingBlock =
			CreateFallbackTextBlock(Section.Heading, FLinearColor::White, ETextJustify::Center))
		{
			SectionsContainer->AddChildToVerticalBox(HeadingBlock);
		}
	}

	for (const FItemTooltipLine& Line : Section.Lines)
	{
		const FText LineText = ItemTooltipWidgetPrivate::MakeFallbackLineText(Line);
		if (LineText.IsEmpty())
		{
			continue;
		}

		if (UTextBlock* TextBlock = CreateFallbackTextBlock(
			LineText,
			Line.TextColor,
			Line.bUseValueColumn ? ETextJustify::Left : ETextJustify::Center))
		{
			SectionsContainer->AddChildToVerticalBox(TextBlock);
		}
	}
}

UTextBlock* UItemTooltipWidget::CreateFallbackTextBlock(
	const FText& Text,
	const FLinearColor Color,
	const ETextJustify::Type Justification)
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

	TextBlock->SetText(Text);
	TextBlock->SetColorAndOpacity(FSlateColor(Color));
	TextBlock->SetJustification(Justification);
	TextBlock->SetAutoWrapText(true);
	return TextBlock;
}
