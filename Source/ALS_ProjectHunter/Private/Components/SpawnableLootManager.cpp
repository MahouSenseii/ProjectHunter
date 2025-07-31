#include "Components/SpawnableLootManager.h"
#include "AbilitySystem/PHAttributeSet.h"
#include "Components/BoxComponent.h"
#include "Interactables/Pickups/ItemPickup.h"
#include "Kismet/KismetMathLibrary.h"
#include "Library/PHItemStructLibrary.h"

DEFINE_LOG_CATEGORY_STATIC(LogLoot, Log, All);

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
	const float PlayerLuck = AttributeSet ? AttributeSet->GetLuck() : 0.0f;

	// Normalize Luck against configurable MaxPlayerLuck
	const float LuckRatio = FMath::Clamp(PlayerLuck / MaxPlayerLuck, 0.0f, 1.0f);
	const float LuckFactor = LuckRatio / (LuckRatio + LuckSoftCapConstant);

	// Quadratic randomness
	const float RandomFloat = UKismetMathLibrary::RandomFloat();
	const float SquaredRandom = RandomFloat * RandomFloat;

	// Tilt the randomness slightly based on Luck
	const float AdjustedRandom = FMath::Lerp(SquaredRandom, RandomFloat, LuckFactor);

	const int32 Min = FMath::TruncToInt(MinMaxLootAmount.X);
	const int32 Max = FMath::TruncToInt(MinMaxLootAmount.Y);
	const int32 Range = Max - Min;

	const int32 ScaledValue = FMath::TruncToInt(AdjustedRandom * Range);
	const int32 LootAmountRoll = Min + ScaledValue;

	UE_LOG(LogLoot, Log, TEXT("Luck=%f | Ratio=%.3f | Factor=%.3f | Roll=%.2f | Loot=%d"),
		PlayerLuck, LuckRatio, LuckFactor, AdjustedRandom, LootAmountRoll);

	return LootAmountRoll;
}




FTransform USpawnableLootManager::GetSpawnLocation() const
{
	if (!SpawnBox || !GetWorld()) 
	{
		UE_LOG(LogLoot, Warning, TEXT("SpawnBox is null or World is not available, using default transform"));
		return FTransform();
	}

	const FVector BoxExtent = SpawnBox->GetScaledBoxExtent();
	const FVector BaseLocation = SpawnBox->GetComponentLocation();

	// Random XY position in box
	const FVector RandomXY = FVector(
		FMath::RandRange(-BoxExtent.X, BoxExtent.X),
		FMath::RandRange(-BoxExtent.Y, BoxExtent.Y),
		0.0f
	);
	const FVector SpawnPointXY = BaseLocation + RandomXY;

	// Line trace settings
	const FVector TraceStart = SpawnPointXY + FVector(0, 0, 500);   // Start above the point
	const FVector TraceEnd = SpawnPointXY - FVector(0, 0, 1000);   // Trace downward

	FHitResult Hit;
	FCollisionQueryParams TraceParams;
	TraceParams.bTraceComplex = true;
	TraceParams.AddIgnoredActor(GetOwner());

	if (GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, TraceParams))
	{
		// Debug visuals
		DrawDebugLine(GetWorld(), TraceStart, TraceEnd, FColor::Green, false, 2.0f, 0, 1.0f);
		DrawDebugPoint(GetWorld(), Hit.ImpactPoint, 10.0f, FColor::Red, false, 2.0f);

		// Use ground hit with optional offset
		FVector HitLocation = Hit.ImpactPoint + FVector(0, 0, GroundOffsetZ);
		return FTransform(HitLocation);
	}
	
		// Fallback: just drop to approximate floor height
		FVector DefaultSpawn = SpawnPointXY + FVector(0, 0, 30.0f);
		UE_LOG(LogLoot, Warning, TEXT("No ground hit. Using fallback spawn height."));
		return FTransform(DefaultSpawn);
	
}


