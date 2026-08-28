#include "Equipment/Components/EquipmentManager.h"

#include "Engine/ActorChannel.h"
#include "Equipment/Helpers/EquipmentDebugItemFactory.h"
#include "Equipment/Helpers/EquipmentMutationHelper.h"
#include "Equipment/Helpers/EquipmentReplicationHelper.h"
#include "Equipment/Helpers/EquipmentSlotResolver.h"
#include "Equipment/Library/EquipmentLog.h"
#include "Equipment/Library/FunctionLibraries/EquipmentFunctionLibrary.h"
#include "Item/ItemInstance.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY(LogEquipmentManager);

UEquipmentManager::UEquipmentManager()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
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

bool UEquipmentManager::ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	bool bWroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);
	if (!Channel)
	{
		return bWroteSomething;
	}

	TSet<UItemInstance*> ReplicatedItems;
	for (const FEquipmentSlotEntry& Entry : EquippedItemsArray)
	{
		if (IsValid(Entry.Item) && !ReplicatedItems.Contains(Entry.Item))
		{
			bWroteSomething |= Channel->ReplicateSubobject(Entry.Item, *Bunch, *RepFlags);
			ReplicatedItems.Add(Entry.Item);
		}
	}

	return bWroteSomething;
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

UItemInstance* UEquipmentManager::GiveWeapon(
	const FDataTableRowHandle& BaseItemHandle,
	int32 ItemLevel,
	EItemRarity Rarity,
	bool bGenerateAffixes)
{
	return FEquipmentDebugItemFactory::GiveWeapon(*this, BaseItemHandle, ItemLevel, Rarity, bGenerateAffixes);
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

EEquipmentSlot UEquipmentManager::ResolveOccupyingSlot(EEquipmentSlot Slot) const
{
	return UEquipmentFunctionLibrary::ResolveOccupyingSlot(Slot, IsSlotOccupied(EEquipmentSlot::ES_TwoHand));
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

UItemInstance* UEquipmentManager::EquipItemInternal(UItemInstance* Item, EEquipmentSlot Slot, bool bSwapToBag,
	bool bUseGroundPickupRules)
{
	return FEquipmentMutationHelper::EquipItemInternal(*this, Item, Slot, bSwapToBag, bUseGroundPickupRules);
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

void UEquipmentManager::MulticastEquipmentChanged_Implementation(EEquipmentSlot Slot, UItemInstance* NewItem,
	UItemInstance* OldItem)
{
	(void)Slot;
	(void)NewItem;
	(void)OldItem;
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
