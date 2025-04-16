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

	virtual void Initialize( FItemInformation& ItemInfo) override;

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
