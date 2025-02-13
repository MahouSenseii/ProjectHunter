// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/InventoryManager.h"

#include "CharacterMovementComponentAsync.h"
#include "Components/InteractableManager.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Interactables/Pickups/ConsumablePickup.h"
#include "Interactables/Pickups/ItemPickup.h"
#include "Interactables/Pickups/WeaponPickup.h"
#include "Item/ConsumableItem.h"
#include "Item/EquippableItem.h"
#include "Item/WeaponItem.h"
#include "Kismet/KismetMathLibrary.h"

// Sets default values for this component's properties
UInventoryManager::UInventoryManager(): OwnerCharacter(nullptr), SpawnableItems(nullptr), MasterDropList(nullptr)
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
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
			UE_LOG(LogTemp, Error, TEXT("Failed to cast owner to AALSBaseCharacter."));
			return;
		}
	}
	else
	{
		
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


void UInventoryManager::ResizeInventory()
{
	InventoryList.SetNum(Colums * Rows);
	ClearAllSlots();
}

void UInventoryManager::ClearAllSlots()
{
	// Set each slot in the InventoryList array to nullptr.
	for (auto& Item : InventoryList)
	{
		Item = nullptr;
	}

	// Logging for debugging purposes
	UE_LOG(LogTemp, Warning, TEXT("All slots in InventoryList have been set to nullptr."));

}

int32 UInventoryManager::TileToIndex(FTile Tile)
{
	int32 Temp = Tile.Y * Colums;
	Temp  = Tile.X + Temp;
	return Temp;
}

void UInventoryManager::IndexToTile(int32 Index, FTile& Tile)
{
	Tile.X = Index % Colums;
	Tile.Y = Index / Colums;
}

TMap<UBaseItem*, FTile> UInventoryManager::GetAllItems()
{
	TMap<UBaseItem*, FTile> AllItemsLocal;
	FTile Tile;

	for (int32 ArrayIndex = 0; ArrayIndex < InventoryList.Num(); ArrayIndex++)
	{
		// Skip null or already processed items
		
		if (!IsValid(InventoryList[ArrayIndex]))
		{
			UE_LOG(LogTemp, Warning, TEXT("Invalid item at index %d"), ArrayIndex);
			continue;
		}
		if (AllItemsLocal.Contains(InventoryList[ArrayIndex]))
		{
			UE_LOG(LogTemp, Warning, TEXT("Duplicate item at index %d"), ArrayIndex);
			continue;
		}

		// Calculate tile and add the item to the map
		IndexToTile(ArrayIndex, Tile);
		AllItemsLocal.Add(InventoryList[ArrayIndex], Tile);
	}

	return AllItemsLocal;
}

void UInventoryManager::RemoveItemInInventory(UBaseItem* Item)
{
	if (IsValid(Item))
	{
		int32 ArrayIndex = 0; // Initialize index counter
		for (const UBaseItem* CurrentItem : InventoryList)
		{
			if (CurrentItem == Item)
			{
				InventoryList[ArrayIndex] = nullptr; // Set the item to null
				OnInventoryChanged.Broadcast(); // Notify that the inventory changed, if needed
			}
			ArrayIndex++; // Increment the index counter
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Item to remove is not valid"));
	}
}

bool UInventoryManager::DropItemInInventory(UBaseItem* Item)
{
    if (!IsValid(Item))
    {
        UE_LOG(LogTemp, Warning, TEXT("DropItemInInventory: Item is invalid."));
        return false;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Warning, TEXT("DropItemInInventory: Invalid world context."));
        return false;
    }

    // Determine spawn location and parameters
    const FVector DropLocation = GetSpawnLocation();
    const FRotator DropRotation = FRotator::ZeroRotator; // Assuming no specific rotation is needed.
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    // Helper lambda to handle mesh setup and common properties
    auto SetupPickup = [&](AItemPickup* Pickup) {
        Pickup->ItemInfo = Item->GetItemInfo();
        Pickup->SetNewMesh(Item->GetItemInfo().StaticMesh);
        Pickup->SetupMesh();
    };

    // Spawn the pickup actor based on the item type
    AItemPickup* CreatedPickup;


	
    switch (Item->GetItemInfo().ItemType)
    {
    case EItemType::IS_Weapon:
        CreatedPickup = World->SpawnActor<AWeaponPickup>(Item->GetItemInfo().PickupClass, DropLocation, DropRotation, SpawnParams);
        if (const auto WeaponPickup = Cast<AWeaponPickup>(CreatedPickup))
        {
            WeaponPickup->WeaponData = Cast<UWeaponItem>(Item)->GetWeaponData();
            WeaponPickup->EquipmentData = Cast<UWeaponItem>(Item)->GetEquippableData();
        }
        break;
    case EItemType::IS_Armor:
    case EItemType::IS_Shield:
        CreatedPickup = World->SpawnActor<AEquipmentPickup>(Item->GetItemInfo().PickupClass, DropLocation, DropRotation, SpawnParams);
        if (const auto EquipPickup = Cast<AEquipmentPickup>(CreatedPickup))
        {
            EquipPickup->EquipmentData = Cast<UEquippableItem>(Item)->GetEquippableData();
        }
        break;
    case EItemType::IS_Consumable:
        CreatedPickup = World->SpawnActor<AConsumablePickup>(Item->GetItemInfo().PickupClass, DropLocation, DropRotation, SpawnParams);
        if (const auto ConsumablePickup = Cast<AConsumablePickup>(CreatedPickup))
        {
            ConsumablePickup->ConsumableData = Cast<UConsumableItem>(Item)->GetConsumableData();
        }
        break;
    default:
        CreatedPickup = World->SpawnActor<AItemPickup>(Item->GetItemInfo().PickupClass, DropLocation, DropRotation, SpawnParams);

        break;
    }

    if (CreatedPickup)
    {
        SetupPickup(CreatedPickup);
        return true;
    }

    UE_LOG(LogTemp, Error, TEXT("Failed to spawn pickup for item %s"), *Item->GetItemInfo().ItemName.ToString());
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
		UE_LOG(LogTemp, Warning, TEXT("Item is not valid."));
		return false;
	}

	bool bIsRoomAvailable = true;  // Assume there is room available by default

	ForEachTile(Item, TopLeftIndex, [&](FTile Tile)
		{
			const int32 Index = TileToIndex(Tile);
			UBaseItem* RetrievedItem;

			if (!IsTileValid(Tile) || !GetItemAtTile(Index, RetrievedItem))
			{
				bIsRoomAvailable = false;
				return;
			}

			if (IsValid(RetrievedItem))
			{
				bIsRoomAvailable = false;
				return;
			}
		});

	return bIsRoomAvailable;
}

