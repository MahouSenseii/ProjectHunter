// Loot/Subsystem/LootSubsystem.h
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Engine/StreamableManager.h"
#include "Systems/Loot/Library/LootStructs.h"
#include "Loot/Generation/LootGenerator.h"
#include "LootSubsystem.generated.h"

// Forward declarations
class UGroundItemSubsystem;
class UItemInstance;
class UDataTable;

DECLARE_LOG_CATEGORY_EXTERN(LogLootSubsystem, Log, All);

// ═══════════════════════════════════════════════════════════════════════
// DELEGATES
// ═══════════════════════════════════════════════════════════════════════

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLootGeneratedDelegate, const FLootResultBatch&, Results, FName, SourceID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnLootSpawnedDelegate, UItemInstance*, Item, FVector, Location, int32, GroundItemID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLootTableLoadedDelegate, FName, SourceID, bool, bSuccess);

/**
 * ULootSubsystem - Central loot generation and registry management
 * 
 * SINGLE RESPONSIBILITY: 
 * - Manage loot source registry (Source ID → Loot Table mapping)
 * - Generate loot from registered sources
 * - Coordinate with GroundItemSubsystem for spawning
 * 
 * DESIGN:
 * - World Subsystem (single instance per world)
 * - Lazy loading of loot tables (TSoftObjectPtr)
 * - Caching of loaded tables
 * - Server-authoritative loot generation
 * - Deterministic with seed support
 * 
 * FIXES APPLIED:
 * - Cache key now uses DataTable path (prevents collisions)
 * - Proper seed fallback generation
 * - Null safety checks
 */
UCLASS()
class ALS_PROJECTHUNTER_API ULootSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// ═══════════════════════════════════════════════
	// SUBSYSTEM LIFECYCLE
	// ═══════════════════════════════════════════════

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override { return true; }

	// ═══════════════════════════════════════════════
	// CONFIGURATION
	// ═══════════════════════════════════════════════

	/** Path to loot source registry DataTable */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot|Config")
	TSoftObjectPtr<UDataTable> LootSourceRegistryPath;

	/** Global drop chance multiplier (for events, difficulty, etc.) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot|Config")
	float GlobalDropChanceMultiplier = 1.0f;

	// ═══════════════════════════════════════════════
	// DELEGATES
	// ═══════════════════════════════════════════════

	/** Called when loot is generated */
	UPROPERTY(BlueprintAssignable, Category = "Loot|Events")
	FOnLootGeneratedDelegate OnLootGenerated;

	/** Called when loot is spawned in world */
	UPROPERTY(BlueprintAssignable, Category = "Loot|Events")
	FOnLootSpawnedDelegate OnLootSpawned;

	/** Called when a loot table is loaded */
	UPROPERTY(BlueprintAssignable, Category = "Loot|Events")
	FOnLootTableLoadedDelegate OnLootTableLoaded;

	// ═══════════════════════════════════════════════
	// PRIMARY API - GENERATION
	// ═══════════════════════════════════════════════

	/**
	 * Generate loot from a registered source
	 * @param Request - Loot generation request
	 * @return Batch of generated loot results
	 */
	UFUNCTION(BlueprintCallable, Category = "Loot|Generation")
	FLootResultBatch GenerateLoot(const FLootRequest& Request);

	/**
	 * Generate and spawn loot at location
	 * @param Request - Loot generation request
	 * @param Location - World location to spawn
	 * @param SpreadRadius - Spread radius for multiple items
	 * @return Generated loot batch
	 */
	UFUNCTION(BlueprintCallable, Category = "Loot|Generation")
	FLootResultBatch GenerateAndSpawnLoot(const FLootRequest& Request, FLootSpawnSettings SpawnSettings);

	// ═══════════════════════════════════════════════
	// PRIMARY API - SPAWNING
	// ═══════════════════════════════════════════════

	/**
	 * Spawn already-generated loot at location (circular scatter)
	 * @param Batch - Pre-generated loot batch
	 * @param Location - World location
	 * @param SpreadRadius - Spread radius
	 * @return True if spawn successful
	 */
	UFUNCTION(BlueprintCallable, Category = "Loot|Spawning")
	bool SpawnLootAtLocation(const FLootResultBatch& Batch, FVector Location, float SpreadRadius = 50.0f);

	/**
	 * Spawn already-generated loot using full spawn settings (supports box area)
	 * @param Batch - Pre-generated loot batch
	 * @param SpawnSettings - Full spawn settings including optional box area
	 * @return True if spawn successful
	 */
	UFUNCTION(BlueprintCallable, Category = "Loot|Spawning")
	bool SpawnLootWithSettings(const FLootResultBatch& Batch, const FLootSpawnSettings& SpawnSettings);

	// ═══════════════════════════════════════════════
	// REGISTRY QUERIES
	// ═══════════════════════════════════════════════

	/**
	 * Check if a source ID is registered
	 */
	UFUNCTION(BlueprintPure, Category = "Loot|Registry")
	bool IsSourceRegistered(FName SourceID) const;

	/**
	 * Get source entry by ID
	 */
	UFUNCTION(BlueprintPure, Category = "Loot|Registry")
	bool GetSourceEntry(FName SourceID, FLootSourceEntry& OutEntry) const;

	/**
	 * Get all registered source IDs
	 */
	UFUNCTION(BlueprintPure, Category = "Loot|Registry")
	TArray<FName> GetAllSourceIDs() const;

	/**
	 * Get source IDs by category
	 */
	UFUNCTION(BlueprintPure, Category = "Loot|Registry")
	TArray<FName> GetSourceIDsByCategory(ELootSourceType Category) const;

	// ═══════════════════════════════════════════════
	// CACHE MANAGEMENT
	// ═══════════════════════════════════════════════

	/**
	 * Preload loot tables for sources (call during loading screen)
	 */
	UFUNCTION(BlueprintCallable, Category = "Loot|Cache")
	void PreloadLootTables(const TArray<FName>& SourceIDs);

	/**
	 * Clear all cached loot tables
	 */
	UFUNCTION(BlueprintCallable, Category = "Loot|Cache")
	void ClearLootTableCache();

	/**
	 * Get number of cached loot tables
	 */
	UFUNCTION(BlueprintPure, Category = "Loot|Cache")
	int32 GetCachedTableCount() const { return LootTableCache.Num(); }

