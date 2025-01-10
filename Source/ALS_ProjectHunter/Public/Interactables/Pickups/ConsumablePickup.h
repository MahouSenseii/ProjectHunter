// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Interactables/Pickups/ItemPickup.h"
#include "ConsumablePickup.generated.h"

/**
 * 
 */
UCLASS()
class ALS_PROJECTHUNTER_API AConsumablePickup : public AItemPickup
{
	GENERATED_BODY()

public:

	virtual void BeginPlay() override;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ItemInfomation")
	FConsumableItemData ConsumableData;
virtual bool HandleInteraction(AActor* Actor, bool WasHeld, FItemInformation ItemInfo, FEquippableItemData EquippableItemData, FWeaponItemData WeaponItemData, FConsumableItemData ConsumableItemData) const override;
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void OnUse();

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TSubclassOf<UGameplayEffect> OnUseEffect;
};
