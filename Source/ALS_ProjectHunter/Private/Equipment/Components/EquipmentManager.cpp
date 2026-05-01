// Character/Component/EquipmentManager.cpp
// PH-1.4 / PH-1.5 / follow-up split
//
// Owner:
//   UEquipmentManager owns equipped slot state, replication, and broadcasts.
// Helpers:
//   FEquipmentMutationHelper, FEquipmentSlotResolver, FEquipmentReplicationHelper.

#include "Equipment/Components/EquipmentManager.h"

#include "Engine/ActorChannel.h"
#include "Item/ItemInstance.h"
#include "Net/UnrealNetwork.h"
#include "Equipment/EquipmentMutationHelper.h"
#include "Equipment/EquipmentReplicationHelper.h"
#include "Equipment/EquipmentSlotResolver.h"

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
	// ── Validate input ────────────────────────────────────────────────────
	if (BaseItemHandle.IsNull() || BaseItemHandle.DataTable == nullptr)
	{
		UE_LOG(LogEquipmentManager, Warning,
			TEXT("GiveWeapon: BaseItemHandle is null or has no DataTable assigned."));
		return nullptr;
	}

	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		UE_LOG(LogEquipmentManager, Warning,
			TEXT("GiveWeapon: EquipmentManager has no owner — cannot create item."));
		return nullptr;
	}

	// ── Construct the item instance ──────────────────────────────────────
	// Outer = owning actor (player pawn).  Once equipped, EquippedItemsArray
	// holds a hard ref so the item is GC-rooted independent of outer.
	UItemInstance* NewItem = NewObject<UItemInstance>(OwnerActor);
	if (!NewItem)
	{
		UE_LOG(LogEquipmentManager, Warning,
			TEXT("GiveWeapon: NewObject<UItemInstance> returned null."));
		return nullptr;
	}

	NewItem->Initialize(BaseItemHandle, ItemLevel, Rarity, bGenerateAffixes);

	if (!NewItem->HasValidBaseData())
	{
		UE_LOG(LogEquipmentManager, Warning,
			TEXT("GiveWeapon: Initialized item '%s' has no valid base data — "
			     "row handle probably points at a row that doesn't exist."),
			*NewItem->GetName());
		return nullptr;
	}

	// ── Auto-equip via the normal path ───────────────────────────────────
	// ES_None lets DetermineEquipmentSlot pick the canonical slot for the
	// item type (MainHand for 1H weapon, TwoHand for 2H, etc.).  bSwapToBag
	// = true so any currently-equipped item is preserved in inventory.
	EquipItem(NewItem, EEquipmentSlot::ES_None, /*bSwapToBag*/ true);

	UE_LOG(LogEquipmentManager, Log,
		TEXT("GiveWeapon: created and equipped '%s' (level=%d, rarity=%d)."),
		*NewItem->GetName(), ItemLevel, (int32)Rarity);

	return NewItem;
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
