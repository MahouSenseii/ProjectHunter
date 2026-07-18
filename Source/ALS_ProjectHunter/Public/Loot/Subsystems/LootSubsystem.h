#pragma once

#include "CoreMinimal.h"
#include "Engine/StreamableManager.h"
#include "Loot/Generation/LootGenerator.h"
#include "Loot/Library/Structs/LootStructs.h"
#include "Subsystems/WorldSubsystem.h"
#include "LootSubsystem.generated.h"

class UDataTable;
class UGroundItemSubsystem;
class UItemInstance;

DECLARE_LOG_CATEGORY_EXTERN(LogLootSubsystem, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLootGeneratedDelegate, const FLootResultBatch&, Results, FName, SourceID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnLootSpawnedDelegate, UItemInstance*, Item, FVector, Location, int32, GroundItemID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLootTableLoadedDelegate, FName, SourceID, bool, bSuccess);

UCLASS()
class ALS_PROJECTHUNTER_API ULootSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override { return true; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot|Config")
	TSoftObjectPtr<UDataTable> LootSourceRegistryPath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot|Config")
	float GlobalDropChanceMultiplier = 1.0f;

	UPROPERTY(BlueprintAssignable, Category = "Loot|Events")
	FOnLootGeneratedDelegate OnLootGenerated;

	UPROPERTY(BlueprintAssignable, Category = "Loot|Events")
	FOnLootSpawnedDelegate OnLootSpawned;

	UPROPERTY(BlueprintAssignable, Category = "Loot|Events")
	FOnLootTableLoadedDelegate OnLootTableLoaded;

	UFUNCTION(BlueprintCallable, Category = "Loot|Generation")
	FLootResultBatch GenerateLoot(const FLootRequest& Request);

	UFUNCTION(BlueprintCallable, Category = "Loot|Generation")
	FLootResultBatch GenerateAndSpawnLoot(const FLootRequest& Request, FLootSpawnSettings SpawnSettings);

	UFUNCTION(BlueprintCallable, Category = "Loot|Spawning")
	bool SpawnLootAtLocation(const FLootResultBatch& Batch, FVector Location, float SpreadRadius = 50.0f);

	UFUNCTION(BlueprintCallable, Category = "Loot|Spawning")
	bool SpawnLootWithSettings(const FLootResultBatch& Batch, const FLootSpawnSettings& SpawnSettings);

	UFUNCTION(BlueprintPure, Category = "Loot|Registry")
	bool IsSourceRegistered(FName SourceID) const;

	UFUNCTION(BlueprintPure, Category = "Loot|Registry")
	bool GetSourceEntry(FName SourceID, FLootSourceEntry& OutEntry) const;

	UFUNCTION(BlueprintPure, Category = "Loot|Registry")
	TArray<FName> GetAllSourceIDs() const;

	UFUNCTION(BlueprintPure, Category = "Loot|Registry")
	TArray<FName> GetSourceIDsByCategory(ELootSourceType Category) const;

	UFUNCTION(BlueprintCallable, Category = "Loot|Cache")
	void PreloadLootTables(const TArray<FName>& SourceIDs);

	UFUNCTION(BlueprintCallable, Category = "Loot|Cache")
	void ClearLootTableCache();

	UFUNCTION(BlueprintPure, Category = "Loot|Cache")
	int32 GetCachedTableCount() const { return LootTableCache.Num(); }

protected:
	void LoadRegistry();
	void OnRegistryLoaded();

	const FLootTable* GetLootTableFromSource(const FLootSourceEntry& Source, FName RowName);
	bool LoadLootTableAsync(const FLootSourceEntry& Source);
	bool EnsureGroundItemSubsystem();

	UPROPERTY()
	UDataTable* CachedRegistry = nullptr;

	UPROPERTY()
	TMap<FName, UDataTable*> LootTableCache;

	UPROPERTY()
	UGroundItemSubsystem* CachedGroundItemSubsystem = nullptr;

	UPROPERTY()
	UWorld* CachedWorld = nullptr;

	FLootGenerator LootGenerator;
	TSharedPtr<FStreamableHandle> RegistryStreamHandle;
	TArray<FName> PendingPreloadSourceIDs;
};
