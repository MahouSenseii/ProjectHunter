#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Loot/Library/Structs/LootStructs.h"
#include "LootSourceFunctionLibrary.generated.h"

UCLASS()
class ALS_PROJECTHUNTER_API ULootSourceFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Loot|Display")
	static FText GetLootSourceTypeName(ELootSourceType Type);

	UFUNCTION(BlueprintPure, Category = "Loot|Settings")
	static FLootDropSettings GetDefaultSettingsForSourceType(ELootSourceType Type);
};
