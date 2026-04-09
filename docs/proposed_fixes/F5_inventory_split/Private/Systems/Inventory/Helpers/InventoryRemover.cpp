// Private/Systems/Inventory/Helpers/InventoryRemover.cpp
#include "Systems/Inventory/Helpers/InventoryRemover.h"

#include "Core/Logging/ProjectHunterLogMacros.h"
#include "Item/ItemInstance.h"
#include "Systems/Inventory/Components/InventoryManager.h"
#include "Systems/Inventory/Library/InventoryLog.h"

bool FInventoryRemover::RemoveItem(UInventoryManager* Inventory, UItemInstance* Item)
{
	if (!Inventory || !Item)
	{
		return false;
	}

	const int32 SlotIndex = Inventory->FindSlotForItem(Item);
	if (SlotIndex == INDEX_NONE)
	{
		return false;
	}
	return RemoveItemAtSlot(Inventory, SlotIndex) != nullptr;
}

UItemInstance* FInventoryRemover::RemoveItemAtSlot(UInventoryManager* Inventory, int32 SlotIndex)
{
	if (!Inventory || !Inventory->Items.IsValidIndex(SlotIndex))
	{
		return nullptr;
	}

	UItemInstance* Item = Inventory->Items[SlotIndex];
	if (!Item)
	{
		return nullptr;
	}

	Inventory->Items[SlotIndex] = nullptr;

	Inventory->OnItemRemoved.Broadcast(Item);
	Inventory->BroadcastInventoryChanged();
	Inventory->BroadcastWeightChanged();

	UE_LOG(LogInventoryManager, Log, TEXT("InventoryManager: Removed %s from slot %d"),
		*Item->GetDisplayName().ToString(), SlotIndex);

	return Item;
}

bool FInventoryRemover::RemoveQuantity(UInventoryManager* Inventory, UItemInstance* Item, int32 Quantity)
{
	if (!Inventory || !Item || Quantity <= 0)
	{
		return false;
	}

	const int32 ActualRemoved = Item->RemoveFromStack(Quantity);

	if (Item->IsConsumed())
	{
		RemoveItem(Inventory, Item);
	}
	else
	{
		Inventory->BroadcastInventoryChanged();
		Inventory->BroadcastWeightChanged();
	}

	return ActualRemoved > 0;
}

void FInventoryRemover::ClearAll(UInventoryManager* Inventory)
{
	if (!Inventory)
	{
		return;
	}

	Inventory->Items.Empty(Inventory->MaxSlots);

	Inventory->BroadcastInventoryChanged();
	Inventory->BroadcastWeightChanged();

	UE_LOG(LogInventoryManager, Log, TEXT("InventoryManager: Cleared all items"));
}

void FInventoryRemover::CleanupInvalidItems(UInventoryManager* Inventory)
{
	if (!Inventory)
	{
		return;
	}

	for (int32 i = Inventory->Items.Num() - 1; i >= 0; --i)
	{
		if (Inventory->Items[i] && !Inventory->Items[i]->HasValidBaseData())
		{
			Inventory->Items[i] = nullptr;
		}
	}
}
