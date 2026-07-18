#include "Menu/Widgets/PHEquipmentMenuPageWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Widget.h"
#include "Equipment/Components/EquipmentManager.h"
#include "Inventory/Components/InventoryManager.h"
#include "Item/ItemInstance.h"
#include "Menu/Library/FunctionLibraries/MenuFunctionLibrary.h"
#include "Menu/Widgets/PHEquipmentSlotWidget.h"
#include "Menu/Widgets/PHInventoryMenuPanelWidget.h"

UPHEquipmentMenuPageWidget::UPHEquipmentMenuPageWidget()
{
	EquipmentSlotOrder = UMenuFunctionLibrary::GetDefaultEquipmentSlotOrder();
}

void UPHEquipmentMenuPageWidget::NativeConstruct()
{
	Super::NativeConstruct();

	CacheChildWidgets();
	BindInventoryPanelDelegates();
	RefreshEquipmentSlotWidgets();
}

void UPHEquipmentMenuPageWidget::NativeDestruct()
{
	UnbindInventoryPanelDelegates();

	Super::NativeDestruct();
}

void UPHEquipmentMenuPageWidget::NativeInitializeForCharacter(APHBaseCharacter* Character)
{
	Super::NativeInitializeForCharacter(Character);

	CacheChildWidgets();
	BindInventoryPanelDelegates();
	BindManagerDelegates();
	RefreshMenuData();
}

void UPHEquipmentMenuPageWidget::NativeReleaseCharacter()
{
	UnbindManagerDelegates();
	UnbindInventoryPanelDelegates();
	ClearSelection();

	EquipmentSlots.Reset();
	InventorySlots.Reset();
	EquipmentSlotWidgets.Reset();
	CurrentCarryWeight = 0.0f;
	MaxCarryWeight = 0.0f;
	OccupiedInventorySlots = 0;
	MaxInventorySlots = 0;

	Super::NativeReleaseCharacter();
}

void UPHEquipmentMenuPageWidget::RefreshMenuData()
{
	RebuildEquipmentSlots();
	RefreshEquipmentSlotWidgets();

	if (InventoryPanel)
	{
		InventoryPanel->SetEquipmentSlotOrder(EquipmentSlotOrder);
		InventoryPanel->RefreshInventoryData();
		SyncInventoryStateFromPanel();
	}
	else
	{
		RebuildInventorySlots();
		UpdateInventorySummary();
	}

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
	if (InventoryPanel)
	{
		return InventoryPanel->GetInventorySlotData(SlotIndex, OutData);
	}

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
	if (InventoryPanel)
	{
		return InventoryPanel->GetInventoryItem(SlotIndex);
	}

	return InventoryManager ? InventoryManager->GetItemAtSlot(SlotIndex) : nullptr;
}

bool UPHEquipmentMenuPageWidget::CanEquipInventorySlotToSlot(int32 SlotIndex, EEquipmentSlot TargetSlot) const
{
	if (InventoryPanel)
	{
		return InventoryPanel->CanEquipInventorySlotToSlot(SlotIndex, TargetSlot);
	}

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
	if (InventoryPanel)
	{
		InventoryPanel->SelectInventorySlot(SlotIndex);
		return;
	}

	SetSelection(GetInventoryItem(SlotIndex), SlotIndex, EEquipmentSlot::ES_None);
}

void UPHEquipmentMenuPageWidget::SelectEquipmentSlot(EEquipmentSlot EquipmentSlot)
{
	SetSelection(GetEquippedItem(EquipmentSlot), INDEX_NONE, EquipmentSlot);
}

void UPHEquipmentMenuPageWidget::ClearSelection()
{
	if (InventoryPanel)
	{
		InventoryPanel->ClearSelection();
	}

	SetSelection(nullptr, INDEX_NONE, EEquipmentSlot::ES_None);
}

bool UPHEquipmentMenuPageWidget::RequestEquipInventorySlot(int32 SlotIndex, EEquipmentSlot TargetSlot)
{
	if (InventoryPanel)
	{
		return InventoryPanel->RequestEquipInventorySlot(SlotIndex, TargetSlot);
	}

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
	return UMenuFunctionLibrary::GetEquipmentSlotDisplayName(EquipmentSlot);
}

void UPHEquipmentMenuPageWidget::CacheChildWidgets()
{
	EquipmentSlotWidgets.Reset();

	if (!WidgetTree)
	{
		return;
	}

	TArray<UWidget*> ChildWidgets;
	WidgetTree->GetAllWidgets(ChildWidgets);

	for (UWidget* ChildWidget : ChildWidgets)
	{
		if (UPHEquipmentSlotWidget* EquipmentSlotWidget = Cast<UPHEquipmentSlotWidget>(ChildWidget))
		{
			EquipmentSlotWidget->SetOwningEquipmentPage(this);
			EquipmentSlotWidgets.AddUnique(EquipmentSlotWidget);
			continue;
		}

		if (!InventoryPanel)
		{
			InventoryPanel = Cast<UPHInventoryMenuPanelWidget>(ChildWidget);
		}
	}
}

