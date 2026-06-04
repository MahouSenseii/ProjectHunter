#include "Equipment/Helpers/EquipmentSlotResolver.h"

#include "Core/Logging/ProjectHunterLogMacros.h"
#include "Equipment/Components/EquipmentManager.h"
#include "Equipment/Helpers/EquipmentMutationHelper.h"
#include "Equipment/Library/EquipmentFunctionLibrary.h"
#include "Equipment/Library/EquipmentLog.h"
#include "Inventory/Components/InventoryManager.h"
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

	EEquipmentSlot ChosenSlot = EEquipmentSlot::ES_None;

	if (UEquipmentFunctionLibrary::IsOneHandedWeapon(Item))
	{
		// Prefer the canonical hand, fall back to the other. Within each, prefer an empty slot.
		const EEquipmentSlot PrimarySlot  = (CanonicalSlot == EEquipmentSlot::ES_OffHand)
			? EEquipmentSlot::ES_OffHand
			: EEquipmentSlot::ES_MainHand;
		const EEquipmentSlot FallbackSlot = (PrimarySlot == EEquipmentSlot::ES_MainHand)
			? EEquipmentSlot::ES_OffHand
			: EEquipmentSlot::ES_MainHand;

		auto CanUseSlot = [&](EEquipmentSlot Slot) -> bool
		{
			return EquipmentSlotResolverPrivate::CanEquipGroundPickupToSlot(Manager, Item, Slot, bSwapToBag);
		};

		if (!Manager.IsSlotOccupied(PrimarySlot) && CanUseSlot(PrimarySlot))
		{
			ChosenSlot = PrimarySlot;
		}
		else if (!Manager.IsSlotOccupied(FallbackSlot) && CanUseSlot(FallbackSlot))
		{
			ChosenSlot = FallbackSlot;
		}
		else if (CanUseSlot(PrimarySlot))
		{
			ChosenSlot = PrimarySlot;
		}
		else if (CanUseSlot(FallbackSlot))
		{
			ChosenSlot = FallbackSlot;
		}
		else
		{
			return false;
		}
	}
	else
	{
		// For all other item types (armor, accessories, rings, two-handers, etc.)
		// DetermineEquipmentSlot handles ring rotation; CanonicalSlot is not sufficient alone.
		ChosenSlot = DetermineEquipmentSlot(Manager, Item);
		if (ChosenSlot == EEquipmentSlot::ES_None)
		{
			return false;
		}
		if (!EquipmentSlotResolverPrivate::CanEquipGroundPickupToSlot(Manager, Item, ChosenSlot, bSwapToBag))
		{
			return false;
		}
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
