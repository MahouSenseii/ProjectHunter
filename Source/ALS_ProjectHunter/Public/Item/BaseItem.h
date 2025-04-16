// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "Library/PHItemStructLibrary.h"
#include "BaseItem.generated.h"

/**
 * 
 */



UCLASS()
class ALS_PROJECTHUNTER_API UBaseItem : public UObject
{
	GENERATED_BODY()

public:

	virtual void Initialize(FItemInformation& ItemInfo) PURE_VIRTUAL(UItemData::Initialize, );

	/** Returns the dimensions of the item, considering rotation */
	UFUNCTION(BlueprintCallable)
	FIntPoint GetDimensions();
	
	/** Check if the item is rotated */
	UFUNCTION(BlueprintCallable)
	bool IsRotated() const { return ItemInfos.ItemInfo.Rotated; }
	
	/** Toggles the rotation state of the item */
	UFUNCTION(BlueprintCallable)
	void Rotate() { ItemInfos.ItemInfo.Rotated = !ItemInfos.ItemInfo.Rotated; }

	UFUNCTION(BlueprintCallable)
	void SetRotated(bool inBool) { ItemInfos.ItemInfo.Rotated =  inBool ;}

	
	/** Returns the icon of the item, considering rotation */
	UFUNCTION(BlueprintCallable)
	UMaterialInstance* GetIcon() const;

	UFUNCTION(BlueprintCallable)
	virtual int32 GetQuantity() const;

	UFUNCTION(BlueprintCallable)
	void AddQuantity(const int32 InQty);

	UFUNCTION(BlueprintCallable)
	bool IsStackable() const { return ItemInfos.ItemInfo.Stackable; }

	UFUNCTION()
	static void ApplyEffectToTarget(AActor* Target, TSubclassOf<UGameplayEffect> GameplayEffectClass);

	UFUNCTION(BlueprintCallable, Category = "Getter")
	FItemInformation& GetItemInfo() {return  ItemInfos; }

	UFUNCTION(BlueprintCallable, Category = "Setter")
	void SetItemInfo(const FItemInformation NewItemInfo) { ItemInfos = NewItemInfo; }

	UFUNCTION(BlueprintCallable, Category = "Setter")
	void SetEquipmentData(const FEquippableItemData InData) { ItemInfos.ItemData = InData;}
	
protected:
	
	/** Information related to the item */
	UPROPERTY()
	FItemInformation ItemInfos;
	
	
};	
