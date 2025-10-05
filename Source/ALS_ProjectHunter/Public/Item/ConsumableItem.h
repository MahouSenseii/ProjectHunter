// Fill out your copyright  notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Item/BaseItem.h"
#include "ConsumableItem.generated.h"

/**
 * Represents a consumable item derived from the base item class.
 * Allows interaction and manipulation of consumable-specific data and functionality.
 */
UCLASS()
class ALS_PROJECTHUNTER_API UConsumableItem : public UBaseItem
{
	GENERATED_BODY()

public:

	virtual void Initialize( UItemDefinitionAsset*& ItemInfo) override;

	/** Use the consumable item, applying its effect */
	UFUNCTION(BlueprintCallable, Category = "Consumable")
	void UseItem(AActor* Target);
	
	UFUNCTION(BlueprintCallable, Category = "Consumable")
	FConsumableItemData GetConsumableData() const { return ConsumableData;}

	UFUNCTION(BlueprintCallable, Category = "Consumable")
	void SetConsumableData(FConsumableItemData Data) { ConsumableData = Data; }
protected:
	UPROPERTY(EditAnywhere,Category = "ItemInfo")
	FConsumableItemData ConsumableData;

};
