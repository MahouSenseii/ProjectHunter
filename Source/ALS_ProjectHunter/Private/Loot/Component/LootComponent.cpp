// Loot/Component/LootComponent.cpp

#include "Loot/Component/LootComponent.h"
#include "Loot/Subsystem/LootSubsystem.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY(LogLootComponent);

ULootComponent::ULootComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	
	// Default spawn settings
	DefaultSpawnSettings.ScatterRadius = 100.0f;
	DefaultSpawnSettings.HeightOffset = 50.0f;
	DefaultSpawnSettings.bRandomScatter = true;
}

void ULootComponent::BeginPlay()
{
	Super::BeginPlay();
	
	// Cache subsystem
	EnsureSubsystem();

		
	// Validate source
	if (!SourceID.IsNone() && !IsSourceValid())
	{
		UE_LOG(LogLootComponent, Warning, TEXT("LootComponent on '%s': Source '%s' not found in registry"),
			*GetOwner()->GetName(), *SourceID.ToString());
	}
	
	UE_LOG(LogLootComponent, Log, TEXT("LootComponent initialized on '%s' with source '%s'"),
		*GetOwner()->GetName(), *SourceID.ToString());
}

// ═══════════════════════════════════════════════════════════════════════
// PRIMARY API
// ═══════════════════════════════════════════════════════════════════════

FLootResultBatch ULootComponent::DropLoot(float PlayerLuck, float PlayerMagicFind)
{
	return DropLootAtLocation(GetOwner()->GetActorLocation(), PlayerLuck, PlayerMagicFind);
}

FLootResultBatch ULootComponent::DropLootAtLocation(
	FVector Location,
	float PlayerLuck,
	float PlayerMagicFind)
{
	if (!EnsureSubsystem())
	{
		UE_LOG(LogLootComponent, Error, TEXT("DropLoot: LootSubsystem unavailable"));
		return FLootResultBatch();
	}
	
	if (SourceID.IsNone())
	{
		UE_LOG(LogLootComponent, Warning, TEXT("DropLoot: No SourceID configured"));
		return FLootResultBatch();
	}
	
	// Build spawn settings
	FLootSpawnSettings SpawnSettings = DefaultSpawnSettings;
	SpawnSettings.SpawnLocation = Location;
	
	// Build and execute a request
	FLootRequest Request = BuildRequest(PlayerLuck, PlayerMagicFind);
	return CachedLootSubsystem->GenerateAndSpawnLoot(Request, SpawnSettings);
}

FLootResultBatch ULootComponent::GenerateLoot(float PlayerLuck, float PlayerMagicFind)
{
	if (!EnsureSubsystem())
	{
		UE_LOG(LogLootComponent, Error, TEXT("GenerateLoot: LootSubsystem unavailable"));
		return FLootResultBatch();
	}
	
	if (SourceID.IsNone())
	{
		UE_LOG(LogLootComponent, Warning, TEXT("GenerateLoot: No SourceID configured"));
		return FLootResultBatch();
	}
	
	FLootRequest Request = BuildRequest(PlayerLuck, PlayerMagicFind);
	
	return CachedLootSubsystem->GenerateLoot(Request);
}

void ULootComponent::SpawnLoot(const FLootResultBatch& Results, FVector Location)
{
	if (!EnsureSubsystem())
	{
		UE_LOG(LogLootComponent, Error, TEXT("SpawnLoot: LootSubsystem unavailable"));
		return;
	}
	
	FLootSpawnSettings SpawnSettings = DefaultSpawnSettings;
	
	// Use provided location or actor location
	if (Location.IsZero())
	{
		SpawnSettings.SpawnLocation = GetOwner()->GetActorLocation();
	}
	else
	{
		SpawnSettings.SpawnLocation = Location;
	}
	
	CachedLootSubsystem->SpawnLootAtLocation(Results, SpawnSettings.SpawnLocation, SpawnSettings.ScatterRadius);
}

// ═══════════════════════════════════════════════════════════════════════
// QUERIES
// ═══════════════════════════════════════════════════════════════════════

bool ULootComponent::IsSourceValid() const
{
	if (!EnsureSubsystem() || SourceID.IsNone())
	{
		return false;
	}
	
	return CachedLootSubsystem->IsSourceRegistered(SourceID);
}

bool ULootComponent::GetSourceEntry(FLootSourceEntry& OutEntry) const
{
	if (!EnsureSubsystem() || SourceID.IsNone())
	{
		return false;
	}
	
	return CachedLootSubsystem->GetSourceEntry(SourceID, OutEntry);
}

ULootSubsystem* ULootComponent::GetLootSubsystem() const
{
	EnsureSubsystem();
	return CachedLootSubsystem;
}

// ═══════════════════════════════════════════════════════════════════════
// INTERNAL
// ═══════════════════════════════════════════════════════════════════════

FLootRequest ULootComponent::BuildRequest(float PlayerLuck, float PlayerMagicFind) const
{
	FLootRequest Request(SourceID);
	
	Request.PlayerLuck = PlayerLuck;
	Request.PlayerMagicFind = PlayerMagicFind;
	Request.OverrideLevel = LevelOverride;
	Request.bUseOverrideSettings = bUseOverrideSettings;
	
	if (bUseOverrideSettings)
	{
		Request.OverrideSettings = OverrideSettings;
	}
	
	return Request;
}

bool ULootComponent::EnsureSubsystem() const
{
	if (CachedLootSubsystem)
	{
		return true;
	}
	
	if (UWorld* World = GetWorld())
	{
		CachedLootSubsystem = World->GetSubsystem<ULootSubsystem>();
	}
	
	return CachedLootSubsystem != nullptr;
}