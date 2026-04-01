// Character/Component/InventoryManager.cpp

#include "Character/Component/InventoryManager.h"

#include "Character/Component/Library/InventoryEnum.h"
#include "Item/ItemInstance.h"
#include "Net/UnrealNetwork.h"
#include "Tower/Subsystem/GroundItemSubsystem.h"

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
	if (!Item || !Item->HasValidBaseData())
	{
		UE_LOG(LogInventoryManager, Warning, TEXT("InventoryManager: Cannot add invalid item"));
		return false;
	}

	// Check if can add
	if (!CanAddItem(Item))
	{
		UE_LOG(LogInventoryManager, Warning, TEXT("InventoryManager: Cannot add %s (full or too heavy)"),
			*Item->GetDisplayName().ToString());
		return false;
	}

	// Try to stack with existing items first
	if (bAutoStack && Item->IsStackable())
	{
		if (TryStackItem(Item))
		{
			// Fully stacked, item consumed
			UE_LOG(LogInventoryManager, Log, TEXT("InventoryManager: Stacked %s"), 
				*Item->GetDisplayName().ToString());
			
			BroadcastInventoryChanged();
			return true;
		}
	}

	// Add to first empty slot
	int32 EmptySlot = FindFirstEmptySlot();
	if (EmptySlot == INDEX_NONE)
	{
		UE_LOG(LogInventoryManager, Warning, TEXT("InventoryManager: No empty slots"));
		return false;
	}

	return AddItemToSlot(Item, EmptySlot);
}

bool UInventoryManager::AddItemToSlot(UItemInstance* Item, int32 SlotIndex)
{
	if (!Item || !Item->HasValidBaseData())
	{
		return false;
	}

	// Validate slot index
	if (SlotIndex < 0 || SlotIndex >= MaxSlots)
	{
		UE_LOG(LogInventoryManager, Warning, TEXT("InventoryManager: Invalid slot index %d"), SlotIndex);
		return false;
	}

	// Ensure array is large enough
	while (Items.Num() <= SlotIndex)
	{
		Items.Add(nullptr);
	}

	// Check if slot is empty
	if (Items[SlotIndex] != nullptr)
	{
		UE_LOG(LogInventoryManager, Warning, TEXT("InventoryManager: Slot %d is occupied"), SlotIndex);
		return false;
	}

	// Add item
	Items[SlotIndex] = Item;
	
	OnItemAdded.Broadcast(Item);
	BroadcastInventoryChanged();
	UpdateWeight();
	
	UE_LOG(LogInventoryManager, Log, TEXT("InventoryManager: Added %s to slot %d"), 
		*Item->GetDisplayName().ToString(), SlotIndex);

	return true;
}

bool UInventoryManager::RemoveItem(UItemInstance* Item)
{
	if (!Item)
	{
		return false;
	}

	int32 SlotIndex = FindSlotForItem(Item);
	if (SlotIndex == INDEX_NONE)
	{
		return false;
	}

	return RemoveItemAtSlot(SlotIndex) != nullptr;
}

UItemInstance* UInventoryManager::RemoveItemAtSlot(int32 SlotIndex)
{
	if (!Items.IsValidIndex(SlotIndex))
	{
		return nullptr;
	}

	UItemInstance* Item = Items[SlotIndex];
	if (!Item)
	{
		return nullptr;
	}

	Items[SlotIndex] = nullptr;
	
	OnItemRemoved.Broadcast(Item);
	BroadcastInventoryChanged();
	UpdateWeight();

	UE_LOG(LogInventoryManager, Log, TEXT("InventoryManager: Removed %s from slot %d"), 
		*Item->GetDisplayName().ToString(), SlotIndex);

	return Item;
}

bool UInventoryManager::RemoveQuantity(UItemInstance* Item, int32 Quantity)
{
	if (!Item || Quantity <= 0)
	{
		return false;
	}

	int32 ActualRemoved = Item->RemoveFromStack(Quantity);
	
	// If item is now consumed, remove from inventory
	if (Item->IsConsumed())
	{
		RemoveItem(Item);
	}
	else
	{
		BroadcastInventoryChanged();
		UpdateWeight();
	}

	return ActualRemoved > 0;
}

