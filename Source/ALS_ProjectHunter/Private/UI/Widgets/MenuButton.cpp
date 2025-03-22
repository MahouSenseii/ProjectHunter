// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/MenuButton.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/TextBlock.h"

void UMenuButton::NativeConstruct()
{
	Super::NativeConstruct();


	
}

void UMenuButton::PHInitialize() const
{
	if (MenuButton && MenuButton->GetContent())
	{
		// The child widget is stored in GetContent(). That widget has a Slot pointer.
		if (UButtonSlot* ButtonSlot = Cast<UButtonSlot>(MenuButton->GetContent()->Slot))
		{
			// 1) Padding around the child (text block)
			ButtonSlot->SetPadding(FMargin(10.f, 0.f, 10.f, 0.f));

			// 2) Horizontal + vertical alignment of that child
			ButtonSlot->SetHorizontalAlignment(HAlign_Center);
			ButtonSlot->SetVerticalAlignment(VAlign_Center);
		}
	}


	if (MenuTextBlock && MenuTextBlock->Slot)
	{
		// Attempt to cast the Slot to UButtonSlot (assuming the parent is indeed a UButton).
		if (UButtonSlot* ButtonSlot = Cast<UButtonSlot>(MenuTextBlock->Slot))
		{
			// Set 4px left/right padding and 2px top/bottom padding
			ButtonSlot->SetPadding(FMargin(4.f, 2.f, 4.f, 2.f));

			// Center the text horizontally and vertically
			ButtonSlot->SetHorizontalAlignment(HAlign_Center);
			ButtonSlot->SetVerticalAlignment(VAlign_Center);
		}
	}
}
