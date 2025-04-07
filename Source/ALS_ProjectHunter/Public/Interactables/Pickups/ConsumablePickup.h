// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Interactables/Pickups/ItemPickup.h"
#include "ConsumablePickup.generated.h"

/**
 
 @class AConsumablePickup
 @brief A class representing a consumable item pickup that can be interacted with
 */
UCLASS()
class ALS_PROJECTHUNTER_API AConsumablePickup : public AItemPickup
{
	GENERATED_BODY()

public:

	virtual void BeginPlay() override;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ItemInfomation")
	FConsumableItemData ConsumableData;
virtual bool HandleInteraction(AActor* Actor, bool WasHeld, FItemInformation ItemInfo,FConsumableItemData ConsumableItemData) const override;
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void OnUse();

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TSubclassOf<UGameplayEffect> OnUseEffect;
};
