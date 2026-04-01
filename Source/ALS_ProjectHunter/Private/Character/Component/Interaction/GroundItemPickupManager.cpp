// Character/Component/GroundItemPickupManager.cpp

#include "Character/Component/Interaction/GroundItemPickupManager.h"
#include "Character/Component/InventoryManager.h"
#include "Character/Component/EquipmentManager.h"
#include "Tower/Subsystem/GroundItemSubsystem.h"
#include "Item/ItemInstance.h"
#include "Item/Library/ItemFunctionLibrary.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY(LogGroundItemPickupManager);

FGroundItemPickupManager::FGroundItemPickupManager()
	: PickupRadius(500.0f)
	, HoldToEquipDuration(0.5f)
	, TapHoldThreshold(0.3f)
	, bShowEquipHint(true)
	, OwnerActor(nullptr)
	, WorldContext(nullptr)
	, CachedInventoryManager(nullptr)
	, CachedEquipmentManager(nullptr)
	, CachedGroundItemSubsystem(nullptr)
	, bIsHoldingForGroundItem(false)
	, CurrentHoldItemID(-1)
	, HoldElapsedTime(0.0f)
	, HoldProgress(0.0f)
{
}

void FGroundItemPickupManager::Initialize(AActor* Owner, UWorld* World)
{
	OwnerActor = Owner;
	WorldContext = World;
	
	if (!OwnerActor || !WorldContext)
	{
		UE_LOG(LogGroundItemPickupManager, Error, TEXT("GroundItemPickupManager: Invalid initialization parameters"));
		return;
	}

	CacheComponents();
	
	UE_LOG(LogGroundItemPickupManager, Log, TEXT("GroundItemPickupManager: Initialized for %s"), *OwnerActor->GetName());
}

// ═══════════════════════════════════════════════════════════════════════
// PRIMARY FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════

bool FGroundItemPickupManager::PickupToInventory(int32 ItemID)
{
	if (!CachedGroundItemSubsystem || !CachedInventoryManager)
	{
		UE_LOG(LogGroundItemPickupManager, Warning, TEXT("GroundItemPickupManager: Missing required components"));
		return false;
	}

	if (!OwnerActor)
	{
		return false;
	}

	// OPT-RPC: Authority routing is now handled by InteractionManager's
	// Server RPCs.  By the time this method is called the code is already
	// executing on the authority (or in a single-player context).
	const FVector ClientLocation = OwnerActor->GetActorLocation();
	return PickupToInventoryInternal(ItemID, ClientLocation);
}

bool FGroundItemPickupManager::PickupAndEquip(int32 ItemID)
{
	if (!CachedGroundItemSubsystem || !CachedEquipmentManager)
	{
		UE_LOG(LogGroundItemPickupManager, Warning, TEXT("GroundItemPickupManager: Missing required components"));
		return false;
	}

	if (!OwnerActor)
	{
		return false;
	}

	// OPT-RPC: Same as PickupToInventory — authority routing lives in
	// InteractionManager::Server_PickupAndEquip.
	const FVector ClientLocation = OwnerActor->GetActorLocation();
	return PickupAndEquipInternal(ItemID, ClientLocation);
}

int32 FGroundItemPickupManager::PickupAllNearby(FVector Location)
{
	if (!CachedGroundItemSubsystem || !CachedInventoryManager)
	{
		return 0;
	}

	// Get all items in radius
	
	TArray<UItemInstance*> NearbyItems = CachedGroundItemSubsystem->GetItemInstancesInRadius(Location, PickupRadius);

	int32 PickedUpCount = 0;
	
	for (UItemInstance* Item : NearbyItems)
	{
		if (!Item)
		{
			continue;
		}

		// NOTE: You may need to add GetInstanceID() to your GroundItemSubsystem
		// It should return the int32 ID for a given UItemInstance*
		int32 ItemID = CachedGroundItemSubsystem->GetInstanceID(Item);
		
		if (ItemID == -1)
		{
			UE_LOG(LogGroundItemPickupManager, Warning, TEXT("GroundItemPickupManager: Could not get ID for item %s"), 
				*Item->GetDisplayName().ToString());
			continue;
		}

		// Reuse existing pickup logic (handles validation, removal, etc.)
		if (PickupToInventory(ItemID))
		{
			PickedUpCount++;
		}
	}

	UE_LOG(LogGroundItemPickupManager, Log, TEXT("GroundItemPickupManager: Picked up %d/%d items from area"), 
		PickedUpCount, NearbyItems.Num());
	return PickedUpCount;
}

void FGroundItemPickupManager::StartHoldInteraction(int32 ItemID)
{
	if (bIsHoldingForGroundItem)
	{
		return; // Already holding
	}

	bIsHoldingForGroundItem = true;
	CurrentHoldItemID = ItemID;
	HoldElapsedTime = 0.0f;
	HoldProgress = 0.0f;

	UE_LOG(LogGroundItemPickupManager, Log, TEXT("GroundItemPickupManager: Hold interaction started for item %d"), ItemID);
}

