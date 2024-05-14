// Fill out your copyright  otice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Item/BaseItem.h"
#include "ConsumableItem.generated.h"

/**
 * 
 */
UCLASS()
class ALS_PROJECTHUNTER_API UConsumableItem : public UBaseItem
{
	GENERATED_BODY()

public:
	
	UPROPERTY(EditAnywhere, Category = "Base")
	FConsumableItemData ConsumableItemData;
};
