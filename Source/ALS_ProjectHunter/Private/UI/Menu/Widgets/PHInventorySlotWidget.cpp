#include "UI/Menu/Widgets/PHInventorySlotWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Item/ItemInstance.h"
#include "UI/Menu/Widgets/PHInventoryMenuPanelWidget.h"

void UPHInventorySlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (SlotButton)
	{
		SlotButton->OnClicked.AddUniqueDynamic(this, &UPHInventorySlotWidget::HandleSlotClicked);
	}

	RefreshVisuals();
}

void UPHInventorySlotWidget::NativeDestruct()
{
	if (SlotButton)
	{
		SlotButton->OnClicked.RemoveDynamic(this, &UPHInventorySlotWidget::HandleSlotClicked);
	}

	Super::NativeDestruct();
}

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
	RefreshVisuals();

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

void UPHInventorySlotWidget::RefreshVisuals()
{
	if (SlotIndexText)
	{
		SlotIndexText->SetText(SlotData.SlotIndex == INDEX_NONE
			? FText::GetEmpty()
			: FText::AsNumber(SlotData.SlotIndex + 1));
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

void UPHInventorySlotWidget::HandleSlotClicked()
{
	SelectSlot();
}
