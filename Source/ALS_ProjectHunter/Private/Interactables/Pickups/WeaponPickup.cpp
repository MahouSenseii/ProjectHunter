// Fill out your copyright notice in the Description page of Project Settings.


#include "Interactables/Pickups/WeaponPickup.h"



bool AWeaponPickup::HandleInteraction(AActor* Actor, bool WasHeld, UItemDefinitionAsset*& ItemData,
	FConsumableItemData ConsumableItemData)
{
	UItemDefinitionAsset*  PassedItemInformation = ObjItem->ItemDefinition;
	Super::HandleInteraction(Actor, WasHeld, ItemData, ConsumableItemData);
	return AItemPickup::HandleInteraction(Actor, WasHeld, PassedItemInformation,  FConsumableItemData());
}

