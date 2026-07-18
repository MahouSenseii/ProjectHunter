#include "Equipment/Helpers/EquipmentGroundPickupEquipHelper.h"

#include "Core/Logging/ProjectHunterLogMacros.h"
#include "Equipment/Components/EquipmentManager.h"
#include "Equipment/Helpers/EquipmentMutationHelper.h"
#include "Equipment/Helpers/EquipmentSlotResolver.h"
#include "Equipment/Library/EquipmentLog.h"
#include "Equipment/Library/FunctionLibraries/EquipmentFunctionLibrary.h"
#include "Inventory/Components/InventoryManager.h"
#include "Item/ItemInstance.h"

namespace EquipmentGroundPickupEquipPrivate
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

	bool CanEquipToSlot(const UEquipmentManager& Manager, UItemInstance* Item, EEquipmentSlot Slot, bool bSwapToBag)
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

	EEquipmentSlot ChooseOneHandedSlot(const UEquipmentManager& Manager, UItemInstance* Item,
		EEquipmentSlot CanonicalSlot, bool bSwapToBag)
	{
		const EEquipmentSlot PrimarySlot = (CanonicalSlot == EEquipmentSlot::ES_OffHand)
			? EEquipmentSlot::ES_OffHand
			: EEquipmentSlot::ES_MainHand;
		const EEquipmentSlot FallbackSlot = (PrimarySlot == EEquipmentSlot::ES_MainHand)
			? EEquipmentSlot::ES_OffHand
			: EEquipmentSlot::ES_MainHand;

		if (!Manager.IsSlotOccupied(PrimarySlot) && CanEquipToSlot(Manager, Item, PrimarySlot, bSwapToBag))
		{
			return PrimarySlot;
		}

		if (!Manager.IsSlotOccupied(FallbackSlot) && CanEquipToSlot(Manager, Item, FallbackSlot, bSwapToBag))
		{
			return FallbackSlot;
		}

		if (CanEquipToSlot(Manager, Item, PrimarySlot, bSwapToBag))
		{
			return PrimarySlot;
		}

		if (CanEquipToSlot(Manager, Item, FallbackSlot, bSwapToBag))
		{
			return FallbackSlot;
		}

		return EEquipmentSlot::ES_None;
	}
}

bool FEquipmentGroundPickupEquipHelper::TryEquipItem(UEquipmentManager& Manager, UItemInstance* Item,
	EEquipmentSlot& OutEquippedSlot, bool bSwapToBag)
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
		ChosenSlot = EquipmentGroundPickupEquipPrivate::ChooseOneHandedSlot(Manager, Item, CanonicalSlot, bSwapToBag);
	}
	else
	{
		ChosenSlot = FEquipmentSlotResolver::DetermineEquipmentSlot(Manager, Item);
		if (!EquipmentGroundPickupEquipPrivate::CanEquipToSlot(Manager, Item, ChosenSlot, bSwapToBag))
		{
			return false;
		}
	}

	if (ChosenSlot == EEquipmentSlot::ES_None)
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
