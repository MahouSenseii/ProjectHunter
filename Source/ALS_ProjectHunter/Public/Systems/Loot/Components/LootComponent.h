// Loot/Component/LootComponent.h
// Systems/Loot/Components/LootComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Systems/Loot/Library/LootStructs.h"
#include "LootComponent.generated.h"

// Forward declarations
class ULootSubsystem;

DECLARE_LOG_CATEGORY_EXTERN(LogLootComponent, Log, All);

/**
 * ULootComponent - Per-actor loot configuration
 * 
 * SINGLE RESPONSIBILITY: Hold actor-specific loot config and delegate to subsystem
 * 
 * DESIGN:
 * - Lightweight component (no generation logic)
 * - Stores source ID and spawn settings
 * - Delegates to LootSubsystem for actual generation
 * - Provides convenient actor-level API
 * 
 * USAGE:
 *   // On NPC Blueprint
 *   UPROPERTY()
 *   ULootComponent* LootComp;
 *   
 *   // Set source ID in editor
 *   LootComp->SourceID = "Goblin_Basic";
 *   
 *   // On death
 *   LootComp->DropLoot();
 */
UCLASS(ClassGroup = (Loot), meta = (BlueprintSpawnableComponent))
class ALS_PROJECTHUNTER_API ULootComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULootComponent();

protected:
	virtual void BeginPlay() override;

public:
	// ═══════════════════════════════════════════════
	// CONFIGURATION
	// ═══════════════════════════════════════════════

	/** Source ID in DT_LootSourceRegistry */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot|Config")
	FName SourceID;

	/** Default spawn settings for this actor */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot|Config")
	FLootSpawnSettings DefaultSpawnSettings;

	/** Override source settings? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot|Config")
	bool bUseOverrideSettings = false;

	/** Override drop settings (if bUseOverrideSettings) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot|Config", meta = (EditCondition = "bUseOverrideSettings"))
	FLootDropSettings OverrideSettings;

	/** Level override (0 = use source default) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot|Config", meta = (ClampMin = "0", ClampMax = "100"))
	int32 LevelOverride = 0;

	// ═══════════════════════════════════════════════
	// PRIMARY API
	// ═══════════════════════════════════════════════

	/**
	 * Generate and drop loot at actor location
	 * @param PlayerLuck - Killing player's luck stat
	 * @param PlayerMagicFind - Killing player's magic find stat
	 * @return Generated loot batch
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Loot")
	FLootResultBatch DropLoot(float PlayerLuck = 0.0f, float PlayerMagicFind = 0.0f);

	/**
	 * Generate and drop loot at specific location
	 * @param Location - Where to spawn loot
	 * @param PlayerLuck - Player's luck stat
	 * @param PlayerMagicFind - Player's magic find stat
	 * @return Generated loot batch
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Loot")
	FLootResultBatch DropLootAtLocation(
		FVector Location,
		float PlayerLuck = 0.0f,
		float PlayerMagicFind = 0.0f);

	/**
	 * Generate loot without spawning
	 * @param PlayerLuck - Player's luck stat
	 * @param PlayerMagicFind - Player's magic find stat
	 * @return Generated loot batch
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Loot")
	FLootResultBatch GenerateLoot(float PlayerLuck = 0.0f, float PlayerMagicFind = 0.0f);

	/**
	 * Spawn already-generated loot
	 * @param Results - Loot to spawn
	 * @param Location - Where to spawn (uses actor location if zero)
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Loot")
	void SpawnLoot(const FLootResultBatch& Results, FVector Location = FVector::ZeroVector);

	// ═══════════════════════════════════════════════
	// QUERIES
	// ═══════════════════════════════════════════════

	/**
	 * Check if source is valid
	 * @return True if SourceID exists in registry
	 */
	UFUNCTION(BlueprintPure, Category = "Loot")
	bool IsSourceValid() const;

	/**
	 * Get source entry data
	 * @param OutEntry - Output source entry
	 * @return True if found
	 */
	UFUNCTION(BlueprintPure, Category = "Loot")
	bool GetSourceEntry(FLootSourceEntry& OutEntry) const;

	/**
	 * Get the loot subsystem
	 * @return Loot subsystem or nullptr
	 */
	UFUNCTION(BlueprintPure, Category = "Loot")
	ULootSubsystem* GetLootSubsystem() const;

protected:
	// ═══════════════════════════════════════════════
	// INTERNAL
	// ═══════════════════════════════════════════════

	/** Cached subsystem reference */
	UPROPERTY()
	mutable ULootSubsystem* CachedLootSubsystem = nullptr;

	/** Build loot request from component config */
	FLootRequest BuildRequest(float PlayerLuck, float PlayerMagicFind) const;

	/** Ensure subsystem is cached */
	bool EnsureSubsystem() const;

	bool HasLootAuthority(const TCHAR* FunctionName) const;
};