bool UInventoryManager::SwapItems(int32 SlotA, int32 SlotB)
{
	if (!Items.IsValidIndex(SlotA) || !Items.IsValidIndex(SlotB))
	{
		return false;
	}

	// Swap
	UItemInstance* Temp = Items[SlotA];
	Items[SlotA] = Items[SlotB];
	Items[SlotB] = Temp;

	BroadcastInventoryChanged();

	UE_LOG(LogInventoryManager, Log, TEXT("InventoryManager: Swapped slots %d and %d"), SlotA, SlotB);

	return true;
}

void UInventoryManager::DropItem(UItemInstance* Item, FVector DropLocation)
{
	if (!Item)
	{
		return;
	}

	// Remove from inventory
	if (!RemoveItem(Item))
	{
		return;
	}

	// N-05 FIX: GetWorld() was called without a null check. If the component's
	// world pointer is invalid (e.g. during level teardown), this would crash.
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogInventoryManager, Warning, TEXT("InventoryManager: DropItem called with null World — item lost"));
		return;
	}

	// Add to ground
	if (UGroundItemSubsystem* GroundSystem = World->GetSubsystem<UGroundItemSubsystem>())
	{
		GroundSystem->AddItemToGround(Item, DropLocation);
		UE_LOG(LogInventoryManager, Log, TEXT("InventoryManager: Dropped %s at %s"),
			*Item->GetDisplayName().ToString(), *DropLocation.ToString());
	}
	else
	{
		UE_LOG(LogInventoryManager, Warning, TEXT("InventoryManager: DropItem — GroundItemSubsystem not found, item lost"));
	}
}

void UInventoryManager::DropItemAtSlot(int32 SlotIndex, FVector DropLocation)
{
	UItemInstance* Item = GetItemAtSlot(SlotIndex);
	if (Item)
	{
		DropItem(Item, DropLocation);
	}
}

// ═══════════════════════════════════════════════
// STACKING
// ═══════════════════════════════════════════════

bool UInventoryManager::TryStackItem(UItemInstance* Item)
{
	if (!Item || !Item->IsStackable())
	{
		return false;
	}

	UItemInstance* StackTarget = FindStackableItem(Item);
	if (!StackTarget)
	{
		return false;
	}

	// Try to stack
	int32 Overflow = StackTarget->AddToStack(Item->Quantity);
	
	if (Overflow > 0)
	{
		// Partial stack, update source item quantity
		Item->Quantity = Overflow;
		Item->UpdateTotalWeight();
		return false;
	}
	else
	{
		// Fully stacked
		return true;
	}
}

bool UInventoryManager::StackItems(UItemInstance* SourceItem, UItemInstance* TargetItem)
{
	if (!SourceItem || !TargetItem)
	{
		return false;
	}

	if (!SourceItem->CanStackWith(TargetItem))
	{
		return false;
	}

	// Add source to target
	int32 Overflow = TargetItem->AddToStack(SourceItem->Quantity);
	
	if (Overflow > 0)
	{
		// Partial stack
		SourceItem->Quantity = Overflow;
		SourceItem->UpdateTotalWeight();
	}
	else
	{
		// Fully stacked, remove source
		RemoveItem(SourceItem);
	}

	BroadcastInventoryChanged();
	UpdateWeight();

	return true;
}

UItemInstance* UInventoryManager::SplitStack(UItemInstance* Item, int32 Amount)
{
	if (!Item || Amount <= 0)
	{
		return nullptr;
	}

	UItemInstance* NewItem = Item->SplitStack(Amount);
	
	if (NewItem)
	{
		// Try to add split item to inventory
		if (!AddItem(NewItem))
		{
			// Failed to add, merge back
			Item->AddToStack(NewItem->Quantity);
			return nullptr;
		}

		BroadcastInventoryChanged();
		UpdateWeight();
	}

	return NewItem;
}

// ═══════════════════════════════════════════════
// QUERIES
// ═══════════════════════════════════════════════

bool UInventoryManager::IsFull() const
{
	return GetAvailableSlots() <= 0;
}

bool UInventoryManager::IsOverweight() const
{
	return GetTotalWeight() > MaxWeight;
}

