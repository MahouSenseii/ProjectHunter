#include "Systems/Inventory/InventoryValidator.h"

#include "Item/ItemInstance.h"
#include "Systems/Inventory/Components/InventoryManager.h"
#include "Systems/Inventory/InventoryWeightCalculator.h"
#include "Systems/Inventory/Library/InventoryFunctionLibrary.h"

bool FInventoryValidator::IsFull(const UInventoryManager& Manager)
{
	return FInventoryWeightCalculator::GetAvailableSlots(Manager) <= 0;
}

bool FInventoryValidator::IsOverweight(const UInventoryManager& Manager)
{
	return FInventoryWeightCalculator::GetTotalWeight(Manager) > Manager.MaxWeight;
}

bool FInventoryValidator::CanAddItem(const UInventoryManager& Manager, UItemInstance* Item)
{
	if (!Item || !Item->HasValidBaseData())
	{
		return false;
	}

	if (FInventoryWeightCalculator::WouldExceedWeight(Manager, Item))
	{
		return false;
	}

	if (Manager.bAutoStack && Item->IsStackable())
	{
		if (UInventoryFunctionLibrary::FindStackableItem(Manager.Items, Item))
		{
			return true;
		}
	}

	return FInventoryWeightCalculator::GetAvailableSlots(Manager) > 0;
}

bool FInventoryValidator::IsSlotEmpty(const UInventoryManager& Manager, int32 SlotIndex)
{
	if (!Manager.Items.IsValidIndex(SlotIndex))
	{
		return true;
	}

	return Manager.Items[SlotIndex] == nullptr;
}

