#include "Loot/Components/LootComponent.h"
#include "Loot/Subsystems/LootSubsystem.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY(LogLootComponent);

ULootComponent::ULootComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	DefaultSpawnSettings.ScatterRadius = 100.0f;
	DefaultSpawnSettings.HeightOffset = 50.0f;
	DefaultSpawnSettings.bRandomScatter = true;
}

void ULootComponent::BeginPlay()
{
	Super::BeginPlay();

	EnsureSubsystem();

	if (!SourceID.IsNone() && !IsSourceValid())
	{
		UE_LOG(LogLootComponent, Warning, TEXT("LootComponent on '%s': Source '%s' not found in registry"),
			*GetOwner()->GetName(), *SourceID.ToString());
	}

	UE_LOG(LogLootComponent, Log, TEXT("LootComponent initialized on '%s' with source '%s'"),
		*GetOwner()->GetName(), *SourceID.ToString());
}

FLootResultBatch ULootComponent::DropLoot(float PlayerLuck, float PlayerMagicFind)
{
	if (!HasLootAuthority(TEXT("DropLoot")))
	{
		return FLootResultBatch();
	}

	return DropLootAtLocation(GetOwner()->GetActorLocation(), PlayerLuck, PlayerMagicFind);
}

FLootResultBatch ULootComponent::DropLootAtLocation(
	FVector Location,
	float PlayerLuck,
	float PlayerMagicFind)
{
	if (!HasLootAuthority(TEXT("DropLootAtLocation")))
	{
		return FLootResultBatch();
	}

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

	FLootSpawnSettings SpawnSettings = DefaultSpawnSettings;
	if (!SpawnSettings.bUseSpawnBox)
	{
		SpawnSettings.SpawnLocation = Location;
	}

	FLootRequest Request = BuildRequest(PlayerLuck, PlayerMagicFind);
	return CachedLootSubsystem->GenerateAndSpawnLoot(Request, SpawnSettings);
}

FLootResultBatch ULootComponent::GenerateLoot(float PlayerLuck, float PlayerMagicFind)
{
	if (!HasLootAuthority(TEXT("GenerateLoot")))
	{
		return FLootResultBatch();
	}

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
	if (!HasLootAuthority(TEXT("SpawnLoot")))
	{
		return;
	}

	if (!EnsureSubsystem())
	{
		UE_LOG(LogLootComponent, Error, TEXT("SpawnLoot: LootSubsystem unavailable"));
		return;
	}

	FLootSpawnSettings SpawnSettings = DefaultSpawnSettings;

	if (Location.IsZero())
	{
		SpawnSettings.SpawnLocation = GetOwner()->GetActorLocation();
	}
	else
	{
		SpawnSettings.SpawnLocation = Location;
	}

	CachedLootSubsystem->SpawnLootWithSettings(Results, SpawnSettings);
}

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
	// A raw pointer truthiness check survives level transitions because the UWorldSubsystem
	// is torn down (Pending Kill) while the pointer still points to valid memory.
	// IsValid() catches the Pending-Kill state.
	if (IsValid(CachedLootSubsystem))
	{
		return true;
	}

	CachedLootSubsystem = nullptr;

	if (UWorld* World = GetWorld())
	{
		CachedLootSubsystem = World->GetSubsystem<ULootSubsystem>();
	}

	return CachedLootSubsystem != nullptr;
}

bool ULootComponent::HasLootAuthority(const TCHAR* FunctionName) const
{
	const AActor* OwnerActor = GetOwner();
	if (OwnerActor && OwnerActor->HasAuthority())
	{
		return true;
	}

	UE_LOG(LogLootComponent, Warning,
		TEXT("%s rejected: loot generation and spawning must run on the server for Owner=%s."),
		FunctionName ? FunctionName : TEXT("LootOperation"),
		*GetNameSafe(OwnerActor));
	return false;
}
