#include "Menu/Widgets/PHEquipmentMenuPageWidget.h"

#include "Equipment/Components/EquipmentManager.h"
#include "Inventory/Components/InventoryManager.h"
#include "Item/ItemInstance.h"

UPHEquipmentMenuPageWidget::UPHEquipmentMenuPageWidget()
{
	EquipmentSlotOrder =
	{
		EEquipmentSlot::ES_MainHand,
		EEquipmentSlot::ES_OffHand,
		EEquipmentSlot::ES_TwoHand,
		EEquipmentSlot::ES_Head,
		EEquipmentSlot::ES_Chest,
		EEquipmentSlot::ES_Hands,
		EEquipmentSlot::ES_Legs,
		EEquipmentSlot::ES_Feet,
		EEquipmentSlot::ES_Amulet,
		EEquipmentSlot::ES_Belt,
		EEquipmentSlot::ES_Ring1,
		EEquipmentSlot::ES_Ring2,
		EEquipmentSlot::ES_Ring3,
		EEquipmentSlot::ES_Ring4,
		EEquipmentSlot::ES_Ring5,
		EEquipmentSlot::ES_Ring6,
		EEquipmentSlot::ES_Ring7,
		EEquipmentSlot::ES_Ring8,
		EEquipmentSlot::ES_Ring9,
		EEquipmentSlot::ES_Ring10
	};
}

void UPHEquipmentMenuPageWidget::NativeInitializeForCharacter(APHBaseCharacter* Character)
{
	Super::NativeInitializeForCharacter(Character);

	BindManagerDelegates();
	RefreshMenuData();
}

void UPHEquipmentMenuPageWidget::NativeReleaseCharacter()
{
	UnbindManagerDelegates();
	ClearSelection();

	EquipmentSlots.Reset();
	InventorySlots.Reset();
	CurrentCarryWeight = 0.0f;
	MaxCarryWeight = 0.0f;
	OccupiedInventorySlots = 0;
	MaxInventorySlots = 0;

	Super::NativeReleaseCharacter();
}

void UPHEquipmentMenuPageWidget::RefreshMenuData()
{
	RebuildEquipmentSlots();
	RebuildInventorySlots();
	UpdateInventorySummary();

	OnMenuDataRefreshed();
}

bool UPHEquipmentMenuPageWidget::GetEquipmentSlotData(EEquipmentSlot EquipmentSlot, FEquipmentMenuSlotViewData& OutData) const
{
	for (const FEquipmentMenuSlotViewData& SlotData : EquipmentSlots)
	{
		if (SlotData.Slot == EquipmentSlot)
		{
			OutData = SlotData;
			return true;
		}
	}

	return false;
}

bool UPHEquipmentMenuPageWidget::GetInventorySlotData(int32 SlotIndex, FEquipmentMenuInventorySlotViewData& OutData) const
{
	for (const FEquipmentMenuInventorySlotViewData& SlotData : InventorySlots)
	{
		if (SlotData.SlotIndex == SlotIndex)
		{
			OutData = SlotData;
			return true;
		}
	}

	return false;
}

UItemInstance* UPHEquipmentMenuPageWidget::GetEquippedItem(EEquipmentSlot EquipmentSlot) const
{
	return EquipmentManager ? EquipmentManager->GetEquippedItem(EquipmentSlot) : nullptr;
}

UItemInstance* UPHEquipmentMenuPageWidget::GetInventoryItem(int32 SlotIndex) const
{
	return InventoryManager ? InventoryManager->GetItemAtSlot(SlotIndex) : nullptr;
}

bool UPHEquipmentMenuPageWidget::CanEquipInventorySlotToSlot(int32 SlotIndex, EEquipmentSlot TargetSlot) const
{
	return CanEquipItemToSlot(GetInventoryItem(SlotIndex), TargetSlot);
}

bool UPHEquipmentMenuPageWidget::CanEquipItemToSlot(UItemInstance* Item, EEquipmentSlot TargetSlot) const
{
	if (!EquipmentManager || !Item || !Item->CanBeEquipped())
	{
		return false;
	}

	if (TargetSlot == EEquipmentSlot::ES_None)
	{
		TargetSlot = EquipmentManager->DetermineEquipmentSlot(Item);
	}

	return TargetSlot != EEquipmentSlot::ES_None
		&& EquipmentManager->CanEquipToSlot(Item, TargetSlot);
}

