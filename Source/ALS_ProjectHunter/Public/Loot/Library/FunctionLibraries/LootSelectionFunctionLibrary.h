#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Loot/Library/Structs/LootStructs.h"
#include "LootSelectionFunctionLibrary.generated.h"

UCLASS()
class ALS_PROJECTHUNTER_API ULootSelectionFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static TArray<FLootEntry> FilterEntries(const TArray<FLootEntry>& Entries, const FLootDropSettings& Settings);

	static int32 CalculateDropCount(const FLootTable& Table, const FLootDropSettings& Settings, FRandomStream& RandStream);

	static TArray<int32> SelectWeighted(
		const TArray<FLootEntry>& Entries,
		int32 NumToSelect,
		bool bAllowDuplicates,
		FRandomStream& RandStream);

	static TArray<int32> SelectSequential(
		const TArray<FLootEntry>& Entries,
		const FLootDropSettings& Settings,
		FRandomStream& RandStream);

	static TArray<int32> SelectGuaranteedOne(const TArray<FLootEntry>& Entries, FRandomStream& RandStream);

	static TArray<int32> SelectAll(const TArray<FLootEntry>& Entries);
};
