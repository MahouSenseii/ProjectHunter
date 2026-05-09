#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Data/Library/BaseStatsEnumLibrary.h"
#include "Data/Library/StatsStructs.h"
#include "BaseStatsData.generated.h"

class UGameplayEffect;
class UAttributeSet;
struct FGameplayAttribute;
struct FPropertyChangedEvent;

// FParsedStatCategory and FStatInitializationEntry have been moved to
// Data/Library/StatsStructs.h — included above.

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
