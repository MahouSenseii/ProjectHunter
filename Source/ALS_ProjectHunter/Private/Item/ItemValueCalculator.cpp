#include "Item/ItemValueCalculator.h"

#include "Item/ItemInstance.h"
#include "Item/Library/FunctionLibraries/ItemCalculationFunctionLibrary.h"

int32 FItemValueCalculator::GetCalculatedValue(const UItemInstance& Item)
{
	FItemBase* Base = Item.GetBaseData();
	if (!Base)
	{
		return 0;
	}

	float Value = Base->Value;

	if (Item.IsStackable())
	{
		Value *= Item.Quantity;
	}

	if (Item.IsEquipment())
	{
		Value += Item.Stats.GetTotalAffixValue() * 10.0f;
		Value *= UItemCalculationFunctionLibrary::GetRarityValueMultiplier(Item.Rarity);

		if (Item.bHasCorruptedAffixes)
		{
			const float CorruptionPenalty = FMath::Clamp(
				FMath::Abs(Item.TotalCorruptionPoints) * 0.05f,
				0.0f,
				0.5f);
			Value *= (1.0f - CorruptionPenalty);
		}

		if (Item.IsBroken())
		{
			Value *= 0.1f;
		}
	}

	Value *= (1.0f + Item.ValueModifier);

	if (Item.IsConsumable())
	{
		FItemBase* BaseData = Item.GetBaseData();
		if (BaseData && BaseData->ConsumableData.MaxUses > 1)
		{
			Value *= static_cast<float>(Item.RemainingUses) / static_cast<float>(BaseData->ConsumableData.MaxUses);
		}
	}

	return FMath::Max(0, FMath::RoundToInt(Value));
}

int32 FItemValueCalculator::GetSellValue(const UItemInstance& Item, float SellPercentage)
{
	return FMath::RoundToInt(GetCalculatedValue(Item) * FMath::Clamp(SellPercentage, 0.0f, 1.0f));
}