bool UInventoryManager::ForEachTile(UBaseItem* Item, int32 TopLeftIndex, const TFunction<void(FTile)>& Func)
{
	if (!Item)  // Check for null Item
	{
		UE_LOG(LogTemp, Warning, TEXT("Item is null. Aborting ForEachTile."));
		return false;
	}

	FTile TempTile;
	IndexToTile(TopLeftIndex, TempTile);

	const FVector2D Dimensions = Item->GetDimensions();
	if (Dimensions.X <= 0 || Dimensions.Y <= 0)  // Check for valid dimensions
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid item dimensions. Aborting ForEachTile."));
		return false;
	}

	for (int32 x = TempTile.X; x < TempTile.X + Dimensions.X; ++x)
	{
		for (int32 y = TempTile.Y; y < TempTile.Y + Dimensions.Y; ++y)
		{
			const FTile Tile(x, y);
			Func(Tile);  // Call the lambda function for each tile

			// Debug log to display the tile coordinates
			UE_LOG(LogTemp, Warning, TEXT("Processing tile at coordinates (%d, %d)"), x, y);
		}
	}
	return true;  // return true when completed
}


bool UInventoryManager::IsTileValid(const FTile Tile) const
{
	return (Tile.X >= 0 && Tile.Y >= 0 && Tile.X < Colums && Tile.Y < Rows);
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
	Item->GetItemInfo().OwnerID = InventoryID;
	ForEachTile(Item, TopLeftIndex, [this, Item](FTile Tile)
		{
			if (const int32 Index = TileToIndex(Tile); InventoryList.IsValidIndex(Index))
			{
				InventoryList[Index] = Item;  // Add item at index
			}
		});

	// When completed, broadcast (if needed)
	OnInventoryChanged.Broadcast();
}

