// Character/Component/EquipmentManager.cpp
// PH-1.4 / PH-1.5 / follow-up split
//
// Owner:
//   UEquipmentManager owns equipped slot state, replication, and broadcasts.
// Helpers:
//   FEquipmentMutationHelper, FEquipmentSlotResolver, FEquipmentReplicationHelper.

#include "Systems/Equipment/Components/EquipmentManager.h"

#include "Item/ItemInstance.h"
#include "Net/UnrealNetwork.h"
#include "Systems/Equipment/EquipmentMutationHelper.h"
#include "Systems/Equipment/EquipmentReplicationHelper.h"
#include "Systems/Equipment/EquipmentSlotResolver.h"

DEFINE_LOG_CATEGORY(LogEquipmentManager);

UEquipmentManager::UEquipmentManager()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UEquipmentManager::BeginPlay()
{
	Super::BeginPlay();
	CacheComponents();
	RebuildEquipmentMap();
}

void UEquipmentManager::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UEquipmentManager, EquippedItemsArray);
}

void UEquipmentManager::CacheComponents()
{
	FEquipmentReplicationHelper::CacheComponents(*this);
}

UItemInstance* UEquipmentManager::EquipItem(UItemInstance* Item, EEquipmentSlot Slot, bool bSwapToBag)
{
	return FEquipmentMutationHelper::EquipItem(*this, Item, Slot, bSwapToBag);
}

UItemInstance* UEquipmentManager::UnequipItem(EEquipmentSlot Slot, bool bMoveToBag)
{
	return FEquipmentMutationHelper::UnequipItem(*this, Slot, bMoveToBag);
}

UItemInstance* UEquipmentManager::SwapEquipment(UItemInstance* Item, EEquipmentSlot Slot)
{
	return EquipItem(Item, Slot, true);
}

UItemInstance* UEquipmentManager::GetEquippedItem(EEquipmentSlot Slot) const
{
	UItemInstance* const* Found = EquippedItemsMap.Find(Slot);
	return Found ? *Found : nullptr;
}

bool UEquipmentManager::IsSlotOccupied(EEquipmentSlot Slot) const
{
	return EquippedItemsMap.Contains(Slot);
}

TArray<UItemInstance*> UEquipmentManager::GetAllEquippedItems() const
{
	TArray<UItemInstance*> Items;
	EquippedItemsMap.GenerateValueArray(Items);
	return Items;
}

void UEquipmentManager::UnequipAll(bool bMoveToBag)
{
	FEquipmentMutationHelper::UnequipAll(*this, bMoveToBag);
}

AEquippedItemRuntimeActor* UEquipmentManager::GetActiveRuntimeItemActor(EEquipmentSlot Slot) const
{
	return FEquipmentMutationHelper::GetActiveRuntimeItemActor(*this, Slot);
}

EEquipmentSlot UEquipmentManager::DetermineEquipmentSlot(UItemInstance* Item) const
{
	return FEquipmentSlotResolver::DetermineEquipmentSlot(*this, Item);
}

bool UEquipmentManager::CanEquipToSlot(UItemInstance* Item, EEquipmentSlot Slot) const
{
	return FEquipmentSlotResolver::CanEquipToSlot(*this, Item, Slot);
}

bool UEquipmentManager::TryEquipGroundPickupItem(UItemInstance* Item, EEquipmentSlot& OutEquippedSlot, bool bSwapToBag)
{
	return FEquipmentSlotResolver::TryEquipGroundPickupItem(*this, Item, OutEquippedSlot, bSwapToBag);
}

EEquipmentSlot UEquipmentManager::GetNextAvailableRingSlot() const
{
	return FEquipmentSlotResolver::GetNextAvailableRingSlot(*this);
}

bool UEquipmentManager::IsRingSlot(EEquipmentSlot Slot) const
{
	return FEquipmentSlotResolver::IsRingSlot(Slot);
}

UItemInstance* UEquipmentManager::EquipItemInternal(UItemInstance* Item, EEquipmentSlot Slot, bool bSwapToBag, bool bUseGroundPickupRules)
{
	return FEquipmentMutationHelper::EquipItemInternal(*this, Item, Slot, bSwapToBag, bUseGroundPickupRules);
}

bool UEquipmentManager::HandleTwoHandedWeapon(UItemInstance* Item, bool bSwapToBag, UItemInstance*& OutOldMainHand, UItemInstance*& OutOldOffHand, UItemInstance*& OutOldTwoHand)
{
	return FEquipmentMutationHelper::HandleTwoHandedWeapon(*this, Item, bSwapToBag, OutOldMainHand, OutOldOffHand, OutOldTwoHand);
}

void UEquipmentManager::OnRep_EquippedItems()
{
	FEquipmentReplicationHelper::OnRepEquippedItems(*this);
}

void UEquipmentManager::ServerEquipItem_Implementation(UItemInstance* Item, EEquipmentSlot Slot, bool bSwapToBag)
{
	FEquipmentReplicationHelper::ServerEquipItem(*this, Item, Slot, bSwapToBag);
}

void UEquipmentManager::ServerUnequipItem_Implementation(EEquipmentSlot Slot, bool bMoveToBag)
{
	FEquipmentReplicationHelper::ServerUnequipItem(*this, Slot, bMoveToBag);
}

void UEquipmentManager::MulticastEquipmentChanged_Implementation(EEquipmentSlot Slot, UItemInstance* NewItem, UItemInstance* OldItem)
{
	(void)Slot;
	(void)NewItem;
	(void)OldItem;
	// Client notifications are delivered through OnRep_EquippedItems only.
}

void UEquipmentManager::RebuildEquipmentMap()
{
	FEquipmentReplicationHelper::RebuildEquipmentMap(*this);
}

void UEquipmentManager::AddEquipment(EEquipmentSlot Slot, UItemInstance* Item)
{
	FEquipmentReplicationHelper::AddEquipment(*this, Slot, Item);
}

void UEquipmentManager::RemoveEquipment(EEquipmentSlot Slot)
{
	FEquipmentReplicationHelper::RemoveEquipment(*this, Slot);
}

UItemInstance* UEquipmentManager::Debug_EquipItemFromTable(
	FDataTableRowHandle ItemRowHandle,
	EEquipmentSlot Slot,
	int32 ItemLevel,
	EItemRarity Rarity)
{
	if (ItemRowHandle.IsNull())
	{
		UE_LOG(LogEquipmentManager, Warning,
			TEXT("Debug_EquipItemFromTable: ItemRowHandle is null — nothing equipped."));
		return nullptr;
	}

	UItemInstance* NewItem = NewObject<UItemInstance>(GetOwner());
	NewItem->Initialize(ItemRowHandle, ItemLevel, Rarity, /*bGenerateAffixes=*/true);

	// Pass bSwapToBag=false: the item is not coming from inventory so we don't
	// want the mutation helper trying to move a non-existent displaced item there.
	return EquipItem(NewItem, Slot, /*bSwapToBag=*/false);
}
