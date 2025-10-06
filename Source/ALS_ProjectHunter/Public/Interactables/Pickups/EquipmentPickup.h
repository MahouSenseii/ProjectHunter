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

	virtual void BeginPlay() override;
	
	virtual bool HandleInteraction(AActor* Actor, bool WasHeld,  UItemDefinitionAsset*& ItemInfo, FConsumableItemData ConsumableItemData) const override;
	
	virtual void HandleHeldInteraction(APHBaseCharacter* Character) const override;
	virtual void HandleSimpleInteraction(APHBaseCharacter* Character) const override;
	virtual void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) override;
	virtual void OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex) override;


	
};
