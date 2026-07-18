#include "Item/Library/FunctionLibraries/ItemComparisonFunctionLibrary.h"

#include "Item/ItemInstance.h"

int32 UItemComparisonFunctionLibrary::CompareItemDamage(const FItemBase& ItemA, const FItemBase& ItemB)
{
	const float DamageA = ItemA.WeaponStats.MinPhysicalDamage + ItemA.WeaponStats.MaxPhysicalDamage;
	const float DamageB = ItemB.WeaponStats.MinPhysicalDamage + ItemB.WeaponStats.MaxPhysicalDamage;

	if (DamageA < DamageB)
	{
		return -1;
	}

	if (DamageA > DamageB)
	{
		return 1;
	}

	return 0;
}

int32 UItemComparisonFunctionLibrary::CompareItemValue(const FItemBase& ItemA, const FItemBase& ItemB)
{
	if (ItemA.Value < ItemB.Value)
	{
		return -1;
	}

	if (ItemA.Value > ItemB.Value)
	{
		return 1;
	}

	return 0;
}

int32 UItemComparisonFunctionLibrary::CompareItemInstanceValue(const UItemInstance* ItemA, const UItemInstance* ItemB)
{
	if (!ItemA || !ItemB)
	{
		return 0;
	}

	const int32 ValueA = ItemA->GetCalculatedValue();
	const int32 ValueB = ItemB->GetCalculatedValue();

	if (ValueA < ValueB)
	{
		return -1;
	}

	if (ValueA > ValueB)
	{
		return 1;
	}

	return 0;
}

int32 UItemComparisonFunctionLibrary::CompareItemInstanceRarity(const UItemInstance* ItemA, const UItemInstance* ItemB)
{
	if (!ItemA || !ItemB)
	{
		return 0;
	}

	const uint8 RarityA = static_cast<uint8>(ItemA->Rarity);
	const uint8 RarityB = static_cast<uint8>(ItemB->Rarity);

	if (RarityA < RarityB)
	{
		return -1;
	}

	if (RarityA > RarityB)
	{
		return 1;
	}

	return 0;
}

int32 UItemComparisonFunctionLibrary::CompareItemInstanceWeight(const UItemInstance* ItemA, const UItemInstance* ItemB)
{
	if (!ItemA || !ItemB)
	{
		return 0;
	}

	const float WeightA = ItemA->GetTotalWeight();
	const float WeightB = ItemB->GetTotalWeight();

	if (WeightA < WeightB)
	{
		return -1;
	}

	if (WeightA > WeightB)
	{
		return 1;
	}

	return 0;
}
