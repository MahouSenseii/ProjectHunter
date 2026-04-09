// Private/Systems/Inventory/Helpers/InventoryAdder.cpp
#include "Systems/Inventory/Helpers/InventoryAdder.h"

#include "Core/Logging/ProjectHunterLogMacros.h"
#include "Item/ItemInstance.h"
#include "Systems/Inventory/Components/InventoryManager.h"
#include "Systems/Inventory/Helpers/InventoryRemover.h"
#include "Systems/Inventory/Helpers/InventoryValidator.h"
#include "Systems/Inventory/Library/InventoryFunctionLibrary.h"
#include "Systems/Inventory/Library/InventoryLog.h"

bool FInventoryAdder::AddItem(UInventoryManager* Inventory, UItemInstance* Item)
{
	if (!Inventory)
	{
		PH_LOG_WARNING(LogInventoryManager, "AddItem failed: Inventory was null.");
		return false;
	}
	if (!FInventoryValidator::IsItemValid(Item))
	{
		PH_LOG_WARNING(LogInventoryManager, "AddItem failed: Item was invalid.");
		return false;
	}
	if (!FInventoryValidator::CanAddItem(Inventory, Item))
	{
		PH_LOG_WARNING(LogInventoryManager,
			"AddItem failed: Cannot add %s because the inventory is full or overweight.",
			*Item->GetDisplayName().ToString());
		return false;
	}

	// Try to stack with existing items first
	if (Inventory->bAutoStack && Item->IsStackable())
	{
		if (TryStackItem(Inventory, Item))
		{
			UE_LOG(LogInventoryManager, Log, TEXT("InventoryManager: Stacked %s"),
				*Item->GetDisplayName().ToString());
			Inventory->BroadcastInventoryChanged();
			return true;
		}
	}

	const int32 EmptySlot = Inventory->FindFirstEmptySlot();
	if (EmptySlot == INDEX_NONE)
	{
		PH_LOG_WARNING(LogInventoryManager,
			"AddItem failed: No empty slots were available for Item=%s.",
			*Item->GetDisplayName().ToString());
		return false;
	}

	return AddItemToSlot(Inventory, Item, EmptySlot);
}

bool FInventoryAdder::AddItemToSlot(UInventoryManager* Inventory, UItemInstance* Item, int32 SlotIndex)
{
	if (!Inventory || !FInventoryValidator::IsItemValid(Item))
	{
		return false;
	}
	if (!FInventoryValidator::IsSlotIndexInRange(SlotIndex, Inventory->MaxSlots))
	{
		PH_LOG_WARNING(LogInventoryManager,
			"AddItemToSlot failed: SlotIndex=%d was outside [0, %d).",
			SlotIndex, Inventory->MaxSlots);
		return false;
	}

	// Ensure array is large enough
	while (Inventory->Items.Num() <= SlotIndex)
	{
		Inventory->Items.Add(nullptr);
	}

	if (Inventory->Items[SlotIndex] != nullptr)
	{
		PH_LOG_WARNING(LogInventoryManager,
			"AddItemToSlot failed: SlotIndex=%d is already occupied.", SlotIndex);
		return false;
	}

	Inventory->Items[SlotIndex] = Item;

	Inventory->OnItemAdded.Broadcast(Item);
	Inventory->BroadcastInventoryChanged();
	Inventory->BroadcastWeightChanged();

	UE_LOG(LogInventoryManager, Log, TEXT("InventoryManager: Added %s to slot %d"),
		*Item->GetDisplayName().ToString(), SlotIndex);

	return true;
}

bool FInventoryAdder::TryStackItem(UInventoryManager* Inventory, UItemInstance* Item)
{
	if (!Inventory || !Item || !Item->IsStackable())
	{
		return false;
	}

	UItemInstance* StackTarget = UInventoryFunctionLibrary::FindStackableItem(Inventory->Items, Item);
	if (!StackTarget)
	{
		return false;
	}

	const int32 Overflow = StackTarget->AddToStack(Item->Quantity);
	if (Overflow > 0)
	{
		// Partial stack — update source quantity, return false (caller will
		// place the remainder in a fresh slot).
		Item->Quantity = Overflow;
		Item->UpdateTotalWeight();
		return false;
	}

	// Fully stacked
	return true;
}

bool FInventoryAdder::StackItems(UInventoryManager* Inventory, UItemInstance* SourceItem, UItemInstance* TargetItem)
{
	if (!Inventory || !SourceItem || !TargetItem || !SourceItem->CanStackWith(TargetItem))
	{
		return false;
	}

	const int32 Overflow = TargetItem->AddToStack(SourceItem->Quantity);
	if (Overflow > 0)
	{
		// Partial stack
		SourceItem->Quantity = Overflow;
		SourceItem->UpdateTotalWeight();
	}
	else
	{
		// Fully stacked — remove the source via the Remover helper
		FInventoryRemover::RemoveItem(Inventory, SourceItem);
	}

	Inventory->BroadcastInventoryChanged();
	Inventory->BroadcastWeightChanged();
	return true;
}

UItemInstance* FInventoryAdder::SplitStack(UInventoryManager* Inventory, UItemInstance* Item, int32 Amount)
{
	if (!Inventory || !Item || Amount <= 0)
	{
		return nullptr;
	}

	UItemInstance* NewItem = Item->SplitStack(Amount);
	if (!NewItem)
	{
		return nullptr;
	}

	if (!AddItem(Inventory, NewItem))
	{
		// Failed to add — merge back into the source so we don't lose stack.
		Item->AddToStack(NewItem->Quantity);
		return nullptr;
	}

	Inventory->BroadcastInventoryChanged();
	Inventory->BroadcastWeightChanged();
	return NewItem;
}
