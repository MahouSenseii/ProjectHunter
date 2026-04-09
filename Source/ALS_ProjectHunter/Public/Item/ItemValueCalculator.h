#pragma once

#include "CoreMinimal.h"

class UItemInstance;

class ALS_PROJECTHUNTER_API FItemValueCalculator
{
public:
	static int32 GetCalculatedValue(const UItemInstance& Item);
	static int32 GetSellValue(const UItemInstance& Item, float SellPercentage = 0.5f);
};
