#include "Menu/Widgets/PHInventorySlotWidget.h"

#include "Menu/Widgets/PHInventoryMenuPanelWidget.h"

void UPHInventorySlotWidget::InitializeInventorySlot(
	UPHInventoryMenuPanelWidget* InOwningPanel,
	const FEquipmentMenuInventorySlotViewData& InSlotData)
{
	OwningInventoryPanel = InOwningPanel;
	SetSlotData(InSlotData);
}

void UPHInventorySlotWidget::SetSlotData(const FEquipmentMenuInventorySlotViewData& InSlotData)
{
	SlotData = InSlotData;

	OnSlotDataRefreshed(SlotData);
	SlotDataRefreshed.Broadcast(SlotData);
}

void UPHInventorySlotWidget::SelectSlot()
{
	if (OwningInventoryPanel && SlotData.SlotIndex != INDEX_NONE)
	{
		OwningInventoryPanel->SelectInventorySlot(SlotData.SlotIndex);
	}
}

bool UPHInventorySlotWidget::CanEquipToSlot(const EEquipmentSlot TargetSlot) const
{
	return OwningInventoryPanel
		&& SlotData.SlotIndex != INDEX_NONE
		&& OwningInventoryPanel->CanEquipInventorySlotToSlot(SlotData.SlotIndex, TargetSlot);
}

bool UPHInventorySlotWidget::RequestEquip(const EEquipmentSlot TargetSlot)
{
	return OwningInventoryPanel
		&& SlotData.SlotIndex != INDEX_NONE
		&& OwningInventoryPanel->RequestEquipInventorySlot(SlotData.SlotIndex, TargetSlot);
}
