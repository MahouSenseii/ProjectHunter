#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Item/Library/Structs/ItemAttributeStructs.h"
#include "ItemAffixSelectionFunctionLibrary.generated.h"

UCLASS()
class ALS_PROJECTHUNTER_API UItemAffixSelectionFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static TArray<FPHAttributeData*> BuildAffixPoolByCorruption(
		const TArray<FPHAttributeData*>& SourceAffixes,
		EItemType ItemType,
		EItemSubType ItemSubType,
		int32 ItemLevel,
		bool bCorruptedOnly,
		const TSet<FName>& ExcludeAffixes,
		const TSet<FName>& ExcludeGroups = TSet<FName>());

	static const FPHAttributeData* SelectWeightedAffix(
		const TArray<FPHAttributeData*>& AvailableAffixes,
		FRandomStream& RandStream);

	static FPHAttributeData CreateRolledAffix(
		const FPHAttributeData& TemplateAffix,
		FRandomStream& RandStream);
};
