// Copyright © 2025 MahouSensei
// Author: Quentin Davis

#include "Menu/Widgets/MenuTabWidget.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

void UMenuTabWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (TabButton)
	{
		TabButton->OnClicked.AddDynamic(this, &UMenuTabWidget::HandleButtonClicked);
		TabButton->OnHovered.AddDynamic(this, &UMenuTabWidget::HandleButtonHovered);
		TabButton->OnUnhovered.AddDynamic(this, &UMenuTabWidget::HandleButtonUnhovered);
	}
}

void UMenuTabWidget::SetTabData(const FMenuEntry& Entry)
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

void UMenuTabWidget::SetSelected(bool bInSelected)
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

void UMenuTabWidget::HandleButtonClicked()
{
	OnTabClicked.Broadcast(MenuType);
}

void UMenuTabWidget::HandleButtonHovered()
{
	OnTabHovered();
}

void UMenuTabWidget::HandleButtonUnhovered()
{
	OnTabUnhovered();
}
