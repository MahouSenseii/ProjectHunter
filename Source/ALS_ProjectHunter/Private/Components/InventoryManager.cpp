// Copyright@2024 Quentin Davis

#include "Components/InventoryManager.h"
#include "CharacterMovementComponentAsync.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Interactables/Pickups/ConsumablePickup.h"
#include "Interactables/Pickups/ItemPickup.h"
#include "Interactables/Pickups/WeaponPickup.h"
#include "Item/ConsumableItem.h"
#include "Kismet/KismetMathLibrary.h"

DEFINE_LOG_CATEGORY(LogInventoryManager);

UInventoryManager::UInventoryManager()
	: SpawnableItems(nullptr)
	, MasterDropList(nullptr)
	, MinMaxLootAmount()
	, RarityMinMaxClamp()
	, OwnerCharacter(nullptr)
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UInventoryManager::BeginPlay()
{
	Super::BeginPlay();
	InitializeInventoryManager();
	GenerateID();
}

void UInventoryManager::InitializeInventoryManager()
{
	// Set Owner Character
	if (GetOwner())
	{
		OwnerCharacter = Cast<APHBaseCharacter>(GetOwner());
		if (!OwnerCharacter)
		{
			UE_LOG(LogInventoryManager, Error, TEXT("Failed to cast owner to APHBaseCharacter."));
			return;
		}
	}
	else
	{
		UE_LOG(LogInventoryManager, Error, TEXT("InitializeInventoryManager: Failed to Get Owner."));
		return;
	}

	// Resize Inventory Grid
	ResizeInventory();
}

void UInventoryManager::GenerateID()
{
	InventoryID = FGuid::NewGuid().ToString();
}

APHBaseCharacter* UInventoryManager::SetPlayerCharacter()
{
	if (GetOwner())
	{
		OwnerCharacter = Cast<APHBaseCharacter>(GetOwner());
	}
	return OwnerCharacter;
}

void UInventoryManager::ResizeInventory()
{
	const int32 NewSize = Columns * Rows;
	InventoryList.SetNum(NewSize);
	ClearAllSlots();
}

void UInventoryManager::ClearAllSlots()
{
	// Clear all data structures
	for (auto& Item : InventoryList)
	{
		Item = nullptr;
	}
	
	TopLeftItemMap.Empty();
	ItemToTopLeftMap.Empty();
	OccupiedSlotCount = 0;
	InvalidateCaches();
	
	OnInventoryChanged.Broadcast();
	
	UE_LOG(LogInventoryManager, Log, TEXT("Inventory cleared."));
}

/* ============================= */
/* ===   Coordinate Conversion  === */
/* ============================= */

int32 UInventoryManager::TileToIndex(const FTile& Tile) const
{
	return Tile.Y * Columns + Tile.X;
}

FTile UInventoryManager::IndexToTile(int32 Index) const
{
	return FTile(Index % Columns, Index / Columns);
}

/* ============================= */
/* ===     Item Queries      === */
/* ============================= */

TMap<UBaseItem*, FTile> UInventoryManager::GetAllItems() const
{
	if (bAllItemsCacheDirty)
	{
		CachedAllItems.Empty();
		for (const auto& Pair : TopLeftItemMap)
		{
			if (IsValid(Pair.Value))
			{
				CachedAllItems.Add(Pair.Value, Pair.Key);
			}
		}
		bAllItemsCacheDirty = false;
	}
	return CachedAllItems;
}

bool UInventoryManager::GetItemAtTile(int32 Index, UBaseItem*& RetrievedItem) const
{
	if (InventoryList.IsValidIndex(Index))
	{
		RetrievedItem = InventoryList[Index];
		return IsValid(RetrievedItem);
	}
	
	RetrievedItem = nullptr;
	return false;
}

UBaseItem* UInventoryManager::GetItemAt(int32 Index) const
{
	return InventoryList.IsValidIndex(Index) ? InventoryList[Index] : nullptr;
}

bool UInventoryManager::ContainsItem(const UBaseItem* Item) const
{
	if (!Item) return false;
	// We need to iterate since we can't use const key in TMap with UPROPERTY
	for (const auto& Pair : ItemToTopLeftMap)
	{
		if (Pair.Key == Item)
		{
			return true;
		}
	}
	return false;
}

