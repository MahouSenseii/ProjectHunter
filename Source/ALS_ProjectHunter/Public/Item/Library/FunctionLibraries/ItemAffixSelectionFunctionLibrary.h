#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Item/Library/Structs/ItemAttributeStructs.h"
#include "Item/Library/Structs/AffixStructs.h"
#include "ItemAffixSelectionFunctionLibrary.generated.h"

class UDataTable;

/**
 * Optional problem sink for ResolveAffixSet.
 *
 * Resolution has to keep working through a bad include so loot still generates,
 * so the validator passes this to find out what was skipped along the way.
 */
struct FAffixSetResolveDiagnostics
{
	/** IncludedSets handles naming a row that does not exist. */
	TArray<FName> MissingIncludeRows;

	/** Sets reached again through their own includes; the repeat visit is skipped. */
	TArray<FName> CyclicIncludes;
};

UCLASS()
class ALS_PROJECTHUNTER_API UItemAffixSelectionFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Row name of the FAffixSet that serves SubType, or NAME_None.
	 * Sets left at IST_None are shared building blocks and are never returned.
	 */
	static FName FindAffixSetRowForSubType(const UDataTable* PoolTable, EItemSubType SubType);

	/**
	 * Flatten one FAffixSet and everything it includes into a single pool.
	 *
	 * Includes are expanded depth-first and appended before the set's own
	 * entries, then duplicates collapse with the last entry winning - so a
	 * sub-type set re-listing an inherited affix overrides it for that sub-type.
	 */
	static bool ResolveAffixSet(
		const UDataTable* PoolTable,
		FName SetRowName,
		FResolvedAffixPool& OutPool,
		FAffixSetResolveDiagnostics* OutDiagnostics = nullptr);

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
