#include "UI/Menu/Widgets/PHEquipmentSlotWidget.h"

#include "Character/PHBaseCharacter.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "Components/TextBlock.h"
#include "Equipment/Components/EquipmentManager.h"
#include "Equipment/Library/FunctionLibraries/EquipmentFunctionLibrary.h"
#include "Inventory/Components/InventoryManager.h"
#include "Item/ItemInstance.h"
#include "UI/Menu/DragDrop/PHItemDragDropOperation.h"
#include "UI/Menu/Helpers/MenuItemDragDropHelper.h"
#include "UI/Menu/Helpers/MenuSlotTooltipHelper.h"
#include "UI/Menu/Library/FunctionLibraries/MenuFunctionLibrary.h"
#include "UI/Menu/Widgets/PHEquipmentMenuPageWidget.h"

UPHEquipmentSlotWidget::UPHEquipmentSlotWidget()
{
	SlotData = UMenuFunctionLibrary::MakeEquipmentSlotViewData(ConnectedEquipmentSlot, nullptr);
}

void UPHEquipmentSlotWidget::SetConnectedEquipmentSlot(EEquipmentSlot NewSlot)
{
	if (ConnectedEquipmentSlot == NewSlot)
	{
		return;
	}

	ConnectedEquipmentSlot = NewSlot;
	RefreshSlot();
	OnConnectedEquipmentSlotChanged(ConnectedEquipmentSlot);
}

void UPHEquipmentSlotWidget::SetOwningEquipmentPage(UPHEquipmentMenuPageWidget* EquipmentPage)
{
	OwningEquipmentPage = EquipmentPage;
}

void UPHEquipmentSlotWidget::RefreshSlot()
{
	// Read through the occupying slot so a two-handed weapon shows in both hands,
	// but keep the connected slot in the view data so the label stays the one the
	// player sees ("Off Hand", not "Two Hand").
	const EEquipmentSlot OccupyingSlot = GetOccupyingEquipmentSlot();
	UItemInstance* Item = (EquipmentManager && OccupyingSlot != EEquipmentSlot::ES_None)
		? EquipmentManager->GetEquippedItem(OccupyingSlot)
		: nullptr;
	SlotData = UMenuFunctionLibrary::MakeEquipmentSlotViewData(ConnectedEquipmentSlot, Item);
	RefreshVisuals();

	OnSlotDataRefreshed(SlotData);
	SlotDataRefreshed.Broadcast(SlotData);
}

bool UPHEquipmentSlotWidget::IsFilledByTwoHandedWeapon() const
{
	return GetOccupyingEquipmentSlot() != ConnectedEquipmentSlot;
}

bool UPHEquipmentSlotWidget::CanAcceptItem(UItemInstance* Item) const
{
	return EquipmentManager
		&& Item
		&& Item->CanBeEquipped()
		&& ConnectedEquipmentSlot != EEquipmentSlot::ES_None
		&& EquipmentManager->CanEquipToSlot(Item, ConnectedEquipmentSlot);
}

bool UPHEquipmentSlotWidget::RequestEquipItem(UItemInstance* Item)
{
	if (!CanAcceptItem(Item))
	{
		return false;
	}

	EquipmentManager->EquipItem(Item, ConnectedEquipmentSlot, true);
	return true;
}

bool UPHEquipmentSlotWidget::RequestEquipSelectedItem()
{
	return OwningEquipmentPage
		&& ConnectedEquipmentSlot != EEquipmentSlot::ES_None
		&& OwningEquipmentPage->RequestEquipSelectedItem(ConnectedEquipmentSlot);
}

bool UPHEquipmentSlotWidget::RequestUnequip(bool bMoveToBag)
{
	const EEquipmentSlot OccupyingSlot = GetOccupyingEquipmentSlot();
	if (!EquipmentManager
		|| OccupyingSlot == EEquipmentSlot::ES_None
		|| !EquipmentManager->IsSlotOccupied(OccupyingSlot))
	{
		return false;
	}

	EquipmentManager->UnequipItem(OccupyingSlot, bMoveToBag);
	return true;
}

void UPHEquipmentSlotWidget::SelectSlot()
{
	if (OwningEquipmentPage && ConnectedEquipmentSlot != EEquipmentSlot::ES_None)
	{
		OwningEquipmentPage->SelectEquipmentSlot(ConnectedEquipmentSlot);
	}
}

FText UPHEquipmentSlotWidget::GetEquipmentSlotDisplayName(EEquipmentSlot EquipmentSlot)
{
	return UMenuFunctionLibrary::GetEquipmentSlotDisplayName(EquipmentSlot);
}

// DRAG AND DROP

