#include "Inventory/InventorySwapper.h"

#include "Inventory/Components/InventoryManager.h"
#include "Inventory/Library/InventoryLog.h"

bool FInventorySwapper::SwapItems(UInventoryManager& Manager, int32 SlotA, int32 SlotB)
{
	if (!Manager.Items.IsValidIndex(SlotA) || !Manager.Items.IsValidIndex(SlotB))
	{
		return false;
	}

	UItemInstance* Temp = Manager.Items[SlotA];
	Manager.Items[SlotA] = Manager.Items[SlotB];
	Manager.Items[SlotB] = Temp;

	Manager.BroadcastInventoryChanged();

	UE_LOG(LogInventoryManager, Log, TEXT("InventoryManager: Swapped slots %d and %d"), SlotA, SlotB);
	return true;
}

