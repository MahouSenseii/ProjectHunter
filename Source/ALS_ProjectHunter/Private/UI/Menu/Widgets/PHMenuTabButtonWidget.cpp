#include "UI/Menu/Widgets/PHMenuTabButtonWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

void UPHMenuTabButtonWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (TabButton)
	{
		TabButton->OnClicked.AddUniqueDynamic(this, &UPHMenuTabButtonWidget::HandleButtonClicked);
		TabButton->OnHovered.AddUniqueDynamic(this, &UPHMenuTabButtonWidget::HandleButtonHovered);
		TabButton->OnUnhovered.AddUniqueDynamic(this, &UPHMenuTabButtonWidget::HandleButtonUnhovered);
	}

	ApplySelectionStyle();
}

void UPHMenuTabButtonWidget::NativeDestruct()
{
	if (TabButton)
	{
		TabButton->OnClicked.RemoveDynamic(this, &UPHMenuTabButtonWidget::HandleButtonClicked);
		TabButton->OnHovered.RemoveDynamic(this, &UPHMenuTabButtonWidget::HandleButtonHovered);
		TabButton->OnUnhovered.RemoveDynamic(this, &UPHMenuTabButtonWidget::HandleButtonUnhovered);
	}

	Super::NativeDestruct();
}

void UPHMenuTabButtonWidget::SetTabData(const FMenuEntry& Entry)
{
	MenuType = Entry.MenuType;

	if (TabLabel)
	{
		TabLabel->SetText(Entry.DisplayName);
	}

	if (TabIcon)
	{
		if (Entry.Icon)
		{
			TabIcon->SetBrushFromTexture(Entry.Icon);
			TabIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			TabIcon->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UPHMenuTabButtonWidget::SetSelected(bool bInSelected)
{
	if (bIsSelected == bInSelected)
	{
		return;
	}

	bIsSelected = bInSelected;
	ApplySelectionStyle();

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
	if (TabButton && !bIsSelected)
	{
		TabButton->SetBackgroundColor(HoveredColor);
	}

	OnTabHovered();
}

void UPHMenuTabButtonWidget::HandleButtonUnhovered()
{
	ApplySelectionStyle();
	OnTabUnhovered();
}

void UPHMenuTabButtonWidget::ApplySelectionStyle()
{
	if (TabButton)
	{
		TabButton->SetBackgroundColor(bIsSelected ? SelectedColor : NormalColor);
	}

	if (TabLabel)
	{
		TabLabel->SetColorAndOpacity(bIsSelected ? SelectedTextColor : NormalTextColor);
	}
}
