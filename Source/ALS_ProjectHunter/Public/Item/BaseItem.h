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
	bool IsRotated() const { return ItemInfos.Rotated; }

	
	/** Toggles the rotation state of the item */
	UFUNCTION(BlueprintCallable)
	void Rotate() { ItemInfos.Rotated = !ItemInfos.Rotated; }

	UFUNCTION(BlueprintCallable)
	void SetRotated(bool inBool) { ItemInfos.Rotated =  inBool ;}

	
	/** Returns the icon of the item, considering rotation */
	UFUNCTION(BlueprintCallable)
	UMaterialInstance* GetIcon() const;

	

	UFUNCTION()
	static void ApplyEffectToTarget(AActor* Target, TSubclassOf<UGameplayEffect> GameplayEffectClass);

	UFUNCTION(BlueprintCallable, Category = "Getter")
	FItemInformation& GetItemInfo() {return  ItemInfos; }

	UFUNCTION(BlueprintCallable, Category = "Setter")
	void SetItemInfo(const FItemInformation NewItemInfo) { ItemInfos = NewItemInfo; }

private:
	
	/** Information related to the item */
	UPROPERTY()
	FItemInformation ItemInfos;
	
	
};	
