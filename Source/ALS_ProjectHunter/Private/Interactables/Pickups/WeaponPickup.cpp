// Fill out your copyright notice in the Description page of Project Settings.


#include "Interactables/Pickups/WeaponPickup.h"



bool AWeaponPickup::HandleInteraction(AActor* Actor, bool WasHeld, UItemDefinitionAsset*& ItemData,
	FConsumableItemData ConsumableItemData)
{
	// Just call the parent once and return its result
	ItemData = ObjItem->ItemDefinition;
	return Super::HandleInteraction(Actor, WasHeld, ItemData, ConsumableItemData);
}
