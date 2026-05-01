#include "Inventory/InventoryRemover.h"

#include "Core/Logging/ProjectHunterLogMacros.h"
#include "Item/ItemInstance.h"
#include "Inventory/Components/InventoryManager.h"
#include "Inventory/InventoryGroundDropResolver.h"
#include "Inventory/Library/InventoryLog.h"

bool FInventoryRemover::RemoveItem(UInventoryManager& Manager, UItemInstance* Item)
{
	if (!Item)
	{
		return false;
	}

	const int32 SlotIndex = Manager.FindSlotForItem(Item);
	if (SlotIndex == INDEX_NONE)
	{
		return false;
	}

	return RemoveItemAtSlot(Manager, SlotIndex) != nullptr;
}

UItemInstance* FInventoryRemover::RemoveItemAtSlot(UInventoryManager& Manager, int32 SlotIndex)
{
	if (!Manager.Items.IsValidIndex(SlotIndex))
	{
		return nullptr;
	}

	UItemInstance* Item = Manager.Items[SlotIndex];
	if (!Item)
	{
		return nullptr;
	}

	Manager.Items[SlotIndex] = nullptr;

	Manager.OnItemRemoved.Broadcast(Item);
	Manager.BroadcastInventoryChanged();
	Manager.UpdateWeight();

	UE_LOG(LogInventoryManager, Log, TEXT("InventoryManager: Removed %s from slot %d"),
		*Item->GetDisplayName().ToString(), SlotIndex);

	return Item;
}

bool FInventoryRemover::RemoveQuantity(UInventoryManager& Manager, UItemInstance* Item, int32 Quantity)
{
	if (!Item || Quantity <= 0)
	{
		return false;
	}

	const int32 ActualRemoved = Item->RemoveFromStack(Quantity);
	if (Item->IsConsumed())
	{
		RemoveItem(Manager, Item);
	}
	else
	{
		Manager.BroadcastInventoryChanged();
		Manager.UpdateWeight();
	}

	return ActualRemoved > 0;
}

void FInventoryRemover::DropItem(UInventoryManager& Manager, UItemInstance* Item, FVector DropLocation)
{
	if (!Item)
	{
		return;
	}

	if (!RemoveItem(Manager, Item))
	{
		return;
	}

	UWorld* World = Manager.GetWorld();
	if (!World)
	{
		PH_LOG_WARNING(LogInventoryManager, "DropItem failed: World was null for Item=%s.",
			*Item->GetDisplayName().ToString());
		return;
	}

	if (FInventoryGroundDropResolver::DropItem(World, Item, DropLocation))
	{
		UE_LOG(LogInventoryManager, Log, TEXT("InventoryManager: Dropped %s at %s"),
			*Item->GetDisplayName().ToString(), *DropLocation.ToString());
	}
	else
	{
		PH_LOG_WARNING(LogInventoryManager, "DropItem failed: Ground drop helper rejected Item=%s at Location=%s.",
			*Item->GetDisplayName().ToString(), *DropLocation.ToString());
	}
}

void FInventoryRemover::DropItemAtSlot(UInventoryManager& Manager, int32 SlotIndex, FVector DropLocation)
{
	if (UItemInstance* Item = Manager.GetItemAtSlot(SlotIndex))
	{
		DropItem(Manager, Item, DropLocation);
	}
}

