// Fill out your copyright notice in the Description page of Project Settings.


#include "Interactables/Pickups/WeaponPickup.h"

bool AWeaponPickup::HandleInteraction(AActor* Actor, bool WasHeld, FItemInformation  PassedItemInfo,
	FEquippableItemData EquippableItemData, FWeaponItemData WeaponItemData,
	FConsumableItemData ConsumableItemData) const
{
	Super::InteractionHandle(Actor, WasHeld);

	FItemInformation  PassedItemInformation = ItemInfo;
	FEquippableItemData  PassedEquippableItemData = EquipmentData; // Assuming you have relevant equippable data
	FWeaponItemData  PassedWeaponItemData = WeaponData;

	return AItemPickup::HandleInteraction(Actor, WasHeld,  PassedItemInformation,  PassedEquippableItemData,  PassedWeaponItemData, FConsumableItemData());
}
