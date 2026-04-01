// Loot/Subsystem/LootSubsystem.cpp

#include "Loot/Subsystem/LootSubsystem.h"
#include "Tower/Subsystem/GroundItemSubsystem.h"
#include "Item/ItemInstance.h"
#include "Engine/AssetManager.h"
#include "Engine/DataTable.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY(LogLootSubsystem);

// ═══════════════════════════════════════════════════════════════════════
// SUBSYSTEM LIFECYCLE
// ═══════════════════════════════════════════════════════════════════════

void ULootSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	CachedWorld = GetWorld();
	
	if (LootSourceRegistryPath.IsNull())
	{
		LootSourceRegistryPath = TSoftObjectPtr<UDataTable>(
			FSoftObjectPath(TEXT("/Game/Data/Loot/DT_LootSourceRegistry"))
		);
	}
	
	LoadRegistry();
	
	UE_LOG(LogLootSubsystem, Log, TEXT("LootSubsystem initialized"));
}

void ULootSubsystem::Deinitialize()
{
	// P-2 FIX: Cancel any in-flight async load before tearing down so the
	// delegate doesn't fire into a partially-destroyed subsystem.
	if (RegistryStreamHandle.IsValid())
	{
		RegistryStreamHandle->CancelHandle();
		RegistryStreamHandle.Reset();
	}

	ClearLootTableCache();
	CachedRegistry = nullptr;
	CachedGroundItemSubsystem = nullptr;
	CachedWorld = nullptr;

	Super::Deinitialize();

	UE_LOG(LogLootSubsystem, Log, TEXT("LootSubsystem deinitialized"));
}

// ═══════════════════════════════════════════════════════════════════════
// PRIMARY API - GENERATION
// ═══════════════════════════════════════════════════════════════════════

FLootResultBatch ULootSubsystem::GenerateLoot(const FLootRequest& Request)
{
	FLootResultBatch Batch;
	Batch.SourceID = Request.SourceID;
	
	// Get source entry
	FLootSourceEntry Source;
	if (!GetSourceEntry(Request.SourceID, Source))
	{
		UE_LOG(LogLootSubsystem, Warning, TEXT("GenerateLoot: Source '%s' not found in registry"),
			*Request.SourceID.ToString());
		return Batch;
	}
	
	if (!Source.IsValid())
	{
		UE_LOG(LogLootSubsystem, Warning, TEXT("GenerateLoot: Source '%s' is disabled or invalid"),
			*Request.SourceID.ToString());
		return Batch;
	}
	
	// Get loot table
	const FLootTable* LootTable = GetLootTableFromSource(Source, Source.LootTableRowName);
	if (!LootTable)
	{
		UE_LOG(LogLootSubsystem, Warning, TEXT("GenerateLoot: Failed to load loot table for '%s'"),
			*Request.SourceID.ToString());
		return Batch;
	}
	
	// Build final settings
	FLootDropSettings FinalSettings = BuildFinalSettings(Source, Request);
	FinalSettings = ApplyGlobalModifiers(FinalSettings);
	FinalSettings = ApplyPlayerModifiers(FinalSettings, Request.PlayerLuck, Request.PlayerMagicFind);
	
	// ═══════════════════════════════════════════════
	// FIX: Proper seed fallback generation
	// ═══════════════════════════════════════════════
	int32 Seed = Request.Seed;
	if (Seed == 0)
	{
		Seed = GetTypeHash(Request.SourceID) ^ FDateTime::Now().GetTicks();
		if (Seed == 0)
		{
			Seed = 1;
		}
	}
	
	// Generate loot
	Batch = LootGenerator.GenerateLoot(*LootTable, FinalSettings, Seed, this);
	Batch.SourceID = Request.SourceID;
	
	OnLootGenerated.Broadcast(Batch, Request.SourceID);
	
	UE_LOG(LogLootSubsystem, Verbose, TEXT("GenerateLoot: Generated %d items from source '%s'"),
		Batch.Results.Num(), *Request.SourceID.ToString());
	
	return Batch;
}

// ═══════════════════════════════════════════════════════════════════════
// SPAWNING
// ═══════════════════════════════════════════════════════════════════════

