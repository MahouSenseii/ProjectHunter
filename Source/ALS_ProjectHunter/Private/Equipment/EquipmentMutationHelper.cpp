#include "Equipment/EquipmentMutationHelper.h"

#include "Core/Logging/ProjectHunterLogMacros.h"
#include "Equipment/Components/EquipmentManager.h"
#include "Equipment/Components/EquipmentPresentationComponent.h"
#include "Equipment/EquipmentReplicationHelper.h"
#include "Equipment/EquipmentSlotResolver.h"
#include "Equipment/Library/EquipmentFunctionLibrary.h"
#include "Inventory/Components/InventoryManager.h"
#include "Item/ItemInstance.h"
#include "Equipment/Actors/EquippedItemRuntimeActor.h"

UItemInstance* FEquipmentMutationHelper::EquipItem(UEquipmentManager& Manager, UItemInstance* Item, EEquipmentSlot Slot, bool bSwapToBag)
{
	if (!Item)
	{
		PH_LOG_WARNING(LogEquipmentManager, "EquipItem failed: Item was null.");
		return nullptr;
	}

	if (!Manager.GetOwner() || !Manager.GetOwner()->HasAuthority())
	{
		Manager.ServerEquipItem(Item, Slot, bSwapToBag);
		return nullptr;
	}

	return EquipItemInternal(Manager, Item, Slot, bSwapToBag, false);
}

UItemInstance* FEquipmentMutationHelper::UnequipItem(UEquipmentManager& Manager, EEquipmentSlot Slot, bool bMoveToBag)
{
	if (!Manager.GetOwner() || !Manager.GetOwner()->HasAuthority())
	{
		Manager.ServerUnequipItem(Slot, bMoveToBag);
		return nullptr;
	}

	UItemInstance* CurrentItem = Manager.GetEquippedItem(Slot);
	if (!CurrentItem)
	{
		UE_LOG(LogEquipmentManager, Verbose, TEXT("EquipmentManager::UnequipItem: Slot %d is already empty."), static_cast<int32>(Slot));
		return nullptr;
	}

	FEquipmentReplicationHelper::RemoveEquipment(Manager, Slot);

	if (bMoveToBag && Manager.InventoryManager)
	{
		if (!Manager.InventoryManager->AddItem(CurrentItem))
		{
			PH_LOG_WARNING(LogEquipmentManager, "UnequipItem failed: Could not return Item=%s to inventory.", *GetNameSafe(CurrentItem));
		}
	}

	Manager.OnEquipmentChanged.Broadcast(Slot, nullptr, CurrentItem);

	UE_LOG(LogEquipmentManager, Log, TEXT("EquipmentManager: Unequipped '%s' from slot %d."),
		*GetNameSafe(CurrentItem), static_cast<int32>(Slot));

	return CurrentItem;
}

void FEquipmentMutationHelper::UnequipAll(UEquipmentManager& Manager, bool bMoveToBag)
{
	if (!Manager.GetOwner() || !Manager.GetOwner()->HasAuthority())
	{
		PH_LOG_WARNING(LogEquipmentManager, "UnequipAll failed: Must be called on the server.");
		return;
	}

	TArray<EEquipmentSlot> Slots;
	Manager.EquippedItemsMap.GetKeys(Slots);

	for (const EEquipmentSlot Slot : Slots)
	{
		UnequipItem(Manager, Slot, bMoveToBag);
	}
}

AEquippedItemRuntimeActor* FEquipmentMutationHelper::GetActiveRuntimeItemActor(const UEquipmentManager& Manager, EEquipmentSlot Slot)
{
	if (Manager.EquipmentPresentation)
	{
		return Manager.EquipmentPresentation->GetActiveRuntimeItemActor(Slot);
	}

	return nullptr;
}

