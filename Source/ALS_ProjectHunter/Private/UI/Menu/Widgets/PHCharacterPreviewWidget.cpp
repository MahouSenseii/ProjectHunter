// Author: Quentin Davis

#include "UI/Menu/Widgets/PHCharacterPreviewWidget.h"

#include "UI/Menu/Camera/PHMenuCameraComponent.h"

UPHCharacterPreviewWidget::UPHCharacterPreviewWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Transparent, but it still has to be hit-testable or the drag never lands.
	SetVisibility(ESlateVisibility::Visible);
}

void UPHCharacterPreviewWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// A designer who leaves this SelfHitTestInvisible gets a preview that looks
	// right and does nothing, which is a miserable thing to debug.
	if (GetVisibility() == ESlateVisibility::SelfHitTestInvisible
		|| GetVisibility() == ESlateVisibility::HitTestInvisible)
	{
		SetVisibility(ESlateVisibility::Visible);
	}
}

UPHMenuCameraComponent* UPHCharacterPreviewWidget::GetMenuCamera() const
{
	return UPHMenuCameraComponent::GetForWidget(this);
}

FReply UPHCharacterPreviewWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	if (bDragToRotate && InMouseEvent.GetEffectingButton() == DragMouseButton)
	{
		SetDragging(true);

		// Capture so the spin keeps following the cursor past the widget edge.
		return FReply::Handled().CaptureMouse(TakeWidget());
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UPHCharacterPreviewWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	if (bDragging && InMouseEvent.GetEffectingButton() == DragMouseButton)
	{
		SetDragging(false);
		return FReply::Handled().ReleaseMouseCapture();
	}

	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

FReply UPHCharacterPreviewWidget::NativeOnMouseMove(const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	if (!bDragging)
	{
		return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
	}

	UPHMenuCameraComponent* MenuCamera = GetMenuCamera();
	if (!MenuCamera)
	{
		return FReply::Handled();
	}

	// GetCursorDelta still reports while the cursor is visible and captured,
	// which the player controller's look axis does not.
	const float DeltaX = InMouseEvent.GetCursorDelta().X;
	if (!FMath::IsNearlyZero(DeltaX))
	{
		MenuCamera->AddTurntableInput(DeltaX * DragSensitivity);
	}

	return FReply::Handled();
}

FReply UPHCharacterPreviewWidget::NativeOnMouseWheel(const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	if (!bWheelToZoom)
	{
		return Super::NativeOnMouseWheel(InGeometry, InMouseEvent);
	}

	UPHMenuCameraComponent* MenuCamera = GetMenuCamera();
	if (!MenuCamera)
	{
		return Super::NativeOnMouseWheel(InGeometry, InMouseEvent);
	}

	MenuCamera->AddZoomInput(InMouseEvent.GetWheelDelta() * WheelZoomStep);
	return FReply::Handled();
}

FReply UPHCharacterPreviewWidget::NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	if (bResetTurntableOnDoubleClick && InMouseEvent.GetEffectingButton() == DragMouseButton)
	{
		if (UPHMenuCameraComponent* MenuCamera = GetMenuCamera())
		{
			MenuCamera->ResetTurntable();
			return FReply::Handled();
		}
	}

	return Super::NativeOnMouseButtonDoubleClick(InGeometry, InMouseEvent);
}

void UPHCharacterPreviewWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);

	// Losing capture without a button-up would otherwise leave the drag latched.
	if (bDragging && !InMouseEvent.IsMouseButtonDown(DragMouseButton))
	{
		SetDragging(false);
	}
}

void UPHCharacterPreviewWidget::SetDragging(const bool bNewDragging)
{
	if (bDragging == bNewDragging)
	{
		return;
	}

	bDragging = bNewDragging;

	if (bDragging)
	{
		OnPreviewDragStarted();
	}
	else
	{
		OnPreviewDragFinished();
	}
}