bool ULootSubsystem::SpawnLootAtLocation(const FLootResultBatch& Batch, FVector Location, float SpreadRadius)
{
	if (!EnsureGroundItemSubsystem())
	{
		UE_LOG(LogLootSubsystem, Error, TEXT("SpawnLootAtLocation: GroundItemSubsystem not available"));
		return false;
	}
	
	if (Batch.Results.Num() == 0)
	{
		return true;
	}
	
	FRandomStream SpreadRandom(GetTypeHash(Location));
	
	for (const FLootResult& Result : Batch.Results)
	{
		if (!Result.IsValid())
		{
			continue;
		}
		
		FVector SpawnLocation = Location;
		if (SpreadRadius > 0.0f)
		{
			// Random direction on XY plane
			FVector RandomDir = SpreadRandom.VRand();
			RandomDir.Z = 0.0f;
			RandomDir.Normalize();

			float Distance = SpreadRandom.FRandRange(0.0f, SpreadRadius);
			SpawnLocation += RandomDir * Distance;
		}
		
		int32 GroundItemID = CachedGroundItemSubsystem->AddItemToGround(Result.Item, SpawnLocation);
		
		if (GroundItemID != INDEX_NONE)
		{
			OnLootSpawned.Broadcast(Result.Item, SpawnLocation, GroundItemID);
		}
	}
	
	return true;
}

FLootResultBatch ULootSubsystem::GenerateAndSpawnLoot(const FLootRequest& Request, FLootSpawnSettings SpawnSettings)
{
	FLootResultBatch Batch = GenerateLoot(Request);
	
	if (Batch.Results.Num() > 0)
	{
		SpawnLootAtLocation(Batch, SpawnSettings.SpawnLocation, SpawnSettings.ScatterRadius);
	}
	
	return Batch;
}

// ═══════════════════════════════════════════════════════════════════════
// INTERNAL - REGISTRY
// ═══════════════════════════════════════════════════════════════════════

void ULootSubsystem::LoadRegistry()
{
	if (LootSourceRegistryPath.IsNull())
	{
		UE_LOG(LogLootSubsystem, Warning, TEXT("LoadRegistry: No registry path configured"));
		return;
	}

	// P-2 FIX: The original code called LoadSynchronous() during Initialize(),
	// which blocks the game thread at level-load time and causes a hitch.
	// Instead, kick off an async request.  Any GenerateLoot() call that arrives
	// before the registry is ready will log a warning and return an empty batch –
	// a safe graceful-degradation path that avoids stalling the loading screen.
	FStreamableManager& Streamable = UAssetManager::GetStreamableManager();
	RegistryStreamHandle = Streamable.RequestAsyncLoad(
		LootSourceRegistryPath.ToSoftObjectPath(),
		FStreamableDelegate::CreateUObject(this, &ULootSubsystem::OnRegistryLoaded),
		FStreamableManager::DefaultAsyncLoadPriority,
		/*bManageActiveHandle=*/true
	);

	UE_LOG(LogLootSubsystem, Log,
		TEXT("LoadRegistry: Async load requested for '%s'"),
		*LootSourceRegistryPath.ToString());
}

void ULootSubsystem::OnRegistryLoaded()
{
	CachedRegistry = LootSourceRegistryPath.Get();

	if (CachedRegistry)
	{
		UE_LOG(LogLootSubsystem, Log,
			TEXT("OnRegistryLoaded: Registry ready – %d sources"),
			CachedRegistry->GetRowNames().Num());
	}
	else
	{
		UE_LOG(LogLootSubsystem, Error,
			TEXT("OnRegistryLoaded: Asset loaded but cast to UDataTable failed for '%s'"),
			*LootSourceRegistryPath.ToString());
	}

	// Release the handle; the subsystem owns CachedRegistry via UPROPERTY now.
	RegistryStreamHandle.Reset();
}

// ═══════════════════════════════════════════════════════════════════════
// INTERNAL - LOOT TABLE LOADING
// ═══════════════════════════════════════════════════════════════════════

