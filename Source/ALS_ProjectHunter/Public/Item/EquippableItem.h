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
	// Sets default values for this actor's properties
	UEquippableItem();

	
	
	virtual void Initialize(FItemInformation& ItemInfo) override;

	UFUNCTION(BlueprintCallable, Category = "Base")
	FEquippableItemData GetEquippableData() const { return EquippableData;}

	UFUNCTION(BlueprintCallable, Category = "Base")
	void SetEquippableData(FEquippableItemData Data) { EquippableData = Data; }

	
	
protected:
	UPROPERTY(EditAnywhere, Category = "ItemInfo")
	FEquippableItemData EquippableData;

	// Set in BP will be all stats that the item can generate
	UPROPERTY(EditAnywhere, Category = "ItemInfo|Stats")
	UDataTable* StatsDataTable;

	UPROPERTY(EditAnywhere, Category = "ItemInfo|Stats")
	FPHItemStats ItemStats;
};
