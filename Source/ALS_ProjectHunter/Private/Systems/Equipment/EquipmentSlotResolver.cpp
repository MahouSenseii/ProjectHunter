#include "Systems/Equipment/EquipmentSlotResolver.h"

#include "Core/Logging/ProjectHunterLogMacros.h"
#include "Systems/Equipment/Components/EquipmentManager.h"
#include "Systems/Equipment/EquipmentMutationHelper.h"
#include "Systems/Equipment/Library/EquipmentFunctionLibrary.h"
#include "Systems/Inventory/Components/InventoryManager.h"
#include "Item/ItemInstance.h"

namespace EquipmentSlotResolverPrivate
{
	UInventoryManager* ResolveInventoryManager(const UEquipmentManager& Manager)
	{
		AActor* Owner = Manager.GetOwner();
		return Owner ? Owner->FindComponentByClass<UInventoryManager>() : nullptr;
	}

	bool CanMoveDisplacedItemToBag(const UEquipmentManager& Manager, UItemInstance* Item, bool bSwapToBag)
	{
		if (!Item || !bSwapToBag)
		{
			return true;
		}

		if (UInventoryManager* InventoryManager = ResolveInventoryManager(Manager))
		{
			return InventoryManager->CanAddItem(Item);
		}

		return false;
	}

	bool CanEquipGroundPickupToSlot(const UEquipmentManager& Manager, UItemInstance* Item, EEquipmentSlot Slot, bool bSwapToBag)
	{
		if (!Item || Slot == EEquipmentSlot::ES_None)
		{
			return false;
		}

		if (!UEquipmentFunctionLibrary::CanGroundPickupEquipToSlot(Item, Slot))
		{
			return false;
		}

		if (Slot == EEquipmentSlot::ES_TwoHand)
		{
			return CanMoveDisplacedItemToBag(Manager, Manager.GetEquippedItem(EEquipmentSlot::ES_MainHand), bSwapToBag)
				&& CanMoveDisplacedItemToBag(Manager, Manager.GetEquippedItem(EEquipmentSlot::ES_OffHand), bSwapToBag)
				&& CanMoveDisplacedItemToBag(Manager, Manager.GetEquippedItem(EEquipmentSlot::ES_TwoHand), bSwapToBag);
		}

		if (!CanMoveDisplacedItemToBag(Manager, Manager.GetEquippedItem(EEquipmentSlot::ES_TwoHand), bSwapToBag))
		{
			return false;
		}

		return CanMoveDisplacedItemToBag(Manager, Manager.GetEquippedItem(Slot), bSwapToBag);
	}
}

EEquipmentSlot FEquipmentSlotResolver::DetermineEquipmentSlot(const UEquipmentManager& Manager, UItemInstance* Item)
{
	if (!Item)
	{
		return EEquipmentSlot::ES_None;
	}

	const EEquipmentSlot CanonicalSlot = UEquipmentFunctionLibrary::DetermineSlotForItem(Item);
	if (CanonicalSlot == EEquipmentSlot::ES_Ring1)
	{
		return GetNextAvailableRingSlot(Manager);
	}

	return CanonicalSlot;
}

bool FEquipmentSlotResolver::CanEquipToSlot(const UEquipmentManager& Manager, UItemInstance* Item, EEquipmentSlot Slot)
{
	return UEquipmentFunctionLibrary::IsItemCompatibleWithSlot(Item, Slot);
}

bool FEquipmentSlotResolver::TryEquipGroundPickupItem(UEquipmentManager& Manager, UItemInstance* Item, EEquipmentSlot& OutEquippedSlot, bool bSwapToBag)
{
	OutEquippedSlot = EEquipmentSlot::ES_None;

	if (!Item)
	{
		return false;
	}

	if (!Manager.GetOwner() || !Manager.GetOwner()->HasAuthority())
	{
		PH_LOG_WARNING(LogEquipmentManager, "TryEquipGroundPickupItem failed: Must be called on the server.");
		return false;
	}

	const EEquipmentSlot CanonicalSlot = UEquipmentFunctionLibrary::DetermineSlotForItem(Item);
	if (CanonicalSlot == EEquipmentSlot::ES_None)
	{
		return false;
	}

	EEquipmentSlot ChosenSlot = DetermineEquipmentSlot(Manager, Item);

	if (UEquipmentFunctionLibrary::IsOneHandedWeapon(Item))
	{
		if (EquipmentSlotResolverPrivate::CanEquipGroundPickupToSlot(Manager, Item, EEquipmentSlot::ES_MainHand, bSwapToBag))
		{
			ChosenSlot = EEquipmentSlot::ES_MainHand;
		}
		else if (!Manager.IsSlotOccupied(EEquipmentSlot::ES_OffHand)
			&& EquipmentSlotResolverPrivate::CanEquipGroundPickupToSlot(Manager, Item, EEquipmentSlot::ES_OffHand, bSwapToBag))
		{
			ChosenSlot = EEquipmentSlot::ES_OffHand;
		}
		else
		{
			return false;
		}
	}
	else if (CanonicalSlot == EEquipmentSlot::ES_OffHand)
	{
		ChosenSlot = EEquipmentSlot::ES_OffHand;
	}

	if (ChosenSlot == EEquipmentSlot::ES_None)
	{
		return false;
	}

	if (!EquipmentSlotResolverPrivate::CanEquipGroundPickupToSlot(Manager, Item, ChosenSlot, bSwapToBag))
	{
		return false;
	}

	FEquipmentMutationHelper::EquipItemInternal(Manager, Item, ChosenSlot, bSwapToBag, true);

	if (Manager.GetEquippedItem(ChosenSlot) != Item)
	{
		return false;
	}

	OutEquippedSlot = ChosenSlot;
	return true;
}

EEquipmentSlot FEquipmentSlotResolver::GetNextAvailableRingSlot(const UEquipmentManager& Manager)
{
	for (int32 i = 0; i < Manager.MaxRingSlots; ++i)
	{
		const EEquipmentSlot RingSlot = static_cast<EEquipmentSlot>(static_cast<int32>(EEquipmentSlot::ES_Ring1) + i);
		if (!Manager.IsSlotOccupied(RingSlot))
		{
			return RingSlot;
		}
	}

	return EEquipmentSlot::ES_None;
}

bool FEquipmentSlotResolver::IsRingSlot(EEquipmentSlot Slot)
{
	return Slot >= EEquipmentSlot::ES_Ring1 && Slot <= EEquipmentSlot::ES_Ring10;
}
