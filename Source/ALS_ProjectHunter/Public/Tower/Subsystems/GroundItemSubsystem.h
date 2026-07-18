#pragma once

#include "CoreMinimal.h"
#include "HAL/CriticalSection.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tower/Library/Structs/GroundItemStructs.h"
#include "GroundItemSubsystem.generated.h"

class AISMContainerActor;
class UInstancedStaticMeshComponent;
class UItemInstance;
class UStaticMesh;

DECLARE_LOG_CATEGORY_EXTERN(LogGroundItemSubsystem, Log, All);

UCLASS()
class ALS_PROJECTHUNTER_API UGroundItemSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override { return true; }

	UFUNCTION(BlueprintCallable, Category = "Ground Items")
	int32 AddItemToGround(UItemInstance* Item, FVector Location, FRotator Rotation = FRotator::ZeroRotator);

	UFUNCTION(BlueprintCallable, Category = "Ground Items")
	UItemInstance* RemoveItemFromGround(int32 ItemID);

	UFUNCTION(BlueprintCallable, Category = "Ground Items")
	TArray<UItemInstance*> RemoveMultipleItemsFromGround(const TArray<int32>& ItemIDs);

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

	int32 FindItemByISMInstance(UInstancedStaticMeshComponent* ISMComponent, int32 InstanceIndex) const;

	UFUNCTION(BlueprintCallable, Category = "Ground Items")
	void UpdateItemLocation(int32 ItemID, FVector NewLocation);

	UFUNCTION(BlueprintCallable, Category = "Ground Items")
	void ClearAllItems();

	UFUNCTION(BlueprintPure, Category = "Ground Items")
	int32 GetTotalItemCount() const { return GroundItems.Num(); }

	const TMap<int32, FVector>& GetInstanceLocations() const { return InstanceLocations; }

#if WITH_EDITOR
	UFUNCTION(BlueprintCallable, Category = "Ground Items|Debug")
	void DebugDrawAllItems(float Duration = 5.0f);
#endif

protected:
	void EnsureISMContainerExists();
	UInstancedStaticMeshComponent* GetOrCreateISMComponent(UStaticMesh* Mesh);
	void UpdateIndexAfterSwap(UInstancedStaticMeshComponent* ISMComponent, int32 RemovedIndex, int32 LastIndex);

private:
	UItemInstance* RemoveItemFromGroundInternal(int32 ItemID);

	UPROPERTY()
	TWeakObjectPtr<AISMContainerActor> ISMContainerActor;

	UPROPERTY()
	TMap<int32, UItemInstance*> GroundItems;

	UPROPERTY()
	TMap<int32, FVector> InstanceLocations;

	UPROPERTY()
	TMap<int32, FGroundItemISMData> ItemISMData;

	UPROPERTY()
	TMap<UStaticMesh*, UInstancedStaticMeshComponent*> MeshToISM;

	UPROPERTY()
	TMap<UItemInstance*, int32> InstanceToIDMap;

	int32 NextItemID = 0;
	bool bIsProcessingRemoval = false;
	FCriticalSection PendingRemovalsCS;
	TArray<int32> PendingRemovals;
};