void USpawnableLootManager::SpawnItemByName(const FName ItemName, UDataTable* DataTable, const FTransform SpawnTransform)
{
	if (!DataTable) 
	{
		UE_LOG(LogLoot, Warning, TEXT("DataTable is null"));
		return;
	}

	// Safe row lookup
	const FItemInformation* ItemInfo = DataTable->FindRow<FItemInformation>(ItemName, TEXT("LookupItemInfo"));
	if (!ItemInfo) 
	{
		UE_LOG(LogLoot, Warning, TEXT("Item %s not found in DataTable"), *ItemName.ToString());
		return;
	}

	// Check if world is valid before spawning
	if (!GetWorld()) 
	{
		UE_LOG(LogLoot, Warning, TEXT("World is null, cannot spawn item %s"), *ItemName.ToString());
		return;
	}

	// Set the Pickup class
	PickUpClass = ItemInfo->ItemInfo.PickupClass;
	if (!PickUpClass) 
	{
		UE_LOG(LogLoot, Warning, TEXT("Pickup class is null for item: %s"), *ItemName.ToString());
		return;
	}

	// Spawn the item
	AItemPickup* SpawnedItem = GetWorld()->SpawnActor<AItemPickup>(PickUpClass, SpawnTransform);
	if (!SpawnedItem) 
	{
		UE_LOG(LogLoot, Warning, TEXT("Failed to spawn actor for item %s"), *ItemName.ToString());
		return;
	}


	// Assign properties
	SpawnedItem->ItemInfo = *ItemInfo;
	SpawnedItem->SetNewMesh(ItemInfo->ItemInfo.StaticMesh);
	SpawnedItem->OnConstruction(SpawnTransform);
}

void USpawnableLootManager::GetSpawnItem(UPHAttributeSet* AttributeSet)
{
    if (!SpawnableItems) 
    {
        UE_LOG(LogLoot, Warning, TEXT("[Loot] SpawnableItems DataTable is NULL!"));
        return;
    }

    if (bLooted)
    {
        UE_LOG(LogLoot, Warning, TEXT("[Loot] Chest already looted. Skipping spawn."));
        return;
    }

    // Calculate total drop rating
    int32 TotalDropRating = 0;
    for (const auto& Row : SpawnableItems->GetRowMap())
    {
        if (const FDropTable* ItemInfo = SpawnableItems->FindRow<FDropTable>(Row.Key, TEXT("SpawnRatingCalc")))
        {
            TotalDropRating += ItemInfo->DropRating;
        }
    }

    UE_LOG(LogLoot, Warning, TEXT("[Loot] TotalDropRating = %d"), TotalDropRating);

    if (TotalDropRating <= 0)
    {
        UE_LOG(LogLoot, Warning, TEXT("[Loot] TotalDropRating is 0. No items can be spawned."));
        return;
    }

    const int32 NumberOfItemsToSpawn = GenerateDropAmount(AttributeSet);
    UE_LOG(LogLoot, Warning, TEXT("[Loot] NumberOfItemsToSpawn = %d"), NumberOfItemsToSpawn);

    if (NumberOfItemsToSpawn <= 0)
    {
        UE_LOG(LogLoot, Warning, TEXT("[Loot] GenerateDropAmount returned 0. No items to spawn."));
        return;
    }

    for (int32 i = 0; i < NumberOfItemsToSpawn; ++i)
    {
        int32 RandomScore = FMath::RandRange(1, TotalDropRating);
        UE_LOG(LogLoot, Warning, TEXT("[Loot] Roll #%d: RandomScore = %d"), i + 1, RandomScore);

        FName SelectedItemName;
        for (const auto& Row : SpawnableItems->GetRowMap())
        {
            if (const FDropTable* ItemInfo = SpawnableItems->FindRow<FDropTable>(Row.Key, TEXT("ItemRoll")))
            {
                RandomScore -= ItemInfo->DropRating;
                if (RandomScore <= 0)
                {
                    SelectedItemName = Row.Key;
                    UE_LOG(LogLoot, Warning, TEXT("[Loot] Selected item: %s"), *SelectedItemName.ToString());
                    break;
                }
            }
        }

        if (SelectedItemName.IsNone())
        {
            UE_LOG(LogLoot, Warning, TEXT("[Loot] No item selected. Check DropRating values in DataTable."));
            continue;
        }

        const FTransform SpawnTransform = GetSpawnLocation();
        UE_LOG(LogLoot, Warning, TEXT("[Loot] Spawning %s at Location: %s"), 
            *SelectedItemName.ToString(), *SpawnTransform.GetLocation().ToString());

        SpawnItemByName(SelectedItemName, MasterDropList, SpawnTransform);
    }

    bLooted = true;
    UE_LOG(LogLoot, Warning, TEXT("[Loot] Spawn completed. Marking chest as looted."));
}

