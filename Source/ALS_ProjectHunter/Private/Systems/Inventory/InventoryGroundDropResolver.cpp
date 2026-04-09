#include "Systems/Inventory/InventoryGroundDropResolver.h"

#include "Core/Logging/ProjectHunterLogMacros.h"
#include "Item/ItemInstance.h"
#include "Systems/Inventory/Library/InventoryLog.h"
#include "Tower/Subsystem/GroundItemSubsystem.h"

DEFINE_LOG_CATEGORY(LogInventoryGroundDropResolver);

bool FInventoryGroundDropResolver::DropItem(UWorld* World, UItemInstance* Item, const FVector& DropLocation)
{
	if (!World)
	{
		PH_LOG_WARNING(LogInventoryGroundDropResolver, "DropItem failed: World was null.");
		return false;
	}

	if (!Item)
	{
		PH_LOG_WARNING(LogInventoryGroundDropResolver, "DropItem failed: Item was null.");
		return false;
	}

	UGroundItemSubsystem* GroundSystem = World->GetSubsystem<UGroundItemSubsystem>();
	if (!GroundSystem)
	{
		PH_LOG_WARNING(LogInventoryGroundDropResolver, "DropItem failed: GroundItemSubsystem was unavailable for Item=%s.", *Item->GetDisplayName().ToString());
		return false;
	}

	GroundSystem->AddItemToGround(Item, DropLocation);
	return true;
}