int32 UInventoryManager::GetItemCount() const
{
	int32 Count = 0;
	for (UItemInstance* Item : Items)
	{
		if (Item != nullptr)
		{
			Count++;
		}
	}
	return Count;
}

int32 UInventoryManager::GetAvailableSlots() const
{
	return MaxSlots - GetItemCount();
}

float UInventoryManager::GetTotalWeight() const
{
	float TotalWeight = 0.0f;
	
	for (UItemInstance* Item : Items)
	{
		if (Item)
		{
			TotalWeight += Item->GetTotalWeight();
		}
	}

	return TotalWeight;
}

float UInventoryManager::GetRemainingWeight() const
{
	return FMath::Max(0.0f, MaxWeight - GetTotalWeight());
}

float UInventoryManager::GetWeightPercent() const
{
	if (MaxWeight <= 0.0f)
	{
		return 0.0f;
	}
	
	return FMath::Clamp(GetTotalWeight() / MaxWeight, 0.0f, 1.0f);
}

bool UInventoryManager::CanAddItem(UItemInstance* Item) const
{
	if (!Item || !Item->HasValidBaseData())
	{
		return false;
	}

	// Check weight
	if (WouldExceedWeight(Item))
	{
		return false;
	}

	// Check if stackable
	if (bAutoStack && Item->IsStackable())
	{
		// Can stack with existing item?
		if (FindStackableItem(Item))
		{
			return true;
		}
	}

	// Check slot availability
	if (GetAvailableSlots() <= 0)
	{
		return false;
	}

	return true;
}

bool UInventoryManager::IsSlotEmpty(int32 SlotIndex) const
{
	if (!Items.IsValidIndex(SlotIndex))
	{
		return true;
	}
	
	return Items[SlotIndex] == nullptr;
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
	for (int32 i = 0; i < MaxSlots; i++)
	{
		if (IsSlotEmpty(i))
		{
			return i;
		}
	}

	return INDEX_NONE;
}

int32 UInventoryManager::FindSlotForItem(UItemInstance* Item) const
{
	if (!Item)
	{
		return INDEX_NONE;
	}

	for (int32 i = 0; i < Items.Num(); i++)
	{
		if (Items[i] == Item)
		{
			return i;
		}
	}

	return INDEX_NONE;
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
	TArray<UItemInstance*> FoundItems;
	
	for (UItemInstance* Item : Items)
	{
		if (Item && Item->BaseItemHandle.RowName == BaseItemID)
		{
			FoundItems.Add(Item);
		}
	}

	return FoundItems;
}

TArray<UItemInstance*> UInventoryManager::FindItemsByType(EItemType ItemType) const
{
	TArray<UItemInstance*> FoundItems;
	
	for (UItemInstance* Item : Items)
	{
		if (Item && Item->GetItemType() == ItemType)
		{
			FoundItems.Add(Item);
		}
	}

	return FoundItems;
}

TArray<UItemInstance*> UInventoryManager::FindItemsByRarity(EItemRarity Rarity) const
{
	TArray<UItemInstance*> FoundItems;
	
	for (UItemInstance* Item : Items)
	{
		if (Item && Item->Rarity == Rarity)
		{
			FoundItems.Add(Item);
		}
	}

	return FoundItems;
}

bool UInventoryManager::HasItemWithID(FGuid UniqueID) const
{
	for (const UItemInstance* Item : Items)
	{
		if (Item && Item->UniqueID == UniqueID)
		{
			return true;
		}
	}

	return false;
}

int32 UInventoryManager::GetTotalQuantityOfItem(FName BaseItemID) const
{
	int32 TotalQuantity = 0;
	
	for (UItemInstance* Item : Items)
	{
		if (Item && Item->BaseItemHandle.RowName == BaseItemID)
		{
			TotalQuantity += Item->Quantity;
		}
	}

	return TotalQuantity;
}

// ═══════════════════════════════════════════════
// ORGANIZATION
// ═══════════════════════════════════════════════

