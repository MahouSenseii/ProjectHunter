// Private/Systems/Inventory/Helpers/InventoryDropper.cpp
#include "Systems/Inventory/Helpers/InventoryDropper.h"

#include "Core/Logging/ProjectHunterLogMacros.h"
#include "Item/ItemInstance.h"
#include "Systems/Inventory/Components/InventoryManager.h"
#include "Systems/Inventory/Helpers/InventoryRemover.h"
#include "Systems/Inventory/Library/InventoryLog.h"
#include "Tower/Subsystem/GroundItemSubsystem.h"

void FInventoryDropper::DropItem(UInventoryManager* Inventory, UItemInstance* Item, const FVector& DropLocation)
{
	if (!Inventory || !Item)
	{
		return;
	}

	// Remove from inventory first
	if (!FInventoryRemover::RemoveItem(Inventory, Item))
	{
		return;
	}

	// N-05 FIX preserved: defensive null check on World during teardown.
	UWorld* World = Inventory->GetWorld();
	if (!World)
	{
		PH_LOG_WARNING(LogInventoryManager, "DropItem failed: World was null for Item=%s.",
			*Item->GetDisplayName().ToString());
		return;
	}

	if (SpawnGroundItem(World, Item, DropLocation))
	{
		UE_LOG(LogInventoryManager, Log, TEXT("InventoryManager: Dropped %s at %s"),
			*Item->GetDisplayName().ToString(), *DropLocation.ToString());
	}
	else
	{
		PH_LOG_WARNING(LogInventoryManager,
			"DropItem failed: Ground subsystem rejected Item=%s at Location=%s.",
			*Item->GetDisplayName().ToString(), *DropLocation.ToString());
	}
}

void FInventoryDropper::DropItemAtSlot(UInventoryManager* Inventory, int32 SlotIndex, const FVector& DropLocation)
{
	if (!Inventory)
	{
		return;
	}

	UItemInstance* Item = Inventory->GetItemAtSlot(SlotIndex);
	if (Item)
	{
		DropItem(Inventory, Item, DropLocation);
	}
}

bool FInventoryDropper::SpawnGroundItem(UWorld* World, UItemInstance* Item, const FVector& DropLocation)
{
	if (!World || !Item)
	{
		return false;
	}

	UGroundItemSubsystem* GroundSystem = World->GetSubsystem<UGroundItemSubsystem>();
	if (!GroundSystem)
	{
		PH_LOG_WARNING(LogInventoryManager,
			"SpawnGroundItem failed: GroundItemSubsystem unavailable for Item=%s.",
			*Item->GetDisplayName().ToString());
		return false;
	}

	GroundSystem->AddItemToGround(Item, DropLocation);
	return true;
}
