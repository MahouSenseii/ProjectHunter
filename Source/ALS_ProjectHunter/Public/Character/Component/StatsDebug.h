#pragma once

#include "CoreMinimal.h"
#include "StatsDebug.generated.h"

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
	bool bShowCustom;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Debug", meta = (TitleProperty = "DisplayName", EditFixedSize))
	TArray<FStatDebugEntry> StatEntries;

	void InitializeDefaults();
	void RegisterStat(const FName StatName, const FText& DisplayName, const FName Category, const FColor& DisplayColor = FColor::White, bool bEnabled = true);
	void RegisterStats();
	bool IsStatEnabled(const FStatDebugEntry& Entry) const;
	bool IsCategoryEnabled(FName Category) const;
	void BuildDisplayLines(UStatsManager* StatsManager, TArray<FString>& OutLines);
	void DrawDebug(UStatsManager* StatsManager, UObject* WorldContext);
	void LogDebug(UStatsManager* StatsManager);

private:
	bool ShouldRefresh(double CurrentTimeSeconds, double& LastExecutionTimeSeconds);
	void ClearDrawnMessages();

	bool bEntriesSynchronized;
	double LastScreenUpdateTimeSeconds;
	double LastLogUpdateTimeSeconds;
	int32 LastDrawnLineCount;
	TArray<FString> CachedDisplayLines;
	TArray<FColor> CachedLineColors;
};
