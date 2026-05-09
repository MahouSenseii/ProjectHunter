// Tower/Library/StashStructs.h
// Shared structs for the lazy-loading stash system.
//
// Dependency chain:
//   StashEnumLibrary.h  →  StashStructs.h  →  (system headers)
//
// Include this alone when you only need to pass or receive stash data
// (e.g. end-of-run screen, analytics) without pulling in UStashSubsystem.

#pragma once

#include "CoreMinimal.h"
#include "Tower/Library/StashEnumLibrary.h"
#include "StashStructs.generated.h"

class UItemInstance;

// ─────────────────────────────────────────────────────────────────────────────
// FStashItemEntry — one item stored in the stash with its grid position
// ─────────────────────────────────────────────────────────────────────────────
USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FStashItemEntry
{
	GENERATED_BODY()

	/** The item itself */
	UPROPERTY(BlueprintReadWrite, Category = "Stash")
	TObjectPtr<UItemInstance> Item;

	/** Top-left grid cell (col, row) in the tab */
	UPROPERTY(BlueprintReadWrite, Category = "Stash")
	FIntPoint GridPosition = FIntPoint::ZeroValue;

	FStashItemEntry() = default;
	FStashItemEntry(UItemInstance* InItem, FIntPoint InPos)
		: Item(InItem), GridPosition(InPos)
	{}
};

// ─────────────────────────────────────────────────────────────────────────────
// FStashTabData — full tab payload, resident in memory only while tab is open/dirty
// ─────────────────────────────────────────────────────────────────────────────
USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FStashTabData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Stash")
	FName TabID;

	UPROPERTY(BlueprintReadWrite, Category = "Stash")
	TArray<FStashItemEntry> Items;

	/** Grid dimensions (cols x rows) */
	UPROPERTY(BlueprintReadOnly, Category = "Stash")
	FIntPoint GridSize = FIntPoint(12, 12);

	FStashTabData() = default;
	explicit FStashTabData(FName InID) : TabID(InID) {}
};

// ─────────────────────────────────────────────────────────────────────────────
// FStashTabHandle — lightweight header, always resident in memory for every tab
// ─────────────────────────────────────────────────────────────────────────────
USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FStashTabHandle
{
	GENERATED_BODY()

	/** Stable identifier used as the sub-slot name when saving. */
	UPROPERTY(BlueprintReadOnly, Category = "Stash")
	FName TabID;

	/** Display name shown in the tab bar. */
	UPROPERTY(BlueprintReadWrite, Category = "Stash")
	FText TabName;

	UPROPERTY(BlueprintReadWrite, Category = "Stash")
	EStashTabType TabType = EStashTabType::STT_Normal;

	/** Cached item count for the tab badge — updated on add/remove without loading data. */
	UPROPERTY(BlueprintReadOnly, Category = "Stash")
	int32 CachedItemCount = 0;

	/** Optional accent colour for premium tabs. */
	UPROPERTY(BlueprintReadWrite, Category = "Stash")
	FLinearColor AccentColor = FLinearColor::White;

	/** True when full data has been loaded into UStashSubsystem::LoadedTabs. */
	UPROPERTY(Transient)
	bool bIsLoaded = false;

	/** True when loaded data has been modified and needs saving. */
	UPROPERTY(Transient)
	bool bIsDirty = false;

	FStashTabHandle() = default;
	FStashTabHandle(FName InID, FText InName, EStashTabType InType = EStashTabType::STT_Normal)
		: TabID(InID), TabName(InName), TabType(InType)
	{}
};
