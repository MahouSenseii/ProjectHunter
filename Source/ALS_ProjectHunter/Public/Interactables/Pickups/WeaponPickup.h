// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Interactables/Pickups/EquipmentPickup.h"
#include "WeaponPickup.generated.h"

/**
 * @class AWeaponPickup
 *
 * A class representing a weapon pickup in the game.
 */
UCLASS()
class ALS_PROJECTHUNTER_API AWeaponPickup : public AEquipmentPickup
{
	GENERATED_BODY()

public:
virtual bool HandleInteraction(AActor* Actor, bool WasHeld,  UItemDefinitionAsset*& ItemData, FConsumableItemData ConsumableItemData) const override;

};