void UPHEquipmentMenuPageWidget::BindInventoryPanelDelegates()
{
	if (!InventoryPanel)
	{
		return;
	}

	InventoryPanel->InventoryDataRefreshed.AddUniqueDynamic(this, &UPHEquipmentMenuPageWidget::HandleInventoryPanelDataRefreshed);
	InventoryPanel->InventorySelectionChanged.AddUniqueDynamic(this, &UPHEquipmentMenuPageWidget::HandleInventoryPanelSelectionChanged);
	InventoryPanel->InventoryCarryWeightChanged.AddUniqueDynamic(this, &UPHEquipmentMenuPageWidget::HandleInventoryPanelCarryWeightChanged);
}

void UPHEquipmentMenuPageWidget::UnbindInventoryPanelDelegates()
{
	if (!InventoryPanel)
	{
		return;
	}

	InventoryPanel->InventoryDataRefreshed.RemoveDynamic(this, &UPHEquipmentMenuPageWidget::HandleInventoryPanelDataRefreshed);
	InventoryPanel->InventorySelectionChanged.RemoveDynamic(this, &UPHEquipmentMenuPageWidget::HandleInventoryPanelSelectionChanged);
	InventoryPanel->InventoryCarryWeightChanged.RemoveDynamic(this, &UPHEquipmentMenuPageWidget::HandleInventoryPanelCarryWeightChanged);
}

void UPHEquipmentMenuPageWidget::RefreshEquipmentSlotWidgets()
{
	for (UPHEquipmentSlotWidget* EquipmentSlotWidget : EquipmentSlotWidgets)
	{
		if (!EquipmentSlotWidget)
		{
			continue;
		}

		EquipmentSlotWidget->SetOwningEquipmentPage(this);
		EquipmentSlotWidget->RefreshSlot();
	}
}

void UPHEquipmentMenuPageWidget::SyncInventoryStateFromPanel()
{
	if (!InventoryPanel)
	{
		return;
	}

	InventorySlots = InventoryPanel->GetInventorySlots();
	CurrentCarryWeight = InventoryPanel->GetCurrentCarryWeight();
	MaxCarryWeight = InventoryPanel->GetMaxCarryWeight();
	OccupiedInventorySlots = InventoryPanel->GetOccupiedInventorySlots();
	MaxInventorySlots = InventoryPanel->GetMaxInventorySlots();
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

		UItemInstance* Item = EquipmentManager ? EquipmentManager->GetEquippedItem(EquipmentSlot) : nullptr;
		const FEquipmentMenuSlotViewData SlotData = UMenuFunctionLibrary::MakeEquipmentSlotViewData(EquipmentSlot, Item);

		EquipmentSlots.Add(SlotData);
	}
}

void UPHEquipmentMenuPageWidget::RebuildInventorySlots()
{
	InventorySlots.Reset();

	if (InventoryPanel)
	{
		SyncInventoryStateFromPanel();
		return;
	}

	if (!InventoryManager)
	{
		return;
	}

	const int32 SlotCount = InventoryManager->GetSlotCount();
	for (int32 SlotIndex = 0; SlotIndex < SlotCount; ++SlotIndex)
	{
		UItemInstance* Item = InventoryManager->GetItemAtSlot(SlotIndex);
		if (!bIncludeEmptyInventorySlots && !Item)
		{
			continue;
		}

		const EEquipmentSlot SuggestedSlot = ResolveSuggestedSlot(Item);
		const FEquipmentMenuInventorySlotViewData SlotData =
			UMenuFunctionLibrary::MakeInventorySlotViewData(SlotIndex, Item, SuggestedSlot);

		InventorySlots.Add(SlotData);
	}
}

void UPHEquipmentMenuPageWidget::UpdateInventorySummary()
{
	if (InventoryPanel)
	{
		SyncInventoryStateFromPanel();
		return;
	}

	if (!InventoryManager)
	{
		CurrentCarryWeight = 0.0f;
		MaxCarryWeight = 0.0f;
		OccupiedInventorySlots = 0;
		MaxInventorySlots = 0;
		return;
	}

	CurrentCarryWeight = InventoryManager->GetTotalWeight();
	MaxCarryWeight = InventoryManager->GetMaxWeight();
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

void UPHEquipmentMenuPageWidget::HandleInventoryPanelDataRefreshed()
{
	SyncInventoryStateFromPanel();
}

void UPHEquipmentMenuPageWidget::HandleInventoryPanelSelectionChanged(UItemInstance* NewSelectedItem, int32 NewInventorySlotIndex, EEquipmentSlot)
{
	SetSelection(NewSelectedItem, NewInventorySlotIndex, EEquipmentSlot::ES_None);
}

void UPHEquipmentMenuPageWidget::HandleInventoryPanelCarryWeightChanged(float NewCurrentWeight, float NewMaxWeight)
{
	CurrentCarryWeight = NewCurrentWeight;
	MaxCarryWeight = NewMaxWeight;

	OnCarryWeightChanged(CurrentCarryWeight, MaxCarryWeight);
}
