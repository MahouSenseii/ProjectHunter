// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#include "UI/HUD/PHFloorBannerWidget.h"

#include "Animation/WidgetAnimation.h"
#include "Components/TextBlock.h"

void UPHFloorBannerWidget::ShowFloor(const int32 FloorNumber)
{
	if (FloorNumber <= 0)
	{
		return;
	}
	CurrentFloor = FloorNumber;
	if (FloorNumberText)
	{
		FloorNumberText->SetText(FText::Format(NSLOCTEXT("HunterHUD", "FloorAnnouncement", "FLOOR {0}"),
			FText::AsNumber(CurrentFloor)));
	}
	if (FloorOpen)
	{
		StopAnimation(FloorOpen);
	}
	if (BannerPanel)
	{
		BannerPanel->SetVisibility(ESlateVisibility::HitTestInvisible);
		BannerPanel->SetRenderOpacity(FloorOpen ? 0.0f : 1.0f);
	}
	if (FloorOpen)
	{
		PlayAnimation(FloorOpen);
	}
	OnShowFloor(FloorNumber);
}

void UPHFloorBannerWidget::HideBanner()
{
	if (FloorOpen)
	{
		StopAnimation(FloorOpen);
	}
	if (BannerPanel)
	{
		BannerPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
}

UWidgetAnimation* UPHFloorBannerWidget::GetEntryAnimation() const
{
	return FloorOpen.Get();
}

void UPHFloorBannerWidget::OnAnimationFinished_Implementation(const UWidgetAnimation* Animation)
{
	Super::OnAnimationFinished_Implementation(Animation);
	if (Animation == FloorOpen && BannerPanel && !IsAnimationPlaying(FloorOpen))
	{
		BannerPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
}
