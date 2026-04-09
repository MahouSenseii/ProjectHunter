#include "Systems/Inventory/InventoryAdder.h"

#include "Core/Logging/ProjectHunterLogMacros.h"
#include "Item/ItemInstance.h"
#include "Systems/Inventory/Components/InventoryManager.h"
#include "Systems/Inventory/InventoryStackHandler.h"
#include "Systems/Inventory/InventoryValidator.h"
#include "Systems/Inventory/Library/InventoryLog.h"

bool FInventoryAdder::AddItem(UInventoryManager& Manager, UItemInstance* Item)
{
	if (!Item || !Item->HasValidBaseData())
	{
		PH_LOG_WARNING(LogInventoryManager, "AddItem failed: Item was invalid.");
		return false;
	}

	if (!FInventoryValidator::CanAddItem(Manager, Item))
	{
		PH_LOG_WARNING(LogInventoryManager, "AddItem failed: Cannot add %s because the inventory is full or overweight.",
			*Item->GetDisplayName().ToString());
		return false;
	}

	if (Manager.bAutoStack && Item->IsStackable())
	{
		if (FInventoryStackHandler::TryStackItem(Manager, Item))
		{
			UE_LOG(LogInventoryManager, Log, TEXT("InventoryManager: Stacked %s"),
				*Item->GetDisplayName().ToString());

			Manager.BroadcastInventoryChanged();
			return true;
		}
	}

	const int32 EmptySlot = Manager.FindFirstEmptySlot();
	if (EmptySlot == INDEX_NONE)
	{
		PH_LOG_WARNING(LogInventoryManager, "AddItem failed: No empty slots were available for Item=%s.",
			*Item->GetDisplayName().ToString());
		return false;
	}

	return AddItemToSlot(Manager, Item, EmptySlot);
}

bool FInventoryAdder::AddItemToSlot(UInventoryManager& Manager, UItemInstance* Item, int32 SlotIndex)
{
	if (!Item || !Item->HasValidBaseData())
	{
		return false;
	}

	if (SlotIndex < 0 || SlotIndex >= Manager.MaxSlots)
	{
		PH_LOG_WARNING(LogInventoryManager, "AddItemToSlot failed: SlotIndex=%d was outside [0, %d).", SlotIndex, Manager.MaxSlots);
		return false;
	}

	while (Manager.Items.Num() <= SlotIndex)
	{
		Manager.Items.Add(nullptr);
	}

	if (Manager.Items[SlotIndex] != nullptr)
	{
		PH_LOG_WARNING(LogInventoryManager, "AddItemToSlot failed: SlotIndex=%d is already occupied.", SlotIndex);
		return false;
	}

	Manager.Items[SlotIndex] = Item;

	Manager.OnItemAdded.Broadcast(Item);
	Manager.BroadcastInventoryChanged();
	Manager.UpdateWeight();

	UE_LOG(LogInventoryManager, Log, TEXT("InventoryManager: Added %s to slot %d"),
		*Item->GetDisplayName().ToString(), SlotIndex);

	return true;
}

