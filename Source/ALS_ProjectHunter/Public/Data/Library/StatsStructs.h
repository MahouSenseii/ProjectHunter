// Data/Library/StatsStructs.h
// Shared structs for the BaseStats data system.
//
// Dependency chain:
//   BaseStatsEnumLibrary.h  →  StatsStructs.h  →  (system headers)
//
// Include this alone when you only need to pass or receive stat data
// (e.g. debug UI, analytics) without pulling in the full UBaseStatsData header.

#pragma once

#include "CoreMinimal.h"
#include "Data/Library/BaseStatsEnumLibrary.h"
#include "StatsStructs.generated.h"

// ─────────────────────────────────────────────────────────────────────────────
// FParsedStatCategory
// Canonical parsed category path used everywhere the stat system needs to
// group or display categories.
// ─────────────────────────────────────────────────────────────────────────────
USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FParsedStatCategory
{
	GENERATED_BODY()

	FParsedStatCategory()
		: RawCategory(TEXT(""))
		, NormalizedCategory(TEXT("Uncategorized"))
		, MainCategory(TEXT("Uncategorized"))
		, SubCategory(NAME_None)
	{
	}

	/** Trimmed source string from metadata or authored data. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	FString RawCategory;

	/** Stable "Parent|Child" form used for storage and comparisons. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	FName NormalizedCategory;

	/** Logical parent group for editor grouping. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	FName MainCategory;

	/** Optional sub-group within the parent. NAME_None if absent. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	FName SubCategory;

	FORCEINLINE bool HasSubCategory() const { return SubCategory != NAME_None; }
};

// ─────────────────────────────────────────────────────────────────────────────
// FStatInitializationEntry
// One reflected stat entry sourced from the configured AttributeSet class.
// ─────────────────────────────────────────────────────────────────────────────
USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FStatInitializationEntry
{
	GENERATED_BODY()

	FStatInitializationEntry()
		: StatName(NAME_None)
		, DisplayName(FText::GetEmpty())
		, RawCategory(TEXT(""))
		, Category(NAME_None)
		, MainCategory(NAME_None)
		, SubCategory(NAME_None)
		, SortOrder(0)
		, Tooltip(FText::GetEmpty())
		, IconName(NAME_None)
		, StatType(EHunterStatType::Neutral)
		, bOverrideValue(false)
		, BaseValue(0.0f)
	{
	}

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	FName StatName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	FText DisplayName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	FString RawCategory;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	FName Category;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	FName MainCategory;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	FName SubCategory;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	int32 SortOrder;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	FText Tooltip;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	FName IconName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	EHunterStatType StatType;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	bool bOverrideValue;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats",
		meta = (EditCondition = "bOverrideValue", EditConditionHides))
	float BaseValue;

	FORCEINLINE bool HasRuntimeValue() const  { return bOverrideValue; }
	FORCEINLINE bool IsValid()         const  { return StatName != NAME_None; }

	FORCEINLINE FString BuildSearchString() const
	{
		return FString::Printf(
			TEXT("%s %s %s %s %s %s %s %s"),
			*StatName.ToString(),
			*DisplayName.ToString(),
			*RawCategory,
			*Category.ToString(),
			*MainCategory.ToString(),
			*SubCategory.ToString(),
			*Tooltip.ToString(),
			*StaticEnum<EHunterStatType>()->GetNameStringByValue(static_cast<int64>(StatType)));
	}
};
