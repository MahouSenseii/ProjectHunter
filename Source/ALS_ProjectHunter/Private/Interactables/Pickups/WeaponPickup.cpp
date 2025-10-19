// Fill out your copyright notice in the Description page of Project Settings.


#include "Interactables/Pickups/WeaponPickup.h"



bool AWeaponPickup::HandleInteraction(AActor* Actor, bool WasHeld, UItemDefinitionAsset*& ItemData,
	FConsumableItemData ConsumableItemData)
{
	UItemDefinitionAsset*  PassedItemInformation = ObjItem->ItemDefinition;
	AEquipmentPickup::HandleInteraction(Actor, WasHeld, PassedItemInformation,  FConsumableItemData());
	return AItemPickup::HandleInteraction(Actor, WasHeld, PassedItemInformation,  FConsumableItemData());
}