bool FGroundItemPickupManager::UpdateHoldProgress(float DeltaTime)
{
	if (!bIsHoldingForGroundItem)
	{
		return false;
	}

	// Update elapsed time
	HoldElapsedTime += DeltaTime;

	// Calculate progress
	HoldProgress = FMath::Clamp(HoldElapsedTime / HoldToEquipDuration, 0.0f, 1.0f);

	// Check if completed
	if (HoldProgress >= 1.0f)
	{
		// Execute pickup and equip
		PickupAndEquip(CurrentHoldItemID);

		// Reset state
		bIsHoldingForGroundItem = false;
		CurrentHoldItemID = -1;
		HoldElapsedTime = 0.0f;
		HoldProgress = 0.0f;

		UE_LOG(LogGroundItemPickupManager, Log, TEXT("GroundItemPickupManager: Hold completed, item equipped"));
		return true; // Completed
	}

	return false; // Still in progress
}

void FGroundItemPickupManager::CancelHoldInteraction()
{
	if (!bIsHoldingForGroundItem)
	{
		return;
	}

	int32 CancelledItemID = CurrentHoldItemID;

	bIsHoldingForGroundItem = false;
	CurrentHoldItemID = -1;
	HoldElapsedTime = 0.0f;
	HoldProgress = 0.0f;

	UE_LOG(LogGroundItemPickupManager, Log, TEXT("GroundItemPickupManager: Hold interaction cancelled for item %d"), CancelledItemID);
}

// ═══════════════════════════════════════════════════════════════════════
// INTERNAL LOGIC
// ═══════════════════════════════════════════════════════════════════════

void FGroundItemPickupManager::CacheComponents()
{
	if (!OwnerActor)
	{
		return;
	}

	// Cache inventory manager
	CachedInventoryManager = OwnerActor->FindComponentByClass<UInventoryManager>();
	if (!CachedInventoryManager)
	{
		UE_LOG(LogGroundItemPickupManager, Warning, TEXT("GroundItemPickupManager: No InventoryManager found"));
	}

	// Cache equipment manager
	CachedEquipmentManager = OwnerActor->FindComponentByClass<UEquipmentManager>();
	if (!CachedEquipmentManager)
	{
		UE_LOG(LogGroundItemPickupManager, Warning, TEXT("GroundItemPickupManager: No EquipmentManager found"));
	}

	// Cache ground item subsystem
	if (WorldContext)
	{
		CachedGroundItemSubsystem = WorldContext->GetSubsystem<UGroundItemSubsystem>();
		if (!CachedGroundItemSubsystem)
		{
			UE_LOG(LogGroundItemPickupManager, Warning, TEXT("GroundItemPickupManager: No GroundItemSubsystem found"));
		}
	}
}

bool FGroundItemPickupManager::PickupToInventoryInternal(int32 ItemID, FVector ClientLocation)
{
	// Remove item from ground
	UItemInstance* Item = CachedGroundItemSubsystem->RemoveItemFromGround(ItemID);
	if (!Item)
	{
		UE_LOG(LogGroundItemPickupManager, Warning, TEXT("GroundItemPickupManager: Item %d not found"), ItemID);
		return false;
	}

	// Add to inventory
	if (CachedInventoryManager->AddItem(Item))
	{
		UE_LOG(LogGroundItemPickupManager, Log, TEXT("GroundItemPickupManager: Picked up %s to inventory"), *Item->GetDisplayName().ToString());
		return true;
	}

	// Failed to add - return to ground
	const FVector* OriginalLocation = CachedGroundItemSubsystem->GetInstanceLocations().Find(ItemID);
	if (OriginalLocation)
	{
		CachedGroundItemSubsystem->AddItemToGround(Item, *OriginalLocation);
	}

	UE_LOG(LogGroundItemPickupManager, Warning, TEXT("GroundItemPickupManager: Inventory full, item returned to ground"));
	return false;
}

bool FGroundItemPickupManager::PickupAndEquipInternal(int32 ItemID, FVector ClientLocation)
{
	// Remove item from ground
	UItemInstance* Item = CachedGroundItemSubsystem->RemoveItemFromGround(ItemID);
	if (!Item)
	{
		UE_LOG(LogGroundItemPickupManager, Warning, TEXT("GroundItemPickupManager: Item %d not found"), ItemID);
		return false;
	}

	// Determine equipment slot
	EEquipmentSlot TargetSlot = DetermineEquipmentSlot(Item);
	if (TargetSlot == EEquipmentSlot::ES_None)
	{
		UE_LOG(LogGroundItemPickupManager, Warning, TEXT("GroundItemPickupManager: Cannot determine equipment slot for %s"), 
			*Item->GetDisplayName().ToString());
		
		// Fallback: try inventory instead
		if (CachedInventoryManager->AddItem(Item))
		{
			return true;
		}
		return false;
	}

	// Try to equip (will swap to inventory if slot occupied and bSwapToBag = true)
	CachedEquipmentManager->EquipItem(Item, TargetSlot, true);
	
	UE_LOG(LogGroundItemPickupManager, Log, TEXT("GroundItemPickupManager: Equipped %s to %s"), 
		*Item->GetDisplayName().ToString(), *UEnum::GetValueAsString(TargetSlot));
	
	return true;
}

EEquipmentSlot FGroundItemPickupManager::DetermineEquipmentSlot(UItemInstance* Item) const
{
	if (!Item)
	{
		return EEquipmentSlot::ES_None;
	}

	// Use ItemFunctionLibrary to determine slot
	return Item->GetEquipmentSlot();
}
