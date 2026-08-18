// Author: Quentin Davis

#include "UI/Menu/Helpers/MenuSlotTooltipHelper.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "GameFramework/PlayerController.h"
#include "UI/HUD/HunterHUD.h"

AHunterHUD* FMenuSlotTooltipHelper::GetHunterHUD(const UUserWidget& Widget)
{
	const APlayerController* PC = Widget.GetOwningPlayer();
	return PC ? Cast<AHunterHUD>(PC->GetHUD()) : nullptr;
}

void FMenuSlotTooltipHelper::ShowForItem(const UUserWidget& Widget, UItemInstance* Item)
{
	if (!Item)
	{
		return;
	}

	if (AHunterHUD* HUD = GetHunterHUD(Widget))
	{
		HUD->ShowItemTooltipAtViewportPosition(
			Item, GetCursorViewportPosition(Widget), EItemTooltipSource::ITS_Menu);
	}
}

void FMenuSlotTooltipHelper::UpdatePosition(const UUserWidget& Widget)
{
	if (AHunterHUD* HUD = GetHunterHUD(Widget))
	{
		HUD->UpdateItemTooltipPosition(GetCursorViewportPosition(Widget), EItemTooltipSource::ITS_Menu);
	}
}

void FMenuSlotTooltipHelper::Hide(const UUserWidget& Widget)
{
	if (AHunterHUD* HUD = GetHunterHUD(Widget))
	{
		HUD->HideItemTooltip(EItemTooltipSource::ITS_Menu);
	}
}

FVector2D FMenuSlotTooltipHelper::GetCursorViewportPosition(const UUserWidget& Widget)
{
	// GetMousePositionOnViewport already divides by the DPI scale, which is the
	// same space UUserWidget::SetPositionInViewport(Pos, bRemoveDPIScale=false)
	// works in. Keep both sides on that space or the tooltip drifts at non-100% UI scale.
	UWorld* World = Widget.GetWorld();
	return World ? UWidgetLayoutLibrary::GetMousePositionOnViewport(World) : FVector2D::ZeroVector;
}
