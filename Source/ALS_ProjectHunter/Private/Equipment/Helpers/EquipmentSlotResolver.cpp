#include "Equipment/Helpers/EquipmentSlotResolver.h"

#include "Equipment/Components/EquipmentManager.h"
#include "Equipment/Helpers/EquipmentGroundPickupEquipHelper.h"
#include "Equipment/Library/FunctionLibraries/EquipmentFunctionLibrary.h"

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
	// A two-handed weapon aimed at either hand fills both, so answer for the
	// slot it actually lands in rather than the one that was asked about.
	Slot = UEquipmentFunctionLibrary::ResolveEquipSlot(Item, Slot);

	if (UEquipmentFunctionLibrary::IsItemCompatibleWithSlot(Item, Slot))
	{
		return true;
	}

	if (UEquipmentFunctionLibrary::IsOneHandedWeapon(Item)
		&& UEquipmentFunctionLibrary::IsHandSlot(Slot))
	{
		return true;
	}

	return false;
}

bool FEquipmentSlotResolver::TryEquipGroundPickupItem(UEquipmentManager& Manager, UItemInstance* Item,
	EEquipmentSlot& OutEquippedSlot, bool bSwapToBag)
{
	return FEquipmentGroundPickupEquipHelper::TryEquipItem(Manager, Item, OutEquippedSlot, bSwapToBag);
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
