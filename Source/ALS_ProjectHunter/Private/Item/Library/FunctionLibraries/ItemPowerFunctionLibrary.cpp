#include "Item/Library/FunctionLibraries/ItemPowerFunctionLibrary.h"

#include "Item/ItemInstance.h"
#include "Item/Settings/ItemPowerSettings.h"

float UItemPowerFunctionLibrary::CalculateItemPower(const float BasePowerValue, const FPHItemStats& Stats)
{
	float Result = BasePowerValue;
	Stats.ForEachStat([&Result](const FPHAttributeData& Affix)
	{
		Result += Affix.PowerValue;
	});
	return FMath::Max(0.0f, Result);
}

EItemRarity UItemPowerFunctionLibrary::GetGradeForPower(const float ItemPower)
{
	const UItemPowerSettings* Settings = GetDefault<UItemPowerSettings>();
	if (ItemPower >= Settings->GradeSS) return EItemRarity::IR_GradeSS;
	if (ItemPower >= Settings->GradeS) return EItemRarity::IR_GradeS;
	if (ItemPower >= Settings->GradeA) return EItemRarity::IR_GradeA;
	if (ItemPower >= Settings->GradeB) return EItemRarity::IR_GradeB;
	if (ItemPower >= Settings->GradeC) return EItemRarity::IR_GradeC;
	if (ItemPower >= Settings->GradeD) return EItemRarity::IR_GradeD;
	if (ItemPower >= Settings->GradeE) return EItemRarity::IR_GradeE;
	return EItemRarity::IR_GradeF;
}

bool UItemPowerFunctionLibrary::RecalculateItemGrade(UItemInstance* Item)
{
	if (!IsValid(Item))
	{
		return false;
	}

	const FItemBase* Base = Item->GetBaseData();
	if (!Base)
	{
		return false;
	}

	Item->ItemPowerScore = CalculateItemPower(Base->BasePowerValue, Item->Stats);
	Item->Rarity = GetGradeForPower(Item->ItemPowerScore);
	Item->RegenerateDisplayName();
	return true;
}
