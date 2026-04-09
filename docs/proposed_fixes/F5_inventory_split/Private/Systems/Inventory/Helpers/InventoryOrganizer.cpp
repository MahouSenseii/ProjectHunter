// Private/Systems/Inventory/Helpers/InventoryOrganizer.cpp
#include "Systems/Inventory/Helpers/InventoryOrganizer.h"

#include "Item/ItemInstance.h"
#include "Systems/Inventory/Components/InventoryManager.h"
#include "Systems/Inventory/Library/InventoryFunctionLibrary.h"
#include "Systems/Inventory/Library/InventoryLog.h"

bool FInventoryOrganizer::SwapItems(UInventoryManager* Inventory, int32 SlotA, int32 SlotB)
{
	if (!Inventory || !Inventory->Items.IsValidIndex(SlotA) || !Inventory->Items.IsValidIndex(SlotB))
	{
		return false;
	}

	UItemInstance* Temp = Inventory->Items[SlotA];
	Inventory->Items[SlotA] = Inventory->Items[SlotB];
	Inventory->Items[SlotB] = Temp;

	Inventory->BroadcastInventoryChanged();

	UE_LOG(LogInventoryManager, Log, TEXT("InventoryManager: Swapped slots %d and %d"), SlotA, SlotB);
	return true;
}

void FInventoryOrganizer::SortInventory(UInventoryManager* Inventory, ESortMode SortMode)
{
	if (!Inventory)
	{
		return;
	}

	UInventoryFunctionLibrary::SortItems(Inventory->Items, SortMode, Inventory->MaxSlots);
	Inventory->BroadcastInventoryChanged();

	UE_LOG(LogInventoryManager, Log, TEXT("InventoryManager: Sorted inventory by %s"),
		*UEnum::GetValueAsString(SortMode));
}

void FInventoryOrganizer::CompactInventory(UInventoryManager* Inventory)
{
	if (!Inventory)
	{
		return;
	}

	UInventoryFunctionLibrary::CompactItems(Inventory->Items, Inventory->MaxSlots);
	Inventory->BroadcastInventoryChanged();

	UE_LOG(LogInventoryManager, Log, TEXT("InventoryManager: Compacted inventory (%d items)"),
		Inventory->GetItemCount());
}
