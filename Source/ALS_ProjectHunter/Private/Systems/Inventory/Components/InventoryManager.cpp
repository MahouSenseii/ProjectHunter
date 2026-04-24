// Character/Component/InventoryManager.cpp

#include "Systems/Inventory/Components/InventoryManager.h"

#include "Core/Logging/ProjectHunterLogMacros.h"
#include "Engine/ActorChannel.h"
#include "Engine/NetConnection.h"
#include "Item/ItemInstance.h"
#include "Net/UnrealNetwork.h"
#include "Systems/Inventory/InventoryAdder.h"
#include "Systems/Inventory/InventoryRemover.h"
#include "Systems/Inventory/InventoryStackHandler.h"
#include "Systems/Inventory/InventorySwapper.h"
#include "Systems/Inventory/InventoryValidator.h"
#include "Systems/Inventory/InventoryWeightCalculator.h"
#include "Systems/Inventory/Library/InventoryFunctionLibrary.h"

DEFINE_LOG_CATEGORY(LogInventoryManager);

UInventoryManager::UInventoryManager()
{
	PrimaryComponentTick.bCanEverTick = false;

	// N-04 FIX: Enable replication so the server has an accurate copy of the
	// inventory, allowing server-side ownership checks (e.g. ServerEquipItem).
	SetIsReplicatedByDefault(true);
}

// N-04 FIX: Register replicated properties
void UInventoryManager::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Only the owning client receives the full item list (saves bandwidth;
	// other players only see equipment via EquipmentManager replication).
	DOREPLIFETIME_CONDITION(UInventoryManager, Items, COND_OwnerOnly);
}

bool UInventoryManager::ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	bool bWroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);

	const AActor* OwnerActor = GetOwner();
	const UNetConnection* OwningConnection = OwnerActor ? OwnerActor->GetNetConnection() : nullptr;
	if (!Channel || !Channel->Connection || Channel->Connection != OwningConnection)
	{
		return bWroteSomething;
	}

	for (UItemInstance* Item : Items)
	{
		if (IsValid(Item))
		{
			bWroteSomething |= Channel->ReplicateSubobject(Item, *Bunch, *RepFlags);
		}
	}

	return bWroteSomething;
}

void UInventoryManager::BeginPlay()
{
	Super::BeginPlay();
	
	// Reserve space for max slots
	Items.Reserve(MaxSlots);
	
	UE_LOG(LogInventoryManager, Log, TEXT("InventoryManager: Initialized with %d slots, %.1f max weight"), 
		MaxSlots, MaxWeight);
}

// ═══════════════════════════════════════════════
// BASIC OPERATIONS
// ═══════════════════════════════════════════════

bool UInventoryManager::AddItem(UItemInstance* Item)
{
	if (!HasInventoryWriteAuthority(TEXT("AddItem")))
	{
		return false;
	}

	return FInventoryAdder::AddItem(*this, Item);
}

bool UInventoryManager::AddItemToSlot(UItemInstance* Item, int32 SlotIndex)
{
	if (!HasInventoryWriteAuthority(TEXT("AddItemToSlot")))
	{
		return false;
	}

	return FInventoryAdder::AddItemToSlot(*this, Item, SlotIndex);
}

bool UInventoryManager::RemoveItem(UItemInstance* Item)
{
	if (!HasInventoryWriteAuthority(TEXT("RemoveItem")))
	{
		return false;
	}

	return FInventoryRemover::RemoveItem(*this, Item);
}

UItemInstance* UInventoryManager::RemoveItemAtSlot(int32 SlotIndex)
{
	if (!HasInventoryWriteAuthority(TEXT("RemoveItemAtSlot")))
	{
		return nullptr;
	}

	return FInventoryRemover::RemoveItemAtSlot(*this, SlotIndex);
}

bool UInventoryManager::RemoveQuantity(UItemInstance* Item, int32 Quantity)
{
	if (!HasInventoryWriteAuthority(TEXT("RemoveQuantity")))
	{
		return false;
	}

	return FInventoryRemover::RemoveQuantity(*this, Item, Quantity);
}

bool UInventoryManager::SwapItems(int32 SlotA, int32 SlotB)
{
	if (!HasInventoryWriteAuthority(TEXT("SwapItems")))
	{
		return false;
	}

	return FInventorySwapper::SwapItems(*this, SlotA, SlotB);
}