FTile UInventoryManager::GetItemTopLeft(const UBaseItem* Item) const
{
	// We need to iterate since we can't use const key in TMap with UPROPERTY
	for (const auto& Pair : ItemToTopLeftMap)
	{
		if (Pair.Key == Item)
		{
			return Pair.Value;
		}
	}
	return FTile(-1, -1);
}

/* ============================= */
/* ===     Item Addition     === */
/* ============================= */

bool UInventoryManager::TryToAddItemToInventory(UBaseItem* Item, bool CheckRotated)
{
	if (!IsValid(Item)) return false;

	// Try stacking first
	if (Item->IsStackable() && TryStackItem(Item))
	{
		return true;
	}

	// Try placing normally
	return TryPlaceItemAtFirstAvailable(Item, CheckRotated);
}

bool UInventoryManager::TryStackItem(UBaseItem* Item)
{
	if (!Item->IsStackable()) return false;

	const FName ItemID = Item->GetItemInfo().ItemInfo.ItemID;
	
	// Find stackable items with matching ID
	// Note: This iterates through all items, but for typical inventory sizes (100-200 items)
	// this is still very fast and avoids complex data structure issues with UPROPERTY
	for (const auto& Pair : TopLeftItemMap)
	{
		UBaseItem* ExistingItem = Pair.Value;
		if (IsValid(ExistingItem) && ExistingItem->GetItemInfo().ItemInfo.ItemID == ItemID)
		{
			if (CanStackItems(ExistingItem, Item))
			{
				ExistingItem->AddQuantity(Item->GetQuantity());
				Item->ConditionalBeginDestroy();
				OnInventoryChanged.Broadcast();
				return true;
			}
		}
	}
	
	return false;
}

bool UInventoryManager::TryPlaceItemAtFirstAvailable(UBaseItem* Item, bool CheckRotated)
{
	// Try normal orientation
	for (int32 y = 0; y < Rows; ++y)
	{
		for (int32 x = 0; x < Columns; ++x)
		{
			const FTile Tile(x, y);
			if (IsRoomAvailableAtTile(Item, Tile))
			{
				return InternalAddItemAt(Item, Tile);
			}
		}
	}

	// Try rotated if requested
	if (CheckRotated)
	{
		return TryToAddItemRotated(Item);
	}

	return false;
}

bool UInventoryManager::TryToAddItemRotated(UBaseItem* Item)
{
	if (!IsValid(Item)) return false;

	Item->SetRotated(true);
	
	for (int32 y = 0; y < Rows; ++y)
	{
		for (int32 x = 0; x < Columns; ++x)
		{
			const FTile Tile(x, y);
			if (IsRoomAvailableAtTile(Item, Tile))
			{
				return InternalAddItemAt(Item, Tile);
			}
		}
	}
	
	Item->SetRotated(false);
	return false;
}

bool UInventoryManager::TryToAddItemAt(UBaseItem* Item, int32 TopLeftIndex)
{
	if (!IsValid(Item)) return false;
	
	const FTile TopLeftTile = IndexToTile(TopLeftIndex);
	return TryToAddItemAtTile(Item, TopLeftTile);
}

bool UInventoryManager::TryToAddItemAtTile(UBaseItem* Item, const FTile& TopLeftTile)
{
	if (!IsValid(Item)) return false;

	// Check if we can stack at this position
	const int32 Index = TileToIndex(TopLeftTile);
	if (UBaseItem* ExistingItem = GetItemAt(Index))
	{
		if (CanStackItems(ExistingItem, Item))
		{
			ExistingItem->AddQuantity(Item->GetQuantity());
			Item->ConditionalBeginDestroy(); 
			OnInventoryChanged.Broadcast();
			return true;
		}
	}

	// Check if there's room for the item
	if (!IsRoomAvailableAtTile(Item, TopLeftTile))
	{
		return false;
	}

	return InternalAddItemAt(Item, TopLeftTile);
}

bool UInventoryManager::InternalAddItemAt(UBaseItem* Item, const FTile& TopLeftTile)
{
	SetItemInTiles(Item, TopLeftTile);
	RegisterItemInLookups(Item, TopLeftTile);
	InvalidateCaches();
	OnInventoryChanged.Broadcast();
	return true;
}

/* ============================= */
/* ===     Item Removal      === */
/* ============================= */

bool UInventoryManager::RemoveItemFromInventory(UBaseItem* Item)
{
	if (!IsValid(Item)) return false;

	const FTile* TopLeftPtr = ItemToTopLeftMap.Find(Item);
	if (!TopLeftPtr) return false;

	const FTile TopLeft = *TopLeftPtr;
	ClearItemFromTiles(Item, TopLeft);
	UnregisterItemFromLookups(Item);
	InvalidateCaches();
	OnInventoryChanged.Broadcast();
	
	return true;
}

