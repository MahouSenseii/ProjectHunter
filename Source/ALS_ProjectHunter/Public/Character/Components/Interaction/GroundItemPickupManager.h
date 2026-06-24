// Character/Components/Interaction/GroundItemPickupManager.h
#pragma once

#include "CoreMinimal.h"
#include "Item/Library/ItemEnums.h"
#include "GroundItemPickupManager.generated.h"

// Forward declarations
class UItemInstance;
class UInventoryManager;
class UEquipmentManager;
class UGroundItemSubsystem;

DECLARE_LOG_CATEGORY_EXTERN(LogGroundItemPickupManager, Log, All);

/**
 * Ground Item Pickup Manager
 * 
 * SINGLE RESPONSIBILITY: Handle ground item pickup logic
 * - Pickup to inventory (tap)
 * - Pickup and equip (hold)
 * - Pickup all nearby
 * - Hold progress tracking
 */
USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FGroundItemPickupManager
{
	GENERATED_BODY()

public:
	FGroundItemPickupManager();

	// ═══════════════════════════════════════════════
	// INITIALIZATION
	// ═══════════════════════════════════════════════

	void Initialize(AActor* Owner, UWorld* World);

	// ═══════════════════════════════════════════════
	// CONFIGURATION
	// ═══════════════════════════════════════════════

	/** Radius for "pickup all nearby" functionality */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup")
	float PickupRadius;

	/** Duration to hold for equip action */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup")
	float HoldToEquipDuration;

	/** Buffer before hold starts; releasing before this is treated as tap */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float TapHoldThreshold;

	/** Show equip hint when hovering over equippable items */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup")
	bool bShowEquipHint;

	// ═══════════════════════════════════════════════
	// PRIMARY FUNCTIONS
	// ═══════════════════════════════════════════════

	/**
	 * Pickup item to inventory (tap action)
	 * @param ItemID - Ground item ID
	 * @return True if pickup successful
	 */
	bool PickupToInventory(int32 ItemID);

	/**
	 * Pickup item and equip immediately (hold action)
	 * @param ItemID - Ground item ID
	 * @return True if pickup and equip successful
	 */
	bool PickupAndEquip(int32 ItemID);

	/**
	 * Pickup all items in radius around location
	 * @param Location - Center point for pickup
	 * @return Number of items picked up
	 */
	int32 PickupAllNearby(FVector Location);

	// Hold progress is driven entirely by FActiveInteraction in UInteractionManager.
	// FGroundItemPickupManager only executes the pickup action (PickupToInventory /
	// PickupAndEquip) — it does not own any hold-timing state.

private:
	// ═══════════════════════════════════════════════
	// INTERNAL HELPERS
	// ═══════════════════════════════════════════════

	void CacheComponents();
	bool PickupToInventoryInternal(int32 ItemID, FVector ClientLocation);
	bool PickupAndEquipInternal(int32 ItemID, FVector ClientLocation);

	// ═══════════════════════════════════════════════
	// CACHED REFERENCES
	// ═══════════════════════════════════════════════

	AActor* OwnerActor;
	UWorld* WorldContext;
	UInventoryManager* CachedInventoryManager;
	UEquipmentManager* CachedEquipmentManager;
	UGroundItemSubsystem* CachedGroundItemSubsystem;

};
