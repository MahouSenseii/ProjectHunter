// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Interactables/Pickups/EquipmentPickup.h"
#include "WeaponPickup.generated.h"

/**
 * 
 */
UCLASS()
class ALS_PROJECTHUNTER_API AWeaponPickup : public AEquipmentPickup
{
	GENERATED_BODY()

public:

	
	FWeaponItemData WeaponData;
	
};
