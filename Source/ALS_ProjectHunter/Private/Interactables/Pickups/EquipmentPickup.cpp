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

	UE_LOG(LogItemPickup, Warning, TEXT("=== AEquipmentPickup::BeginPlay ==="));
	UE_LOG(LogItemPickup, Warning, TEXT("ObjItemClass: %s"), ObjItemClass ? *ObjItemClass->GetName() : TEXT("NULL"));
	UE_LOG(LogItemPickup, Warning, TEXT("ObjItem before creation: %s"), ObjItem ? *ObjItem->GetClass()->GetName() : TEXT("NULL"));

	if (!ObjItem && ObjItemClass)
	{
		UE_LOG(LogItemPickup, Warning, TEXT("Creating new ObjItem with class: %s"), *ObjItemClass->GetName());
		ObjItem = CreateItemObjectWClass(ObjItemClass, this); 
        
		if (ObjItem)
		{
			UE_LOG(LogItemPickup, Warning, TEXT("✅ Created ObjItem! Class: %s"), *ObjItem->GetClass()->GetName());
			UE_LOG(LogItemPickup, Warning, TEXT("Is UEquippableItem: %s"), 
				ObjItem->IsA(UEquippableItem::StaticClass()) ? TEXT("YES") : TEXT("NO"));
		}
		else
		{
			UE_LOG(LogItemPickup, Error, TEXT("❌ Failed to create ObjItem!"));
		}
	}
	else
	{
		UE_LOG(LogItemPickup, Warning, TEXT("ObjItem already exists or ObjItemClass is null - NOT creating new one"));
		if (ObjItem)
		{
			UE_LOG(LogItemPickup, Warning, TEXT("Existing ObjItem class: %s"), *ObjItem->GetClass()->GetName());
		}
	}
	// Generate unique instance data
	GenerateRandomAffixes();
}


void AEquipmentPickup::GenerateRandomAffixes()
{
    if (!ObjItem->ItemDefinition || !ObjItem->ItemDefinition->GetStatsDataTable())
    {
        UE_LOG(LogItemPickup, Warning, TEXT("EquipmentPickup: Cannot generate affixes - missing definition or stats table"));
        return;
    }

    
    InstanceData.UniqueID = FGuid::NewGuid().ToString();

    
    FPHItemStats GeneratedStats = UPHItemFunctionLibrary::GenerateStats(
        ObjItem->ItemDefinition->GetStatsDataTable()
    );

    // Store in instance data
    InstanceData.Prefixes = GeneratedStats.Prefixes;
    InstanceData.Suffixes = GeneratedStats.Suffixes;

    // Copy any fixed implicits from definition to instance
    InstanceData.Implicits = ObjItem->ItemDefinition->GetImplicits();

    
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
        ObjItem->ItemDefinition
    );
    
    InstanceData.bHasNameBeenGenerated = true;

    // Initialize other instance data
    InstanceData.Quantity = 1;
    InstanceData.ItemLevel = 1;  
    InstanceData.bIdentified = true;
    
    // Copy durability from definition
    InstanceData.Durability = ObjItem->ItemDefinition->Equip.Durability;

    UE_LOG(LogItemPickup, Log, TEXT("Generated item: %s (Rarity: %d, Affixes: %d)"), 
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
    
    return ObjItem->ItemDefinition ? ObjItem->ItemDefinition->Base.ItemName : FText::GetEmpty();
}

EItemRarity AEquipmentPickup::GetInstanceRarity() const
{
    // Return instance rarity if valid, otherwise base rarity
    if (InstanceData.Rarity != EItemRarity::IR_None)
    {
        return InstanceData.Rarity;
    }
    
    return ObjItem->ItemDefinition ? ObjItem->ItemDefinition->Base.ItemRarity : EItemRarity::IR_None;
}

bool AEquipmentPickup::HandleInteraction(AActor* Actor, bool WasHeld, UItemDefinitionAsset*& PassedItemInfo, FConsumableItemData ConsumableItemData) 
{
	Super::InteractionHandle(Actor, WasHeld);
	
	PassedItemInfo = Cast<UItemDefinitionAsset>(ObjItem->ItemDefinition);
	return Super::HandleInteraction(Actor, WasHeld, PassedItemInfo, FConsumableItemData());
}	

void AEquipmentPickup::HandleHeldInteraction(APHBaseCharacter* Character) const
{
	
	if (Character)
	{
		if (Character->GetEquipmentManager()->IsItemEquippable(ObjItem) && ObjItem->ItemDefinition->Equip.MeetsRequirements(Character->GetCurrentStats()))
		{
			Character->GetEquipmentManager()->TryToEquip(ObjItem, true);
			UE_LOG(LogItemPickup, Warning, TEXT("Item Equip"));
		}
		else
		{
			
			if (UInventoryManager* OwnersInventory = Character->GetInventoryManager())
			{
				OwnersInventory->TryToAddItemToInventory(ObjItem, true);
			}
			else
			{
				UE_LOG(LogItemPickup, Warning, TEXT("ALSCharacter does not have an UInventoryManager component."));
			}
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