protected:
	// ═══════════════════════════════════════════════
	// INTERNAL - REGISTRY
	// ═══════════════════════════════════════════════

	/** P-2 FIX: Kick off an async load of the registry DataTable. */
	void LoadRegistry();

	/** Called by the streamable manager when the registry asset finishes loading. */
	void OnRegistryLoaded();
	
	// ═══════════════════════════════════════════════
	// INTERNAL - LOOT TABLE LOADING
	// ═══════════════════════════════════════════════

	const FLootTable* GetLootTableFromSource(const FLootSourceEntry& Source, FName RowName);
	bool LoadLootTableAsync(const FLootSourceEntry& Source);

	// ═══════════════════════════════════════════════
	// INTERNAL - SETTINGS BUILDING
	// ═══════════════════════════════════════════════

	FLootDropSettings BuildFinalSettings(const FLootSourceEntry& Source, const FLootRequest& Request) const;
	FLootDropSettings ApplyGlobalModifiers(const FLootDropSettings& Settings) const;
	FLootDropSettings ApplyPlayerModifiers(const FLootDropSettings& Settings, float Luck, float MagicFind) const;

	// ═══════════════════════════════════════════════
	// INTERNAL - SUBSYSTEM CACHING
	// ═══════════════════════════════════════════════

	bool EnsureGroundItemSubsystem();

	// ═══════════════════════════════════════════════
	// CACHED DATA
	// ═══════════════════════════════════════════════

	/** Cached registry DataTable */
	UPROPERTY()
	UDataTable* CachedRegistry;

	/** 
	 * Cached loot tables 
	 * FIX: Key is now DataTable path, not row name (prevents collisions)
	 */
	UPROPERTY()
	TMap<FName, UDataTable*> LootTableCache;

	/** Cached GroundItemSubsystem reference */
	UPROPERTY()
	UGroundItemSubsystem* CachedGroundItemSubsystem;

	/** Cached world reference for subsystem lookups */
	UPROPERTY()
	UWorld* CachedWorld;

	/** Loot generator instance */
	FLootGenerator LootGenerator;

	/**
	 * P-2 FIX: Holds the async streamable handle so the GC doesn't collect the
	 * in-flight request before the registry DataTable finishes loading.
	 */
	TSharedPtr<FStreamableHandle> RegistryStreamHandle;

	/** Source IDs that asked for preload before the registry finished loading. */
	TArray<FName> PendingPreloadSourceIDs;
};

// ═══════════════════════════════════════════════════════════════════════
// INLINE HELPER - Quick loot generation
// ═══════════════════════════════════════════════════════════════════════

/**
 * Helper function for quick loot generation
 * Use when you don't need full FLootRequest control
 */
FORCEINLINE FLootResultBatch QuickGenerateLoot(
	UWorld* World,
	FName SourceID,
	float PlayerLuck = 0.0f,
	float PlayerMagicFind = 0.0f,
	int32 Seed = 0)
{
	if (!World)
	{
		return FLootResultBatch();
	}

	ULootSubsystem* LootSub = World->GetSubsystem<ULootSubsystem>();
	if (!LootSub)
	{
		return FLootResultBatch();
	}

	FLootRequest Request(SourceID);
	Request.PlayerLuck = PlayerLuck;
	Request.PlayerMagicFind = PlayerMagicFind;
	Request.Seed = Seed;

	return LootSub->GenerateLoot(Request);
}
