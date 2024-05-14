// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EquippableItem.h"
#include "WeaponItem.generated.h"

/**
 * 
 */
UCLASS()
class ALS_PROJECTHUNTER_API UWeaponItem : public UEquippableItem
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, Category = "Base")
	FWeaponItemData WeaponItemData;
};
