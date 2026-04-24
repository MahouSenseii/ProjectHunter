#include "Systems/Inventory/InventoryStackHandler.h"

#include "Item/ItemInstance.h"
#include "Systems/Inventory/Components/InventoryManager.h"

bool FInventoryStackHandler::TryStackItem(UInventoryManager& Manager, UItemInstance* Item)
{
	if (!Item || !Item->IsStackable())
	{
		return false;
	}

	UItemInstance* StackTarget = Manager.FindStackableItem(Item);
	if (!StackTarget)
	{
		return false;
	}

	const int32 RemainingSpace = StackTarget->GetRemainingStackSpace();
	if (RemainingSpace <= 0)
	{
		return false;
	}

	if (Item->Quantity > RemainingSpace && Manager.FindFirstEmptySlot() == INDEX_NONE)
	{
		return false;
	}

	const int32 Overflow = StackTarget->AddToStack(Item->Quantity);
	if (Overflow > 0)
	{
		Item->Quantity = Overflow;
		Item->UpdateTotalWeight();
		return false;
	}

	return true;
}

bool FInventoryStackHandler::StackItems(UInventoryManager& Manager, UItemInstance* SourceItem, UItemInstance* TargetItem)
{
	if (!SourceItem || !TargetItem)
	{
		return false;
	}

	if (!SourceItem->CanStackWith(TargetItem))
	{
		return false;
	}

	const int32 Overflow = TargetItem->AddToStack(SourceItem->Quantity);
	if (Overflow > 0)
	{
		SourceItem->Quantity = Overflow;
		SourceItem->UpdateTotalWeight();
	}
	else
	{
		Manager.RemoveItem(SourceItem);
	}

	Manager.BroadcastInventoryChanged();
	Manager.UpdateWeight();
	return true;
}

UItemInstance* FInventoryStackHandler::SplitStack(UInventoryManager& Manager, UItemInstance* Item, int32 Amount)
{
	if (!Item || Amount <= 0)
	{
		return nullptr;
	}

	UItemInstance* NewItem = Item->SplitStack(Amount);
	if (!NewItem)
	{
		return nullptr;
	}

	if (!Manager.AddItem(NewItem))
	{
		Item->AddToStack(NewItem->Quantity);
		return nullptr;
	}

	Manager.BroadcastInventoryChanged();
	Manager.UpdateWeight();
	return NewItem;
}

