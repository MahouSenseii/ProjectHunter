#include "Inventory/Library/InventoryFunctionLibrary.h"

#include "Item/ItemInstance.h"

namespace InventoryFunctionLibraryPrivate
{
	TArray<UItemInstance*> GetOccupiedItems(const TArray<UItemInstance*>& Items)
	{
		TArray<UItemInstance*> OccupiedItems;
		OccupiedItems.Reserve(Items.Num());

		for (UItemInstance* Item : Items)
		{
			if (Item)
			{
				OccupiedItems.Add(Item);
			}
		}

		return OccupiedItems;
	}
}

UItemInstance* UInventoryFunctionLibrary::FindStackableItem(const TArray<UItemInstance*>& Items, const UItemInstance* Item)
{
	if (!Item || !Item->IsStackable())
	{
		return nullptr;
	}

	for (UItemInstance* ExistingItem : Items)
	{
		if (ExistingItem && ExistingItem->CanStackWith(Item) && ExistingItem->GetRemainingStackSpace() > 0)
		{
			return ExistingItem;
		}
	}

	return nullptr;
}

int32 UInventoryFunctionLibrary::FindFirstEmptySlot(const TArray<UItemInstance*>& Items, int32 MaxSlots)
{
	for (int32 SlotIndex = 0; SlotIndex < MaxSlots; ++SlotIndex)
	{
		if (!Items.IsValidIndex(SlotIndex) || Items[SlotIndex] == nullptr)
		{
			return SlotIndex;
		}
	}

	return INDEX_NONE;
}

int32 UInventoryFunctionLibrary::FindSlotForItem(const TArray<UItemInstance*>& Items, const UItemInstance* Item)
{
	if (!Item)
	{
		return INDEX_NONE;
	}

	for (int32 SlotIndex = 0; SlotIndex < Items.Num(); ++SlotIndex)
	{
		if (Items[SlotIndex] == Item)
		{
			return SlotIndex;
		}
	}

	return INDEX_NONE;
}

TArray<UItemInstance*> UInventoryFunctionLibrary::FindItemsByBaseID(const TArray<UItemInstance*>& Items, FName BaseItemID)
{
	TArray<UItemInstance*> FoundItems;

	for (UItemInstance* Item : Items)
	{
		if (Item && Item->BaseItemHandle.RowName == BaseItemID)
		{
			FoundItems.Add(Item);
		}
	}

	return FoundItems;
}

TArray<UItemInstance*> UInventoryFunctionLibrary::FindItemsByType(const TArray<UItemInstance*>& Items, EItemType ItemType)
{
	TArray<UItemInstance*> FoundItems;

	for (UItemInstance* Item : Items)
	{
		if (Item && Item->GetItemType() == ItemType)
		{
			FoundItems.Add(Item);
		}
	}

	return FoundItems;
}

TArray<UItemInstance*> UInventoryFunctionLibrary::FindItemsByRarity(const TArray<UItemInstance*>& Items, EItemRarity Rarity)
{
	TArray<UItemInstance*> FoundItems;

	for (UItemInstance* Item : Items)
	{
		if (Item && Item->Rarity == Rarity)
		{
			FoundItems.Add(Item);
		}
	}

	return FoundItems;
}

bool UInventoryFunctionLibrary::HasItemWithID(const TArray<UItemInstance*>& Items, FGuid UniqueID)
{
	for (const UItemInstance* Item : Items)
	{
		if (Item && Item->UniqueID == UniqueID)
		{
			return true;
		}
	}

	return false;
}

int32 UInventoryFunctionLibrary::GetTotalQuantityOfItem(const TArray<UItemInstance*>& Items, FName BaseItemID)
{
	int32 TotalQuantity = 0;

	for (UItemInstance* Item : Items)
	{
		if (Item && Item->BaseItemHandle.RowName == BaseItemID)
		{
			TotalQuantity += Item->Quantity;
		}
	}

	return TotalQuantity;
}

void UInventoryFunctionLibrary::SortItems(TArray<UItemInstance*>& InOutItems, ESortMode SortMode, int32 MaxSlots)
{
	TArray<UItemInstance*> OccupiedItems = InventoryFunctionLibraryPrivate::GetOccupiedItems(InOutItems);

	switch (SortMode)
	{
	case ESortMode::SM_Type:
		OccupiedItems.Sort([](const UItemInstance& A, const UItemInstance& B)
		{
			return static_cast<uint8>(A.GetItemType()) < static_cast<uint8>(B.GetItemType());
		});
		break;

	case ESortMode::SM_Rarity:
		OccupiedItems.Sort([](const UItemInstance& A, const UItemInstance& B)
		{
			return static_cast<uint8>(A.Rarity) > static_cast<uint8>(B.Rarity);
		});
		break;

	case ESortMode::SM_Name:
		OccupiedItems.Sort([](const UItemInstance& A, const UItemInstance& B)
		{
			return A.GetBaseItemName().CompareTo(B.GetBaseItemName()) < 0;
		});
		break;

	case ESortMode::SM_Weight:
		OccupiedItems.Sort([](const UItemInstance& A, const UItemInstance& B)
		{
			return A.GetTotalWeight() < B.GetTotalWeight();
		});
		break;

	case ESortMode::SM_Value:
		OccupiedItems.Sort([](const UItemInstance& A, const UItemInstance& B)
		{
			return A.GetCalculatedValue() > B.GetCalculatedValue();
		});
		break;

	default:
		break;
	}

	InOutItems.Empty(MaxSlots);
	InOutItems.Append(OccupiedItems);

	while (InOutItems.Num() < MaxSlots)
	{
		InOutItems.Add(nullptr);
	}
}

void UInventoryFunctionLibrary::CompactItems(TArray<UItemInstance*>& InOutItems, int32 MaxSlots)
{
	TArray<UItemInstance*> OccupiedItems = InventoryFunctionLibraryPrivate::GetOccupiedItems(InOutItems);

	InOutItems.Empty(MaxSlots);
	InOutItems.Append(OccupiedItems);

	while (InOutItems.Num() < MaxSlots)
	{
		InOutItems.Add(nullptr);
	}
}
