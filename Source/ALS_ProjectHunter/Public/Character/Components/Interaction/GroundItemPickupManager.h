#pragma once

#include "CoreMinimal.h"
#include "Item/Library/Enums/ItemEnums.h"
#include "GroundItemPickupManager.generated.h"
class UItemInstance;
class UInventoryManager;
class UEquipmentManager;
class UGroundItemSubsystem;

DECLARE_LOG_CATEGORY_EXTERN(LogGroundItemPickupManager, Log, All);

// Executes ground-item pickup commands for UInteractionManager; hold timing stays on FActiveInteraction.
USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FGroundItemPickupManager
{
	GENERATED_BODY()

public:
	FGroundItemPickupManager();

	void Initialize(AActor* Owner, UWorld* World);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup")
	float PickupRadius;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup")
	float HoldToEquipDuration;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float TapHoldThreshold;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup")
	bool bShowEquipHint;

	bool PickupToInventory(int32 ItemID);

	bool PickupAndEquip(int32 ItemID);

	int32 PickupAllNearby(FVector Location);

private:
	void CacheComponents();
	bool PickupToInventoryInternal(int32 ItemID, FVector ClientLocation);
	bool PickupAndEquipInternal(int32 ItemID, FVector ClientLocation);

	AActor* OwnerActor;
	UWorld* WorldContext;
	UInventoryManager* CachedInventoryManager;
	UEquipmentManager* CachedEquipmentManager;
	UGroundItemSubsystem* CachedGroundItemSubsystem;

};
