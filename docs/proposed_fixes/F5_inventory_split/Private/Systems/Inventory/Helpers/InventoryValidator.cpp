// Private/Systems/Inventory/Helpers/InventoryValidator.cpp
#include "Systems/Inventory/Helpers/InventoryValidator.h"

#include "Item/ItemInstance.h"
#include "Systems/Inventory/Components/InventoryManager.h"
#include "Systems/Inventory/Helpers/InventoryWeightCalculator.h"
#include "Systems/Inventory/Library/InventoryFunctionLibrary.h"

bool FInventoryValidator::IsItemValid(const UItemInstance* Item)
{
	return Item != nullptr && Item->HasValidBaseData();
}

bool FInventoryValidator::IsSlotIndexInRange(int32 SlotIndex, int32 MaxSlots)
{
	return SlotIndex >= 0 && SlotIndex < MaxSlots;
}

bool FInventoryValidator::CanAddItem(const UInventoryManager* Inventory, UItemInstance* Item)
{
	if (!Inventory || !IsItemValid(Item))
	{
		return false;
	}

	// Weight check (delegated to weight helper for single source of truth)
	if (FInventoryWeightCalculator::WouldExceedWeight(Inventory, Item))
	{
		return false;
	}

	// Stackable shortcut: if it can stack onto an existing item, slot count
	// doesn't matter.
	if (Inventory->bAutoStack && Item->IsStackable())
	{
		if (UInventoryFunctionLibrary::FindStackableItem(Inventory->Items, Item))
		{
			return true;
		}
	}

	// Slot availability
	return Inventory->GetAvailableSlots() > 0;
}