bool UInventoryManager::DropItemFromInventory(UBaseItem* Item)
{
	if (!IsValid(Item))
	{
		UE_LOG(LogInventoryManager, Warning, TEXT("DropItemFromInventory: Invalid item."));
		return false;
	}

	// Validate pickup class
	if (!Item->GetItemInfo().ItemInfo.PickupClass)
	{
		UE_LOG(LogInventoryManager, Warning, TEXT("DropItemFromInventory: Item has no pickup class."));
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogInventoryManager, Warning, TEXT("DropItemFromInventory: Invalid world context."));
		return false;
	}

	// Setup spawn parameters
	const FVector DropLocation = GetDropLocation();
	const FRotator DropRotation = FRotator::ZeroRotator;
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	// Spawn appropriate pickup type
	AItemPickup* CreatedPickup = nullptr;
	const EItemType ItemType = Item->GetItemInfo().ItemInfo.ItemType;

	switch (ItemType)
	{
	case EItemType::IT_Weapon:
		CreatedPickup = World->SpawnActor<AWeaponPickup>(
			Item->GetItemInfo().ItemInfo.PickupClass, DropLocation, DropRotation, SpawnParams);
		break;

	case EItemType::IT_Armor:
	case EItemType::IT_Shield:
		CreatedPickup = World->SpawnActor<AEquipmentPickup>(
			Item->GetItemInfo().ItemInfo.PickupClass, DropLocation, DropRotation, SpawnParams);
		break;

	case EItemType::IT_Consumable:
		CreatedPickup = World->SpawnActor<AConsumablePickup>(
			Item->GetItemInfo().ItemInfo.PickupClass, DropLocation, DropRotation, SpawnParams);
		break;

	default:
		CreatedPickup = World->SpawnActor<AItemPickup>(
			Item->GetItemInfo().ItemInfo.PickupClass, DropLocation, DropRotation, SpawnParams);
		break;
	}

	if (CreatedPickup)
	{
		// Configure the pickup
		CreatedPickup->ItemInfo = Item->GetItemInfo();
		CreatedPickup->SetNewMesh(Item->GetItemInfo().ItemInfo.StaticMesh);
		CreatedPickup->SetupMesh();

		// Handle special cases
		if (ItemType == EItemType::IT_Consumable)
		{
			if (AConsumablePickup* ConsumablePickup = Cast<AConsumablePickup>(CreatedPickup))
			{
				if (UConsumableItem* ConsumableItem = Cast<UConsumableItem>(Item))
				{
					ConsumablePickup->ConsumableData = ConsumableItem->GetConsumableData();
				}
			}
		}

		// Remove from inventory after successful spawn
		RemoveItemFromInventory(Item);
		return true;
	}

	UE_LOG(LogInventoryManager, Error, TEXT("Failed to spawn pickup for item %s"), 
		*Item->GetItemInfo().ItemInfo.ItemName.ToString());
	return false;
}

/* ============================= */
/* ===    Space Checking     === */
/* ============================= */

bool UInventoryManager::IsRoomAvailable(UBaseItem* Item, int32 TopLeftIndex) const
{
	if (!IsValid(Item)) return false;
	const FTile TopLeftTile = IndexToTile(TopLeftIndex);
	return IsRoomAvailableAtTile(Item, TopLeftTile);
}

bool UInventoryManager::IsRoomAvailableAtTile(UBaseItem* Item, const FTile& TopLeftTile) const
{
	if (!IsValid(Item)) return false;

	const FIntPoint Dimensions = Item->GetDimensions();
	
	// Bounds check first
	if (TopLeftTile.X + Dimensions.X > Columns || TopLeftTile.Y + Dimensions.Y > Rows)
	{
		return false;
	}
	
	if (TopLeftTile.X < 0 || TopLeftTile.Y < 0)
	{
		return false;
	}

	// Check each tile for occupancy
	bool bHasRoom = true;
	ForEachTile(Item, TopLeftTile, [&](const FTile& Tile) -> bool
	{
		const int32 Index = TileToIndex(Tile);
		if (InventoryList.IsValidIndex(Index) && InventoryList[Index] != nullptr)
		{
			bHasRoom = false;
			return true; // Stop iteration
		}
		return false; // Continue
	});

	return bHasRoom;
}

