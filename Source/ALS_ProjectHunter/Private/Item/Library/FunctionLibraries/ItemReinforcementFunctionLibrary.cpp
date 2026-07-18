#include "Item/Library/FunctionLibraries/ItemReinforcementFunctionLibrary.h"

float UItemReinforcementFunctionLibrary::CalculateReinforcementMultiplier(const int32 ReinforcementLevel)
{
	if (ReinforcementLevel <= 0)
	{
		return 1.0f;
	}

	const float NormalizedLevel = FMath::LogX(10.0f, 1.0f + static_cast<float>(ReinforcementLevel)) / FMath::LogX(10.0f, 1000.0f);
	return 1.0f + NormalizedLevel * 0.07f;
}