const FLootTable* ULootSubsystem::GetLootTableFromSource(const FLootSourceEntry& Source, FName RowName)
{
	if (Source.LootTable.IsNull())
	{
		UE_LOG(LogLootSubsystem, Warning, TEXT("GetLootTableFromSource: Source has null LootTable reference"));
		return nullptr;
	}
	
	// ═══════════════════════════════════════════════
	// FIX: Use DataTable path as cache key, NOT row name
	// This prevents cache collisions when multiple sources
	// reference different DataTables with the same row names
	// ═══════════════════════════════════════════════
	FName CacheKey = FName(*Source.LootTable.ToString());
	
	UDataTable* CachedTable = nullptr;
	
	if (UDataTable** FoundTable = LootTableCache.Find(CacheKey))
	{
		CachedTable = *FoundTable;
		
		if (!IsValid(CachedTable))
		{
			LootTableCache.Remove(CacheKey);
			CachedTable = nullptr;
		}
	}
	
	if (!CachedTable)
	{
		// N-14 FIX: Prefer the already-resident asset to avoid a synchronous load hitch.
		// LoadSynchronous() stalls the game thread for the full asset load time when the
		// DataTable is not yet in memory.  Check Get() first so the common in-memory case
		// is free.  The synchronous fallback is kept as a safety net but now emits a
		// warning so designers know to pre-load hot loot tables via UAssetManager or a
		// Primary Asset label before the first chest interaction.
		CachedTable = Source.LootTable.Get();
		if (!CachedTable)
		{
			UE_LOG(LogLootSubsystem, Warning,
				TEXT("GetLootTableFromSource: '%s' is not in memory — falling back to synchronous load. "
				     "Pre-load this DataTable to avoid a frame hitch."),
				*Source.LootTable.ToString());
			CachedTable = Source.LootTable.LoadSynchronous();
		}

		if (CachedTable)
		{
			LootTableCache.Add(CacheKey, CachedTable);
			OnLootTableLoaded.Broadcast(RowName, true);

			UE_LOG(LogLootSubsystem, Verbose, TEXT("Cached loot table: %s"), *CacheKey.ToString());
		}
		else
		{
			OnLootTableLoaded.Broadcast(RowName, false);
			UE_LOG(LogLootSubsystem, Error, TEXT("Failed to load loot table: %s"), *Source.LootTable.ToString());
			return nullptr;
		}
	}
	
	if (!RowName.IsNone())
	{
		return CachedTable->FindRow<FLootTable>(RowName, TEXT("GetLootTableFromSource"));
	}
	
	TArray<FName> RowNames = CachedTable->GetRowNames();
	if (RowNames.Num() > 0)
	{
		return CachedTable->FindRow<FLootTable>(RowNames[0], TEXT("GetLootTableFromSource_FirstRow"));
	}
	
	UE_LOG(LogLootSubsystem, Warning, TEXT("GetLootTableFromSource: DataTable has no rows"));
	return nullptr;
}

bool ULootSubsystem::LoadLootTableAsync(const FLootSourceEntry& Source)
{
	const FLootTable* Table = GetLootTableFromSource(Source, Source.LootTableRowName);
	return Table != nullptr;
}

// ═══════════════════════════════════════════════════════════════════════
// INTERNAL - SETTINGS BUILDING
// ═══════════════════════════════════════════════════════════════════════

FLootDropSettings ULootSubsystem::BuildFinalSettings(const FLootSourceEntry& Source, const FLootRequest& Request) const
{
	FLootDropSettings Settings = Source.DefaultSettings;
	
	Settings.SourceLevel = Source.BaseLevel;
	Settings.SourceRarity = Source.SourceRarity;
	
	if (Request.OverrideSettings.MinDrops > 0)
	{
		Settings.MinDrops = Request.OverrideSettings.MinDrops;
	}
	if (Request.OverrideSettings.MaxDrops > 0)
	{
		Settings.MaxDrops = Request.OverrideSettings.MaxDrops;
	}
	if (Request.OverrideSettings.DropChanceMultiplier != 1.0f)
	{
		Settings.DropChanceMultiplier = Request.OverrideSettings.DropChanceMultiplier;
	}
	
	return Settings;
}

FLootDropSettings ULootSubsystem::ApplyGlobalModifiers(const FLootDropSettings& Settings) const
{
	FLootDropSettings Modified = Settings;
	Modified.DropChanceMultiplier *= GlobalDropChanceMultiplier;
	return Modified;
}

