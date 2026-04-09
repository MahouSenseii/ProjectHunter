#include "Item/ItemStackingHandler.h"

#include "Item/ItemInstance.h"

void FItemStackingHandler::UpdateTotalWeight(UItemInstance& Item)
{
	Item.TotalWeight = Item.GetBaseWeight() * Item.Quantity;
}

bool FItemStackingHandler::IsStackable(const UItemInstance& Item)
{
	FItemBase* Base = Item.GetBaseData();
	return Base ? Base->bStackable : false;
}

bool FItemStackingHandler::CanStackWith(const UItemInstance& Item, const UItemInstance* Other)
{
	if (!Other || !IsStackable(Item))
	{
		return false;
	}

	if (Item.BaseItemHandle != Other->BaseItemHandle)
	{
		return false;
	}

	if (Item.IsEquipment() && Item.Stats.bAffixesGenerated)
	{
		return false;
	}

	if (Item.IsConsumable() && Item.RemainingUses != Other->RemainingUses)
	{
		return false;
	}

	return true;
}

bool FItemStackingHandler::IsConsumed(const UItemInstance& Item)
{
	if (Item.IsConsumable())
	{
		return Item.RemainingUses <= 0 || Item.Quantity <= 0;
	}

	return Item.Quantity <= 0;
}

int32 FItemStackingHandler::AddToStack(UItemInstance& Item, int32 Amount)
{
	if (!IsStackable(Item) || Amount <= 0)
	{
		return Amount;
	}

	const int32 MaxStack = Item.GetMaxStackSize();
	const int32 Available = MaxStack - Item.Quantity;
	const int32 ToAdd = FMath::Min(Amount, Available);

	Item.Quantity += ToAdd;
	UpdateTotalWeight(Item);

	return Amount - ToAdd;
}

int32 FItemStackingHandler::RemoveFromStack(UItemInstance& Item, int32 Amount)
{
	if (Amount <= 0)
	{
		return 0;
	}

	const int32 ToRemove = FMath::Min(Amount, Item.Quantity);
	Item.Quantity -= ToRemove;
	UpdateTotalWeight(Item);

	return ToRemove;
}

UItemInstance* FItemStackingHandler::SplitStack(UItemInstance& Item, int32 Amount)
{
	if (!IsStackable(Item) || Amount <= 0 || Amount >= Item.Quantity)
	{
		return nullptr;
	}

	UItemInstance* NewInstance = NewObject<UItemInstance>(Item.GetOuter(), Item.GetClass());
	NewInstance->BaseItemHandle = Item.BaseItemHandle;
	NewInstance->ItemLevel = Item.ItemLevel;
	NewInstance->Rarity = Item.Rarity;
	NewInstance->Quantity = Amount;
	NewInstance->RemainingUses = Item.RemainingUses;
	NewInstance->UniqueID = FGuid::NewGuid();
	NewInstance->bHasCorruptedAffixes = Item.bHasCorruptedAffixes;
	NewInstance->TotalCorruptionPoints = Item.TotalCorruptionPoints;
	NewInstance->bCanBeModified = Item.bCanBeModified;

	RemoveFromStack(Item, Amount);
	UpdateTotalWeight(*NewInstance);

	return NewInstance;
}

int32 FItemStackingHandler::GetRemainingStackSpace(const UItemInstance& Item)
{
	if (!IsStackable(Item))
	{
		return 0;
	}

	return FMath::Max(0, Item.GetMaxStackSize() - Item.Quantity);
}