bool UInventoryManager::IsTileValid(const FTile& Tile) const
{
	return Tile.X >= 0 && Tile.Y >= 0 && Tile.X < Columns && Tile.Y < Rows;
}

/* ============================= */
/* ===    Helper Functions   === */
/* ============================= */

bool UInventoryManager::AreItemsStackable(const UBaseItem* A, const UBaseItem* B)
{
	if (!A || !B) return false;
	
	return A->IsStackable() && 
		   B->IsStackable() && 
		   A->GetItemInfo().ItemInfo.ItemID == B->GetItemInfo().ItemInfo.ItemID;
}

bool UInventoryManager::CanStackItems(const UBaseItem* ExistingItem, const UBaseItem* NewItem)
{
	if (!AreItemsStackable(ExistingItem, NewItem)) return false;
	
	const int32 MaxStack = ExistingItem->GetItemInfo().ItemInfo.MaxStackSize;
	if (MaxStack <= 0) return true; // No limit
	
	const int32 CurrentQty = ExistingItem->GetQuantity();
	const int32 NewQty = NewItem->GetQuantity();
	
	return (CurrentQty + NewQty) <= MaxStack;
}

TArray<FTile> UInventoryManager::GetOccupiedTilesForItem(const UBaseItem* Item) const
{
	TArray<FTile> OccupiedTiles;
	if (!IsValid(Item)) return OccupiedTiles;

	// Find the item's top-left position
	FTile TopLeft = GetItemTopLeft(Item);
	if (TopLeft.X < 0 || TopLeft.Y < 0) return OccupiedTiles;

	const FIntPoint Dimensions = Item->GetDimensions();
	OccupiedTiles.Reserve(Dimensions.X * Dimensions.Y);

	for (int32 x = 0; x < Dimensions.X; ++x)
	{
		for (int32 y = 0; y < Dimensions.Y; ++y)
		{
			OccupiedTiles.Add(FTile(TopLeft.X + x, TopLeft.Y + y));
		}
	}

	return OccupiedTiles;
}

/* ============================= */
/* ===   Internal Helpers    === */
/* ============================= */

void UInventoryManager::SetItemInTiles(UBaseItem* Item, const FTile& TopLeft)
{
	if (!IsValid(Item)) return;

	const FIntPoint Dimensions = Item->GetDimensions();
	int32 TilesSet = 0;

	for (int32 X = 0; X < Dimensions.X; ++X)
	{
		for (int32 Y = 0; Y < Dimensions.Y; ++Y)
		{
			const FTile Tile(TopLeft.X + X, TopLeft.Y + Y);
			const int32 Index = TileToIndex(Tile);

			if (InventoryList.IsValidIndex(Index))
			{
				InventoryList[Index] = Item;
				TilesSet++;
			}
			else
			{
				UE_LOG(LogInventoryManager, Warning, 
					TEXT("SetItemInTiles: Invalid index for Tile (%d, %d)"), Tile.X, Tile.Y);
			}
		}
	}

	OccupiedSlotCount += TilesSet;
}

void UInventoryManager::ClearItemFromTiles(const UBaseItem* Item, const FTile& TopLeft)
{
	if (!IsValid(Item)) return;

	const FIntPoint Dimensions = Item->GetDimensions();
	int32 TilesCleared = 0;

	for (int32 X = 0; X < Dimensions.X; ++X)
	{
		for (int32 Y = 0; Y < Dimensions.Y; ++Y)
		{
			const FTile Tile(TopLeft.X + X, TopLeft.Y + Y);
			const int32 Index = TileToIndex(Tile);

			if (InventoryList.IsValidIndex(Index) && InventoryList[Index] == Item)
			{
				InventoryList[Index] = nullptr;
				TilesCleared++;
			}
		}
	}

	OccupiedSlotCount -= TilesCleared;
}

void UInventoryManager::RegisterItemInLookups(UBaseItem* Item, const FTile& TopLeft)
{
	if (!IsValid(Item)) return;

	// Add to position maps
	TopLeftItemMap.Add(TopLeft, Item);
	ItemToTopLeftMap.Add(Item, TopLeft);
}

void UInventoryManager::UnregisterItemFromLookups(UBaseItem* Item)
{
	if (!IsValid(Item)) return;

	// Remove from position maps
	if (const FTile* TopLeft = ItemToTopLeftMap.Find(Item))
	{
		TopLeftItemMap.Remove(*TopLeft);
	}
	ItemToTopLeftMap.Remove(Item);
}

