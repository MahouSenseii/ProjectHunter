#include "Components/SpawnableLootManager.h"
#include "AbilitySystem/PHAttributeSet.h"
#include "Components/BoxComponent.h"
#include "Interactables/Pickups/ItemPickup.h"
#include "Item/Data/UItemDefinitionAsset.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Library/PHItemStructLibrary.h"

DEFINE_LOG_CATEGORY_STATIC(LogLoot, Log, All);

// Sets default values for this component's properties
USpawnableLootManager::USpawnableLootManager()
	: MinMaxLootAmount(), SpawnableItems(nullptr), MasterDropList(nullptr), SpawnBox(nullptr)
{
	PrimaryComponentTick.bCanEverTick = false; // Disabled ticking for performance
}

void USpawnableLootManager::BeginPlay()
{
	Super::BeginPlay();

	MinMaxLootAmount = FVector2D(0, 10);
}

int32 USpawnableLootManager::GenerateDropAmount(UPHAttributeSet* AttributeSet)
{
	const float PlayerLuck = AttributeSet ? AttributeSet->GetLuck() : 0.0f;

	// Normalize Luck against configurable MaxPlayerLuck
	const float LuckRatio  = (MaxPlayerLuck > 0.f) ? FMath::Clamp(PlayerLuck / MaxPlayerLuck, 0.0f, 1.0f) : 0.f;
	const float LuckFactor = (LuckRatio + LuckSoftCapConstant > 0.f)
		? (LuckRatio / (LuckRatio + LuckSoftCapConstant))
		: 0.f;


	// First, decide if *any* loot should drop at all
	const float LuckBonus = 0.35f * LuckFactor; // tweak bonus scale
	const float DropChance = FMath::Clamp(BaseDropChance + LuckBonus, 0.f, 0.99f);

	if (UKismetMathLibrary::RandomFloat() >= DropChance)
	{
		UE_LOG(LogLoot, Log, TEXT("No drop: DropChance=%.2f | LuckFactor=%.3f"), DropChance, LuckFactor);
		return 0; // nothing drops
	}

	// Quadratic randomness, then tilt toward linear as Luck increases
	const float Rand0_1       = UKismetMathLibrary::RandomFloat();         // [0,1)
	const float SquaredRandom = Rand0_1 * Rand0_1;                         // favors low values
	const float AdjustedRandom = FMath::Lerp(SquaredRandom, Rand0_1, LuckFactor); // favors higher values with luck

	// Resolve min/max and range
	const int32 MinIn  = FMath::FloorToInt(MinMaxLootAmount.X);
	const int32 MaxIn  = FMath::FloorToInt(MinMaxLootAmount.Y);
	const int32 RealMin = FMath::Min(MinIn, MaxIn);
	const int32 RealMax = FMath::Max(MinIn, MaxIn);
	const int32 Range   = RealMax - RealMin;

	// Degenerate ranges (e.g., Min==Max) just return that value
	if (Range <= 0)
	{
		UE_LOG(LogLoot, Log, TEXT("Luck=%f | Ratio=%.3f | Factor=%.3f | Roll=%.3f | Loot=%d (degenerate range %d==%d)"),
			PlayerLuck, LuckRatio, LuckFactor, AdjustedRandom, RealMin, RealMin, RealMax);
		return RealMin;
	}

	// Make MAX inclusive by using (Range + 1) before flooring
	const int32 LootUnclamped = RealMin + FMath::FloorToInt(AdjustedRandom * (Range + 1));
	const int32 Loot = FMath::Clamp(LootUnclamped, RealMin, RealMax);

	UE_LOG(LogLoot, Log, TEXT("Luck=%f | Ratio=%.3f | Factor=%.3f | Roll=%.3f | Min=%d Max=%d -> Loot=%d"),
		PlayerLuck, LuckRatio, LuckFactor, AdjustedRandom, RealMin, RealMax, Loot);

	return Loot;
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

    // NEW: lookup the Data-Asset row instead of FItemInformation
    const FSpawnableItemRow_DA* Row = DataTable->FindRow<FSpawnableItemRow_DA>(ItemName, TEXT("LookupItemDef"));
    if (!Row || !Row->BaseDef.IsValid())
    {
        UE_LOG(LogLoot, Warning, TEXT("Item %s not found or BaseDef missing."), *ItemName.ToString());
        return;
    }

    if (!GetWorld())
    {
        UE_LOG(LogLoot, Warning, TEXT("World is null, cannot spawn item %s"), *ItemName.ToString());
        return;
    }

    // Resolve the asset
    UItemDefinitionAsset* Def = Row->BaseDef.LoadSynchronous();
    if (!Def || !Def->IsValidDefinition())
    {
        UE_LOG(LogLoot, Warning, TEXT("Invalid ItemDefinitionAsset for %s"), *ItemName.ToString());
        return;
    }

    // Choose pickup class: row override > asset default
    TSubclassOf<AItemPickup> ClassToSpawn = Def->Base.PickupClass; // uses your existing FItemBase.PickupClass
    if (!*ClassToSpawn)
    {
        UE_LOG(LogLoot, Warning, TEXT("Pickup class is null for item: %s"), *ItemName.ToString());
        return;
    }

    FActorSpawnParameters Params;
    Params.Owner = GetOwner();
    Params.Instigator = Cast<APawn>(GetOwner());
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    // Keep your deferred spawn pattern
    AItemPickup* Item = GetWorld()->SpawnActorDeferred<AItemPickup>(ClassToSpawn, SpawnTransform, Params.Owner, Params.Instigator, Params.SpawnCollisionHandlingOverride);
    if (!Item)
    {
        UE_LOG(LogLoot, Warning, TEXT("Failed to begin deferred spawn for item %s"), *ItemName.ToString());
        return;
    }

  
    {
    	UItemDefinitionAsset* TempView = nullptr;
        TempView->Base= Def->Base;   
        TempView->Equip = Def->Equip;  

      
        TempView->Equip.Affixes.Implicits = Def->Implicits;
        TempView->Equip.Affixes.bAffixesGenerated = TempView->Equip.Affixes.GetTotalAffixCount() > 0;

        Item->ObjItem->ItemDefinition = MoveTemp(TempView); // same place you previously assigned the row
    }

    
    if (Item->ObjItem->ItemDefinition->Base.StaticMesh)
    {
        Item->SetNewMesh(Item->ObjItem->ItemDefinition->Base.StaticMesh);
    }

    UGameplayStatics::FinishSpawningActor(Item, SpawnTransform); // unchanged

    UE_LOG(LogLoot, Log, TEXT("Spawned %s | Mesh=%s | Class=%s"),
        *ItemName.ToString(),
        Item->ObjItem->ItemDefinition->Base.StaticMesh ? *Item->ObjItem->ItemDefinition->Base.StaticMesh->GetName() : TEXT("None"),
        *ClassToSpawn->GetName());
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

    
    int32 TotalDropRating = 0;
    for (const auto& Row : SpawnableItems->GetRowMap())
    {
        if (const FSpawnableItemRow_DA* ItemInfo = SpawnableItems->FindRow<FSpawnableItemRow_DA>(Row.Key, TEXT("SpawnRatingCalc")))
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
     
            if (const FSpawnableItemRow_DA* ItemInfo = SpawnableItems->FindRow<FSpawnableItemRow_DA>(Row.Key, TEXT("ItemRoll")))
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

        FTransform SpawnTransform = GetSpawnLocation();
        const FRotator DesiredRot(0.f, 180.f, 0.f);
        SpawnTransform.SetRotation(DesiredRot.Quaternion());
        SpawnTransform.AddToTranslation(FVector(0.f, 0.f, 180.f));

        UE_LOG(LogLoot, Warning, TEXT("[Loot] Spawning %s at Location: %s, Rot: %s"),
            *SelectedItemName.ToString(),
            *SpawnTransform.GetLocation().ToString(),
            *SpawnTransform.GetRotation().Rotator().ToCompactString());

        SpawnItemByName(SelectedItemName, MasterDropList, SpawnTransform);
    }

    bLooted = true;
    UE_LOG(LogLoot, Warning, TEXT("[Loot] Spawn completed. bLooted set to true."));
}

