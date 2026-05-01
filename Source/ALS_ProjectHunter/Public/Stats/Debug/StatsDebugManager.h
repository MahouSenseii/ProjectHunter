#pragma once

#include "CoreMinimal.h"
#include "StatsDebugManager.generated.h"

class UObject;
class UStatsManager;

DECLARE_LOG_CATEGORY_EXTERN(LogStatsDebugManager, Log, All);

USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FStatDebugEntry
{
	GENERATED_BODY()

	FStatDebugEntry();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Debug|Entry")
	FName StatName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats|Debug|Entry")
	FText DisplayName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats|Debug|Entry")
	FName Category;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats|Debug|Entry")
	int32 SortOrder;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats|Debug|Entry")
	FText Tooltip;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats|Debug|Entry")
	FName IconName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats|Debug|Entry")
	FName StatType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Debug|Entry")
	bool bEnabled;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Debug|Entry")
	FColor DisplayColor;
};

USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FStatsDebugManager
{
	GENERATED_BODY()

	FStatsDebugManager();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Debug")
	bool bEnableDebug;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Debug|Display")
	bool bDrawToScreen;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Debug|Display")
	bool bLogToOutput;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Debug|Display", meta = (ClampMin = "0.05", UIMin = "0.05"))
	float DebugRefreshRate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Debug|Display", meta = (ClampMin = "0"))
	int32 BaseMessageKey;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Debug|Filters")
	FString FilterString;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Debug|Categories")
	bool bShowVitals;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Debug|Categories")
	bool bShowResources;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Debug|Categories")
	bool bShowRegeneration;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Debug|Categories")
	bool bShowPrimary;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Debug|Categories")
	bool bShowSecondary;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Debug|Categories")
	bool bShowCombat;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Debug|Categories")
	bool bShowDefense;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Debug|Categories")
	bool bShowResistances;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Debug|Categories")
	bool bShowMovement;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Debug|Categories")
	bool bShowUtility;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Debug|Categories")
	bool bShowLoot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Debug|Categories")
	bool bShowSpecial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Debug|Categories")
	bool bShowCustom;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Debug", meta = (TitleProperty = "DisplayName", EditFixedSize))
	TArray<FStatDebugEntry> StatEntries;

	void InitializeDefaults();
	void RegisterStat(const FStatDebugEntry& Entry);
	void RegisterStats(UStatsManager* StatsManager);
	bool IsStatEnabled(const FStatDebugEntry& Entry) const;
	bool IsCategoryEnabled(FName Category) const;
	void BuildDisplayLines(UStatsManager* StatsManager, TArray<FString>& OutLines, TArray<FColor>& OutColors);
	void DrawDebug(UStatsManager* StatsManager, UObject* WorldContext);
	void LogDebug(UStatsManager* StatsManager);

private:
	/**
	 * Walks every enabled stat, compares the live GAS value against CachedLiveValues,
	 * and updates the cache for any entry that moved.
	 * Returns true if at least one value changed (or if this is the first call for a stat).
	 */
	bool CheckForValueChanges(UStatsManager* StatsManager);

	bool ShouldRefresh(double CurrentTimeSeconds, double& LastExecutionTimeSeconds);
	void ClearDrawnMessages();

	bool bEntriesSynchronized;
	double LastLogUpdateTimeSeconds;
	int32 LastDrawnLineCount;
	TArray<FString> CachedDisplayLines;
	TArray<FColor> CachedLineColors;
	TSet<FName> WarnedCustomBucketStats;

	/**
	 * Per-stat live-value cache used for change detection.
	 * Populated on the first call to CheckForValueChanges and updated whenever a value moves.
	 * Cleared when debug is disabled or the character changes so the first re-enable
	 * always triggers a full redraw.
	 */
	TMap<FName, float> CachedLiveValues;
};