void UInventoryManager::InvalidateCaches() const
{
	bAllItemsCacheDirty = true;
}

bool UInventoryManager::ForEachTile(const UBaseItem* Item, const FTile& StartTile, 
	TFunctionRef<bool(const FTile&)> Func)
{
	if (!Item) return false;

	const FIntPoint Dimensions = Item->GetDimensions();
	
	for (int32 X = 0; X < Dimensions.X; ++X)
	{
		for (int32 Y = 0; Y < Dimensions.Y; ++Y)
		{
			const FTile Tile(StartTile.X + X, StartTile.Y + Y);
			if (Func(Tile))
			{
				return true; // Early exit requested
			}
		}
	}
	return false;
}

FVector UInventoryManager::GetDropLocation() const
{
	if (!OwnerCharacter) return FVector::ZeroVector;

	// Get randomized forward direction
	const FVector ForwardVector = OwnerCharacter->GetActorForwardVector();
	const FVector RandomOffset = UKismetMathLibrary::RandomUnitVector() * FVector(1.f, 1.f, 0.f);
	const FVector Direction = (ForwardVector + RandomOffset * 0.3f).GetSafeNormal();

	// Calculate spawn location
	FVector SpawnLocation = OwnerCharacter->GetActorLocation() + (Direction * ItemDropDistance);

	// Find floor
	if (UCharacterMovementComponent* Movement = OwnerCharacter->GetCharacterMovement())
	{
		FFindFloorResult OutFloorResult;
		Movement->FindFloor(SpawnLocation, OutFloorResult, true);
		
		if (OutFloorResult.bBlockingHit)
		{
			SpawnLocation.Z = OutFloorResult.HitResult.ImpactPoint.Z + ItemDropHeightOffset;
		}
	}

	return SpawnLocation;
}

/* ============================= */
/* ===     Validation        === */
/* ============================= */

#if WITH_EDITOR
void UInventoryManager::ValidateInventoryIntegrity() const
{
	// Validate TopLeftItemMap matches InventoryList
	for (const auto& Pair : TopLeftItemMap)
	{
		const FTile& TopLeft = Pair.Key;
		UBaseItem* Item = Pair.Value;
		
		if (!IsValid(Item))
		{
			UE_LOG(LogInventoryManager, Error, TEXT("Invalid item in TopLeftItemMap at (%d, %d)"), 
				TopLeft.X, TopLeft.Y);
			continue;
		}

		// Check all tiles this item should occupy
		const FIntPoint Dimensions = Item->GetDimensions();
		for (int32 x = 0; x < Dimensions.X; ++x)
		{
			for (int32 y = 0; y < Dimensions.Y; ++y)
			{
				const FTile Tile(TopLeft.X + x, TopLeft.Y + y);
				const int32 Index = TileToIndex(Tile);

				if (!InventoryList.IsValidIndex(Index))
				{
					UE_LOG(LogInventoryManager, Error, 
						TEXT("Item %s extends outside inventory bounds at tile (%d, %d)"),
						*Item->GetItemInfo().ItemInfo.ItemName.ToString(), Tile.X, Tile.Y);
				}
				else if (InventoryList[Index] != Item)
				{
					UE_LOG(LogInventoryManager, Error,
						TEXT("Tile (%d, %d) doesn't reference expected item %s"),
						Tile.X, Tile.Y, *Item->GetItemInfo().ItemInfo.ItemName.ToString());
				}
			}
		}
	}

	// Validate reverse lookup
	if (ItemToTopLeftMap.Num() != TopLeftItemMap.Num())
	{
		UE_LOG(LogInventoryManager, Error, 
			TEXT("ItemToTopLeftMap size (%d) doesn't match TopLeftItemMap size (%d)"),
			ItemToTopLeftMap.Num(), TopLeftItemMap.Num());
	}

	// Validate occupied slot count
	int32 ActualOccupied = 0;
	for (const auto& Slot : InventoryList)
	{
		if (IsValid(Slot)) ActualOccupied++;
	}
	
	if (ActualOccupied != OccupiedSlotCount)
	{
		UE_LOG(LogInventoryManager, Error,
			TEXT("OccupiedSlotCount (%d) doesn't match actual count (%d)"),
			OccupiedSlotCount, ActualOccupied);
	}

	UE_LOG(LogInventoryManager, Log, TEXT("Inventory validation complete."));
}
#endif