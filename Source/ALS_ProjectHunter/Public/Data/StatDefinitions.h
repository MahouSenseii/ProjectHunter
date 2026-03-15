#pragma once

#include "CoreMinimal.h"
#include "StatDefinitions.generated.h"

struct FGameplayAttribute;

USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FHunterStatDefinition
{
	GENERATED_BODY()

	FHunterStatDefinition();

	/** Gameplay attribute/property name from UHunterAttributeSet. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	FName StatName;

	/** Shared human-readable label for editor tools, UI, and debug output. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	FText DisplayName;

	/** Shared organizational category derived from the registry ruleset. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	FName Category;

	/** Reserved for future explicit per-stat ordering inside a category. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	int32 SortOrder;

	/** Shared debug visualization color used by stat debugging tools. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	FLinearColor DebugColor;
};

/**
 * Shared single source of truth for stat discovery and metadata.
 * All valid gameplay stats originate from reflected FGameplayAttributeData properties
 * on UHunterAttributeSet and are cached here for reuse across systems.
 */
class ALS_PROJECTHUNTER_API FHunterStatDefinitions
{
public:
	static const TArray<FHunterStatDefinition>& GetAllStatDefinitions();
	static void GatherAttributeDefinitionsFromAttributeSet(TArray<FHunterStatDefinition>& OutDefinitions);
	static const FHunterStatDefinition* GetStatDefinition(FName StatName);
	static bool HasStatDefinition(FName StatName);
	static TArray<FName> GetAllStatNames();
	static TArray<FName> GetAllCategories();
	static bool TryGetGameplayAttribute(FName StatName, FGameplayAttribute& OutAttribute);
	static FName BuildDefaultCategoryForStat(FName StatName, const FString& SourceCategory = FString());
	static FText BuildDefaultDisplayName(FName StatName, const FString& ExplicitDisplayName = FString());
	static FLinearColor BuildDefaultDebugColor(FName Category);
	static int32 GetCategorySortPriority(FName Category);
	static int32 GetStatSortOrder(FName StatName);
	static int32 GetSortIndex(FName StatName);
	static void SortDefinitions(TArray<FHunterStatDefinition>& InOutDefinitions);
};