bool UPHEquipmentSlotWidget::CanAcceptDroppedItem(UPHItemDragDropOperation* Operation) const
{
	if (!Operation || !Operation->IsValidDrag())
	{
		return false;
	}

	// Dropping a slot back onto itself is a no-op, not a valid target. A
	// two-handed weapon dragged between the hands lands on its own slot too.
	if (Operation->IsSameEquipmentSlot(GetOccupyingEquipmentSlot()))
	{
		return false;
	}

	return CanAcceptItem(Operation->Item);
}

bool UPHEquipmentSlotWidget::HandleItemDropped(UPHItemDragDropOperation* Operation)
{
	if (!CanAcceptDroppedItem(Operation))
	{
		return false;
	}

	if (Operation->IsFromEquipment())
	{
		// Return the item to the bag before re-equipping it. Both calls are
		// reliable and ordered, and routing through the bag means a rejected
		// equip leaves the item in the inventory rather than nowhere.
		EquipmentManager->UnequipItem(Operation->SourceEquipmentSlot, /*bMoveToBag=*/true);
	}

	return RequestEquipItem(Operation->Item);
}

// LIFECYCLE

void UPHEquipmentSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (SlotButton)
	{
		SlotButton->OnClicked.AddUniqueDynamic(this, &UPHEquipmentSlotWidget::HandleSlotClicked);
	}

	if (bEnableDragAndDrop)
	{
		// See UPHInventorySlotWidget: the slot must own the mouse for drag
		// detection, so a UButton child is made non-hit-testable and clicks are
		// handled in NativeOnMouseButtonUp instead.
		SetVisibility(ESlateVisibility::Visible);

		if (SlotButton)
		{
			SlotButton->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
	}

	RefreshVisuals();
}

void UPHEquipmentSlotWidget::NativeDestruct()
{
	if (SlotButton)
	{
		SlotButton->OnClicked.RemoveDynamic(this, &UPHEquipmentSlotWidget::HandleSlotClicked);
	}

	if (bSlotHovered)
	{
		FMenuSlotTooltipHelper::Hide(*this);
		bSlotHovered = false;
	}

	Super::NativeDestruct();
}

void UPHEquipmentSlotWidget::NativeInitializeForCharacter(APHBaseCharacter* Character)
{
	Super::NativeInitializeForCharacter(Character);

	EquipmentManager = Character ? Character->GetEquipmentManager() : nullptr;
	BindManagerDelegates();
	RefreshSlot();
}

void UPHEquipmentSlotWidget::NativeReleaseCharacter()
{
	UnbindManagerDelegates();
	EquipmentManager = nullptr;
	SlotData = UMenuFunctionLibrary::MakeEquipmentSlotViewData(ConnectedEquipmentSlot, nullptr);
	RefreshVisuals();

	Super::NativeReleaseCharacter();
}

// HOVER / TOOLTIP

void UPHEquipmentSlotWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	SetHovered(true);
}

void UPHEquipmentSlotWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
	SetHovered(false);
}

FReply UPHEquipmentSlotWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// Mouse-move rather than tick: UUserWidget ticking is opt-in and the tooltip
	// only ever needs to move when the cursor does.
	if (bSlotHovered && bShowTooltipOnHover && bTooltipFollowsMouse && SlotData.Item)
	{
		FMenuSlotTooltipHelper::UpdatePosition(*this);
	}

	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

// MOUSE / DRAG

FReply UPHEquipmentSlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bEnableDragAndDrop
		&& SlotData.Item
		&& InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		return FReply::Handled().DetectDrag(TakeWidget(), EKeys::LeftMouseButton);
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UPHEquipmentSlotWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		SelectSlot();
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

void UPHEquipmentSlotWidget::NativeOnDragDetected(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent,
	UDragDropOperation*& OutOperation)
{
	Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);

	const EEquipmentSlot OccupyingSlot = GetOccupyingEquipmentSlot();
	if (!bEnableDragAndDrop || !SlotData.Item || OccupyingSlot == EEquipmentSlot::ES_None)
	{
		return;
	}

	FMenuSlotTooltipHelper::Hide(*this);
	SetHovered(false);

	// Carry the slot the item is stored in: dragging the two-handed weapon out of
	// the off hand still has to unequip ES_TwoHand.
	UPHItemDragDropOperation* Operation = FMenuItemDragDropHelper::MakeEquipmentDrag(
		*this, SlotData.Item, OccupyingSlot, DragVisualWidgetClass, DragVisualSize);
	if (!Operation)
	{
		return;
	}

	Operation->OnDragCancelled.AddUniqueDynamic(this, &UPHEquipmentSlotWidget::HandleDragCancelled);

	OutOperation = Operation;
	OnSlotDragStarted(Operation);
}

void UPHEquipmentSlotWidget::NativeOnDragEnter(
	const FGeometry& InGeometry,
	const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation)
{
	Super::NativeOnDragEnter(InGeometry, InDragDropEvent, InOperation);

	if (UPHItemDragDropOperation* ItemOperation = Cast<UPHItemDragDropOperation>(InOperation))
	{
		SetDragOverState(true, CanAcceptDroppedItem(ItemOperation));
	}
}

