#include "Equipment/Helpers/EquipmentHandSlotMutationHelper.h"

#include "Core/Logging/ProjectHunterLogMacros.h"
#include "Equipment/Components/EquipmentManager.h"
#include "Equipment/Helpers/EquipmentReplicationHelper.h"
#include "Equipment/Library/EquipmentLog.h"
#include "Inventory/Components/InventoryManager.h"
#include "Item/ItemInstance.h"

namespace EquipmentHandSlotMutationPrivate
{
	bool IsHandSlot(const EEquipmentSlot Slot)
	{
		return Slot == EEquipmentSlot::ES_MainHand || Slot == EEquipmentSlot::ES_OffHand;
	}

	void MoveDisplacedItemToInventory(UInventoryManager* InventoryManager, UItemInstance* Item, const TCHAR* Context)
	{
		if (!Item || !InventoryManager)
		{
			return;
		}

		if (!InventoryManager->AddItem(Item))
		{
			PH_LOG_WARNING(LogEquipmentManager, "%s failed: Could not return displaced Item=%s to inventory.",
				Context, *GetNameSafe(Item));
		}
	}
}

UItemInstance* FEquipmentHandSlotMutationHelper::EquipTwoHandedItem(UEquipmentManager& Manager, UItemInstance* Item,
	bool bSwapToBag)
{
	UItemInstance* OldMainHand = Manager.GetEquippedItem(EEquipmentSlot::ES_MainHand);
	UItemInstance* OldOffHand = Manager.GetEquippedItem(EEquipmentSlot::ES_OffHand);
	UItemInstance* OldTwoHand = Manager.GetEquippedItem(EEquipmentSlot::ES_TwoHand);

	FEquipmentReplicationHelper::RemoveEquipment(Manager, EEquipmentSlot::ES_MainHand);
	FEquipmentReplicationHelper::RemoveEquipment(Manager, EEquipmentSlot::ES_OffHand);
	FEquipmentReplicationHelper::RemoveEquipment(Manager, EEquipmentSlot::ES_TwoHand);
	FEquipmentReplicationHelper::AddEquipment(Manager, EEquipmentSlot::ES_TwoHand, Item);

	if (Manager.InventoryManager)
	{
		Manager.InventoryManager->RemoveItem(Item);
	}

	if (bSwapToBag)
	{
		EquipmentHandSlotMutationPrivate::MoveDisplacedItemToInventory(Manager.InventoryManager, OldMainHand,
			TEXT("EquipTwoHandedItem"));
		EquipmentHandSlotMutationPrivate::MoveDisplacedItemToInventory(Manager.InventoryManager, OldOffHand,
			TEXT("EquipTwoHandedItem"));
		EquipmentHandSlotMutationPrivate::MoveDisplacedItemToInventory(Manager.InventoryManager, OldTwoHand,
			TEXT("EquipTwoHandedItem"));
	}

	Manager.OnEquipmentChanged.Broadcast(EEquipmentSlot::ES_TwoHand, Item, OldTwoHand);

	if (OldMainHand)
	{
		Manager.OnEquipmentChanged.Broadcast(EEquipmentSlot::ES_MainHand, nullptr, OldMainHand);
	}

	if (OldOffHand)
	{
		Manager.OnEquipmentChanged.Broadcast(EEquipmentSlot::ES_OffHand, nullptr, OldOffHand);
	}

	return OldMainHand ? OldMainHand : (OldOffHand ? OldOffHand : OldTwoHand);
}

UItemInstance* FEquipmentHandSlotMutationHelper::UnequipConflictingTwoHandedItem(UEquipmentManager& Manager,
	EEquipmentSlot IncomingSlot, bool bSwapToBag)
{
	if (!EquipmentHandSlotMutationPrivate::IsHandSlot(IncomingSlot))
	{
		return nullptr;
	}

	UItemInstance* OldTwoHandItem = Manager.GetEquippedItem(EEquipmentSlot::ES_TwoHand);
	if (!OldTwoHandItem)
	{
		return nullptr;
	}

	FEquipmentReplicationHelper::RemoveEquipment(Manager, EEquipmentSlot::ES_TwoHand);

	if (bSwapToBag)
	{
		EquipmentHandSlotMutationPrivate::MoveDisplacedItemToInventory(Manager.InventoryManager, OldTwoHandItem,
			TEXT("UnequipConflictingTwoHandedItem"));
	}

	return OldTwoHandItem;
}
