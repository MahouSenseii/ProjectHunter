// Tower/Subsystem/StashSubsystem.h
// Lazy-loading stash system.
//
// ARCHITECTURE — Why lazy loading?
// In Diablo-style games, loading all stash data on login caused hitches and
// memory spikes because every item, including ones in tabs the player never
// opened, was fully deserialised upfront.
//
// This system keeps only lightweight FStashTabHandle metadata in memory at all
// times.  Full FStashTabData (the actual item arrays) is loaded from the save
// slot on demand the FIRST TIME a player opens that tab, then cached in memory
// for the session.  Tabs that have not been touched stay as handles only.
//
// Typical flow:
//   1. PlayerController::BeginPlay → StashSubsystem::LoadStashHandles()
//      — loads only the tab headers (names, types, item counts).  Fast.
//   2. Player opens Tab 3 → StashSubsystem::RequestTabData(2)
//      — loads only Tab 3's item array from disk.  Other tabs untouched.
//   3. Player moves item → StashSubsystem::MarkTabDirty(2)
//   4. Zone transition / logout → StashSubsystem::FlushDirtyTabs()
//      — only saves tabs flagged dirty.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "StashSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogStashSubsystem, Log, All);

class UItemInstance;

// ─────────────────────────────────────────────────────────────────────────────
// Tab type — controls display and sorting behaviour in the UI
// ─────────────────────────────────────────────────────────────────────────────
UENUM(BlueprintType)
enum class EStashTabType : uint8
{
	STT_Normal      UMETA(DisplayName = "Normal (Grid)"),
	STT_Quad        UMETA(DisplayName = "Quad (Large Grid)"),
	STT_Currency    UMETA(DisplayName = "Currency (Auto-Sort)"),
	STT_Premium     UMETA(DisplayName = "Premium (Colour/Named)"),
};

// ─────────────────────────────────────────────────────────────────────────────
// One item stored in the stash — position within the tab's grid
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
// Full tab payload — only resident in memory while the tab is open/dirty
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
// Lightweight header — always resident in memory for every tab
// ─────────────────────────────────────────────────────────────────────────────
USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FStashTabHandle
{
	GENERATED_BODY()

	/** Stable identifier used as the sub-slot name when saving */
	UPROPERTY(BlueprintReadOnly, Category = "Stash")
	FName TabID;

	/** Display name shown in the tab bar */
	UPROPERTY(BlueprintReadWrite, Category = "Stash")
	FText TabName;

	UPROPERTY(BlueprintReadWrite, Category = "Stash")
	EStashTabType TabType = EStashTabType::STT_Normal;

	/** Cached item count for the tab badge — updated on add/remove without loading data */
	UPROPERTY(BlueprintReadOnly, Category = "Stash")
	int32 CachedItemCount = 0;

	/** Optional accent colour for premium tabs */
	UPROPERTY(BlueprintReadWrite, Category = "Stash")
	FLinearColor AccentColor = FLinearColor::White;

	/** True when full data has been loaded into StashSubsystem::LoadedTabs */
	UPROPERTY(Transient)
	bool bIsLoaded = false;

	/** True when loaded data has been modified and needs saving */
	UPROPERTY(Transient)
	bool bIsDirty = false;

	FStashTabHandle() = default;
	FStashTabHandle(FName InID, FText InName, EStashTabType InType = EStashTabType::STT_Normal)
		: TabID(InID), TabName(InName), TabType(InType)
	{}
};

// ─────────────────────────────────────────────────────────────────────────────
// Delegates
// ─────────────────────────────────────────────────────────────────────────────
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStashTabLoaded, FName, TabID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStashItemAdded,
	FName, TabID, UItemInstance*, Item);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStashItemRemoved,
	FName, TabID, UItemInstance*, Item);

