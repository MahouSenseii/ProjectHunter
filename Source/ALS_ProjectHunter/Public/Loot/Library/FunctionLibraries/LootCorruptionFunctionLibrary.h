#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Loot/Library/Enums/LootEnums.h"
#include "LootCorruptionFunctionLibrary.generated.h"

UCLASS()
class ALS_PROJECTHUNTER_API ULootCorruptionFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Loot|Corruption")
	static FText GetCorruptionTypeName(ECorruptionType Type);

	UFUNCTION(BlueprintPure, Category = "Loot|Corruption")
	static FLinearColor GetCorruptionTypeColor(ECorruptionType Type);

	UFUNCTION(BlueprintPure, Category = "Loot|Corruption")
	static float GetCorruptionSeverity(ECorruptionType Type);
};
