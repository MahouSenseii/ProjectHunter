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

virtual bool HandleInteraction(AActor* Actor, bool WasHeld, FItemInformation ItemInfo, FEquippableItemData EquippableItemData, FWeaponItemData WeaponItemData, FConsumableItemData ConsumableItemData) const override;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ItemInfomation")
	FWeaponItemData WeaponData;
	
};