void UPHEquipmentSlotWidget::NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDragLeave(InDragDropEvent, InOperation);
	SetDragOverState(false, false);
}

bool UPHEquipmentSlotWidget::NativeOnDrop(
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

	HandleItemDropped(ItemOperation);

	// Claim the drop even when rejected - an unhandled drop is what triggers
	// OnDragCancelled, which means "the player let go outside the menu".
	return true;
}

// INTERNAL

void UPHEquipmentSlotWidget::RefreshVisuals()
{
	UItemInstance* Item = SlotData.Item;

	if (SlotNameText)
	{
		SlotNameText->SetText(SlotData.DisplayName);
		SlotNameText->SetVisibility(Item && bHideSlotNameWhenOccupied
			? ESlateVisibility::Collapsed
			: ESlateVisibility::HitTestInvisible);
	}

	if (ItemNameText)
	{
		ItemNameText->SetText(Item && bShowItemNameInSlot ? Item->GetDisplayName() : FText::GetEmpty());
		ItemNameText->SetVisibility(bShowItemNameInSlot
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
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

void UPHEquipmentSlotWidget::SetHovered(const bool bNewHovered)
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

void UPHEquipmentSlotWidget::SetDragOverState(const bool bNewDragOver, const bool bNewValidTarget)
{
	if (bDragOver == bNewDragOver && bValidDropTarget == bNewValidTarget)
	{
		return;
	}

	bDragOver = bNewDragOver;
	bValidDropTarget = bNewValidTarget;
	OnSlotDragOverChanged(bDragOver, bValidDropTarget);
}

void UPHEquipmentSlotWidget::HandleSlotClicked()
{
	SelectSlot();
}

void UPHEquipmentSlotWidget::HandleEquipmentChanged(EEquipmentSlot EquipmentSlot, UItemInstance* NewItem, UItemInstance* OldItem)
{
	if (!IsAffectedByEquipmentChange(EquipmentSlot))
	{
		return;
	}

	RefreshSlot();
	OnEquipmentSlotChanged(EquipmentSlot, NewItem, OldItem);
	EquipmentSlotChanged.Broadcast(EquipmentSlot, NewItem, OldItem);
}

void UPHEquipmentSlotWidget::HandleDragCancelled(UDragDropOperation* Operation)
{
	UPHItemDragDropOperation* ItemOperation = Cast<UPHItemDragDropOperation>(Operation);
	if (!ItemOperation || !bDropToWorldWhenDraggedOutOfMenu || !EquipmentManager)
	{
		return;
	}

	if (!EquipmentManager->IsSlotOccupied(ItemOperation->SourceEquipmentSlot))
	{
		return;
	}

	// Unequip into the bag, then drop that item. Routing through the bag means a
	// failed drop leaves the item in the inventory rather than nowhere.
	EquipmentManager->UnequipItem(ItemOperation->SourceEquipmentSlot, /*bMoveToBag=*/true);

	if (const APHBaseCharacter* Character = GetBoundCharacter())
	{
		if (UInventoryManager* InventoryManager = Character->FindComponentByClass<UInventoryManager>())
		{
			InventoryManager->DropItemToGround(ItemOperation->Item);
		}
	}
}

EEquipmentSlot UPHEquipmentSlotWidget::GetOccupyingEquipmentSlot() const
{
	return EquipmentManager
		? EquipmentManager->ResolveOccupyingSlot(ConnectedEquipmentSlot)
		: ConnectedEquipmentSlot;
}

bool UPHEquipmentSlotWidget::IsAffectedByEquipmentChange(EEquipmentSlot ChangedSlot) const
{
	if (ChangedSlot == ConnectedEquipmentSlot)
	{
		return true;
	}

	// Both hands render the shared two-hand entry, and this fires after the
	// change - so testing the slot rather than the current occupant is what
	// catches the two-hander being unequipped as well as equipped.
	return ChangedSlot == EEquipmentSlot::ES_TwoHand
		&& UEquipmentFunctionLibrary::IsHandSlot(ConnectedEquipmentSlot);
}

void UPHEquipmentSlotWidget::BindManagerDelegates()
{
	if (EquipmentManager)
	{
		EquipmentManager->OnEquipmentChanged.AddUniqueDynamic(this, &UPHEquipmentSlotWidget::HandleEquipmentChanged);
	}
}

void UPHEquipmentSlotWidget::UnbindManagerDelegates()
{
	if (EquipmentManager)
	{
		EquipmentManager->OnEquipmentChanged.RemoveDynamic(this, &UPHEquipmentSlotWidget::HandleEquipmentChanged);
	}
}
