#include "Menu/Widgets/PHEquipmentSlotWidget.h"

#include "Character/PHBaseCharacter.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
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
	RefreshVisuals();

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

void UPHEquipmentSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (SlotButton)
	{
		SlotButton->OnClicked.AddUniqueDynamic(this, &UPHEquipmentSlotWidget::HandleSlotClicked);
	}

	RefreshVisuals();
}

void UPHEquipmentSlotWidget::NativeDestruct()
{
	if (SlotButton)
	{
		SlotButton->OnClicked.RemoveDynamic(this, &UPHEquipmentSlotWidget::HandleSlotClicked);
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

void UPHEquipmentSlotWidget::RefreshVisuals()
{
	if (SlotNameText)
	{
		SlotNameText->SetText(SlotData.DisplayName);
	}

	UItemInstance* Item = SlotData.Item;
	if (ItemNameText)
	{
		ItemNameText->SetText(Item ? Item->GetDisplayName() : FText::GetEmpty());
	}

	if (ItemIcon)
	{
		if (Item && Item->GetInventoryIcon())
		{
			ItemIcon->SetBrushFromMaterial(Item->GetInventoryIcon());
			ItemIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			ItemIcon->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UPHEquipmentSlotWidget::HandleSlotClicked()
{
	SelectSlot();
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