void UInventoryManager::DropItem(UItemInstance* Item, FVector DropLocation)
{
	if (!HasInventoryWriteAuthority(TEXT("DropItem")))
	{
		return;
	}

	FInventoryRemover::DropItem(*this, Item, DropLocation);
}

void UInventoryManager::DropItemAtSlot(int32 SlotIndex, FVector DropLocation)
{
	if (!HasInventoryWriteAuthority(TEXT("DropItemAtSlot")))
	{
		return;
	}

	FInventoryRemover::DropItemAtSlot(*this, SlotIndex, DropLocation);
}

// ═══════════════════════════════════════════════
// STACKING
// ═══════════════════════════════════════════════

bool UInventoryManager::TryStackItem(UItemInstance* Item)
{
	if (!HasInventoryWriteAuthority(TEXT("TryStackItem")))
	{
		return false;
	}

	return FInventoryStackHandler::TryStackItem(*this, Item);
}

bool UInventoryManager::StackItems(UItemInstance* SourceItem, UItemInstance* TargetItem)
{
	if (!HasInventoryWriteAuthority(TEXT("StackItems")))
	{
		return false;
	}

	return FInventoryStackHandler::StackItems(*this, SourceItem, TargetItem);
}

UItemInstance* UInventoryManager::SplitStack(UItemInstance* Item, int32 Amount)
{
	if (!HasInventoryWriteAuthority(TEXT("SplitStack")))
	{
		return nullptr;
	}

	return FInventoryStackHandler::SplitStack(*this, Item, Amount);
}

// ═══════════════════════════════════════════════
// QUERIES
// ═══════════════════════════════════════════════

bool UInventoryManager::IsFull() const
{
	return FInventoryValidator::IsFull(*this);
}

bool UInventoryManager::IsOverweight() const
{
	return FInventoryValidator::IsOverweight(*this);
}

int32 UInventoryManager::GetItemCount() const
{
	return FInventoryWeightCalculator::GetItemCount(*this);
}

int32 UInventoryManager::GetAvailableSlots() const
{
	return FInventoryWeightCalculator::GetAvailableSlots(*this);
}

float UInventoryManager::GetTotalWeight() const
{
	return FInventoryWeightCalculator::GetTotalWeight(*this);
}

float UInventoryManager::GetRemainingWeight() const
{
	return FInventoryWeightCalculator::GetRemainingWeight(*this);
}

float UInventoryManager::GetWeightPercent() const
{
	return FInventoryWeightCalculator::GetWeightPercent(*this);
}

bool UInventoryManager::CanAddItem(UItemInstance* Item) const
{
	return FInventoryValidator::CanAddItem(*this, Item);
}

bool UInventoryManager::IsSlotEmpty(int32 SlotIndex) const
{
	return FInventoryValidator::IsSlotEmpty(*this, SlotIndex);
}

UItemInstance* UInventoryManager::GetItemAtSlot(int32 SlotIndex) const
{
	if (!Items.IsValidIndex(SlotIndex))
	{
		return nullptr;
	}

	return Items[SlotIndex];
}

int32 UInventoryManager::FindFirstEmptySlot() const
{
	return UInventoryFunctionLibrary::FindFirstEmptySlot(Items, MaxSlots);
}

int32 UInventoryManager::FindSlotForItem(UItemInstance* Item) const
{
	return UInventoryFunctionLibrary::FindSlotForItem(Items, Item);
}

bool UInventoryManager::ContainsItem(UItemInstance* Item) const
{
	// B-4 FIX: Delegates to FindSlotForItem so the search logic lives in one place.
	// Returns true if the item occupies any slot in this inventory.
	return FindSlotForItem(Item) != INDEX_NONE;
}

// ═══════════════════════════════════════════════
// SEARCH & FILTER
// ═══════════════════════════════════════════════

TArray<UItemInstance*> UInventoryManager::FindItemsByBaseID(FName BaseItemID) const
{
	return UInventoryFunctionLibrary::FindItemsByBaseID(Items, BaseItemID);
}

TArray<UItemInstance*> UInventoryManager::FindItemsByType(EItemType ItemType) const
{
	return UInventoryFunctionLibrary::FindItemsByType(Items, ItemType);
}

TArray<UItemInstance*> UInventoryManager::FindItemsByRarity(EItemRarity Rarity) const
{
	return UInventoryFunctionLibrary::FindItemsByRarity(Items, Rarity);
}

