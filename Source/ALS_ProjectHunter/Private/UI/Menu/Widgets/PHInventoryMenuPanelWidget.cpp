#include "UI/Menu/Widgets/PHInventoryMenuPanelWidget.h"

#include "Character/PHBaseCharacter.h"
#include "Components/PanelSlot.h"
#include "Components/PanelWidget.h"
#include "Components/UniformGridSlot.h"
#include "Equipment/Components/EquipmentManager.h"
#include "Inventory/Components/InventoryManager.h"
#include "Item/ItemInstance.h"
#include "UI/Menu/DragDrop/PHItemDragDropOperation.h"
#include "UI/Menu/Helpers/MenuInventoryGridBuilder.h"
#include "Core/Logging/ProjectHunterLogMacros.h"
#include "UI/Menu/Library/MenuLog.h"
#include "UI/Menu/Library/FunctionLibraries/MenuFunctionLibrary.h"
#include "UI/Menu/Widgets/PHInventorySlotWidget.h"

UPHInventoryMenuPanelWidget::UPHInventoryMenuPanelWidget()
{
	EquipmentSlotOrder = UMenuFunctionLibrary::GetDefaultEquipmentSlotOrder();
}

void UPHInventoryMenuPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	RebuildInventorySlotWidgets();
}

void UPHInventoryMenuPanelWidget::RefreshInventoryData()
{
	RebuildInventorySlots();
	UpdateInventorySummary();
	RebuildInventorySlotWidgets();

	OnInventoryDataRefreshed();
	InventoryDataRefreshed.Broadcast();
}

void UPHInventoryMenuPanelWidget::RebuildInventorySlotWidgets()
{
	if (!bAutoBuildInventorySlotWidgets)
	{
		return;
	}

	const bool bRecreated = FMenuInventoryGridBuilder::Rebuild(
		*this,
		TScriptInterface<IPHInventorySlotHost>(this),
		InventorySlotContainer,
		InventorySlotWidgetClass,
		GridColumns,
		InventoryCellSize,
		InventorySlots,
		InventorySlotWidgets);

	// Only announce a genuine rebuild - an in-place refresh keeps the same widgets.
	if (bRecreated)
	{
		OnInventorySlotWidgetsRebuilt();
		InventorySlotWidgetsRebuilt.Broadcast();
	}
}

void UPHInventoryMenuPanelWidget::SetEquipmentSlotOrder(const TArray<EEquipmentSlot>& NewEquipmentSlotOrder)
{
	if (EquipmentSlotOrder == NewEquipmentSlotOrder)
	{
		return;
	}

	EquipmentSlotOrder = NewEquipmentSlotOrder;
	RefreshInventoryData();
}

bool UPHInventoryMenuPanelWidget::GetInventorySlotData(int32 SlotIndex, FEquipmentMenuInventorySlotViewData& OutData) const
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

UItemInstance* UPHInventoryMenuPanelWidget::GetInventoryItem(int32 SlotIndex) const
{
	return InventoryManager ? InventoryManager->GetItemAtSlot(SlotIndex) : nullptr;
}

bool UPHInventoryMenuPanelWidget::CanEquipInventorySlotToSlot(int32 SlotIndex, EEquipmentSlot TargetSlot) const
{
	return CanEquipItemToSlot(GetInventoryItem(SlotIndex), TargetSlot);
}

bool UPHInventoryMenuPanelWidget::CanEquipItemToSlot(UItemInstance* Item, EEquipmentSlot TargetSlot) const
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

void UPHInventoryMenuPanelWidget::SelectInventorySlot(int32 SlotIndex)
{
	UItemInstance* Item = GetInventoryItem(SlotIndex);
	SetSelection(Item, SlotIndex, ResolveSuggestedSlot(Item));
}

void UPHInventoryMenuPanelWidget::ClearSelection()
{
	SetSelection(nullptr, INDEX_NONE, EEquipmentSlot::ES_None);
}

bool UPHInventoryMenuPanelWidget::RequestEquipInventorySlot(int32 SlotIndex, EEquipmentSlot TargetSlot)
{
	UItemInstance* Item = GetInventoryItem(SlotIndex);
	if (!CanEquipItemToSlot(Item, TargetSlot))
	{
		return false;
	}

	EquipmentManager->EquipItem(Item, TargetSlot, true);
	return true;
}

