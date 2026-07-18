#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Item/Library/Enums/ItemEnums.h"
#include "Loot/Library/Enums/LootEnums.h"
#include "LootRarityFunctionLibrary.generated.h"

UCLASS()
class ALS_PROJECTHUNTER_API ULootRarityFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Loot|Display")
	static FText GetDropRarityDisplayName(EDropRarity Rarity);

	UFUNCTION(BlueprintPure, Category = "Loot|Display")
	static FLinearColor GetDropRarityColor(EDropRarity Rarity);

	UFUNCTION(BlueprintPure, Category = "Loot|Conversion")
	static EItemRarity DropRarityToItemRarity(EDropRarity DropRarity);

	UFUNCTION(BlueprintPure, Category = "Loot|Calculation")
	static float GetRarityMultiplier(EDropRarity Rarity);
};