void UInventoryManager::SortInventory(ESortMode SortMode)
{
	// Collect all non-null items
	TArray<UItemInstance*> NonNullItems;
	NonNullItems.Reserve(GetItemCount());
	
	for (UItemInstance* Item : Items)
	{
		if (Item)
		{
			NonNullItems.Add(Item);
		}
	}

	// Sort based on mode
	switch (SortMode)
	{
		case ESortMode::SM_Type:
			NonNullItems.Sort([](const UItemInstance& A, const UItemInstance& B)
			{
				return static_cast<uint8>(A.GetItemType()) < static_cast<uint8>(B.GetItemType());
			});
			break;

		case ESortMode::SM_Rarity:
			NonNullItems.Sort([](const UItemInstance& A, const UItemInstance& B)
			{
				return static_cast<uint8>(A.Rarity) > static_cast<uint8>(B.Rarity); // Highest first
			});
			break;

		case ESortMode::SM_Name:
			NonNullItems.Sort([](const UItemInstance& A, const UItemInstance& B)
			{
				return A.GetBaseItemName().CompareTo(B.GetBaseItemName()) < 0;
			});
			break;

		case ESortMode::SM_Weight:
			NonNullItems.Sort([](const UItemInstance& A, const UItemInstance& B)
			{
				return A.GetTotalWeight() < B.GetTotalWeight();
			});
			break;

		case ESortMode::SM_Value:
			NonNullItems.Sort([](const UItemInstance& A, const UItemInstance& B)
			{
				return A.GetCalculatedValue() > B.GetCalculatedValue(); // Highest first
			});
			break;

		default:
			break;
	}

	// Rebuild array with sorted items
	Items.Empty(MaxSlots);
	Items.Append(NonNullItems);

	// Pad with nulls if needed
	while (Items.Num() < MaxSlots)
	{
		Items.Add(nullptr);
	}

	BroadcastInventoryChanged();

	UE_LOG(LogInventoryManager, Log, TEXT("InventoryManager: Sorted inventory by %s"), 
		*UEnum::GetValueAsString(SortMode));
}

void UInventoryManager::CompactInventory()
{
	// Remove all nulls and compact
	TArray<UItemInstance*> NonNullItems;
	NonNullItems.Reserve(GetItemCount());
	
	for (UItemInstance* Item : Items)
	{
		if (Item)
		{
			NonNullItems.Add(Item);
		}
	}

	Items.Empty(MaxSlots);
	Items.Append(NonNullItems);

	BroadcastInventoryChanged();

	UE_LOG(LogInventoryManager, Log, TEXT("InventoryManager: Compacted inventory (%d items)"), 
		NonNullItems.Num());
}

void UInventoryManager::ClearAll()
{
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
	float NewMaxWeight = Strength * WeightPerStrength;
	SetMaxWeight(NewMaxWeight);
}

void UInventoryManager::SetMaxWeight(float NewMaxWeight)
{
	MaxWeight = FMath::Max(0.0f, NewMaxWeight);
	UpdateWeight();
	
	UE_LOG(LogInventoryManager, Log, TEXT("InventoryManager: Max weight set to %.1f"), MaxWeight);
}

bool UInventoryManager::WouldExceedWeight(UItemInstance* Item) const
{
	if (!Item || MaxWeight <= 0.0f)
	{
		return false;
	}

	float ItemWeight = Item->GetTotalWeight();
	return (GetTotalWeight() + ItemWeight) > MaxWeight;
}

// ═══════════════════════════════════════════════
// PRIVATE HELPERS
// ═══════════════════════════════════════════════

void UInventoryManager::UpdateWeight()
{
	float CurrentWeight = GetTotalWeight();
	OnWeightChanged.Broadcast(CurrentWeight, MaxWeight);
}

void UInventoryManager::BroadcastInventoryChanged()
{
	OnInventoryChanged.Broadcast();
}

UItemInstance* UInventoryManager::FindStackableItem(UItemInstance* Item) const
{
	if (!Item || !Item->IsStackable())
	{
		return nullptr;
	}

	for (UItemInstance* ExistingItem : Items)
	{
		if (!ExistingItem)
		{
			continue;
		}

		if (ExistingItem->CanStackWith(Item))
		{
			// Found stackable item with available space
			if (ExistingItem->GetRemainingStackSpace() > 0)
			{
				return ExistingItem;
			}
		}
	}

	return nullptr;
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