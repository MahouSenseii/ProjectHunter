// Fill out your copyright notice in the Description page of Project Settings.


#include "Interactables/Pickups/EquipmentPickup.h"
#include "Components/InteractableManager.h"
#include "Components/InventoryManager.h"
#include  "Library/PHItemFunctionLibrary.h"
#include "Library/FL_InteractUtility.h"

AEquipmentPickup::AEquipmentPickup()
{
	// Set Location (X, Y, Z)
	MeshTransform.SetLocation(FVector(0.0f, 0.0f, 80.0f));

	// Set Rotation (Pitch, Yaw, Roll)
	MeshTransform.SetRotation(FQuat(FRotator(0.0f, 180.0f, 0.0f)));

	// Set Scale (X, Y, Z)
	MeshTransform.SetScale3D(FVector(1.0f, 1.0f, 1.0f));
}

void AEquipmentPickup::BeginPlay()
{
	Super::BeginPlay();

	if (!ItemDefinition)
	{
		UE_LOG(LogTemp, Warning, TEXT("EquipmentPickup: No ItemDefinition set!"));
		return;
	}

	// Generate unique instance data
	GenerateRandomAffixes();
}

void AEquipmentPickup::GenerateRandomAffixes()
{
    if (!ItemDefinition || !ItemDefinition->GetStatsDataTable())
    {
        UE_LOG(LogTemp, Warning, TEXT("EquipmentPickup: Cannot generate affixes - missing definition or stats table"));
        return;
    }

    
    InstanceData.UniqueID = FGuid::NewGuid().ToString();

    
    FPHItemStats GeneratedStats = UPHItemFunctionLibrary::GenerateStats(
        ItemDefinition->GetStatsDataTable()
    );

    // Store in instance data
    InstanceData.Prefixes = GeneratedStats.Prefixes;
    InstanceData.Suffixes = GeneratedStats.Suffixes;

    // Copy any fixed implicits from definition to instance
    InstanceData.Implicits = ItemDefinition->GetImplicits();

    
    int32 BaseRankPoints = 0; 
    
    FPHItemStats CombinedStats;
    CombinedStats.Prefixes = InstanceData.Prefixes;
    CombinedStats.Suffixes = InstanceData.Suffixes;
    
    InstanceData.Rarity = UPHItemFunctionLibrary::DetermineWeaponRank(
        BaseRankPoints,
        CombinedStats
    );

   
    InstanceData.DisplayName = UPHItemFunctionLibrary::GenerateItemName(
        CombinedStats,
        ItemDefinition
    );
    
    InstanceData.bHasNameBeenGenerated = true;

    // Initialize other instance data
    InstanceData.Quantity = 1;
    InstanceData.ItemLevel = 1;  // Or calculate based on area level, etc.
    InstanceData.bIdentified = true;  // Or false for unidentified items
    
    // Copy durability from definition
    InstanceData.Durability = ItemDefinition->Equip.Durability;

    UE_LOG(LogTemp, Log, TEXT("Generated item: %s (Rarity: %d, Affixes: %d)"), 
           *InstanceData.DisplayName.ToString(),
           static_cast<int32>(InstanceData.Rarity),
           InstanceData.Prefixes.Num() + InstanceData.Suffixes.Num());
}

FText AEquipmentPickup::GetDisplayName() const
{
    // Return instance name if generated, otherwise base name
    if (InstanceData.bHasNameBeenGenerated)
    {
        return InstanceData.DisplayName;
    }
    
    return ItemDefinition ? ItemDefinition->Base.ItemName : FText::GetEmpty();
}

EItemRarity AEquipmentPickup::GetInstanceRarity() const
{
    // Return instance rarity if valid, otherwise base rarity
    if (InstanceData.Rarity != EItemRarity::IR_None)
    {
        return InstanceData.Rarity;
    }
    
    return ItemDefinition ? ItemDefinition->Base.ItemRarity : EItemRarity::IR_None;
}

bool AEquipmentPickup::HandleInteraction(AActor* Actor, bool WasHeld, UItemDefinitionAsset*& PassedItemInfo, FConsumableItemData ConsumableItemData) const
{
	Super::InteractionHandle(Actor, WasHeld);
	PassedItemInfo = ItemInfo; 
	return Super::HandleInteraction(Actor, WasHeld, PassedItemInfo, FConsumableItemData());
}

void AEquipmentPickup::HandleHeldInteraction(APHBaseCharacter* Character) const
{
	Super::HandleHeldInteraction(Character);

if (Character->GetEquipmentManager()->IsItemEquippable(ObjItem) && (UFL_InteractUtility::AreRequirementsMet(ObjItem, Character)))
	{
		Character->GetEquipmentManager()->TryToEquip(ObjItem, true, ObjItem->GetItemInfo()->Base.EquipmentSlot);
	}
	else
	{
		// Attempt to get the Inventory Manager component from the ALSCharacter
		if (UInventoryManager* OwnersInventory = Cast<UInventoryManager>(Owner->GetComponentByClass(UInventoryManager::StaticClass())))
		{
		// If the Inventory Manager exists, try to add the item to the inventory
			OwnersInventory->TryToAddItemToInventory(ObjItem, true);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("ALSCharacter does not have an UInventoryManager component."));
		}
	}
	InteractableManager->RemoveInteraction();
}

void AEquipmentPickup::HandleSimpleInteraction(APHBaseCharacter* Character) const
{
	Super::HandleSimpleInteraction(Character);

	if (Cast<UInventoryManager>(Character->GetComponentByClass(UInventoryManager::StaticClass()))->TryToAddItemToInventory(ObjItem, true))
	{
		if (InteractableManager->DestroyAfterInteract)
		{
			InteractableManager->RemoveInteraction();
		}
	}
}

void AEquipmentPickup::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnOverlapBegin(OverlappedComponent,OtherActor,OtherComp, OtherBodyIndex, bFromSweep,SweepResult);

	if( APHBaseCharacter* BaseCharacter =  Cast<APHBaseCharacter>(OtherActor))
	{
		if(BaseCharacter->IsPlayerControlled())
		{
		}
	}
}

void AEquipmentPickup::OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	Super::OnOverlapEnd(OverlappedComponent,OtherActor,OtherComp,OtherBodyIndex);
	
}

