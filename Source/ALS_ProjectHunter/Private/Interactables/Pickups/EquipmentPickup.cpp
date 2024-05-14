// Fill out your copyright notice in the Description page of Project Settings.


#include "Interactables/Pickups/EquipmentPickup.h"

#include "Components/InteractableManager.h"
#include "Components/InventoryManager.h"

AEquipmentPickup::AEquipmentPickup()
{
	
}

bool AEquipmentPickup::InteractionHandle(AActor* Actor, bool WasHeld) const
{
	return Super::InteractionHandle(Actor, WasHeld);
}

void AEquipmentPickup::HandleHeldInteraction(APHBaseCharacter* Character) const
{
	Super::HandleHeldInteraction(Character);

/*	if (Character->EquipmentManager->IsItemEquippable(ObjItem) && (UFL_InteractUtility::AreRequirementsMet(ObjItem, Character)))
	{
		Character->EquipmentManager->TryToEquip(ObjItem, true, ObjItem->ItemInfo.EquipmentSlot);
	}
	else
	{
		// Attempt to get the Inventory Manager component from the ALSCharacter
		if (UInventoryManager* OwnersInventory = Cast<UInventoryManager>(AlsCharacter->GetComponentByClass(UInventoryManager::StaticClass())))
		{
			// If the Inventory Manager exists, try to add the item to the inventory
			OwnersInventory->TryToAddItemToInventory(ObjItem, true);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("ALSCharacter does not have an UInventoryManager component."));
		}
	}
	InteractableManager->RemoveInteraction();*/
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


UBaseItem* AEquipmentPickup::GetItemInformation() const
{
	UBaseItem* CreatedItem = NewObject<UBaseItem>(const_cast<AEquipmentPickup*>(this), UBaseItem::StaticClass());
	CreatedItem->ItemInfo = ItemInfo;
	return CreatedItem;
}
