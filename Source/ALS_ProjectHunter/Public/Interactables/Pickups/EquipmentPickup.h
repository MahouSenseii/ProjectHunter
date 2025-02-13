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

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ItemInfomation" )
	FEquippableItemData EquipmentData;
	
	virtual bool HandleInteraction(AActor* Actor, bool WasHeld, FItemInformation ItemInfo, FEquippableItemData EquippableItemData, FWeaponItemData WeaponItemData, FConsumableItemData ConsumableItemData) const override;
	virtual void HandleHeldInteraction(APHBaseCharacter* Character) const override;
	virtual void HandleSimpleInteraction(APHBaseCharacter* Character) const override;

	// allows to for setting the Items Stats used mainly when transfer between equipped and pick etc. 
	UFUNCTION(BlueprintCallable) void SetItemStats(const FPHItemStats& InStats) {ItemStats = InStats;}

	//returns the generated stats on the item 
	UFUNCTION(BlueprintCallable) FPHItemStats GetItemStats(){ return ItemStats;}
	
public:
	UPROPERTY(EditAnywhere, Category = "ItemInfo")
	FEquippableItemData EquippableData;

	// Set in BP will be all stats that the item can generate
	UPROPERTY(EditAnywhere, Category = "ItemInfo|Stats")
	UDataTable* StatsDataTable;
	
	UPROPERTY(EditAnywhere, Category = "ItemInfo|Stats")
	FPHItemStats ItemStats;
	
};
