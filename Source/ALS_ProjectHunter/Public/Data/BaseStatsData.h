#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "BaseStatsData.generated.h"

class UGameplayEffect;
class UAttributeSet;
struct FGameplayAttribute;
struct FPropertyChangedEvent;

UENUM(BlueprintType)
enum class EHunterStatType : uint8
{
	Neutral		UMETA(DisplayName = "Neutral"),
	Primary		UMETA(DisplayName = "Primary"),
	Vital		UMETA(DisplayName = "Vital"),
	Offense		UMETA(DisplayName = "Offense"),
	Defense		UMETA(DisplayName = "Defense"),
	Resource	UMETA(DisplayName = "Resource"),
	Utility		UMETA(DisplayName = "Utility"),
	Special		UMETA(DisplayName = "Special")
};

/**
 * Canonical parsed category path used everywhere the stat system needs to group or display categories.
 *
 * RawCategory keeps the trimmed source string from metadata or authored data.
 * NormalizedCategory is the stable "Parent|Child" form used for storage and comparisons.
 * MainCategory/SubCategory expose the logical split for grouping in runtime debug and editor UI.
 */
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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	FString RawCategory;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	FName NormalizedCategory;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	FName MainCategory;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	FName SubCategory;

	FORCEINLINE bool HasSubCategory() const
	{
		return SubCategory != NAME_None;
	}
};

/**
 * One reflected stat entry sourced from the configured AttributeSet class.
 *
 * Identity and presentation fields are rebuilt from reflection:
 * - StatName
 * - DisplayName
 * - Category
 * - SortOrder
 * - Tooltip
 * - IconName
 * - StatType
 *
 * Authored runtime values remain editable per asset:
 * - bOverrideValue
 * - BaseValue
 */
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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats", meta = (EditCondition = "bOverrideValue", EditConditionHides))
	float BaseValue;

	FORCEINLINE bool HasRuntimeValue() const
	{
		return bOverrideValue;
	}

	FORCEINLINE bool IsValid() const
	{
		return StatName != NAME_None;
	}

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

UCLASS(BlueprintType)
class ALS_PROJECTHUNTER_API UBaseStatsData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UBaseStatsData();
	static TSubclassOf<UAttributeSet> ResolveSourceAttributeSetClass(const UBaseStatsData* Data);
	static void GatherStatDefinitionsFromAttributeSet(TSubclassOf<UAttributeSet> AttributeSetClass, TArray<FStatInitializationEntry>& OutDefinitions);
	static FParsedStatCategory ParseCategoryPath(const FString& CategoryString);
	static FParsedStatCategory ParseCategoryPath(const FName& CategoryName);
	static FName NormalizeCategoryName(const FName& CategoryName);
	static int32 GetCategorySortPriority(const FName& CategoryName);
	static FName CallNormalizeCategoryName(const FName& CategoryName);
	static FLinearColor GetStatTypeColor(EHunterStatType StatType);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	FText StatSetName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity", meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	FGameplayTagContainer Tags;

	/**
	 * The AttributeSet class scanned for reflected FGameplayAttributeData properties.
	 *
	 * Supported metadata contract on AttributeSet properties:
	 * - meta=(StatCategory="Vital|Health")
	 * - meta=(SortOrder="10")
	 * - meta=(StatTooltip="Base melee power")
	 * - meta=(StatIcon="Strength")
	 * - meta=(StatType="Primary")
	 * - meta=(HideInStatsData)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup")
	TSubclassOf<UAttributeSet> SourceAttributeSetClass;

	/**
	 * Reflected stat rows rebuilt from SourceAttributeSetClass while preserving matching authored overrides.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "All Attributes", meta = (TitleProperty = "DisplayName"))
	TArray<FStatInitializationEntry> BaseAttributes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Advanced")
	TArray<TSubclassOf<UGameplayEffect>> InitializationEffects;

	UFUNCTION(BlueprintPure, Category = "Stats")
	TMap<FName, float> GetAllStatsAsMap() const;

	UFUNCTION(BlueprintPure, Category = "Stats")
	TMap<FName, float> GetStatsByCategory(FName CategoryName) const;

	UFUNCTION(BlueprintPure, Category = "Stats")
	TArray<FName> GetSupportedCategories() const;

	UFUNCTION(BlueprintPure, Category = "Stats")
	TArray<FName> GetSupportedStatNames() const;

	UFUNCTION(BlueprintPure, Category = "Stats")
	TArray<FStatInitializationEntry> GetStatEntriesByCategory(FName CategoryName) const;

	const TArray<FStatInitializationEntry>& GetBaseAttributes() const
	{
		return BaseAttributes;
	}

	UFUNCTION(BlueprintPure, Category = "Stats")
	bool GetStatValue(FName AttributeName, float& OutValue) const;

	UFUNCTION(BlueprintPure, Category = "Stats")
	bool HasAttribute(FName AttributeName) const;

	UFUNCTION(BlueprintPure, Category = "Stats")
	bool HasCategory(FName CategoryName) const;

	UFUNCTION(BlueprintCallable, Category = "Stats")
	void SortStatsByCategoryThenName();

	UFUNCTION(BlueprintCallable, Category = "Stats")
	void RefreshCategoriesFromDefinitions();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Stats|Editor")
	void RefreshFromAttributeSetDefinition();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Stats|Editor")
	void SortStats();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Stats|Editor")
	void ValidateStats();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Stats|Editor", meta = (DisplayName = "Start Stats"))
	void StartStats();

	static bool IsInlineAttributeStat(FName StatName);
	virtual void PostLoad() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(TEXT("BaseStatsData"), GetFName());
	}
};
