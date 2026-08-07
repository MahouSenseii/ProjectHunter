#include "Interactable/Widget/ItemTooltipSectionWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Interactable/Widget/ItemTooltipLineWidget.h"

namespace ItemTooltipSectionWidgetPrivate
{
	const FSlateColor PureWhiteText(FLinearColor::White);
	const FName HeadingBackgroundName(TEXT("Image"));
	const FName HeadingDividerName(TEXT("Image_HeadingDivider"));

	void SetImageColor(UWidgetTree* WidgetTree, const FName WidgetName, const FLinearColor& Color)
	{
		if (WidgetTree)
		{
			if (UImage* Image = WidgetTree->FindWidget<UImage>(WidgetName))
			{
				Image->SetColorAndOpacity(Color);
			}
		}
	}

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

void UItemTooltipSectionWidget::SetSectionData(
	const FItemTooltipSection& InSectionData,
	const FLinearColor InHeadingColor)
{
	SectionData = InSectionData;
	HeadingColor = InHeadingColor;

	if (HeadingText)
	{
		const bool bShowHeading = SectionData.bShowHeading && !SectionData.Heading.IsEmpty();
		HeadingText->SetText(SectionData.Heading);
		HeadingText->SetColorAndOpacity(ItemTooltipSectionWidgetPrivate::PureWhiteText);
		HeadingText->SetVisibility(bShowHeading
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}

	ItemTooltipSectionWidgetPrivate::SetImageColor(
		WidgetTree,
		ItemTooltipSectionWidgetPrivate::HeadingBackgroundName,
		HeadingColor);
	ItemTooltipSectionWidgetPrivate::SetImageColor(
		WidgetTree,
		ItemTooltipSectionWidgetPrivate::HeadingDividerName,
		HeadingColor);

	PopulateLines();
	OnSectionDataUpdated(SectionData);
}

void UItemTooltipSectionWidget::PopulateLines()
{
	if (!LinesContainer)
	{
		return;
	}

	LinesContainer->ClearChildren();

	for (const FItemTooltipLine& Line : SectionData.Lines)
	{
		if (Line.Label.IsEmpty() && Line.Value.IsEmpty())
		{
			continue;
		}

		if (LineWidgetClass && GetWorld())
		{
			if (UItemTooltipLineWidget* LineWidget =
				CreateWidget<UItemTooltipLineWidget>(GetWorld(), LineWidgetClass))
			{
				LineWidget->SetLineData(Line);
				LinesContainer->AddChildToVerticalBox(LineWidget);
				continue;
			}
		}

		AddFallbackLine(Line);
	}
}

void UItemTooltipSectionWidget::AddFallbackLine(const FItemTooltipLine& Line)
{
	if (!WidgetTree || !LinesContainer)
	{
		return;
	}

	UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	if (!TextBlock)
	{
		return;
	}

	TextBlock->SetText(ItemTooltipSectionWidgetPrivate::MakeFallbackLineText(Line));
	TextBlock->SetColorAndOpacity(FSlateColor(Line.TextColor));
	TextBlock->SetJustification(Line.bUseValueColumn ? ETextJustify::Left : ETextJustify::Center);
	TextBlock->SetAutoWrapText(true);
	LinesContainer->AddChildToVerticalBox(TextBlock);
}
