#include "Inventory/Helpers/InventorySwapper.h"

#include "Core/Logging/ProjectHunterLogMacros.h"
#include "Inventory/Components/InventoryManager.h"
#include "Inventory/Library/InventoryLog.h"

bool FInventorySwapper::SwapItems(UInventoryManager& Manager, int32 SlotA, int32 SlotB)
{
	if (SlotA == SlotB)
	{
		return false;
	}

	// Bound against MaxSlots, not Items.Num(). Items grows lazily (see
	// FInventoryAdder::AddItemToSlot), so with 3 items held it has 3 entries
	// while the menu shows MaxSlots cells - an IsValidIndex check here made
	// every drag onto an empty slot past the last occupied one fail silently.
	if (SlotA < 0 || SlotA >= Manager.MaxSlots || SlotB < 0 || SlotB >= Manager.MaxSlots)
	{
		PH_LOG_WARNING(LogInventoryManager, "SwapItems failed: slots %d/%d outside [0, %d).",
			SlotA, SlotB, Manager.MaxSlots);
		return false;
	}

	const int32 HighestSlot = FMath::Max(SlotA, SlotB);
	while (Manager.Items.Num() <= HighestSlot)
	{
		Manager.Items.Add(nullptr);
	}

	if (!Manager.Items[SlotA] && !Manager.Items[SlotB])
	{
		// Two empty slots - nothing to move, and broadcasting would churn the UI.
		return false;
	}

	UItemInstance* Temp = Manager.Items[SlotA];
	Manager.Items[SlotA] = Manager.Items[SlotB];
	Manager.Items[SlotB] = Temp;

	Manager.BroadcastInventoryChanged();

	UE_LOG(LogInventoryManager, Log, TEXT("InventoryManager: Swapped slots %d and %d"), SlotA, SlotB);
	return true;
}

