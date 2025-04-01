// Fill out your copyright notice in the Description page of Project Settings.


#include "Interactables/Pickups/WeaponPickup.h"

bool AWeaponPickup::HandleInteraction(AActor* Actor, bool WasHeld, FItemInformation  PassedItemInfo,
	FEquippableItemData EquippableItemData,
	FConsumableItemData ConsumableItemData) const
{
	Super::InteractionHandle(Actor, WasHeld);

	const FItemInformation  PassedItemInformation = ItemInfo;
	FEquippableItemData  PassedEquippableItemData = EquipmentData; // Assuming you have relevant equippable data

	return AItemPickup::HandleInteraction(Actor, WasHeld,  PassedItemInformation,  PassedEquippableItemData, FConsumableItemData());
}