UItemInstance* FEquipmentMutationHelper::EquipItemInternal(UEquipmentManager& Manager, UItemInstance* Item, EEquipmentSlot Slot, bool bSwapToBag, bool bUseGroundPickupRules)
{
	if (!Item)
	{
		return nullptr;
	}

	FItemBase* BaseData = Item->GetBaseData();
	if (!BaseData)
	{
		PH_LOG_WARNING(LogEquipmentManager, "EquipItemInternal failed: Item=%s had no base data.", *GetNameSafe(Item));
		return nullptr;
	}

	
	if (!BaseData->IsEquippable())
	{
		PH_LOG_WARNING(LogEquipmentManager, "EquipItemInternal rejected Item=%s because it is not equippable.", *GetNameSafe(Item));
		return nullptr;
	}

	if (Slot == EEquipmentSlot::ES_None && Manager.bAutoSlotSelection)
	{
		Slot = FEquipmentSlotResolver::DetermineEquipmentSlot(Manager, Item);
	}

	if (Slot == EEquipmentSlot::ES_None)
	{
		PH_LOG_WARNING(LogEquipmentManager, "EquipItemInternal failed: Could not determine a slot for Item=%s.", *GetNameSafe(Item));
		return nullptr;
	}

	const bool bCanEquip = bUseGroundPickupRules
		? UEquipmentFunctionLibrary::CanGroundPickupEquipToSlot(Item, Slot)
		: FEquipmentSlotResolver::CanEquipToSlot(Manager, Item, Slot);
	if (!bCanEquip)
	{
		PH_LOG_WARNING(LogEquipmentManager, "EquipItemInternal rejected Item=%s for Slot=%d.", *GetNameSafe(Item), static_cast<int32>(Slot));
		return nullptr;
	}

	if (Item->bIsTwoHanded() && Slot == EEquipmentSlot::ES_TwoHand)
	{
		UItemInstance* OldMainHand = nullptr;
		UItemInstance* OldOffHand = nullptr;
		UItemInstance* OldTwoHand = nullptr;

		if (HandleTwoHandedWeapon(Manager, Item, bSwapToBag, OldMainHand, OldOffHand, OldTwoHand))
		{
			UE_LOG(LogEquipmentManager, Log, TEXT("EquipmentManager: Equipped two-handed '%s'."), *GetNameSafe(Item));
			return OldMainHand ? OldMainHand : (OldOffHand ? OldOffHand : OldTwoHand);
		}

		return nullptr;
	}

	UItemInstance* OldTwoHandItem = nullptr;
	if (Slot != EEquipmentSlot::ES_TwoHand)
	{
		OldTwoHandItem = Manager.GetEquippedItem(EEquipmentSlot::ES_TwoHand);
		if (OldTwoHandItem)
		{
			FEquipmentReplicationHelper::RemoveEquipment(Manager, EEquipmentSlot::ES_TwoHand);

			if (bSwapToBag && Manager.InventoryManager && !Manager.InventoryManager->AddItem(OldTwoHandItem))
			{
				PH_LOG_WARNING(LogEquipmentManager, "EquipItemInternal failed: Could not return displaced two-hand Item=%s to inventory.", *GetNameSafe(OldTwoHandItem));
			}
		}
	}

	UItemInstance* OldItem = Manager.GetEquippedItem(Slot);

	FEquipmentReplicationHelper::AddEquipment(Manager, Slot, Item);

	if (Manager.InventoryManager)
	{
		Manager.InventoryManager->RemoveItem(Item);
	}

	if (OldItem && bSwapToBag && Manager.InventoryManager)
	{
		if (!Manager.InventoryManager->AddItem(OldItem))
		{
			PH_LOG_WARNING(LogEquipmentManager, "EquipItemInternal failed: Could not return displaced Item=%s to inventory.", *GetNameSafe(OldItem));
		}
	}

	if (OldTwoHandItem)
	{
		Manager.OnEquipmentChanged.Broadcast(EEquipmentSlot::ES_TwoHand, nullptr, OldTwoHandItem);
	}
	Manager.OnEquipmentChanged.Broadcast(Slot, Item, OldItem);

	UE_LOG(LogEquipmentManager, Log, TEXT("EquipmentManager: Equipped '%s' to slot %d."),
		*GetNameSafe(Item), static_cast<int32>(Slot));

	return OldItem;
}

bool FEquipmentMutationHelper::HandleTwoHandedWeapon(UEquipmentManager& Manager, UItemInstance* Item, bool bSwapToBag, UItemInstance*& OutOldMainHand, UItemInstance*& OutOldOffHand, UItemInstance*& OutOldTwoHand)
{
	OutOldMainHand = Manager.GetEquippedItem(EEquipmentSlot::ES_MainHand);
	OutOldOffHand = Manager.GetEquippedItem(EEquipmentSlot::ES_OffHand);
	OutOldTwoHand = Manager.GetEquippedItem(EEquipmentSlot::ES_TwoHand);

	FEquipmentReplicationHelper::RemoveEquipment(Manager, EEquipmentSlot::ES_MainHand);
	FEquipmentReplicationHelper::RemoveEquipment(Manager, EEquipmentSlot::ES_OffHand);
	FEquipmentReplicationHelper::RemoveEquipment(Manager, EEquipmentSlot::ES_TwoHand);
	FEquipmentReplicationHelper::AddEquipment(Manager, EEquipmentSlot::ES_TwoHand, Item);

	if (Manager.InventoryManager)
	{
		Manager.InventoryManager->RemoveItem(Item);
	}

	if (OutOldMainHand && bSwapToBag && Manager.InventoryManager)
	{
		Manager.InventoryManager->AddItem(OutOldMainHand);
	}

	if (OutOldOffHand && bSwapToBag && Manager.InventoryManager)
	{
		Manager.InventoryManager->AddItem(OutOldOffHand);
	}

	if (OutOldTwoHand && bSwapToBag && Manager.InventoryManager)
	{
		Manager.InventoryManager->AddItem(OutOldTwoHand);
	}

	Manager.OnEquipmentChanged.Broadcast(EEquipmentSlot::ES_TwoHand, Item, OutOldTwoHand);

	if (OutOldMainHand)
	{
		Manager.OnEquipmentChanged.Broadcast(EEquipmentSlot::ES_MainHand, nullptr, OutOldMainHand);
	}

	if (OutOldOffHand)
	{
		Manager.OnEquipmentChanged.Broadcast(EEquipmentSlot::ES_OffHand, nullptr, OutOldOffHand);
	}

	return true;
}
