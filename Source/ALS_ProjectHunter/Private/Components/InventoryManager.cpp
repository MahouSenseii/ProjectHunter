// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/InventoryManager.h"

#include "CharacterMovementComponentAsync.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Interactables/Pickups/ConsumablePickup.h"
#include "Interactables/Pickups/ItemPickup.h"
#include "Interactables/Pickups/WeaponPickup.h"
#include "Item/ConsumableItem.h"
#include "Kismet/KismetMathLibrary.h"
DEFINE_LOG_CATEGORY(LogInventoryManager)
// Sets default values for this component's properties
UInventoryManager::UInventoryManager(): RarityMinMaxClamp(), SpawnableItems(nullptr), MasterDropList(nullptr),
                                        MinMaxLootAmount(), OwnerCharacter(nullptr)
{
	// Set this component to be initialized when the game starts and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}


// Called when the game starts
void UInventoryManager::BeginPlay()
{
	Super::BeginPlay();

	// Initialize Inventory Manager and associated properties
	InitializeInventoryManager();
	GenerateID();
	// ...
	
}

void UInventoryManager::GenerateID()
{
	// Generate a new GUID
	const FGuid NewGuid = FGuid::NewGuid();

	// Convert the GUID to a string
	InventoryID = NewGuid.ToString();
}


void UInventoryManager::InitializeInventoryManager()
{
	// Set Owner Character
	if (GetOwner())
	{
		OwnerCharacter = Cast<APHBaseCharacter>(GetOwner());
		if (!OwnerCharacter)
		{
			UE_LOG(LogInventoryManager, Error, TEXT("Failed to cast owner to AALSBaseCharacter."));
			return;
		}
	}
	else
	{
		UE_LOG(LogInventoryManager, Error, TEXT("InventoryManager: InitializeInventoryManager :Failed to  Get Owner  ."));
		return;
	}

	// Resize Inventory Grid
	ResizeInventory();
}

APHBaseCharacter* UInventoryManager::SetPlayerCharacter()
{
	if(GetOwner())
	{
		OwnerCharacter = Cast<APHBaseCharacter>(GetOwner());
	}
	return OwnerCharacter;
}

APHBaseCharacter* UInventoryManager::GetOwnerCharacter() const
{
	return OwnerCharacter;
}


FVector UInventoryManager::GetSpawnLocation() const
{
	check(OwnerCharacter);
	// Get the forward vector of the OwnerCharacter and create a random direction vector from it
	FVector ForwardVector = OwnerCharacter->GetActorForwardVector();
	FVector RandomUnitVector = UKismetMathLibrary::RandomUnitVector() * FVector(1.f, 1.f, 0.f); // Zero out Z to keep it horizontal
	FVector Direction = (ForwardVector + RandomUnitVector).GetSafeNormal(); // Ensures the vector has a length of 1

	// Define a spawn distance in front of the character
	float SpawnDistance = 200.f;
	FVector SpawnLocation = OwnerCharacter->GetActorLocation() + (Direction * SpawnDistance);

	// Trace down to find the floor from this point
	FFindFloorResult OutFloorResult;
	OwnerCharacter->GetCharacterMovement()->FindFloor(SpawnLocation, OutFloorResult, true);
	if (OutFloorResult.bBlockingHit) // Check if the floor was found
	{
		// Offset the Z by a certain amount to place the item above the floor
		return FVector(SpawnLocation.X, SpawnLocation.Y, OutFloorResult.HitResult.ImpactPoint.Z + 120.0f);
	}
	else
	{
		// If no floor is found, return the SpawnLocation without the Z offset
		return SpawnLocation;
	}
}

void UInventoryManager::SetItemInTiles(UBaseItem* Item, const FTile& TopLeft)
{
	if (!IsValid(Item)) return;

	// Save to the TopLeft map (in case it wasn't already)
	TopLeftItemMap.FindOrAdd(TopLeft) = Item;

	const FIntPoint Dimensions = Item->GetDimensions();  // Use rotated-aware getter

	for (int32 X = 0; X < Dimensions.X; ++X)
	{
		for (int32 Y = 0; Y < Dimensions.Y; ++Y)
		{
			const FTile Tile(TopLeft.X + X, TopLeft.Y + Y);
			const int32 Index = TileToIndex(Tile);

			if (InventoryList.IsValidIndex(Index))
			{
				InventoryList[Index] = Item;
			}
			else
			{
				UE_LOG(LogInventoryManager, Warning, TEXT("SetItemInTiles: Invalid index for Tile (%d, %d)"), Tile.X, Tile.Y);
			}
		}
	}
}


void UInventoryManager::ClearItemFromTiles(UBaseItem* Item, const FTile& TopLeft)
{
	if (!IsValid(Item)) return;

	// Double-check TopLeftItemMap consistency (optional warning)
	if (!TopLeftItemMap.Contains(TopLeft) || TopLeftItemMap[TopLeft] != Item)
	{
		UE_LOG(LogInventoryManager, Warning, TEXT("ClearItemFromTiles: TopLeft map mismatch for item %s at tile (%d, %d)"),
			*Item->GetItemInfo().ItemInfo.ItemName.ToString(), TopLeft.X, TopLeft.Y);
	}

	const FIntPoint Dimensions = Item->GetDimensions();

	for (int32 X = 0; X < Dimensions.X; ++X)
	{
		for (int32 Y = 0; Y < Dimensions.Y; ++Y)
		{
			const FTile Tile(TopLeft.X + X, TopLeft.Y + Y);
			const int32 Index = TileToIndex(Tile);

			if (InventoryList.IsValidIndex(Index) && InventoryList[Index] == Item)
			{
				InventoryList[Index] = nullptr;
			}
		}
	}

	// Remove from TopLeft map
	TopLeftItemMap.Remove(TopLeft);
}


bool UInventoryManager::AreItemsStackable(UBaseItem* A, UBaseItem* B)
{
		if (!A || !B) return false;
    
    	return A->IsStackable() &&
    	       B->IsStackable() &&
    	       	A->GetItemInfo().ItemInfo.ItemID == B->GetItemInfo().ItemInfo.ItemID;

}

bool UInventoryManager::CanAcceptItemAt(UBaseItem* NewItem, const int32 Index)
{
	if (!IsValid(NewItem) || !InventoryList.IsValidIndex(Index))
	{
		UE_LOG(LogInventoryManager, Warning, TEXT("CanAcceptItemAt: Invalid item or index %d"), Index);
		return false;
	}

	UBaseItem* ExistingItem = InventoryList[Index];

	// 🟢 Case 1: Stacking into an existing stackable item
	if (AreItemsStackable(ExistingItem, NewItem))
	{
		const int32 MaxStack = ExistingItem->GetItemInfo().ItemInfo.MaxStackSize;
		const int32 ExistingQty = ExistingItem->GetQuantity();
		const int32 NewQty = NewItem->GetQuantity();

		if (MaxStack == 0 || (ExistingQty + NewQty) <= MaxStack)
		{
			return true;
		}
		else
		{
			UE_LOG(LogInventoryManager, Warning, TEXT("CanAcceptItemAt: Stack size would exceed max (%d + %d > %d)"), ExistingQty, NewQty, MaxStack);
			return false;
		}
	}

	// 🟡 Case 2: Empty slot — check if the entire item can fit starting from Index
	if (!ExistingItem)
	{
		return IsRoomAvailable(NewItem, Index);
	}

	// 🔴 Case 3: Slot is occupied with a different item — reject
	return false;
}


UBaseItem* UInventoryManager::GetItemAt(int32 Index) const
{
	return InventoryList.IsValidIndex(Index) ? InventoryList[Index] : nullptr;
}

TArray<FTile> UInventoryManager::GetOccupiedTilesForItem(UBaseItem* Item) const
{
	if (!IsValid(Item)) return {};

	const FTile* TopLeftPtr = TopLeftItemMap.FindKey(Item);
	if (!TopLeftPtr) return {};

	const FTile TopLeft = *TopLeftPtr;
	const FIntPoint Dimensions = Item->GetDimensions();

	TArray<FTile> OccupiedTiles;
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

void UInventoryManager::ValidateTileMap()
{
	for (const auto& Pair : TopLeftItemMap)
	{
		UBaseItem* Item = Pair.Value;
		if (!IsValid(Item)) continue;

		const FTile TopLeft = Pair.Key;
		const FIntPoint Dimensions = Item->GetDimensions();

		for (int32 x = 0; x < Dimensions.X; ++x)
		{
			for (int32 y = 0; y < Dimensions.Y; ++y)
			{
				const FTile Tile(TopLeft.X + x, TopLeft.Y + y);
				const int32 Index = TileToIndex(Tile);

				if (!InventoryList.IsValidIndex(Index))
				{
					UE_LOG(LogInventoryManager, Error, TEXT("[ValidateTileMap] Invalid index for tile (%d, %d)"), Tile.X, Tile.Y);
					continue;
				}

				if (InventoryList[Index] != Item)
				{
					UE_LOG(LogInventoryManager, Error, TEXT("[ValidateTileMap] Tile (%d, %d) does not reference item %s (Expected: %s)"),
						Tile.X, Tile.Y,
						*GetNameSafe(InventoryList[Index]),
						*Item->GetItemInfo().ItemInfo.ItemName.ToString());
				}
			}
		}
	}
}

void UInventoryManager::ResizeInventory()
{
	InventoryList.SetNum(Columns * Rows);
	ClearAllSlots();
}

void UInventoryManager::ClearAllSlots()
{
	// 1. Clear tile occupation
	for (auto& Item : InventoryList)
	{
		Item = nullptr;
	}

	// 2. Clear top-left tracking
	TopLeftItemMap.Empty();

	// 3. Optional: broadcast inventory change
	OnInventoryChanged.Broadcast();

	// 4. Debug log
	UE_LOG(LogInventoryManager, Warning, TEXT("InventoryList and TopLeftItemMap have been cleared."));
}


int32 UInventoryManager::TileToIndex(FTile Tile)
{
	int32 Temp = Tile.Y * Columns;
	Temp  = Tile.X + Temp;
	return Temp;
}

void UInventoryManager::IndexToTile(int32 Index, FTile& Tile)
{
	Tile.X = Index % Columns;
	Tile.Y = Index / Columns;
}

TMap<UBaseItem*, FTile> UInventoryManager::GetAllItems()
{
	TMap<UBaseItem*, FTile> AllItemsLocal;

	for (const auto& Pair : TopLeftItemMap)
	{
		if (IsValid(Pair.Value))
		{
			AllItemsLocal.Add(Pair.Value, Pair.Key);
		}
		else
		{
			UE_LOG(LogInventoryManager, Warning, TEXT("GetAllItems: Found null item at tile (%d, %d)"), Pair.Key.X, Pair.Key.Y);
		}
	}

	return AllItemsLocal;
}


void UInventoryManager::RemoveItemInInventory(UBaseItem* Item)
{
	if (!IsValid(Item)) return;

	const FTile* TopLeftPtr = TopLeftItemMap.FindKey(Item);
	if (!TopLeftPtr) return;

	const FTile TopLeft = *TopLeftPtr;

	// Step 1: Clear the tiles
	ClearItemFromTiles(Item, TopLeft);

	// Step 2: Remove the top-left reference
	TopLeftItemMap.Remove(TopLeft);

	// Step 3: Broadcast change
	OnInventoryChanged.Broadcast();
}

void UInventoryManager::ForEachOccupiedTile(UBaseItem* Item, const TFunctionRef<void(const FTile&)>& Func) const
{
	if (!IsValid(Item)) return;

	const FTile* TopLeftPtr = TopLeftItemMap.FindKey(Item);
	if (!TopLeftPtr) return;

	const FIntPoint Dimensions = Item->GetDimensions();

	for (int32 X = 0; X < Dimensions.X; ++X)
	{
		for (int32 Y = 0; Y < Dimensions.Y; ++Y)
		{
			Func(FTile(TopLeftPtr->X + X, TopLeftPtr->Y + Y));
		}
	}
}


bool UInventoryManager::DropItemInInventory(UBaseItem* Item)
{
    if (!IsValid(Item))
    {
        UE_LOG(LogInventoryManager, Warning, TEXT("DropItemInInventory: Item is invalid."));
        return false;
    }

    // Validate pickup class first
    if (!Item->GetItemInfo().ItemInfo.PickupClass)
    {
        UE_LOG(LogInventoryManager, Warning, TEXT("DropItemInInventory: Item has no pickup class."));
        return false;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogInventoryManager, Warning, TEXT("DropItemInInventory: Invalid world context."));
        return false;
    }

    // Determine spawn location and parameters
    const FVector DropLocation = GetSpawnLocation();
    const FRotator DropRotation = FRotator::ZeroRotator;
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    // Spawn the pickup actor based on the item type
    AItemPickup* CreatedPickup = nullptr;

    switch (Item->GetItemInfo().ItemInfo.ItemType)
    {
    case EItemType::IT_Weapon:
        CreatedPickup = World->SpawnActor<AWeaponPickup>(Item->GetItemInfo().ItemInfo.PickupClass, DropLocation, DropRotation, SpawnParams);
        if (AWeaponPickup* WeaponPickup = Cast<AWeaponPickup>(CreatedPickup))
        {
            WeaponPickup->ItemInfo = Item->GetItemInfo();
            WeaponPickup->ItemInfo.ItemData = Item->GetItemInfo().ItemData;
        }
        break;

    case EItemType::IT_Armor:
    case EItemType::IT_Shield:
        CreatedPickup = World->SpawnActor<AEquipmentPickup>(Item->GetItemInfo().ItemInfo.PickupClass, DropLocation, DropRotation, SpawnParams);
        if (AEquipmentPickup* EquipPickup = Cast<AEquipmentPickup>(CreatedPickup))
        {
            EquipPickup->ItemInfo = Item->GetItemInfo();
            EquipPickup->ItemInfo.ItemData = Item->GetItemInfo().ItemData;
        }
        break;

    case EItemType::IT_Consumable:
        CreatedPickup = World->SpawnActor<AConsumablePickup>(Item->GetItemInfo().ItemInfo.PickupClass, DropLocation, DropRotation, SpawnParams);
        if (AConsumablePickup* ConsumablePickup = Cast<AConsumablePickup>(CreatedPickup))
        {
            ConsumablePickup->ItemInfo = Item->GetItemInfo();
            if (UConsumableItem* ConsumableItem = Cast<UConsumableItem>(Item))
            {
                ConsumablePickup->ConsumableData = ConsumableItem->GetConsumableData();
            }
        }
        break;

    default:
        CreatedPickup = World->SpawnActor<AItemPickup>(Item->GetItemInfo().ItemInfo.PickupClass, DropLocation, DropRotation, SpawnParams);
        if (CreatedPickup)
        {
            CreatedPickup->ItemInfo = Item->GetItemInfo();
        }
        break;
    }

    // Only remove from inventory if spawn succeeded
    if (CreatedPickup)
    {
        CreatedPickup->SetNewMesh(Item->GetItemInfo().ItemInfo.StaticMesh);
        CreatedPickup->SetupMesh();
        
        // NOW remove from inventory after successful spawn
        RemoveItemInInventory(Item);
        return true;
    }

    UE_LOG(LogInventoryManager, Error, TEXT("Failed to spawn pickup for item %s"), *Item->GetItemInfo().ItemInfo.ItemName.ToString());
    return false;
}


bool UInventoryManager::GetItemAtTile(int32 Index, UBaseItem*& RetrievedItem)
{
	if (InventoryList.IsValidIndex(Index))
	{
		RetrievedItem = InventoryList[Index];
		return true;
	}
	else
	{
		RetrievedItem = nullptr;
		return false;
	}
}

bool UInventoryManager::IsRoomAvailable(UBaseItem* Item, int32 TopLeftIndex)
{
	if (!IsValid(Item))
	{
		UE_LOG(LogInventoryManager, Warning, TEXT("Item is not valid."));
		return false;
	}

	bool bIsRoomAvailable = true;  // Assume there is a room available by default
	ForEachTile(Item, TopLeftIndex, [&](FTile Tile) -> bool
	{
		const int32 Index = TileToIndex(Tile);
    
		if (InventoryList.IsValidIndex(Index) && IsValid(InventoryList[Index]))
		{
			bIsRoomAvailable = false;
			return true; // Signal early exit
		}
		return false; // Continue processing
	});

	return bIsRoomAvailable;
}

bool UInventoryManager::ForEachTile(UBaseItem* Item, int32 TopLeftIndex, const TFunction<bool(FTile)>& Func)
{
	if (!Item)
	{
		UE_LOG(LogInventoryManager, Warning, TEXT("Item is null. Aborting ForEachTile."));
		return false;
	}

	FTile TempTile;
	IndexToTile(TopLeftIndex, TempTile);

	const FIntPoint Dimensions = Item->GetDimensions(); // Use FIntPoint instead
	if (Dimensions.X <= 0 || Dimensions.Y <= 0)
	{
		UE_LOG(LogInventoryManager, Warning, TEXT("Invalid item dimensions. Aborting ForEachTile."));
		return false;
	}

	for (int32 x = TempTile.X; x < TempTile.X + Dimensions.X; ++x)
	{
		for (int32 y = TempTile.Y; y < TempTile.Y + Dimensions.Y; ++y)
		{
			const FTile Tile(x, y);
            
			// Bounds checking
			if (!IsTileValid(Tile))
			{
				UE_LOG(LogInventoryManager, Warning, TEXT("Tile (%d, %d) is out of bounds"), x, y);
				return false;
			}

			// Allow early exit - return false from lambda to continue, true to break
			if (Func(Tile))
			{
				return false; // Early exit requested
			}
		}
	}
	return true;
}


bool UInventoryManager::IsTileValid(const FTile Tile) const
{
	return (Tile.X >= 0 && Tile.Y >= 0 && Tile.X < Columns && Tile.Y < Rows);
}

bool UInventoryManager::ContainsItem(const UBaseItem* Item) const
{
	if (!Item) return false;  // Early return if the item is null.

	for (const UBaseItem* InventoryItem : InventoryList)
	{
		// Check if the current item in the inventory matches the item we're looking for.
		if (InventoryItem->GetUniqueID() == Item->GetUniqueID())
		{
			return true;  // Item found in the inventory.
		}
	}

	return false;  // Item not found.
}

bool UInventoryManager::TryToAddItemToInventory(UBaseItem* Item, const bool CheckRotated)
{


	if (Item->IsStackable())
	{
		for (UBaseItem* ExistingItem : InventoryList)
		{
			if (AreItemsStackable(ExistingItem, Item))
			{
				ExistingItem->AddQuantity(Item->GetQuantity());
				Item->ConditionalBeginDestroy(); // Or safely discard
				OnInventoryChanged.Broadcast();
				return true;
			}
		}
	}
	
	if (IsValid(Item))
	{
		for (int32 x = 0; x < InventoryList.Num(); x++)
		{
			if (IsRoomAvailable(Item, x))
			{
				AddItemAt(Item, x);
				return true;
			}
		}
	}
	if(CheckRotated)
	{
		return TryToAddItemToInventoryRotated(Item);
	}
	return false;
}

// The AddItemAt function
void UInventoryManager::AddItemAt(UBaseItem* Item, int32 TopLeftIndex)
{
	FTile TopLeftTile;
	IndexToTile(TopLeftIndex, TopLeftTile);

	TopLeftItemMap.Add(TopLeftTile, Item);
	SetItemInTiles(Item, TopLeftTile);

	OnInventoryChanged.Broadcast();
}

bool UInventoryManager::TryToAddItemToInventoryRotated(UBaseItem* Item)
{
	if (IsValid(Item))
	{
		
		Item->SetRotated(true);
		
		for(int32 x = 0; x < InventoryList.Num(); x++)
		{
			if(IsRoomAvailable(Item, x))
			{
				AddItemAt(Item, x);
				return true;
			}
		}
	}
	
	Item->SetRotated(false);
	return false;
}

void UInventoryManager::RebuildTopLeftMap()
{
	TopLeftItemMap.Empty();
	FTile Tile;
	for (int32 Index = 0; Index < InventoryList.Num(); ++Index)
	{
		if (UBaseItem* Item = InventoryList[Index])
		{
			IndexToTile(Index, Tile);
			if (!TopLeftItemMap.Contains(Tile))
			{
				TopLeftItemMap.Add(Tile, Item);
			}
		}
	}
}



