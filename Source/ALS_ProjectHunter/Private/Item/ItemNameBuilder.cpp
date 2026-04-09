#include "Item/ItemNameBuilder.h"

#include "Item/ItemInstance.h"

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
		switch (Item.Rarity)
		{
		case EItemRarity::IR_GradeF:
		case EItemRarity::IR_GradeE:
		case EItemRarity::IR_GradeD:
		case EItemRarity::IR_GradeC:
		case EItemRarity::IR_GradeB:
			Item.DisplayName = FText::Format(
				FText::FromString("{0}{1}"),
				FText::FromString(NamePrefix),
				Base->ItemName);
			break;

		case EItemRarity::IR_GradeA:
		case EItemRarity::IR_GradeS:
			Item.DisplayName = FText::Format(
				FText::FromString("{0}{1}"),
				FText::FromString(NamePrefix),
				GenerateRareName(Item));
			break;

		default:
			Item.DisplayName = FText::Format(
				FText::FromString("{0}{1}"),
				FText::FromString(NamePrefix),
				Base->ItemName);
			break;
		}
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
