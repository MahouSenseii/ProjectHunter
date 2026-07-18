#include "Loot/Library/FunctionLibraries/LootCalculationFunctionLibrary.h"

float ULootCalculationFunctionLibrary::ApplyLuckToDropChance(const float BaseChance, const float Luck)
{
	const float LuckBonus = Luck / (Luck + 500.0f);
	return FMath::Clamp(BaseChance * (1.0f + LuckBonus), 0.0f, 1.0f);
}

int32 ULootCalculationFunctionLibrary::ApplyMagicFindToQuantity(const int32 BaseQuantity, const float MagicFind)
{
	const float Multiplier = 1.0f + (MagicFind / 200.0f);
	return FMath::Max(1, FMath::RoundToInt(BaseQuantity * Multiplier));
}
