#include "Item/ItemValueCalculator.h"

#include "Item/ItemInstance.h"

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

		switch (Item.Rarity)
		{
		case EItemRarity::IR_GradeF: Value *= 1.0f; break;
		case EItemRarity::IR_GradeE: Value *= 1.5f; break;
		case EItemRarity::IR_GradeD: Value *= 2.5f; break;
		case EItemRarity::IR_GradeC: Value *= 5.0f; break;
		case EItemRarity::IR_GradeB: Value *= 10.0f; break;
		case EItemRarity::IR_GradeA: Value *= 25.0f; break;
		case EItemRarity::IR_GradeS: Value *= 100.0f; break;
		case EItemRarity::IR_GradeSS: Value *= 1000.0f; break;
		default: break;
		}

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