void UPHEquipmentMenuPageWidget::SelectInventorySlot(int32 SlotIndex)
{
	SetSelection(GetInventoryItem(SlotIndex), SlotIndex, EEquipmentSlot::ES_None);
}

void UPHEquipmentMenuPageWidget::SelectEquipmentSlot(EEquipmentSlot EquipmentSlot)
{
	SetSelection(GetEquippedItem(EquipmentSlot), INDEX_NONE, EquipmentSlot);
}

void UPHEquipmentMenuPageWidget::ClearSelection()
{
	SetSelection(nullptr, INDEX_NONE, EEquipmentSlot::ES_None);
}

bool UPHEquipmentMenuPageWidget::RequestEquipInventorySlot(int32 SlotIndex, EEquipmentSlot TargetSlot)
{
	return RequestEquipItem(GetInventoryItem(SlotIndex), TargetSlot);
}

bool UPHEquipmentMenuPageWidget::RequestEquipItem(UItemInstance* Item, EEquipmentSlot TargetSlot)
{
	if (!CanEquipItemToSlot(Item, TargetSlot))
	{
		return false;
	}

	EquipmentManager->EquipItem(Item, TargetSlot, true);
	return true;
}

bool UPHEquipmentMenuPageWidget::RequestEquipSelectedItem(EEquipmentSlot TargetSlot)
{
	return RequestEquipItem(SelectedItem, TargetSlot);
}

bool UPHEquipmentMenuPageWidget::RequestUnequipSlot(EEquipmentSlot EquipmentSlot, bool bMoveToBag)
{
	if (!EquipmentManager || EquipmentSlot == EEquipmentSlot::ES_None || !EquipmentManager->IsSlotOccupied(EquipmentSlot))
	{
		return false;
	}

	EquipmentManager->UnequipItem(EquipmentSlot, bMoveToBag);
	return true;
}

bool UPHEquipmentMenuPageWidget::RequestUnequipSelectedSlot(bool bMoveToBag)
{
	return RequestUnequipSlot(SelectedEquipmentSlot, bMoveToBag);
}

FText UPHEquipmentMenuPageWidget::GetEquipmentSlotDisplayName(EEquipmentSlot EquipmentSlot)
{
	if (const UEnum* EquipmentSlotEnum = StaticEnum<EEquipmentSlot>())
	{
		return EquipmentSlotEnum->GetDisplayNameTextByValue(static_cast<int64>(EquipmentSlot));
	}

	return FText::FromString(TEXT("None"));
}

void UPHEquipmentMenuPageWidget::BindManagerDelegates()
{
	if (EquipmentManager)
	{
		EquipmentManager->OnEquipmentChanged.AddUniqueDynamic(this, &UPHEquipmentMenuPageWidget::HandleEquipmentChanged);
	}

	if (InventoryManager)
	{
		InventoryManager->OnInventoryChanged.AddUniqueDynamic(this, &UPHEquipmentMenuPageWidget::HandleInventoryChanged);
		InventoryManager->OnWeightChanged.AddUniqueDynamic(this, &UPHEquipmentMenuPageWidget::HandleCarryWeightChanged);
	}
}

void UPHEquipmentMenuPageWidget::UnbindManagerDelegates()
{
	if (EquipmentManager)
	{
		EquipmentManager->OnEquipmentChanged.RemoveDynamic(this, &UPHEquipmentMenuPageWidget::HandleEquipmentChanged);
	}

	if (InventoryManager)
	{
		InventoryManager->OnInventoryChanged.RemoveDynamic(this, &UPHEquipmentMenuPageWidget::HandleInventoryChanged);
		InventoryManager->OnWeightChanged.RemoveDynamic(this, &UPHEquipmentMenuPageWidget::HandleCarryWeightChanged);
	}
}

void UPHEquipmentMenuPageWidget::RebuildEquipmentSlots()
{
	EquipmentSlots.Reset();

	for (const EEquipmentSlot EquipmentSlot : EquipmentSlotOrder)
	{
		if (EquipmentSlot == EEquipmentSlot::ES_None)
		{
			continue;
		}

		FEquipmentMenuSlotViewData SlotData;
		SlotData.Slot = EquipmentSlot;
		SlotData.DisplayName = GetEquipmentSlotDisplayName(EquipmentSlot);
		SlotData.Item = EquipmentManager ? EquipmentManager->GetEquippedItem(EquipmentSlot) : nullptr;
		SlotData.bOccupied = SlotData.Item != nullptr;

		EquipmentSlots.Add(SlotData);
	}
}

