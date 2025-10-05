// Fill out your copyright notice in the Description page of Project Settings.


#include "Interactables/Pickups/WeaponPickup.h"

bool AWeaponPickup::HandleInteraction(AActor* Actor, bool WasHeld, UItemDefinitionAsset*  PassedItemInfo,
	FConsumableItemData ConsumableItemData) const
{
	Super::InteractionHandle(Actor, WasHeld);

	 UItemDefinitionAsset*  PassedItemInformation = ItemInfo;

	return AItemPickup::HandleInteraction(Actor, WasHeld, PassedItemInformation,  FConsumableItemData());
}
