// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/SpawnableLootManager.h"

// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/SpawnableLootManager.h"

#include "AbilitySystem/PHAttributeSet.h"
#include "Components/BoxComponent.h"
#include "Interactables/Pickups/ItemPickup.h"
#include "Kismet/KismetMathLibrary.h"
#include "Library/PHItemStructLibrary.h"




// Sets default values for this component's properties
USpawnableLootManager::USpawnableLootManager(): SpawnableItems(nullptr), MasterDropList(nullptr), SpawnBox(nullptr)
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void USpawnableLootManager::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}

int32 USpawnableLootManager::GenerateDropAmount(UPHAttributeSet* AttributeSet)
{

    // Determine if we should return the minimum value based on a 75% chance.
    if (const float MinChance = UKismetMathLibrary::RandomFloat(); MinChance < MinThreshold) {
        return MinMaxLootAmount.X; // 75% chance to return minimum value.
    }

    // Generate a random float between 0 and 1 for other calculations
    const float RandomFloat = UKismetMathLibrary::RandomFloat();

    // Square the random float to create a quadratic distribution
    const float SquaredRandom = RandomFloat * RandomFloat;

    // Calculate the LuckFactor based on the player's luck
    const float PlayerLuck = AttributeSet->GetLuck();
    const float LuckFactor = FMath::Clamp(PlayerLuck / 9999.0f, 0.0f, 1.0f);

    // Adjust the random factor based on luck - more luck results in a value closer to RandomFloat than to SquaredRandom
    const float AdjustedRandom = FMath::Lerp(SquaredRandom, RandomFloat, LuckFactor);

    // Convert the adjusted random float to a range within our desired min and max
    const int32 Range = MinMaxLootAmount.Y - MinMaxLootAmount.X;
    const int32 ScaledValue = UKismetMathLibrary::FTrunc(AdjustedRandom * Range);

    // Add the minimum loot amount to shift the range back to the original location
    int32 LootAmountRoll = ScaledValue + MinMaxLootAmount.X;

    // Apply LuckFactor to potentially increase the drop amount further
    LootAmountRoll += FMath::CeilToInt(LootAmountRoll * LuckFactor);
    
    return LootAmountRoll;
}





FTransform USpawnableLootManager::GetSpawnLocation()
{
    // Get the extent of the spawn box
    if (SpawnBox != nullptr)
    {
        const FVector BoxExtent = SpawnBox->GetScaledBoxExtent();

        // Create a random vector within the extents of the spawn box
        FVector RandomVector;
        RandomVector.X = FMath::RandRange(-BoxExtent.X, BoxExtent.X);
        RandomVector.Y = FMath::RandRange(-BoxExtent.Y, BoxExtent.Y);
        RandomVector.Z = 30.0f; // Fixed Z value, adjust if necessary

        // Add the random vector to the base location of the spawn box
        const FVector BaseLocation = SpawnBox->GetSocketLocation("None"); // Consider error checking for valid socket
        const FVector FinalLocation = BaseLocation + RandomVector;

        // Create the transform with location, default rotation (no rotation), and default scale (1.0f)
        const FTransform CreatedTransform(FinalLocation);

        return CreatedTransform;
    }
    const FTransform EmptyTransform;
    return  EmptyTransform;
}

void USpawnableLootManager::SpawnItemByName(const FName ItemName, UDataTable* DataTable, const FTransform SpawnTransform)
{
    // Check if the item exists in the data table
    const FItemInformation* ItemInfo = DataTable->FindRow<FItemInformation>(ItemName, TEXT("LookupItemInfo"));
    if (ItemInfo == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("Item %s not found in DataTable"), *ItemName.ToString());
        return;
    }

    // Use the item info to set properties of the pickup class (if needed)

    // Define spawn parameters (if any specific parameters are required)
    const FActorSpawnParameters SpawnParameters;
    // Set any specific spawn parameters here

    // Spawn the actor in the world
    const FVector SpawnLocation = SpawnTransform.GetLocation();
    const FRotator SpawnRotation = SpawnTransform.GetRotation().Rotator();
    PickUpClass = ItemInfo->PickupClass;
    if (AItemPickup* SpawnedActor = GetWorld()->SpawnActor<AItemPickup>(PickUpClass, SpawnLocation, SpawnRotation, SpawnParameters); IsValid(SpawnedActor))
    {
        SpawnedActor->ItemInfo = *ItemInfo;
        SpawnedActor->SetNewMesh(ItemInfo->StaticMesh);
        SpawnedActor->OnConstruction(SpawnTransform);
        
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to spawn actor for item %s"), *ItemName.ToString());
    }
}

void USpawnableLootManager::GetSpawnItem(UPHAttributeSet* AttributeSet)
{
    if (SpawnableItems == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("SpawnableItems DataTable is null."));
        return;
    }

    int32 TotalDropRating = 0;
    for (const auto& Row : SpawnableItems->GetRowMap())
    {
        if (const FDropTable* ItemInfo = reinterpret_cast<FDropTable*>(Row.Value))
        {
            TotalDropRating += ItemInfo->DropRating;
        }
    }

    // Determine the number of items to spawn
    const int32 NumberOfItemsToSpawn = GenerateDropAmount(AttributeSet);

    for (int32 i = 0; i < NumberOfItemsToSpawn; ++i)
    {
        // Generate a random number within the total score range
        int32 RandomScore = FMath::RandRange(0, TotalDropRating);

        // Select an item based on the random score
        FName SelectedItemName;
        for (const auto& Row : SpawnableItems->GetRowMap())
        {
            if (const FDropTable* ItemInfo = reinterpret_cast<FDropTable*>(Row.Value))
            {
                RandomScore -= ItemInfo->DropRating;
                if (RandomScore <= 0)
                {
                    SelectedItemName = Row.Key;
                    if(GEngine)
                    {
	                  GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Selected Item: %s"), *SelectedItemName.ToString()));
                    }

                    break;
                }
            }
        }

        // Spawn the selected item
        if (!SelectedItemName.IsNone())
        {
            // Get a spawn location for each item
            const FTransform SpawnTransform = GetSpawnLocation();
            SpawnItemByName(SelectedItemName, MasterDropList, SpawnTransform);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("No item selected to spawn."));
        }
    }
}


