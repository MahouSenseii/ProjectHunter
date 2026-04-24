#include "Systems/Equipment/EquipmentReplicationHelper.h"

#include "Core/Logging/ProjectHunterLogMacros.h"
#include "Systems/Character/Components/CharacterSystemCoordinatorComponent.h"
#include "Systems/Equipment/Components/EquipmentManager.h"
#include "Systems/Equipment/Components/EquipmentPresentationComponent.h"
#include "Systems/Inventory/Components/InventoryManager.h"
#include "Item/ItemInstance.h"

void FEquipmentReplicationHelper::CacheComponents(UEquipmentManager& Manager)
{
	AActor* Owner = Manager.GetOwner();
	if (!Owner)
	{
		return;
	}

	Manager.InventoryManager = Owner->FindComponentByClass<UInventoryManager>();
	if (!Manager.InventoryManager)
	{
		PH_LOG_ERROR(LogEquipmentManager, "CacheComponents failed: No InventoryManager found on Owner=%s.", *GetNameSafe(Owner));
	}

	Manager.CharacterSystemCoordinator = Owner->FindComponentByClass<UCharacterSystemCoordinatorComponent>();
	Manager.EquipmentPresentation = Owner->FindComponentByClass<UEquipmentPresentationComponent>();
}

void FEquipmentReplicationHelper::OnRepEquippedItems(UEquipmentManager& Manager)
{
	TMap<EEquipmentSlot, UItemInstance*> OldEquipmentMap = Manager.EquippedItemsMap;

	RebuildEquipmentMap(Manager);

	TSet<EEquipmentSlot> NewSlots;
	for (const FEquipmentSlotEntry& Entry : Manager.EquippedItemsArray)
	{
		NewSlots.Add(Entry.Slot);
	}

	for (const FEquipmentSlotEntry& Entry : Manager.EquippedItemsArray)
	{
		UItemInstance* OldItem = OldEquipmentMap.FindRef(Entry.Slot);
		if (OldItem != Entry.Item)
		{
			Manager.OnEquipmentChanged.Broadcast(Entry.Slot, Entry.Item, OldItem);
		}
	}

	for (const TPair<EEquipmentSlot, UItemInstance*>& OldEntry : OldEquipmentMap)
	{
		if (!NewSlots.Contains(OldEntry.Key))
		{
			Manager.OnEquipmentChanged.Broadcast(OldEntry.Key, nullptr, OldEntry.Value);
		}
	}

	UE_LOG(LogEquipmentManager, Verbose, TEXT("EquipmentManager: Replicated equipment changes."));
}

void FEquipmentReplicationHelper::ServerEquipItem(UEquipmentManager& Manager, UItemInstance* Item, EEquipmentSlot Slot, bool bSwapToBag)
{
	if (!Item)
	{
		return;
	}

	if (Manager.InventoryManager && !Manager.InventoryManager->ContainsItem(Item))
	{
		PH_LOG_WARNING(LogEquipmentManager, "ServerEquipItem rejected Item=%s because it was not found in Owner=%s inventory.", *GetNameSafe(Item), *GetNameSafe(Manager.GetOwner()));
		return;
	}

	const EEquipmentSlot ResolvedSlot = (Slot == EEquipmentSlot::ES_None)
		? Manager.DetermineEquipmentSlot(Item) : Slot;

	if (!Manager.CanEquipToSlot(Item, ResolvedSlot))
	{
		PH_LOG_WARNING(LogEquipmentManager, "ServerEquipItem rejected Item=%s for Slot=%d.", *GetNameSafe(Item), static_cast<int32>(ResolvedSlot));
		return;
	}

	Manager.EquipItem(Item, Slot, bSwapToBag);
}

void FEquipmentReplicationHelper::ServerUnequipItem(UEquipmentManager& Manager, EEquipmentSlot Slot, bool bMoveToBag)
{
	Manager.UnequipItem(Slot, bMoveToBag);
}

void FEquipmentReplicationHelper::RebuildEquipmentMap(UEquipmentManager& Manager)
{
	Manager.EquippedItemsMap.Empty();
	for (const FEquipmentSlotEntry& Entry : Manager.EquippedItemsArray)
	{
		if (Entry.Item)
		{
			Manager.EquippedItemsMap.Add(Entry.Slot, Entry.Item);
		}
	}
}

void FEquipmentReplicationHelper::AddEquipment(UEquipmentManager& Manager, EEquipmentSlot Slot, UItemInstance* Item)
{
	if (!Manager.GetOwner() || !Manager.GetOwner()->HasAuthority())
	{
		PH_LOG_WARNING(LogEquipmentManager, "AddEquipment failed: Must be called on the server.");
		return;
	}

	RemoveEquipment(Manager, Slot);

	if (Item)
	{
		if (AActor* OwnerActor = Manager.GetOwner())
		{
			if (Item->GetOuter() != OwnerActor)
			{
				Item->Rename(nullptr, OwnerActor, REN_DontCreateRedirectors | REN_NonTransactional);
			}
		}
	}

	Manager.EquippedItemsArray.Add(FEquipmentSlotEntry(Slot, Item));
	Manager.EquippedItemsMap.Add(Slot, Item);
}

void FEquipmentReplicationHelper::RemoveEquipment(UEquipmentManager& Manager, EEquipmentSlot Slot)
{
	if (!Manager.GetOwner() || !Manager.GetOwner()->HasAuthority())
	{
		PH_LOG_WARNING(LogEquipmentManager, "RemoveEquipment failed: Must be called on the server.");
		return;
	}

	for (int32 i = Manager.EquippedItemsArray.Num() - 1; i >= 0; --i)
	{
		if (Manager.EquippedItemsArray[i].Slot == Slot)
		{
			Manager.EquippedItemsArray.RemoveAtSwap(i, 1, EAllowShrinking::No);
			break;
		}
	}

	Manager.EquippedItemsMap.Remove(Slot);
}