bool UInventoryManager::HasItemWithID(FGuid UniqueID) const
{
	return UInventoryFunctionLibrary::HasItemWithID(Items, UniqueID);
}

int32 UInventoryManager::GetTotalQuantityOfItem(FName BaseItemID) const
{
	return UInventoryFunctionLibrary::GetTotalQuantityOfItem(Items, BaseItemID);
}

// ═══════════════════════════════════════════════
// ORGANIZATION
// ═══════════════════════════════════════════════

void UInventoryManager::SortInventory(ESortMode SortMode)
{
	if (!HasInventoryWriteAuthority(TEXT("SortInventory")))
	{
		return;
	}

	UInventoryFunctionLibrary::SortItems(Items, SortMode, MaxSlots);

	BroadcastInventoryChanged();

	UE_LOG(LogInventoryManager, Log, TEXT("InventoryManager: Sorted inventory by %s"), 
		*UEnum::GetValueAsString(SortMode));
}

void UInventoryManager::CompactInventory()
{
	if (!HasInventoryWriteAuthority(TEXT("CompactInventory")))
	{
		return;
	}

	UInventoryFunctionLibrary::CompactItems(Items, MaxSlots);

	BroadcastInventoryChanged();

	UE_LOG(LogInventoryManager, Log, TEXT("InventoryManager: Compacted inventory (%d items)"), 
		GetItemCount());
}

void UInventoryManager::ClearAll()
{
	if (!HasInventoryWriteAuthority(TEXT("ClearAll")))
	{
		return;
	}

	Items.Empty(MaxSlots);
	
	BroadcastInventoryChanged();
	UpdateWeight();
	
	UE_LOG(LogInventoryManager, Log, TEXT("InventoryManager: Cleared all items"));
}

// ═══════════════════════════════════════════════
// WEIGHT MANAGEMENT (Hunter Manga)
// ═══════════════════════════════════════════════

void UInventoryManager::UpdateMaxWeightFromStrength(int32 Strength)
{
	if (!HasInventoryWriteAuthority(TEXT("UpdateMaxWeightFromStrength")))
	{
		return;
	}

	FInventoryWeightCalculator::UpdateMaxWeightFromStrength(*this, Strength);
}

void UInventoryManager::SetMaxWeight(float NewMaxWeight)
{
	if (!HasInventoryWriteAuthority(TEXT("SetMaxWeight")))
	{
		return;
	}

	FInventoryWeightCalculator::SetMaxWeight(*this, NewMaxWeight);
}

bool UInventoryManager::WouldExceedWeight(UItemInstance* Item) const
{
	return FInventoryWeightCalculator::WouldExceedWeight(*this, Item);
}

// ═══════════════════════════════════════════════
// PRIVATE HELPERS
// ═══════════════════════════════════════════════

void UInventoryManager::UpdateWeight()
{
	FInventoryWeightCalculator::BroadcastWeightChange(*this);
}

void UInventoryManager::BroadcastInventoryChanged()
{
	OnInventoryChanged.Broadcast();
}

UItemInstance* UInventoryManager::FindStackableItem(UItemInstance* Item) const
{
	return UInventoryFunctionLibrary::FindStackableItem(Items, Item);
}

void UInventoryManager::CleanupInvalidItems()
{
	for (int32 i = Items.Num() - 1; i >= 0; i--)
	{
		if (Items[i] && !Items[i]->HasValidBaseData())
		{
			Items[i] = nullptr;
		}
	}
}

bool UInventoryManager::HasInventoryWriteAuthority(const TCHAR* FunctionName) const
{
	const AActor* OwnerActor = GetOwner();
	if (OwnerActor && OwnerActor->HasAuthority())
	{
		return true;
	}

	UE_LOG(LogInventoryManager, Warning,
		TEXT("%s rejected: inventory writes must run on the server for Owner=%s."),
		FunctionName ? FunctionName : TEXT("InventoryWrite"),
		*GetNameSafe(OwnerActor));
	return false;
}

// ═══════════════════════════════════════════════
// N-04 FIX: Replication callback
// ═══════════════════════════════════════════════

void UInventoryManager::OnRep_Items()
{
	// Items array has been replicated from the server to the owning client.
	// Rebroadcast delegates so the inventory UI and weight bar refresh correctly.
	BroadcastInventoryChanged();
	UpdateWeight();

	UE_LOG(LogInventoryManager, Verbose, TEXT("OnRep_Items: inventory synced (%d slots occupied)"),
		GetItemCount());
}
