#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Loot/Library/Structs/LootStructs.h"
#include "LootSpawnFunctionLibrary.generated.h"

UCLASS()
class ALS_PROJECTHUNTER_API ULootSpawnFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static FVector GetCircularScatterLocation(const FVector& Location, float SpreadRadius, FRandomStream& RandStream);
	static FVector GetSpawnLocationFromSettings(const FLootSpawnSettings& SpawnSettings, FRandomStream& RandStream);
};
