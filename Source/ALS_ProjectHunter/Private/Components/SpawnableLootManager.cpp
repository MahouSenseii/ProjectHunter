#include "Components/SpawnableLootManager.h"
#include "AbilitySystem/PHAttributeSet.h"
#include "Components/BoxComponent.h"
#include "Interactables/Pickups/ItemPickup.h"
#include "Kismet/KismetMathLibrary.h"
#include "Library/PHItemStructLibrary.h"

// Sets default values for this component's properties
USpawnableLootManager::USpawnableLootManager()
	: SpawnableItems(nullptr), MasterDropList(nullptr), SpawnBox(nullptr)
{
	PrimaryComponentTick.bCanEverTick = false; // Disabled ticking for performance
}

void USpawnableLootManager::BeginPlay()
{
	Super::BeginPlay();
}

int32 USpawnableLootManager::GenerateDropAmount(UPHAttributeSet* AttributeSet)
{
	if (!AttributeSet) return MinMaxLootAmount.X; // Return minimum if no attribute set

	// 75% chance to return minimum drop amount
	if (UKismetMathLibrary::RandomFloat() < MinThreshold) 
	{
		return MinMaxLootAmount.X;
	}

	// Generate a quadratic random value
	const float RandomFloat = UKismetMathLibrary::RandomFloat();
	const float SquaredRandom = RandomFloat * RandomFloat;

	// Calculate luck factor (normalized to [0,1])
	const float PlayerLuck = AttributeSet->GetLuck();
	const float LuckFactor = FMath::Clamp(PlayerLuck / 9999.0f, 0.0f, 1.0f);

	// Adjust randomness based on LuckFactor
	const float AdjustedRandom = FMath::Lerp(SquaredRandom, RandomFloat, LuckFactor);

	// Scale random value within min-max range
	const int32 Range = MinMaxLootAmount.Y - MinMaxLootAmount.X;
	const int32 ScaledValue = FMath::TruncToInt(AdjustedRandom * Range);

	// Apply final LuckFactor bonus
	int32 LootAmountRoll = ScaledValue + MinMaxLootAmount.X;
	LootAmountRoll += FMath::CeilToInt(LootAmountRoll * LuckFactor);

	return LootAmountRoll;
}

FTransform USpawnableLootManager::GetSpawnLocation() const
{
	if (!SpawnBox) 
	{
		UE_LOG(LogTemp, Warning, TEXT("SpawnBox is null, using default transform"));
		return FTransform();
	}

	// Get SpawnBox extent and generate a random position inside
	const FVector BoxExtent = SpawnBox->GetScaledBoxExtent();
	const FVector RandomVector = FVector(
		FMath::RandRange(-BoxExtent.X, BoxExtent.X),
		FMath::RandRange(-BoxExtent.Y, BoxExtent.Y),
		30.0f // Adjust for ground-level spawn
	);

	// Get Base Location
	const FVector BaseLocation = SpawnBox->GetComponentLocation();
	return FTransform(BaseLocation + RandomVector);
}

void USpawnableLootManager::SpawnItemByName(const FName ItemName, UDataTable* DataTable, const FTransform SpawnTransform)
{
	if (!DataTable) 
	{
		UE_LOG(LogTemp, Warning, TEXT("DataTable is null"));
		return;
	}

	const FItemInformation* ItemInfo = DataTable->FindRow<FItemInformation>(ItemName, TEXT("LookupItemInfo"));
	if (!ItemInfo) 
	{
		UE_LOG(LogTemp, Warning, TEXT("Item %s not found in DataTable"), *ItemName.ToString());
		return;
	}

	// Set the Pickup class
	PickUpClass = ItemInfo->PickupClass;
	if (!PickUpClass) 
	{
		UE_LOG(LogTemp, Warning, TEXT("Pickup class is null for item: %s"), *ItemName.ToString());
		return;
	}

	// Spawn the item
	AItemPickup* SpawnedItem = GetWorld()->SpawnActor<AItemPickup>(PickUpClass, SpawnTransform);
	if (!SpawnedItem) 
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to spawn actor for item %s"), *ItemName.ToString());
		return;
	}

	// Assign properties
	SpawnedItem->ItemInfo = *ItemInfo;
	SpawnedItem->SetNewMesh(ItemInfo->StaticMesh);
	SpawnedItem->OnConstruction(SpawnTransform);
}

void USpawnableLootManager::GetSpawnItem(UPHAttributeSet* AttributeSet)
{
	if (!SpawnableItems) 
	{
		UE_LOG(LogTemp, Warning, TEXT("SpawnableItems DataTable is null."));
		return;
	}

	// Compute total drop rating
	int32 TotalDropRating = 0;
	for (const auto& Row : SpawnableItems->GetRowMap())
	{
		const FDropTable* ItemInfo = reinterpret_cast<FDropTable*>(Row.Value);
		if (ItemInfo)
		{
			TotalDropRating += ItemInfo->DropRating;
		}
	}

	// Determine number of items to spawn
	const int32 NumberOfItemsToSpawn = GenerateDropAmount(AttributeSet);
	for (int32 i = 0; i < NumberOfItemsToSpawn; ++i)
	{
		// Randomize item selection
		int32 RandomScore = FMath::RandRange(0, TotalDropRating);
		FName SelectedItemName;

		// Find the item corresponding to the random score
		for (const auto& Row : SpawnableItems->GetRowMap())
		{
			if (const FDropTable* ItemInfo = reinterpret_cast<FDropTable*>(Row.Value))
			{
				RandomScore -= ItemInfo->DropRating;
				if (RandomScore <= 0)
				{
					SelectedItemName = Row.Key;
					break;
				}
			}
		}

		// Spawn the selected item
		if (!SelectedItemName.IsNone())
		{
			const FTransform SpawnTransform = GetSpawnLocation();
			SpawnItemByName(SelectedItemName, MasterDropList, SpawnTransform);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("No item selected to spawn."));
		}
	}
}