void UPHEquipmentMenuPageWidget::RebuildInventorySlots()
{
	InventorySlots.Reset();

	if (!InventoryManager)
	{
		return;
	}

	const int32 SlotCount = FMath::Max(InventoryManager->GetMaxSlots(), InventoryManager->Items.Num());
	for (int32 SlotIndex = 0; SlotIndex < SlotCount; ++SlotIndex)
	{
		UItemInstance* Item = InventoryManager->GetItemAtSlot(SlotIndex);
		if (!bIncludeEmptyInventorySlots && !Item)
		{
			continue;
		}

		FEquipmentMenuInventorySlotViewData SlotData;
		SlotData.SlotIndex = SlotIndex;
		SlotData.Item = Item;
		SlotData.bOccupied = Item != nullptr;
		SlotData.SuggestedEquipmentSlot = ResolveSuggestedSlot(Item);
		SlotData.bCanEquip = SlotData.SuggestedEquipmentSlot != EEquipmentSlot::ES_None;

		InventorySlots.Add(SlotData);
	}
}

void UPHEquipmentMenuPageWidget::UpdateInventorySummary()
{
	if (!InventoryManager)
	{
		CurrentCarryWeight = 0.0f;
		MaxCarryWeight = 0.0f;
		OccupiedInventorySlots = 0;
		MaxInventorySlots = 0;
		return;
	}

	CurrentCarryWeight = InventoryManager->GetTotalWeight();
	MaxCarryWeight = InventoryManager->MaxWeight;
	OccupiedInventorySlots = InventoryManager->GetItemCount();
	MaxInventorySlots = InventoryManager->GetMaxSlots();
}

EEquipmentSlot UPHEquipmentMenuPageWidget::ResolveSuggestedSlot(UItemInstance* Item) const
{
	if (!EquipmentManager || !Item || !Item->CanBeEquipped())
	{
		return EEquipmentSlot::ES_None;
	}

	EEquipmentSlot SuggestedSlot = EquipmentManager->DetermineEquipmentSlot(Item);
	if (SuggestedSlot != EEquipmentSlot::ES_None &&
		EquipmentManager->CanEquipToSlot(Item, SuggestedSlot))
	{
		return SuggestedSlot;
	}

	for (const EEquipmentSlot EquipmentSlot : EquipmentSlotOrder)
	{
		if (EquipmentManager->CanEquipToSlot(Item, EquipmentSlot))
		{
			return EquipmentSlot;
		}
	}

	return EEquipmentSlot::ES_None;
}

void UPHEquipmentMenuPageWidget::SetSelection(UItemInstance* Item, int32 InventorySlotIndex, EEquipmentSlot EquipmentSlot)
{
	if (SelectedItem == Item &&
		SelectedInventorySlotIndex == InventorySlotIndex &&
		SelectedEquipmentSlot == EquipmentSlot)
	{
		return;
	}

	SelectedItem = Item;
	SelectedInventorySlotIndex = InventorySlotIndex;
	SelectedEquipmentSlot = EquipmentSlot;

	OnSelectionChanged(SelectedItem, SelectedInventorySlotIndex, SelectedEquipmentSlot);
}

void UPHEquipmentMenuPageWidget::HandleEquipmentChanged(EEquipmentSlot EquipmentSlot, UItemInstance* NewItem, UItemInstance* OldItem)
{
	RefreshMenuData();
	OnEquipmentSlotChanged(EquipmentSlot, NewItem, OldItem);
}

void UPHEquipmentMenuPageWidget::HandleInventoryChanged()
{
	RefreshMenuData();
	OnInventoryChanged();
}

void UPHEquipmentMenuPageWidget::HandleCarryWeightChanged(float NewCurrentWeight, float NewMaxWeight)
{
	CurrentCarryWeight = NewCurrentWeight;
	MaxCarryWeight = NewMaxWeight;

	OnCarryWeightChanged(CurrentCarryWeight, MaxCarryWeight);
}
