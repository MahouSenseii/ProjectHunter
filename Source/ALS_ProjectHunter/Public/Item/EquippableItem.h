// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Item/BaseItem.h"
#include "EquippableItem.generated.h"

/**
 * 
 */
UCLASS()
class ALS_PROJECTHUNTER_API UEquippableItem : public UBaseItem
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, Category = "Base")
	FEquippableItemData EquippableItemData;

	
	
};