FLootDropSettings ULootSubsystem::ApplyPlayerModifiers(const FLootDropSettings& Settings, float Luck, float MagicFind) const
{
	FLootDropSettings Modified = Settings;
	
	// Luck affects RARITY (quality)
	Modified.PlayerLuckBonus = Luck;
	Modified.RarityBonusChance += Luck * 0.005f;
	
	// Magic Find affects QUANTITY
	Modified.PlayerMagicFindBonus = MagicFind;
	Modified.QuantityMultiplier *= (1.0f + MagicFind * 0.01f);
	
	return Modified;
}

// ═══════════════════════════════════════════════════════════════════════
// REGISTRY QUERIES
// ═══════════════════════════════════════════════════════════════════════

bool ULootSubsystem::IsSourceRegistered(FName SourceID) const
{
	if (!CachedRegistry)
	{
		return false;
	}
	
	return CachedRegistry->FindRow<FLootSourceEntry>(SourceID, TEXT("IsSourceRegistered")) != nullptr;
}

bool ULootSubsystem::GetSourceEntry(FName SourceID, FLootSourceEntry& OutEntry) const
{
	if (!CachedRegistry)
	{
		return false;
	}
	
	const FLootSourceEntry* Entry = CachedRegistry->FindRow<FLootSourceEntry>(
		SourceID, TEXT("GetSourceEntry"));
	
	if (Entry)
	{
		OutEntry = *Entry;
		return true;
	}
	
	return false;
}

TArray<FName> ULootSubsystem::GetAllSourceIDs() const
{
	TArray<FName> SourceIDs;
	
	if (!CachedRegistry)
	{
		return SourceIDs;
	}
	
	SourceIDs = CachedRegistry->GetRowNames();
	return SourceIDs;
}

TArray<FName> ULootSubsystem::GetSourceIDsByCategory(ELootSourceType Category) const
{
	TArray<FName> SourceIDs;

	if (!CachedRegistry)
	{
		return SourceIDs;
	}

	// P-1 FIX: GetRowMap() + reinterpret_cast<FLootSourceEntry*> is unsafe –
	// the raw uint8* pointer skips the struct's vtable / alignment guarantees.
	// GetAllRows<T> is the correct typed DataTable API.
	TArray<FLootSourceEntry*> AllRows;
	CachedRegistry->GetAllRows<FLootSourceEntry>(TEXT("GetSourceIDsByCategory"), AllRows);

	// GetAllRows doesn't return the row names alongside the data, so we walk
	// the row names in parallel.  Both arrays are in the same insertion order.
	TArray<FName> AllNames = CachedRegistry->GetRowNames();
	ensure(AllRows.Num() == AllNames.Num());

	for (int32 i = 0; i < AllRows.Num(); ++i)
	{
		if (AllRows[i] && AllRows[i]->Category == Category)
		{
			SourceIDs.Add(AllNames[i]);
		}
	}

	return SourceIDs;
}

// ═══════════════════════════════════════════════════════════════════════
// CACHE MANAGEMENT
// ═══════════════════════════════════════════════════════════════════════

void ULootSubsystem::PreloadLootTables(const TArray<FName>& SourceIDs)
{
	for (const FName& SourceID : SourceIDs)
	{
		FLootSourceEntry Source;
		if (GetSourceEntry(SourceID, Source))
		{
			LoadLootTableAsync(Source);
		}
	}
	
	UE_LOG(LogLootSubsystem, Log, TEXT("Preloading %d loot tables"), SourceIDs.Num());
}

void ULootSubsystem::ClearLootTableCache()
{
	LootTableCache.Empty();
	UE_LOG(LogLootSubsystem, Log, TEXT("Loot table cache cleared"));
}

// ═══════════════════════════════════════════════════════════════════════
// INTERNAL - SUBSYSTEM CACHING
// ═══════════════════════════════════════════════════════════════════════

bool ULootSubsystem::EnsureGroundItemSubsystem()
{
	if (CachedGroundItemSubsystem && IsValid(CachedGroundItemSubsystem))
	{
		return true;
	}
	
	if (CachedWorld)
	{
		CachedGroundItemSubsystem = CachedWorld->GetSubsystem<UGroundItemSubsystem>();
	}
	
	return CachedGroundItemSubsystem != nullptr;
}