bool UPHInventoryMenuPanelWidget::RequestEquipSelectedItem(EEquipmentSlot TargetSlot)
{
	if (!CanEquipItemToSlot(SelectedItem, TargetSlot))
	{
		return false;
	}

	EquipmentManager->EquipItem(SelectedItem, TargetSlot, true);
	return true;
}

bool UPHInventoryMenuPanelWidget::CanAcceptDroppedItem(UPHItemDragDropOperation* Operation, int32 TargetSlotIndex) const
{
	if (!Operation || !Operation->IsValidDrag() || TargetSlotIndex == INDEX_NONE)
	{
		return false;
	}

	if (Operation->IsFromInventory())
	{
		// Dropping a slot back onto itself is a no-op, not a valid target.
		return InventoryManager != nullptr && !Operation->IsSameInventorySlot(TargetSlotIndex);
	}

	if (Operation->IsFromEquipment())
	{
		return EquipmentManager != nullptr
			&& Operation->SourceEquipmentSlot != EEquipmentSlot::ES_None;
	}

	return false;
}

bool UPHInventoryMenuPanelWidget::HandleItemDroppedOnSlot(UPHItemDragDropOperation* Operation, int32 TargetSlotIndex)
{
	if (!CanAcceptDroppedItem(Operation, TargetSlotIndex))
	{
		return false;
	}

	if (Operation->IsFromInventory())
	{
		return RequestMoveInventoryItem(Operation->SourceInventorySlotIndex, TargetSlotIndex);
	}

	return RequestUnequipToInventory(Operation->SourceEquipmentSlot);
}

bool UPHInventoryMenuPanelWidget::RequestMoveInventoryItem(int32 FromSlotIndex, int32 ToSlotIndex)
{
	if (!InventoryManager
		|| FromSlotIndex == INDEX_NONE
		|| ToSlotIndex == INDEX_NONE
		|| FromSlotIndex == ToSlotIndex)
	{
		return false;
	}

	// SwapItems covers both cases: an occupied target swaps, an empty one moves.
	// On a client this forwards to the server and returns false, so the return
	// value only tells us the request was issued.
	InventoryManager->SwapItems(FromSlotIndex, ToSlotIndex);
	return true;
}

bool UPHInventoryMenuPanelWidget::RequestUnequipToInventory(EEquipmentSlot EquipmentSlot)
{
	if (!EquipmentManager
		|| EquipmentSlot == EEquipmentSlot::ES_None
		|| !EquipmentManager->IsSlotOccupied(EquipmentSlot))
	{
		return false;
	}

	// The item goes to the first free slot rather than the exact slot dropped on:
	// placing it precisely would need a dedicated server path, and landing in the
	// bag at all is what matters here.
	EquipmentManager->UnequipItem(EquipmentSlot, /*bMoveToBag=*/true);
	return true;
}

bool UPHInventoryMenuPanelWidget::RequestDropInventorySlotToGround(int32 SlotIndex)
{
	if (!InventoryManager || SlotIndex == INDEX_NONE || !GetInventoryItem(SlotIndex))
	{
		return false;
	}

	InventoryManager->DropItemAtSlotToGround(SlotIndex);

	if (SelectedInventorySlotIndex == SlotIndex)
	{
		ClearSelection();
	}

	return true;
}

void UPHInventoryMenuPanelWidget::NativeInitializeForCharacter(APHBaseCharacter* Character)
{
	Super::NativeInitializeForCharacter(Character);

	EquipmentManager = Character ? Character->GetEquipmentManager() : nullptr;
	InventoryManager = Character ? Character->FindComponentByClass<UInventoryManager>() : nullptr;

	BindManagerDelegates();
	RefreshInventoryData();
}

void UPHInventoryMenuPanelWidget::NativeReleaseCharacter()
{
	UnbindManagerDelegates();
	ClearSelection();

	EquipmentManager = nullptr;
	InventoryManager = nullptr;
	InventorySlots.Reset();
	InventorySlotWidgets.Reset();
	CurrentCarryWeight = 0.0f;
	MaxCarryWeight = 0.0f;
	OccupiedInventorySlots = 0;
	MaxInventorySlots = 0;

	Super::NativeReleaseCharacter();
}

