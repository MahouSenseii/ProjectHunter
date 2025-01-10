// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PHItemStructLibrary.h"
#include "Item/BaseItem.h"
#include "Item/ConsumableItem.h"
#include "Item/EquippableItem.h"
#include "Item/WeaponItem.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PHItemFunctionLibrary.generated.h"

/**
 * 
 */
UCLASS()
class ALS_PROJECTHUNTER_API UPHItemFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "Checker")
	static bool AreItemSlotsEqual(FItemInformation FirstItem, FItemInformation SecondItem);
	static UBaseItem* GetItemInformation(FItemInformation ItemInfo, FEquippableItemData EquippableItemData,
	                              FWeaponItemData WeaponItemData, FConsumableItemData ConsumableItemData);
	static UEquippableItem* CreateEquippableItem(const FItemInformation& ItemInfo, const FEquippableItemData& EquippableItemData);
	static UConsumableItem* CreateConsumableItem(const FItemInformation& ItemInfo, FConsumableItemData ConsumableItemData);
	static UWeaponItem* CreateWeaponItem(const FItemInformation ItemInfo, const FEquippableItemData EquippableItemData,
	                              const FWeaponItemData WeaponItemData);
};