bool UInventoryManager::TryToAddItemToInventoryRotated(UBaseItem* Item)
{
	if (IsValid(Item))
	{
		FItemInformation TempItemInfo = Item->GetItemInfo();
		TempItemInfo.Rotated = true;
		Item->SetItemInfo(TempItemInfo);
		for(int32 x = 0; x < InventoryList.Num(); x++)
		{
			if(IsRoomAvailable(Item, x))
			{
				AddItemAt(Item, x);
				return true;
			}
		}
	}
	return false;
}


void UInventoryManager::RandomizeInventory()
{
	if (SpawnableItems == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("SpawnableItems DataTable is null."));
		return;
	}
	if (OwnerCharacter)
	{
		if (!OwnerCharacter->IsPlayerControlled() && (!Generated))
		{
			Generated = true;

			int32 TotalDropRating = 0;
			for (const auto& Row : SpawnableItems->GetRowMap())
			{
				if (const FDropTable* ItemInfo = reinterpret_cast<FDropTable*>(Row.Value))
				{
					TotalDropRating += ItemInfo->DropRating;
				}
			}

			//Determine the number of items to spawn.
			const int32 AmountToGenerate = GetGeneratedItemAmount();

			// Select an item based on the random score
			FName SelectedItemName;

			for (int i = 0; i < AmountToGenerate; i++)
			{
				// Generate a random number within the total score range
				int32 RandomScore = FMath::RandRange(0, TotalDropRating);

				for (const auto& Row : SpawnableItems->GetRowMap())
				{
					if (const FDropTable* ItemInfo = reinterpret_cast<FDropTable*>(Row.Value))
					{
						RandomScore -= ItemInfo->DropRating;
						if (RandomScore <= 0)
						{
							
							SelectedItemName = Row.Key;
						}
					}
				}

				if (!SelectedItemName.IsNone())
				{
					if (const bool bItemAdded = SpawnItemByName(SelectedItemName, SpawnableItems); !bItemAdded)
					{
						UE_LOG(LogTemp, Warning, TEXT("Failed to add item %s to inventory, stopping item generation."), *SelectedItemName.ToString());
						break; // Stop the loop as item could not be added to the inventory
					}
				}
			}
		}
	}
}

int32 UInventoryManager::GetGeneratedItemAmount() const
{
	// Determine if we should return the minimum value based on a 35% chance.
	if (const float MinChance = UKismetMathLibrary::RandomFloat(); MinChance < MinThreshold) {
		return MinMaxLootAmount.X; 
	}

	// Generate a random float between 0 and 1 for other calculations.
	const float RandomFloat = UKismetMathLibrary::RandomFloat();

	// Square the random float to create a bias towards lower values (quadratic distribution).
	const float SquaredRandom = RandomFloat * RandomFloat;

	// Convert the squared random float to a range within our desired minimum and maximum loot amounts.
	const int32 Range = MinMaxLootAmount.Y - MinMaxLootAmount.X;
	const int32 ScaledValue = UKismetMathLibrary::FTrunc(SquaredRandom * Range);

	// Add the minimum loot amount to shift the generated number back to the correct loot range.
	const int32 LocalLootAmountRoll = ScaledValue + MinMaxLootAmount.X;

	// Return the final calculated loot amount.
	return LocalLootAmountRoll;
}