void UPHInventoryMenuPanelWidget::BindManagerDelegates()
{
	if (EquipmentManager)
	{
		EquipmentManager->OnEquipmentChanged.AddUniqueDynamic(this, &UPHInventoryMenuPanelWidget::HandleEquipmentChanged);
	}

	if (InventoryManager)
	{
		InventoryManager->OnInventoryChanged.AddUniqueDynamic(this, &UPHInventoryMenuPanelWidget::HandleInventoryChanged);
		InventoryManager->OnWeightChanged.AddUniqueDynamic(this, &UPHInventoryMenuPanelWidget::HandleCarryWeightChanged);
	}
}

void UPHInventoryMenuPanelWidget::UnbindManagerDelegates()
{
	if (EquipmentManager)
	{
		EquipmentManager->OnEquipmentChanged.RemoveDynamic(this, &UPHInventoryMenuPanelWidget::HandleEquipmentChanged);
	}

	if (InventoryManager)
	{
		InventoryManager->OnInventoryChanged.RemoveDynamic(this, &UPHInventoryMenuPanelWidget::HandleInventoryChanged);
		InventoryManager->OnWeightChanged.RemoveDynamic(this, &UPHInventoryMenuPanelWidget::HandleCarryWeightChanged);
	}
}

void UPHInventoryMenuPanelWidget::RebuildInventorySlots()
{
	InventorySlots.Reset();

	if (!InventoryManager)
	{
		// Either the panel has not been initialised for a character yet, or the
		// character genuinely has no UInventoryManager component.
		PH_LOG_WARNING(LogPHMenu,
			"%s: no UInventoryManager on the bound character - the inventory grid "
			"will stay empty. APHBaseCharacter creates one as 'InventoryComponent'; "
			"check it was not removed in the character Blueprint.",
			*GetName());
		return;
	}

	const int32 SlotCount = InventoryManager->GetSlotCount();
	UE_LOG(LogPHMenu, Log, TEXT("%s: building %d inventory slots."), *GetName(), SlotCount);
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

void UPHInventoryMenuPanelWidget::UpdateInventorySummary()
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
	MaxCarryWeight = InventoryManager->GetMaxWeight();
	OccupiedInventorySlots = InventoryManager->GetItemCount();
	MaxInventorySlots = InventoryManager->GetMaxSlots();
}

EEquipmentSlot UPHInventoryMenuPanelWidget::ResolveSuggestedSlot(UItemInstance* Item) const
{
	if (!EquipmentManager || !Item || !Item->CanBeEquipped())
	{
		return EEquipmentSlot::ES_None;
	}

	EEquipmentSlot SuggestedSlot = EquipmentManager->DetermineEquipmentSlot(Item);
	if (SuggestedSlot != EEquipmentSlot::ES_None && EquipmentManager->CanEquipToSlot(Item, SuggestedSlot))
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

void UPHInventoryMenuPanelWidget::SetSelection(UItemInstance* Item, int32 InventorySlotIndex, EEquipmentSlot SuggestedEquipmentSlot)
{
	if (SelectedItem == Item
		&& SelectedInventorySlotIndex == InventorySlotIndex
		&& SelectedSuggestedEquipmentSlot == SuggestedEquipmentSlot)
	{
		return;
	}

	SelectedItem = Item;
	SelectedInventorySlotIndex = InventorySlotIndex;
	SelectedSuggestedEquipmentSlot = SuggestedEquipmentSlot;

	OnInventorySelectionChanged(SelectedItem, SelectedInventorySlotIndex, SelectedSuggestedEquipmentSlot);
	InventorySelectionChanged.Broadcast(SelectedItem, SelectedInventorySlotIndex, SelectedSuggestedEquipmentSlot);
}

void UPHInventoryMenuPanelWidget::HandleEquipmentChanged(EEquipmentSlot, UItemInstance*, UItemInstance*)
{
	RefreshInventoryData();
}

void UPHInventoryMenuPanelWidget::HandleInventoryChanged()
{
	RefreshInventoryData();
	OnInventoryChanged();
}

void UPHInventoryMenuPanelWidget::HandleCarryWeightChanged(float NewCurrentWeight, float NewMaxWeight)
{
	CurrentCarryWeight = NewCurrentWeight;
	MaxCarryWeight = NewMaxWeight;

	OnCarryWeightChanged(CurrentCarryWeight, MaxCarryWeight);
	InventoryCarryWeightChanged.Broadcast(CurrentCarryWeight, MaxCarryWeight);
}