// ─────────────────────────────────────────────────────────────────────────────
UCLASS()
class ALS_PROJECTHUNTER_API UStashSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// ── Initialisation ────────────────────────────────────────────────────────

	/**
	 * Load tab handle metadata for a character.
	 * Call once on possession/login.  Fast — only loads headers.
	 * @param CharacterSlotName  Identifies the character's stash data in the save file.
	 */
	UFUNCTION(BlueprintCallable, Category = "Stash")
	void LoadStashHandles(const FString& CharacterSlotName);

	// ── Tab access (lazy load) ────────────────────────────────────────────────

	/**
	 * Request full data for a tab.
	 * Loads from disk if not already in memory.  Broadcasts OnStashTabLoaded when ready.
	 * Subsequent calls return immediately (data is cached).
	 * @param TabIndex  Zero-based index into TabHandles.
	 */
	UFUNCTION(BlueprintCallable, Category = "Stash")
	bool RequestTabData(int32 TabIndex);

	/**
	 * Get a pointer to loaded tab data (C++ only — UHT cannot expose USTRUCT pointers).
	 * Returns nullptr if the tab is not loaded yet — call RequestTabData first.
	 * Blueprint callers: use GetLoadedTabItems / IsTabLoaded instead.
	 */
	FStashTabData* GetLoadedTabData(int32 TabIndex);

	/** True if the tab's full data is currently in memory. */
	UFUNCTION(BlueprintPure, Category = "Stash")
	bool IsTabLoaded(int32 TabIndex) const;

	// ── Item management ───────────────────────────────────────────────────────

	/**
	 * Add an item to a tab at an explicit grid position.  Tab must be loaded.
	 */
	UFUNCTION(BlueprintCallable, Category = "Stash")
	bool AddItemToTab(int32 TabIndex, UItemInstance* Item, FIntPoint GridPos);

	/**
	 * Add an item to a tab, auto-placing it at the first free grid cell.
	 * Convenience wrapper; equivalent to AddItemToTab with GridPos (-1,-1).
	 */
	UFUNCTION(BlueprintCallable, Category = "Stash")
	bool AddItemToTabAutoPlace(int32 TabIndex, UItemInstance* Item);

	/**
	 * Remove an item from a tab by its grid position.
	 * Returns the item so the caller can place it elsewhere.
	 */
	UFUNCTION(BlueprintCallable, Category = "Stash")
	UItemInstance* RemoveItemFromTab(int32 TabIndex, FIntPoint GridPos);

	/**
	 * Move an item within the same tab or to a different tab.
	 */
	UFUNCTION(BlueprintCallable, Category = "Stash")
	bool MoveItem(int32 FromTabIndex, FIntPoint FromPos, int32 ToTabIndex, FIntPoint ToPos);

	// ── Persistence ───────────────────────────────────────────────────────────

	/**
	 * Mark a tab as dirty so it will be included in the next FlushDirtyTabs.
	 * Called automatically by AddItemToTab / RemoveItemFromTab.
	 */
	UFUNCTION(BlueprintCallable, Category = "Stash")
	void MarkTabDirty(int32 TabIndex);

	/**
	 * Save only tabs flagged as dirty.  Call on zone transition or logout.
	 * This is the key performance win over Diablo-style full-stash saves.
	 */
	UFUNCTION(BlueprintCallable, Category = "Stash")
	void FlushDirtyTabs();

	/**
	 * Unload tab data from memory without saving.
	 * Use after the stash UI is closed to reclaim memory.
	 * Only unloads non-dirty tabs — dirty tabs stay resident until saved.
	 */
	UFUNCTION(BlueprintCallable, Category = "Stash")
	void UnloadCleanTabs();

	// ── Tab management ────────────────────────────────────────────────────────

	/** Add a new empty tab. */
	UFUNCTION(BlueprintCallable, Category = "Stash")
	int32 AddTab(const FText& Name, EStashTabType Type = EStashTabType::STT_Normal);

	/** Rename a tab. */
	UFUNCTION(BlueprintCallable, Category = "Stash")
	void RenameTab(int32 TabIndex, const FText& NewName);

	/** Read-only access to tab handles (always in memory). */
	UFUNCTION(BlueprintPure, Category = "Stash")
	const TArray<FStashTabHandle>& GetTabHandles() const { return TabHandles; }

	UFUNCTION(BlueprintPure, Category = "Stash")
	int32 GetTabCount() const { return TabHandles.Num(); }

	// ── Events ────────────────────────────────────────────────────────────────

	UPROPERTY(BlueprintAssignable, Category = "Stash|Events")
	FOnStashTabLoaded OnStashTabLoaded;

	UPROPERTY(BlueprintAssignable, Category = "Stash|Events")
	FOnStashItemAdded OnStashItemAdded;

	UPROPERTY(BlueprintAssignable, Category = "Stash|Events")
	FOnStashItemRemoved OnStashItemRemoved;

private:
	/** Always-resident tab headers */
	TArray<FStashTabHandle> TabHandles;

	/** Lazily-loaded tab data — keyed by TabID */
	TMap<FName, FStashTabData> LoadedTabs;

	/** The character slot name used as the save file identifier */
	FString ActiveSlotName;

	/** Build the save slot name for a specific tab */
	FString BuildTabSlotName(FName TabID) const;

	/** Find the first empty grid position in a tab */
	bool FindFreeGridPosition(const FStashTabData& Tab, FIntPoint& OutPos) const;

	/** Save a single tab to disk (synchronous for now; async override possible) */
	void SaveTab(int32 TabIndex);

	/** Persist tab handle metadata (called automatically by FlushDirtyTabs) */
	void SaveHandles();

	/** Load a single tab from disk */
	bool LoadTab(int32 TabIndex);

	bool IsValidTabIndex(int32 TabIndex) const;
};
