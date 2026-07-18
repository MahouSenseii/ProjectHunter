#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Loot/Library/Structs/LootStructs.h"
#include "LootSettingsFunctionLibrary.generated.h"

UCLASS()
class ALS_PROJECTHUNTER_API ULootSettingsFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Loot|Settings")
	static FLootDropSettings BuildSettingsFromRequest(const FLootSourceEntry& Source, const FLootRequest& Request);

	UFUNCTION(BlueprintPure, Category = "Loot|Settings")
	static FLootDropSettings ApplyGlobalDropChanceMultiplier(const FLootDropSettings& Settings, float GlobalDropChanceMultiplier);

	UFUNCTION(BlueprintPure, Category = "Loot|Settings")
	static FLootDropSettings ApplyPlayerDropModifiers(const FLootDropSettings& Settings, float Luck, float MagicFind);
};
