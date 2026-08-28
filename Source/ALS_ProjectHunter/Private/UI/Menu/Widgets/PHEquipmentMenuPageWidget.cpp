#include "UI/Menu/Widgets/PHEquipmentMenuPageWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Widget.h"
#include "Equipment/Components/EquipmentManager.h"
#include "Inventory/Components/InventoryManager.h"
#include "Item/ItemInstance.h"
#include "Core/Logging/ProjectHunterLogMacros.h"
#include "UI/Menu/DragDrop/PHItemDragDropOperation.h"
#include "UI/Menu/Helpers/MenuInventoryGridBuilder.h"
#include "UI/Menu/Library/FunctionLibraries/MenuFunctionLibrary.h"
#include "UI/Menu/Library/MenuLog.h"
#include "UI/Menu/Widgets/PHInventorySlotWidget.h"
#include "UI/Menu/Widgets/PHEquipmentMenuPanelWidget.h"
#include "UI/Menu/Widgets/PHEquipmentSlotWidget.h"
#include "UI/Menu/Widgets/PHInventoryMenuPanelWidget.h"

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

	// The panel resolves its own InventoryManager inside InitializeForCharacter,
	// which the base class only runs AFTER this function returns. Without this,
	// the RefreshMenuData() below drives the panel while its manager is still
	// null - it builds zero cells and clears the grid container.
	// InitializeForCharacter is idempotent, so the later child pass is a no-op.
	if (InventoryPanel)
	{
		InventoryPanel->InitializeForCharacter(Character);
	}

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
		// No dedicated panel: this page owns the inventory grid itself. Before,
		// this path built the slot data and then rendered nothing at all.
		RebuildInventorySlots();
		UpdateInventorySummary();
		RebuildInventorySlotWidgets();
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
	// There is no two-hand slot in the menu: both hands read the two-handed
	// weapon that fills them.
	return EquipmentManager ? EquipmentManager->GetEquippedItem(ResolveOccupyingSlot(EquipmentSlot)) : nullptr;
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
	EquipmentSlot = ResolveOccupyingSlot(EquipmentSlot);

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

void UPHEquipmentMenuPageWidget::RebuildInventorySlotWidgets()
{
	// The panel owns its own cells when one exists.
	if (InventoryPanel || !bAutoBuildInventorySlotWidgets)
	{
		return;
	}

	FMenuInventoryGridBuilder::Rebuild(
		*this,
		TScriptInterface<IPHInventorySlotHost>(this),
		InventorySlotContainer,
		InventorySlotWidgetClass,
		GridColumns,
		InventoryCellSize,
		InventorySlots,
		InventorySlotWidgets);
}

bool UPHEquipmentMenuPageWidget::CanAcceptDroppedItem(
	UPHItemDragDropOperation* Operation,
	int32 TargetSlotIndex) const
{
	if (!Operation || !Operation->IsValidDrag() || TargetSlotIndex == INDEX_NONE)
	{
		return false;
	}

	if (Operation->IsFromInventory())
	{
		return InventoryManager != nullptr && !Operation->IsSameInventorySlot(TargetSlotIndex);
	}

	if (Operation->IsFromEquipment())
	{
		return EquipmentManager != nullptr
			&& Operation->SourceEquipmentSlot != EEquipmentSlot::ES_None;
	}

	return false;
}

bool UPHEquipmentMenuPageWidget::HandleItemDroppedOnSlot(
	UPHItemDragDropOperation* Operation,
	int32 TargetSlotIndex)
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

bool UPHEquipmentMenuPageWidget::RequestMoveInventoryItem(int32 FromSlotIndex, int32 ToSlotIndex)
{
	if (!InventoryManager
		|| FromSlotIndex == INDEX_NONE
		|| ToSlotIndex == INDEX_NONE
		|| FromSlotIndex == ToSlotIndex)
	{
		return false;
	}

	// SwapItems covers both cases: an occupied target swaps, an empty one moves.
	InventoryManager->SwapItems(FromSlotIndex, ToSlotIndex);
	return true;
}

bool UPHEquipmentMenuPageWidget::RequestUnequipToInventory(EEquipmentSlot EquipmentSlot)
{
	EquipmentSlot = ResolveOccupyingSlot(EquipmentSlot);

	if (!EquipmentManager
		|| EquipmentSlot == EEquipmentSlot::ES_None
		|| !EquipmentManager->IsSlotOccupied(EquipmentSlot))
	{
		return false;
	}

	EquipmentManager->UnequipItem(EquipmentSlot, /*bMoveToBag=*/true);
	return true;
}

bool UPHEquipmentMenuPageWidget::RequestDropInventorySlotToGround(int32 SlotIndex)
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
		if (!EquipmentPanel)
		{
			EquipmentPanel = Cast<UPHEquipmentMenuPanelWidget>(ChildWidget);
			if (EquipmentPanel)
			{
				EquipmentPanel->SetOwningEquipmentPage(this);
				EquipmentPanel->SetEquipmentSlotOrder(EquipmentSlotOrder);
				continue;
			}
		}

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

	// A blank inventory grid is otherwise silent, so state which mode we are in.
	if (InventoryPanel)
	{
		UE_LOG(LogPHMenu, Log, TEXT("%s: inventory grid owned by panel '%s'."),
			*GetName(), *InventoryPanel->GetName());
	}
	else if (InventorySlotContainer)
	{
		UE_LOG(LogPHMenu, Log, TEXT("%s: no InventoryPanel - hosting the grid directly."),
			*GetName());
	}
	else
	{
		PH_LOG_WARNING(LogPHMenu,
			"%s: no inventory grid will render. Add either a UPHInventoryMenuPanelWidget "
			"child, or a panel named 'InventorySlotContainer' plus an "
			"InventorySlotWidgetClass in this page's Blueprint defaults.",
			*GetName());
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
	if (EquipmentPanel)
	{
		EquipmentPanel->SetOwningEquipmentPage(this);
		EquipmentPanel->SetEquipmentSlotOrder(EquipmentSlotOrder);
		EquipmentPanel->RefreshEquipmentSlotWidgets();
	}

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

		UItemInstance* Item = GetEquippedItem(EquipmentSlot);
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

	// The suggestion has to name a slot the menu shows. A two-handed weapon is
	// stored in ES_TwoHand, which is not one of them, so it falls through to the
	// loop below and suggests the main hand it fills.
	const EEquipmentSlot SuggestedSlot = EquipmentManager->DetermineEquipmentSlot(Item);
	if (SuggestedSlot != EEquipmentSlot::ES_None &&
		EquipmentSlotOrder.Contains(SuggestedSlot) &&
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

EEquipmentSlot UPHEquipmentMenuPageWidget::ResolveOccupyingSlot(EEquipmentSlot EquipmentSlot) const
{
	return EquipmentManager ? EquipmentManager->ResolveOccupyingSlot(EquipmentSlot) : EquipmentSlot;
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
