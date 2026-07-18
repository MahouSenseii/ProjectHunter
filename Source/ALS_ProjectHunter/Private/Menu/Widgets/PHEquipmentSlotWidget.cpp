#include "Menu/Widgets/PHEquipmentSlotWidget.h"

#include "Character/PHBaseCharacter.h"
#include "Equipment/Components/EquipmentManager.h"
#include "Item/ItemInstance.h"
#include "Menu/Library/FunctionLibraries/MenuFunctionLibrary.h"
#include "Menu/Widgets/PHEquipmentMenuPageWidget.h"

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
	UItemInstance* Item = (EquipmentManager && ConnectedEquipmentSlot != EEquipmentSlot::ES_None)
		? EquipmentManager->GetEquippedItem(ConnectedEquipmentSlot)
		: nullptr;
	SlotData = UMenuFunctionLibrary::MakeEquipmentSlotViewData(ConnectedEquipmentSlot, Item);

	OnSlotDataRefreshed(SlotData);
	SlotDataRefreshed.Broadcast(SlotData);
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
	if (!EquipmentManager
		|| ConnectedEquipmentSlot == EEquipmentSlot::ES_None
		|| !EquipmentManager->IsSlotOccupied(ConnectedEquipmentSlot))
	{
		return false;
	}

	EquipmentManager->UnequipItem(ConnectedEquipmentSlot, bMoveToBag);
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

	Super::NativeReleaseCharacter();
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

void UPHEquipmentSlotWidget::HandleEquipmentChanged(EEquipmentSlot EquipmentSlot, UItemInstance* NewItem, UItemInstance* OldItem)
{
	if (EquipmentSlot != ConnectedEquipmentSlot)
	{
		return;
	}

	RefreshSlot();
	OnEquipmentSlotChanged(EquipmentSlot, NewItem, OldItem);
	EquipmentSlotChanged.Broadcast(EquipmentSlot, NewItem, OldItem);
}
