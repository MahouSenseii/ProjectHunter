#include "UI/Menu/Widgets/PHInventorySlotWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "Components/TextBlock.h"
#include "Item/ItemInstance.h"
#include "UI/Menu/DragDrop/PHItemDragDropOperation.h"
#include "UI/Menu/Camera/PHMenuCameraComponent.h"
#include "UI/Menu/Helpers/MenuItemDragDropHelper.h"
#include "UI/Menu/Helpers/MenuSlotTooltipHelper.h"
#include "UI/Menu/Interfaces/PHInventorySlotHost.h"

void UPHInventorySlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (SlotButton)
	{
		SlotButton->OnClicked.AddUniqueDynamic(this, &UPHInventorySlotWidget::HandleSlotClicked);
	}

	// Cells sit shoulder to shoulder in a uniform grid. Without clipping, any
	// child wider than the cell (a long item name, a stray design-time label)
	// bleeds across its neighbours.
	SetClipping(EWidgetClipping::ClipToBounds);

	if (bEnableDragAndDrop)
	{
		// The slot widget itself must own the mouse for drag detection to work.
		// A UButton child would otherwise swallow the press before we see it, so
		// it is made non-hit-testable and clicks are handled in NativeOnMouseButtonUp.
		SetVisibility(ESlateVisibility::Visible);

		if (SlotButton)
		{
			SlotButton->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
	}

	RefreshVisuals();
}

void UPHInventorySlotWidget::NativeDestruct()
{
	if (SlotButton)
	{
		SlotButton->OnClicked.RemoveDynamic(this, &UPHInventorySlotWidget::HandleSlotClicked);
	}

	// A widget torn down mid-hover would otherwise leave the tooltip on screen.
	if (bSlotHovered)
	{
		FMenuSlotTooltipHelper::Hide(*this);
		bSlotHovered = false;
	}

	Super::NativeDestruct();
}

void UPHInventorySlotWidget::InitializeInventorySlot(
	const TScriptInterface<IPHInventorySlotHost>& InSlotHost,
	const FEquipmentMenuInventorySlotViewData& InSlotData)
{
	SlotHost = InSlotHost;
	SetSlotData(InSlotData);
}

void UPHInventorySlotWidget::SetSlotData(const FEquipmentMenuInventorySlotViewData& InSlotData)
{
	SlotData = InSlotData;
	RefreshVisuals();

	OnSlotDataRefreshed(SlotData);
	SlotDataRefreshed.Broadcast(SlotData);
}

void UPHInventorySlotWidget::SelectSlot()
{
	if (SlotHost.GetInterface() && SlotData.SlotIndex != INDEX_NONE)
	{
		SlotHost->SelectInventorySlot(SlotData.SlotIndex);
	}
}

bool UPHInventorySlotWidget::CanEquipToSlot(const EEquipmentSlot TargetSlot) const
{
	return SlotHost.GetInterface()
		&& SlotData.SlotIndex != INDEX_NONE
		&& SlotHost->CanEquipInventorySlotToSlot(SlotData.SlotIndex, TargetSlot);
}

bool UPHInventorySlotWidget::RequestEquip(const EEquipmentSlot TargetSlot)
{
	return SlotHost.GetInterface()
		&& SlotData.SlotIndex != INDEX_NONE
		&& SlotHost->RequestEquipInventorySlot(SlotData.SlotIndex, TargetSlot);
}

// HOVER / TOOLTIP

void UPHInventorySlotWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	SetHovered(true);
	SetMenuCameraFocus(true);
}

void UPHInventorySlotWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
	SetHovered(false);
	SetMenuCameraFocus(false);
}

void UPHInventorySlotWidget::SetMenuCameraFocus(const bool bFocused) const
{
	// An empty cell has nothing to preview, and no suggested slot to aim at.
	if (!bFocusMenuCameraOnHover || SlotData.SuggestedEquipmentSlot == EEquipmentSlot::ES_None)
	{
		return;
	}

	UPHMenuCameraComponent* MenuCamera = UPHMenuCameraComponent::GetForWidget(this);
	if (!MenuCamera)
	{
		return;
	}

	if (bFocused)
	{
		MenuCamera->FocusEquipmentSlot(SlotData.SuggestedEquipmentSlot);
	}
	else if (MenuCamera->GetFocusedEquipmentSlot() == SlotData.SuggestedEquipmentSlot)
	{
		// Only release the focus this cell took - the cursor may already have
		// entered the next cell and claimed it.
		MenuCamera->ClearEquipmentSlotFocus();
	}
}

FReply UPHInventorySlotWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// Mouse-move rather than tick: UUserWidget ticking is opt-in and the tooltip
	// only ever needs to move when the cursor does.
	if (bSlotHovered && bShowTooltipOnHover && bTooltipFollowsMouse && SlotData.Item)
	{
		FMenuSlotTooltipHelper::UpdatePosition(*this);
	}

	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

// DRAG AND DROP

FReply UPHInventorySlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bEnableDragAndDrop
		&& SlotData.Item
		&& InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		// Arms drag detection. A release without movement still lands in
		// NativeOnMouseButtonUp, so a plain click keeps selecting the slot.
		return FReply::Handled().DetectDrag(TakeWidget(), EKeys::LeftMouseButton);
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UPHInventorySlotWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		SelectSlot();
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

