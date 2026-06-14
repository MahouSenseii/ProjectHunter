#include "Item/ItemNameBuilder.h"

#include "Item/ItemInstance.h"
#include "Item/Library/ItemFunctionLibrary.h"

FText FItemNameBuilder::GetDisplayName(UItemInstance& Item)
{
	if (Item.bHasNameBeenGenerated && !Item.bCacheDirty)
	{
		return Item.DisplayName;
	}

	FItemBase* Base = Item.GetBaseData();
	if (!Base)
	{
		return FText::FromString("Unknown Item");
	}

	if (Item.IsQuestItem() || Item.Rarity == EItemRarity::IR_GradeSS || (Item.IsEquipment() && Base->bIsUnique))
	{
		if (Item.Rarity == EItemRarity::IR_GradeSS)
		{
			Item.DisplayName = FText::Format(
				FText::FromString("[{0}]"),
				Base->ItemName);
		}
		else
		{
			Item.DisplayName = Base->ItemName;
		}

		Item.bHasNameBeenGenerated = true;
		Item.bCacheDirty = false;
		return Item.DisplayName;
	}

	if (!Item.IsEquipment())
	{
		Item.DisplayName = Base->ItemName;
		Item.bHasNameBeenGenerated = true;
		Item.bCacheDirty = false;
		return Item.DisplayName;
	}

	FString NamePrefix;
	if (Item.bHasCorruptedAffixes)
	{
		NamePrefix = TEXT("Corrupted ");
	}

	if (!Item.bIdentified)
	{
		Item.DisplayName = FText::Format(
			FText::FromString("Unidentified {0}{1}"),
			FText::FromString(NamePrefix),
			Base->ItemName);
	}
	else
	{
		// Route through the affix-aware composer (it was fully implemented but
		// previously had no callers):
		//   F/E        → base name
		//   D/C/B      → "<BestPrefix> <Base> <BestSuffix>" from rolled AffixNames
		//   A/S/SS     → "[Base]"
		// Falls back to the plain base name when no affix has an AffixName.
		Item.DisplayName = FText::Format(
			FText::FromString("{0}{1}"),
			FText::FromString(NamePrefix),
			UItemFunctionLibrary::GenerateItemName(Item.Stats, *Base, Item.Rarity));
	}

	Item.bHasNameBeenGenerated = true;
	Item.bCacheDirty = false;
	return Item.DisplayName;
}

void FItemNameBuilder::RegenerateDisplayName(UItemInstance& Item)
{
	Item.bHasNameBeenGenerated = false;
	Item.bCacheDirty = true;
	GetDisplayName(Item);
}

FText FItemNameBuilder::GenerateRareName(const UItemInstance& Item)
{
	FItemBase* Base = Item.GetBaseData();
	return Base ? Base->ItemName : FText::FromString("Legendary Item");
}
