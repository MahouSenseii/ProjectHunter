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

	FConsumableItemData ConsumableData;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void OnUse();

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TSubclassOf<UGameplayEffect> OnUseEffect;
};
