#pragma once

#include "CoreMinimal.h"
#include "Stats/Library/Enums/BaseStatsEnumLibrary.h"
#include "Stats/Library/Structs/BaseStatsStructs.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "BaseStatsData.generated.h"

class UGameplayEffect;
class UAttributeSet;
struct FGameplayAttribute;
struct FPropertyChangedEvent;

UCLASS(BlueprintType)
class ALS_PROJECTHUNTER_API UBaseStatsData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Bump when serialized stat defaults require a one-time data migration. */
	static constexpr int32 CurrentStatsSchemaVersion = 3;

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

	/**
	 * When true, initialization effects that modify an explicitly overridden stat row are skipped.
	 * Leave enabled when this data asset should be the authored source of truth for regen, degen, and max values.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Advanced")
	bool bSkipInitializationEffectsThatModifyAuthoredStats;

	/**
	 * Internal version for one-time stat-default migrations. Version 1 repairs
	 * legacy assets whose reflected rows authored neutral multipliers as zero.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Advanced")
	int32 StatsSchemaVersion;

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


#if WITH_EDITOR
	/**
	 * Unticks Override Value on every stat an InitializationEffect modifies.
	 *
	 * An authored row blocks the entire effect that would drive it - the guard
	 * is per-effect, not per-attribute - so one stray override silently stops a
	 * whole set of derived values from being calculated. Reads the effects'
	 * modifiers rather than a hardcoded list, so adding a derived effect and
	 * pressing this again is all that is needed.
	 */
	UFUNCTION(CallInEditor, Category = "Stats|Editor")
	void ClearOverridesDrivenByInitializationEffects();

	/** Reports which authored rows are blocking which effects, without changing anything. */
	UFUNCTION(CallInEditor, Category = "Stats|Editor")
	void LogInitializationEffectConflicts();

	/**
	 * Throws away every override and re-authors only the rows that are actually
	 * data: progression, primaries, the neutral combat multipliers that break
	 * the game at zero, and the vital inputs nothing else drives.
	 *
	 * Deliberately leaves alone anything an InitializationEffect modifies (that
	 * effect should own it) and the MaxEffective* outputs the attribute set
	 * recomputes (authoring them does nothing). Add either back through the
	 * picker if a specific test needs a flat value.
	 *
	 * Destructive: authored values not in the baseline are lost.
	 */
	UFUNCTION(CallInEditor, Category = "Stats|Editor")
	void ResetToBaseline();

	/**
	 * The value ResetToBaseline would give this stat, if it has one.
	 *
	 * Lets the details panel tell "authored because I changed it" apart from
	 * "authored because the tool stamped a required default" - bOverrideValue
	 * alone cannot distinguish the two, and the defaults outnumber real edits
	 * roughly three to one.
	 */
	static bool GetBaselineValueForStat(FName StatName, float& OutValue);
#endif

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
