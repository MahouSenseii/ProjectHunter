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
	if (UEquipmentFunctionLibrary::IsItemCompatibleWithSlot(Item, Slot))
	{
		return true;
	}

	// Allow a one-handed weapon to be placed into the off hand via the
	// normal (non-ground-pickup) EquipItem() path. Without this, any UI
	// drag-drop onto the off-hand slot or any BP/code call of the form
	// EquipmentManager::EquipItem(sword, ES_OffHand) is silently rejected
	// by EquipItemInternal because the sword's canonical slot is MainHand.
	if (Slot == EEquipmentSlot::ES_OffHand && UEquipmentFunctionLibrary::IsOneHandedWeapon(Item))
	{
		return true;
	}

	return false;
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
		// Dual-wield-friendly priority:
		//   1. Empty main hand  -> main hand
		//   2. Empty off hand   -> off hand   (this was unreachable before)
		//   3. Swap main hand into the bag (displacement) -> main hand
		//   4. Swap off hand into the bag -> off hand
		// Previously we checked CanEquipGroundPickupToSlot(MainHand) first,
		// which returns true whenever the bag has room to absorb the old
		// main hand item. That meant a 1H weapon *always* displaced main
		// hand and the off-hand branch was effectively dead code whenever
		// the player had any free bag space.
		const bool bMainHandEmpty = !Manager.IsSlotOccupied(EEquipmentSlot::ES_MainHand);
		const bool bOffHandEmpty  = !Manager.IsSlotOccupied(EEquipmentSlot::ES_OffHand);

		// Two-handed weapon currently equipped blocks either hand slot
		// regardless, because CanEquipGroundPickupToSlot will still need
		// to swap the two-hand item to the bag. Keep that check via the
		// helper below.
		auto CanUseSlot = [&](EEquipmentSlot Slot) -> bool
		{
			return EquipmentSlotResolverPrivate::CanEquipGroundPickupToSlot(Manager, Item, Slot, bSwapToBag);
		};

		if (bMainHandEmpty && CanUseSlot(EEquipmentSlot::ES_MainHand))
		{
			ChosenSlot = EEquipmentSlot::ES_MainHand;
		}
		else if (bOffHandEmpty && CanUseSlot(EEquipmentSlot::ES_OffHand))
		{
			ChosenSlot = EEquipmentSlot::ES_OffHand;
		}
		else if (CanUseSlot(EEquipmentSlot::ES_MainHand))
		{
			// Both hands occupied (or main hand occupied + off hand
			// unusable): displace main hand to bag.
			ChosenSlot = EEquipmentSlot::ES_MainHand;
		}
		else if (CanUseSlot(EEquipmentSlot::ES_OffHand))
		{
			// Last resort: displace off hand to bag.
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
