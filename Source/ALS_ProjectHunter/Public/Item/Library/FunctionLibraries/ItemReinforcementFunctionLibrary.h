#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ItemReinforcementFunctionLibrary.generated.h"

UCLASS()
class ALS_PROJECTHUNTER_API UItemReinforcementFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Item|Reinforcement")
	static float CalculateReinforcementMultiplier(int32 ReinforcementLevel);
};
