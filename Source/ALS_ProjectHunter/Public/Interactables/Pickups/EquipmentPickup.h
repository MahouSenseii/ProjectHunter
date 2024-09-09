// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Interactables/Pickups/ItemPickup.h"
#include "EquipmentPickup.generated.h"

/**
 * 
 */
UCLASS()
class ALS_PROJECTHUNTER_API AEquipmentPickup : public AItemPickup
{
	GENERATED_BODY()

public:

	AEquipmentPickup();

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ItemInfomation" )
	FEquippableItemData EquipmentData;
	
	virtual bool InteractionHandle(AActor* Actor, bool WasHeld) const override;
	virtual void HandleHeldInteraction(APHBaseCharacter* Character) const override;
	virtual void HandleSimpleInteraction(APHBaseCharacter* Character) const override;

	
	
};
