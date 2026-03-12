// Character/Component/Interaction/GroundItemPickupManager.h
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

	// ═══════════════════════════════════════════════
	// HOLD INTERACTION
	// ═══════════════════════════════════════════════

	/**
	 * Start hold interaction for an item
	 * @param ItemID - Ground item ID to track
	 */
	void StartHoldInteraction(int32 ItemID);

	/**
	 * Update hold progress
	 * @param DeltaTime - Time since last update
	 * @return True if hold completed (item was equipped)
	 */
	bool UpdateHoldProgress(float DeltaTime);

	/**
	 * Cancel current hold interaction
	 */
	void CancelHoldInteraction();

	// ═══════════════════════════════════════════════
	// GETTERS (Used by InteractionManager)
	// ═══════════════════════════════════════════════

	/** Check if currently holding for ground item pickup */
	FORCEINLINE bool IsHoldingForGroundItem() const { return bIsHoldingForGroundItem; }

	/** Get current hold progress (0.0 - 1.0) */
	FORCEINLINE float GetHoldProgress() const { return HoldProgress; }

	/** Get the item ID being held for */
	FORCEINLINE int32 GetCurrentHoldItemID() const { return CurrentHoldItemID; }

private:
	// ═══════════════════════════════════════════════
	// INTERNAL HELPERS
	// ═══════════════════════════════════════════════

	void CacheComponents();
	bool PickupToInventoryInternal(int32 ItemID, FVector ClientLocation);
	bool PickupAndEquipInternal(int32 ItemID, FVector ClientLocation);
	EEquipmentSlot DetermineEquipmentSlot(UItemInstance* Item) const;

	// ═══════════════════════════════════════════════
	// CACHED REFERENCES
	// ═══════════════════════════════════════════════

	AActor* OwnerActor;
	UWorld* WorldContext;
	UInventoryManager* CachedInventoryManager;
	UEquipmentManager* CachedEquipmentManager;
	UGroundItemSubsystem* CachedGroundItemSubsystem;

	// ═══════════════════════════════════════════════
	// HOLD STATE
	// ═══════════════════════════════════════════════

	bool bIsHoldingForGroundItem;
	int32 CurrentHoldItemID;
	float HoldElapsedTime;
	float HoldProgress;
};