void UPHInventorySlotWidget::NativeOnDragDetected(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent,
	UDragDropOperation*& OutOperation)
{
	Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);

	if (!bEnableDragAndDrop || !SlotData.Item || SlotData.SlotIndex == INDEX_NONE)
	{
		return;
	}

	// The tooltip would sit under the dragged icon for the whole drag.
	FMenuSlotTooltipHelper::Hide(*this);
	SetHovered(false);

	UPHItemDragDropOperation* Operation = FMenuItemDragDropHelper::MakeInventoryDrag(
		*this, SlotData.Item, SlotData.SlotIndex, DragVisualWidgetClass, DragVisualSize);
	if (!Operation)
	{
		return;
	}

	// Fires only when no slot handled the drop, i.e. released outside the menu.
	Operation->OnDragCancelled.AddUniqueDynamic(this, &UPHInventorySlotWidget::HandleDragCancelled);

	OutOperation = Operation;
	OnSlotDragStarted(Operation);
}

void UPHInventorySlotWidget::NativeOnDragEnter(
	const FGeometry& InGeometry,
	const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation)
{
	Super::NativeOnDragEnter(InGeometry, InDragDropEvent, InOperation);

	UPHItemDragDropOperation* ItemOperation = Cast<UPHItemDragDropOperation>(InOperation);
	if (!ItemOperation)
	{
		return;
	}

	const bool bAccepts = SlotHost.GetInterface()
		&& SlotHost->CanAcceptDroppedItem(ItemOperation, SlotData.SlotIndex);

	SetDragOverState(true, bAccepts);
}

void UPHInventorySlotWidget::NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDragLeave(InDragDropEvent, InOperation);
	SetDragOverState(false, false);
}

bool UPHInventorySlotWidget::NativeOnDrop(
	const FGeometry& InGeometry,
	const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation)
{
	SetDragOverState(false, false);

	UPHItemDragDropOperation* ItemOperation = Cast<UPHItemDragDropOperation>(InOperation);
	if (!ItemOperation)
	{
		return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
	}

	if (SlotHost.GetInterface())
	{
		SlotHost->HandleItemDroppedOnSlot(ItemOperation, SlotData.SlotIndex);
	}

	// Always claim the drop, even when it was rejected: an unhandled drop is
	// what triggers OnDragCancelled, which means "drop it on the ground".
	return true;
}

// INTERNAL

void UPHInventorySlotWidget::RefreshVisuals()
{
	if (SlotNameText)
	{
		// Never meaningful on an inventory cell - see the binding's comment.
		SlotNameText->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (SlotIndexText)
	{
		SlotIndexText->SetText(SlotData.SlotIndex == INDEX_NONE
			? FText::GetEmpty()
			: FText::AsNumber(SlotData.SlotIndex + 1));
	}

	UItemInstance* Item = SlotData.Item;

	if (ItemNameText)
	{
		ItemNameText->SetText(Item && bShowItemNameInSlot ? Item->GetDisplayName() : FText::GetEmpty());
		ItemNameText->SetVisibility(bShowItemNameInSlot
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}

	if (StackText)
	{
		// Only worth showing once a stack actually forms.
		const bool bShowStack = Item && Item->Quantity > 1;
		StackText->SetText(bShowStack ? FText::AsNumber(Item->Quantity) : FText::GetEmpty());
		StackText->SetVisibility(bShowStack ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (ItemIcon)
	{
		if (UTexture2D* IconTexture = Item ? Item->GetInventoryIcon() : nullptr)
		{
			// bMatchSize=false: the slot box drives the size, not the texture.
			ItemIcon->SetBrushFromTexture(IconTexture, /*bMatchSize=*/false);
			ItemIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			ItemIcon->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UPHInventorySlotWidget::SetHovered(const bool bNewHovered)
{
	if (bSlotHovered == bNewHovered)
	{
		return;
	}

	bSlotHovered = bNewHovered;

	if (bShowTooltipOnHover)
	{
		if (bSlotHovered && SlotData.Item)
		{
			FMenuSlotTooltipHelper::ShowForItem(*this, SlotData.Item);
		}
		else
		{
			FMenuSlotTooltipHelper::Hide(*this);
		}
	}

	OnSlotHoverChanged(bSlotHovered);
}

void UPHInventorySlotWidget::SetDragOverState(const bool bNewDragOver, const bool bNewValidTarget)
{
	if (bDragOver == bNewDragOver && bValidDropTarget == bNewValidTarget)
	{
		return;
	}

	bDragOver = bNewDragOver;
	bValidDropTarget = bNewValidTarget;
	OnSlotDragOverChanged(bDragOver, bValidDropTarget);
}

void UPHInventorySlotWidget::HandleSlotClicked()
{
	SelectSlot();
}

void UPHInventorySlotWidget::HandleDragCancelled(UDragDropOperation* Operation)
{
	const UPHItemDragDropOperation* ItemOperation = Cast<UPHItemDragDropOperation>(Operation);
	if (!ItemOperation || !bDropToGroundWhenDraggedOutOfMenu || !SlotHost.GetInterface())
	{
		return;
	}

	SlotHost->RequestDropInventorySlotToGround(ItemOperation->SourceInventorySlotIndex);
}
