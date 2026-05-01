// Character/Component/GroundItemPickupManager.cpp

#include "Character/Component/Interaction/GroundItemPickupManager.h"
#include "Inventory/Components/InventoryManager.h"
#include "Inventory/InventoryGroundDropResolver.h"
#include "Equipment/Components/EquipmentManager.h"
#include "Equipment/Library/EquipmentFunctionLibrary.h"
#include "Tower/Subsystem/GroundItemSubsystem.h"
#include "Item/ItemInstance.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY(LogGroundItemPickupManager);

namespace GroundItemPickupManagerPrivate
{
	bool IsMainHandWeaponPickup(const UItemInstance* Item)
	{
		return UEquipmentFunctionLibrary::IsOneHandedWeapon(Item)
			&& UEquipmentFunctionLibrary::DetermineSlotForItem(Item) == EEquipmentSlot::ES_MainHand;
	}

	bool TryForceReplaceMainHand(UEquipmentManager* EquipmentManager, UWorld* WorldContext, UItemInstance* Item,
		const FVector& DropLocation, EEquipmentSlot& OutEquippedSlot)
	{
		OutEquippedSlot = EEquipmentSlot::ES_None;

		if (!EquipmentManager || !WorldContext || !Item)
		{
			return false;
		}

		UItemInstance* OldMainHandItem = EquipmentManager->GetEquippedItem(EEquipmentSlot::ES_MainHand);
		if (!OldMainHandItem)
		{
			return false;
		}

		if (!UEquipmentFunctionLibrary::CanGroundPickupEquipToSlot(Item, EEquipmentSlot::ES_MainHand))
		{
			return false;
		}

		UItemInstance* UnequippedMainHandItem = EquipmentManager->UnequipItem(EEquipmentSlot::ES_MainHand, false);
		if (UnequippedMainHandItem != OldMainHandItem)
		{
			if (UnequippedMainHandItem)
			{
				EquipmentManager->EquipItem(UnequippedMainHandItem, EEquipmentSlot::ES_MainHand, false);
			}
			return false;
		}

		EquipmentManager->EquipItem(Item, EEquipmentSlot::ES_MainHand, false);
		if (EquipmentManager->GetEquippedItem(EEquipmentSlot::ES_MainHand) != Item)
		{
			EquipmentManager->EquipItem(UnequippedMainHandItem, EEquipmentSlot::ES_MainHand, false);
			return false;
		}

		if (!FInventoryGroundDropResolver::DropItem(WorldContext, UnequippedMainHandItem, DropLocation))
		{
			EquipmentManager->UnequipItem(EEquipmentSlot::ES_MainHand, false);
			EquipmentManager->EquipItem(UnequippedMainHandItem, EEquipmentSlot::ES_MainHand, false);
			return false;
		}

		OutEquippedSlot = EEquipmentSlot::ES_MainHand;
		return true;
	}
}

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
	const FVector OriginalLocation = CachedGroundItemSubsystem->GetInstanceLocations().FindRef(ItemID);
	const bool bHadOriginalLocation = CachedGroundItemSubsystem->GetInstanceLocations().Contains(ItemID);

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
	const FVector ReturnLocation = bHadOriginalLocation ? OriginalLocation : ClientLocation;
	CachedGroundItemSubsystem->AddItemToGround(Item, ReturnLocation);

	UE_LOG(LogGroundItemPickupManager, Warning, TEXT("GroundItemPickupManager: Inventory full, item returned to ground"));
	return false;
}

bool FGroundItemPickupManager::PickupAndEquipInternal(int32 ItemID, FVector ClientLocation)
{
	const FVector OriginalLocation = CachedGroundItemSubsystem->GetInstanceLocations().FindRef(ItemID);
	const bool bHadOriginalLocation = CachedGroundItemSubsystem->GetInstanceLocations().Contains(ItemID);

	// Remove item from ground
	UItemInstance* Item = CachedGroundItemSubsystem->RemoveItemFromGround(ItemID);
	if (!Item)
	{
		UE_LOG(LogGroundItemPickupManager, Warning, TEXT("GroundItemPickupManager: Item %d not found"), ItemID);
		return false;
	}

	const bool bIsMainHandWeaponPickup = GroundItemPickupManagerPrivate::IsMainHandWeaponPickup(Item);
	EEquipmentSlot EquippedSlot = EEquipmentSlot::ES_None;
	if (CachedEquipmentManager->TryEquipGroundPickupItem(Item, EquippedSlot, true))
	{
		UE_LOG(LogGroundItemPickupManager, Log, TEXT("GroundItemPickupManager: Equipped %s to %s"),
			*Item->GetDisplayName().ToString(), *UEnum::GetValueAsString(EquippedSlot));
		return true;
	}

	if (CachedInventoryManager && CachedInventoryManager->AddItem(Item))
	{
		UE_LOG(LogGroundItemPickupManager, Log, TEXT("GroundItemPickupManager: Sent %s to inventory after equip fallback"),
			*Item->GetDisplayName().ToString());
		return true;
	}

	if (bIsMainHandWeaponPickup
		&& GroundItemPickupManagerPrivate::TryForceReplaceMainHand(CachedEquipmentManager, WorldContext, Item, ClientLocation, EquippedSlot))
	{
		UE_LOG(LogGroundItemPickupManager, Log,
			TEXT("GroundItemPickupManager: Dropped current main hand and equipped %s to %s"),
			*Item->GetDisplayName().ToString(), *UEnum::GetValueAsString(EquippedSlot));
		return true;
	}

	const FVector ReturnLocation = bHadOriginalLocation ? OriginalLocation : ClientLocation;
	CachedGroundItemSubsystem->AddItemToGround(Item, ReturnLocation);

	UE_LOG(LogGroundItemPickupManager, Warning,
		TEXT("GroundItemPickupManager: Could not equip or inventory %s, returned item to ground"),
		*Item->GetDisplayName().ToString());
	return false;
}
