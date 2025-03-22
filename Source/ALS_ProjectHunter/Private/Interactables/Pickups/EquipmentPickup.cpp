// Fill out your copyright notice in the Description page of Project Settings.


#include "Interactables/Pickups/EquipmentPickup.h"
#include "Components/InteractableManager.h"
#include "Components/InventoryManager.h"
#include  "Library/PHItemFunctionLibrary.h"
#include "Library/FL_InteractUtility.h"
#include "..\..\..\Public\UI\ToolTip\EquippableToolTip.h"

AEquipmentPickup::AEquipmentPickup(): StatsDataTable(nullptr)
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

	if(!ItemStats.bHasGenerated)
	{
		ItemStats = UPHItemFunctionLibrary::GenerateStats(StatsDataTable);
		ItemInfo =  UPHItemFunctionLibrary::GenerateItemName(ItemStats,ItemInfo);
	}
}


bool AEquipmentPickup::HandleInteraction(AActor* Actor, bool WasHeld, FItemInformation PassedItemInfo,
                                         FEquippableItemData EquippableItemData, FWeaponItemData WeaponItemData,
                                         FConsumableItemData ConsumableItemData) const
{
	Super::InteractionHandle(Actor, WasHeld);

	FItemInformation  PassedItemInformation = ItemInfo;
	FEquippableItemData  PassedEquippableItemData = EquipmentData; // Assuming you have relevant equippable data
	FWeaponItemData  PassedWeaponItemData = WeaponItemData;
	
	return Super::HandleInteraction(Actor, WasHeld,   PassedItemInformation,  PassedEquippableItemData,  PassedWeaponItemData, FConsumableItemData());
}

void AEquipmentPickup::HandleHeldInteraction(APHBaseCharacter* Character) const
{
	Super::HandleHeldInteraction(Character);

if (Character->GetEquipmentManager()->IsItemEquippable(ObjItem) && (UFL_InteractUtility::AreRequirementsMet(ObjItem, Character)))
	{
		Character->GetEquipmentManager()->TryToEquip(ObjItem, true, ObjItem->GetItemInfo().EquipmentSlot);
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

