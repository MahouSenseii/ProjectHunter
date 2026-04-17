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

	// Linked-hand fallback for one-handed weapons:
	//   MainHand canonical → try MainHand first, then OffHand, then displace MainHand.
	//   OffHand  canonical → try OffHand  first, then MainHand, then displace OffHand.
	// Dedicated off-hand-only items (shields, etc.) are not 1H weapons so they
	// skip this block and always land on their canonical slot.
	if (UEquipmentFunctionLibrary::IsOneHandedWeapon(Item)
		&& (CanonicalSlot == EEquipmentSlot::ES_MainHand || CanonicalSlot == EEquipmentSlot::ES_OffHand))
	{
		const EEquipmentSlot FallbackSlot = (CanonicalSlot == EEquipmentSlot::ES_MainHand)
			? EEquipmentSlot::ES_OffHand
			: EEquipmentSlot::ES_MainHand;

		if (!Manager.IsSlotOccupied(CanonicalSlot))
		{
			return CanonicalSlot;
		}
		if (!Manager.IsSlotOccupied(FallbackSlot))
		{
			return FallbackSlot;
		}
		// Both slots occupied — displace the canonical slot; old item goes to inventory.
		return CanonicalSlot;
	}

	return CanonicalSlot;
}

bool FEquipmentSlotResolver::CanEquipToSlot(const UEquipmentManager& Manager, UItemInstance* Item, EEquipmentSlot Slot)
{
	if (UEquipmentFunctionLibrary::IsItemCompatibleWithSlot(Item, Slot))
	{
		return true;
	}

	// Allow a one-handed weapon to be placed into either hand via the normal
	// (non-ground-pickup) EquipItem() path. Both rules are needed for the
	// linked-hand fallback in DetermineEquipmentSlot: without the MainHand
	// rule an OffHand-canonical 1H weapon redirected to MainHand would be
	// silently rejected here.
	if (UEquipmentFunctionLibrary::IsOneHandedWeapon(Item)
		&& (Slot == EEquipmentSlot::ES_MainHand || Slot == EEquipmentSlot::ES_OffHand))
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
		// Canonical-first, linked-hand fallback priority for ground pickups:
		//   MainHand canonical: empty MH → empty OH → displace MH → displace OH
		//   OffHand  canonical: empty OH → empty MH → displace OH → displace MH
		// This mirrors DetermineEquipmentSlot so that ground-pickup and
		// inventory-equip paths behave identically.
		const EEquipmentSlot PrimarySlot  = (CanonicalSlot == EEquipmentSlot::ES_OffHand)
			? EEquipmentSlot::ES_OffHand
			: EEquipmentSlot::ES_MainHand;
		const EEquipmentSlot FallbackSlot = (PrimarySlot == EEquipmentSlot::ES_MainHand)
			? EEquipmentSlot::ES_OffHand
			: EEquipmentSlot::ES_MainHand;

		const bool bPrimaryEmpty  = !Manager.IsSlotOccupied(PrimarySlot);
		const bool bFallbackEmpty = !Manager.IsSlotOccupied(FallbackSlot);

		// Two-handed weapon currently equipped blocks either hand slot
		// regardless, because CanEquipGroundPickupToSlot will still need
		// to swap the two-hand item to the bag. Keep that check via the
		// helper below.
		auto CanUseSlot = [&](EEquipmentSlot Slot) -> bool
		{
			return EquipmentSlotResolverPrivate::CanEquipGroundPickupToSlot(Manager, Item, Slot, bSwapToBag);
		};

		if (bPrimaryEmpty && CanUseSlot(PrimarySlot))
		{
			ChosenSlot = PrimarySlot;
		}
		else if (bFallbackEmpty && CanUseSlot(FallbackSlot))
		{
			ChosenSlot = FallbackSlot;
		}
		else if (CanUseSlot(PrimarySlot))
		{
			// Both hands occupied — displace the primary slot to bag.
			ChosenSlot = PrimarySlot;
		}
		else if (CanUseSlot(FallbackSlot))
		{
			// Last resort — displace the fallback slot to bag.
			ChosenSlot = FallbackSlot;
		}
		else
		{
			return false;
		}
	}
	else if (CanonicalSlot == EEquipmentSlot::ES_OffHand)
	{
		// Dedicated off-hand-only items (shields, etc.) — no hand-link fallback.
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
