// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/HUD/HunterHUD.h"

#include "Interactable/Widget/ItemTooltipWidget.h"

void AHunterHUD::BeginPlay()
{
	Super::BeginPlay();
    
	// Create tooltip widget (hidden by default)
	if (ItemTooltipWidgetClass)
	{
		ItemTooltipWidget = CreateWidget<UItemTooltipWidget>(GetWorld(), ItemTooltipWidgetClass);
		ItemTooltipWidget->AddToViewport(100);  // High Z-order
		ItemTooltipWidget->SetVisibility(ESlateVisibility::Hidden);
	}
}

void AHunterHUD::ShowItemTooltip(UItemInstance* Item, FVector2D ScreenPosition)
{
	if (ItemTooltipWidget && Item)
	{
		ItemTooltipWidget->UpdateTooltip(Item);
		ItemTooltipWidget->SetPositionInViewport(ScreenPosition);
		ItemTooltipWidget->SetVisibility(ESlateVisibility::Visible);
	}
}

void AHunterHUD::HideItemTooltip()
{
	if (ItemTooltipWidget)
	{
		ItemTooltipWidget->SetVisibility(ESlateVisibility::Hidden);
	}
}
void AHunterHUD::ShowMashProgressWidget(const FText& Text, int32 INT32)
{
}

void AHunterHUD::HideMashProgressWidget()
{
}
