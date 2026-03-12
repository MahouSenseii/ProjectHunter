// Fill out your copyright notice in the Description page of Project Settings.


#include "Interactable/Widget/ItemTooltipWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"

void UItemTooltipWidget::UpdateTooltip(UItemInstance* Item)
{
}

void UItemTooltipWidget::SetGradeVisuals(EItemRarity Grade)
{
}

FLinearColor UItemTooltipWidget::GetGradeColor(EItemRarity Grade) const
{
	return FLinearColor::White; //TODO: setup the get grade.
}

void UItemTooltipWidget::PopulateBaseStats(UItemInstance* Item)
{
}

void UItemTooltipWidget::PopulateAffixes(UItemInstance* Item)
{
}

void UItemTooltipWidget::PopulateLore(UItemInstance* Item)
{
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