bool UInventoryManager::SpawnItemByName(const FName ItemName, const UDataTable* DataTable)
{
	// Check if the item exists in the data table
	const FItemInformation* ItemInfo = DataTable->FindRow<FItemInformation>(ItemName, TEXT("LookupItemInfo"));

	if (ItemInfo == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Item %s not found in DataTable"), *ItemName.ToString());
		return false;
	}

		// Create a base pointer for item creation
	UBaseItem* Item;

	// Determine the item type and create accordingly
switch (ItemInfo->ItemType)
	{
	case EItemType::IS_Weapon:
		{
			const FWeaponItemData* WeaponInfo = DataTable->FindRow<FWeaponItemData>(ItemName, TEXT("LookupItemInfo"));
			Item = NewObject<UWeaponItem>(this, UWeaponItem::StaticClass());
			if (WeaponInfo && Item)
			{
				const auto WeaponItem = Cast<UWeaponItem>(Item);
				WeaponItem->SetWeaponData(*WeaponInfo);
				WeaponItem->SetItemInfo(*ItemInfo);
				WeaponItem->SetEquippableData(*DataTable->FindRow<FEquippableItemData>(ItemName, TEXT("LookupItemInfo")));
			}
		}
		break;
	case EItemType::IS_Armor:
	case EItemType::IS_Shield:
		{
			Item = NewObject<UEquippableItem>(this, UEquippableItem::StaticClass());
			const auto EquippableItem = Cast<UEquippableItem>(Item);
			EquippableItem->SetEquippableData(*DataTable->FindRow<FEquippableItemData>(ItemName, TEXT("LookupItemInfo")));
			EquippableItem->SetItemInfo(*ItemInfo);
		}
		break;
	case EItemType::IS_Consumable:
		{
			const FConsumableItemData* ConsumableInfo = DataTable->FindRow<FConsumableItemData>(ItemName, TEXT("LookupItemInfo"));
			Item = NewObject<UConsumableItem>(this, UConsumableItem::StaticClass());
			if (ConsumableInfo && Item)
			{
				const auto ConsumableItem = Cast<UConsumableItem>(Item);
				ConsumableItem->SetConsumableData(*ConsumableInfo);
				ConsumableItem->SetItemInfo(*ItemInfo);
			}
		}
		break;
	default:
		Item = NewObject<UBaseItem>(this, UBaseItem::StaticClass());
		if (Item)
		{
			const auto BaseItem = Cast<UBaseItem>(Item);
			BaseItem->SetItemInfo(*ItemInfo);
		}
		break;
	}

	if (!Item)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create item instance for %s"), *ItemName.ToString());
		return false;
	}

	// Set the owner ID and try to add to inventory
	FItemInformation TempItemInfo = Item->GetItemInfo();
	TempItemInfo.OwnerID = InventoryID;
	Item->SetItemInfo(TempItemInfo);
	if (!TryToAddItemToInventory(Item, true))
	{
		UE_LOG(LogTemp, Warning, TEXT("Could not add item %s to inventory"), *ItemName.ToString());
		Item->ConditionalBeginDestroy(); // Properly dispose of the item if not added to inventory
		return false;
	}

	return true; // Item was successfully created and added
}

bool UInventoryManager::HasEnoughGems(UBaseItem* Item) const
{
	if(Item->GetItemInfo().Stackable && IsValid(Item))
	{
		return  (CalculateStackedItemValue(Item->GetItemInfo()) <= Gems);
	}
	return (CalculateValue(Item->GetItemInfo()) <= Gems);
}

int32 UInventoryManager::CalculateStackedItemValue(const FItemInformation& ItemData)
{
	return 	UKismetMathLibrary::Round((ItemData.Value* ItemData.ValueModifier) * ItemData.Quantity);
}

int32 UInventoryManager::CalculateValue(const FItemInformation& ItemData)
{
	return UKismetMathLibrary::Round(ItemData.Value * ItemData.ValueModifier);
}






