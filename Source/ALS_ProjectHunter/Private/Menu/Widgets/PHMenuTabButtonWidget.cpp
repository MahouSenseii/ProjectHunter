// Copyright © 2025 MahouSensei
// Author: Quentin Davis

#include "Menu/Widgets/PHMenuTabButtonWidget.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

void UPHMenuTabButtonWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (TabButton)
	{
		TabButton->OnClicked.AddDynamic(this, &UPHMenuTabButtonWidget::HandleButtonClicked);
		TabButton->OnHovered.AddDynamic(this, &UPHMenuTabButtonWidget::HandleButtonHovered);
		TabButton->OnUnhovered.AddDynamic(this, &UPHMenuTabButtonWidget::HandleButtonUnhovered);
	}
}

void UPHMenuTabButtonWidget::SetTabData(const FMenuEntry& Entry)
{
	MenuType = Entry.MenuType;

	if (TabLabel)
	{
		TabLabel->SetText(Entry.DisplayName);
	}

	if (TabIcon && Entry.Icon)
	{
		TabIcon->SetBrushFromTexture(Entry.Icon);
	}
}

void UPHMenuTabButtonWidget::SetSelected(bool bInSelected)
{
	if (bIsSelected == bInSelected)
	{
		return;
	}

	bIsSelected = bInSelected;

	if (bIsSelected)
	{
		OnTabSelected();
	}
	else
	{
		OnTabDeselected();
	}
}

void UPHMenuTabButtonWidget::HandleButtonClicked()
{
	OnTabClicked.Broadcast(MenuType);
}

void UPHMenuTabButtonWidget::HandleButtonHovered()
{
	OnTabHovered();
}

void UPHMenuTabButtonWidget::HandleButtonUnhovered()
{
	OnTabUnhovered();
}
