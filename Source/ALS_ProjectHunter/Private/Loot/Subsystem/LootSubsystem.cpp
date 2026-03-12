// Loot/Subsystem/LootSubsystem.cpp

#include "Loot/Subsystem/LootSubsystem.h"
#include "Tower/Subsystem/GroundItemSubsystem.h"
#include "Item/ItemInstance.h"
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
	
	CachedRegistry = LootSourceRegistryPath.LoadSynchronous();
	
	if (CachedRegistry)
	{
		UE_LOG(LogLootSubsystem, Log, TEXT("Loaded loot registry with %d sources"), 
			CachedRegistry->GetRowNames().Num());
	}
	else
	{
		UE_LOG(LogLootSubsystem, Error, TEXT("Failed to load loot registry from '%s'"),
			*LootSourceRegistryPath.ToString());
	}
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
		CachedTable = Source.LootTable.LoadSynchronous();
		
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
	
	const TMap<FName, uint8*>& RowMap = CachedRegistry->GetRowMap();
	
	for (const TPair<FName, uint8*>& Pair : RowMap)
	{
		const FLootSourceEntry* Entry = reinterpret_cast<FLootSourceEntry*>(Pair.Value);
		if (Entry && Entry->Category == Category)
		{
			SourceIDs.Add(Pair.Key);
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
