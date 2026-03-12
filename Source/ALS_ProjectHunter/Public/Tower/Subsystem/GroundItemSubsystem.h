// Tower/Subsystem/GroundItemSubsystem.h
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GroundItemSubsystem.generated.h"

// Forward declarations
class UItemInstance;
class AISMContainerActor;
class UInstancedStaticMeshComponent;
class UStaticMesh;

/**
 * Struct to track ISM instance data
 */
USTRUCT()
struct FGroundItemISMData
{
	GENERATED_BODY()

	UPROPERTY()
	UInstancedStaticMeshComponent* ISMComponent = nullptr;

	int32 InstanceIndex = INDEX_NONE;

	UPROPERTY()
	UStaticMesh* Mesh = nullptr;

	FGroundItemISMData() = default;

	FGroundItemISMData(UInstancedStaticMeshComponent* InISM, int32 InIndex, UStaticMesh* InMesh)
		: ISMComponent(InISM)
		, InstanceIndex(InIndex)
		, Mesh(InMesh)
	{}

	bool IsValid() const { return ISMComponent != nullptr && InstanceIndex != INDEX_NONE; }
};

/**
 * UGroundItemSubsystem - Manages items on the ground using ISM
 */
UCLASS()
class ALS_PROJECTHUNTER_API UGroundItemSubsystem : public UWorldSubsystem
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
	// PRIMARY API
	// ═══════════════════════════════════════════════

	UFUNCTION(BlueprintCallable, Category = "Ground Items")
	int32 AddItemToGround(UItemInstance* Item, FVector Location, FRotator Rotation = FRotator::ZeroRotator);

	UFUNCTION(BlueprintCallable, Category = "Ground Items")
	UItemInstance* RemoveItemFromGround(int32 ItemID);

	/**
	 * Remove multiple items efficiently (FIX: batch removal)
	 */
	UFUNCTION(BlueprintCallable, Category = "Ground Items")
	TArray<UItemInstance*> RemoveMultipleItemsFromGround(const TArray<int32>& ItemIDs);

	// ═══════════════════════════════════════════════
	// QUERIES
	// ═══════════════════════════════════════════════

	UFUNCTION(BlueprintPure, Category = "Ground Items")
	UItemInstance* GetItemByID(int32 ItemID) const;

	UFUNCTION(BlueprintCallable, Category = "Ground Items")
	UItemInstance* GetNearestItem(FVector Location, float MaxDistance, int32& OutItemID);

	UFUNCTION(BlueprintCallable, Category = "Ground Items")
	int32 GetItemsInRadius(FVector Location, float Radius, TArray<int32>& OutItemIDs);

	UFUNCTION(BlueprintPure, Category = "Ground Items")
	TArray<UItemInstance*> GetItemInstancesInRadius(FVector Location, float Radius);

	UFUNCTION(BlueprintPure, Category = "Ground Items")
	int32 GetInstanceID(UItemInstance* Item) const;

	UFUNCTION(BlueprintCallable, Category = "Ground Items")
	void UpdateItemLocation(int32 ItemID, FVector NewLocation);

	UFUNCTION(BlueprintCallable, Category = "Ground Items")
	void ClearAllItems();

	// ═══════════════════════════════════════════════
	// ACCESSORS
	// ═══════════════════════════════════════════════

	UFUNCTION(BlueprintPure, Category = "Ground Items")
	int32 GetTotalItemCount() const { return GroundItems.Num(); }

	const TMap<int32, FVector>& GetInstanceLocations() const { return InstanceLocations; }

#if WITH_EDITOR
	UFUNCTION(BlueprintCallable, Category = "Ground Items|Debug")
	void DebugDrawAllItems(float Duration = 5.0f);
#endif

protected:
	// ═══════════════════════════════════════════════
	// ISM MANAGEMENT
	// ═══════════════════════════════════════════════

	void EnsureISMContainerExists();
	UInstancedStaticMeshComponent* GetOrCreateISMComponent(UStaticMesh* Mesh);
	void ReindexAfterRemoval(UInstancedStaticMeshComponent* ISMComponent, int32 RemovedIndex);

private:
	// ═══════════════════════════════════════════════
	// INTERNAL
	// ═══════════════════════════════════════════════

	UItemInstance* RemoveItemFromGroundInternal(int32 ItemID);

	// ═══════════════════════════════════════════════
	// DATA
	// ═══════════════════════════════════════════════

	UPROPERTY()
	AISMContainerActor* ISMContainerActor;

	/*
	 *TMap cannot be used in anything Online related
	 */
	
	UPROPERTY()
	TMap<int32, UItemInstance*> GroundItems;

	UPROPERTY()
	TMap<int32, FVector> InstanceLocations;

	UPROPERTY()
	TMap<int32, FGroundItemISMData> ItemISMData;

	UPROPERTY()
	TMap<UStaticMesh*, UInstancedStaticMeshComponent*> MeshToISM;

	int32 NextItemID = 0;

	// ═══════════════════════════════════════════════
	// THREAD SAFETY 
	// ═══════════════════════════════════════════════

	bool bIsProcessingRemoval = false;
	TArray<int32> PendingRemovals;
};
