// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/PHAttributeSet.h"
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

	virtual void Initialize(const FItemInformation& ItemInfo) PURE_VIRTUAL(UItemData::Initialize, );

	/** Returns the dimensions of the item, considering rotation */
	UFUNCTION(BlueprintCallable)
	FIntPoint GetDimensions();
	
	/** Check if the item is rotated */
	UFUNCTION(BlueprintCallable)
	bool IsRotated() const { return Rotated; }

	/** Toggles the rotation state of the item */
	UFUNCTION(BlueprintCallable)
	void Rotate() { Rotated = !Rotated; }

	
	/** Returns the icon of the item, considering rotation */
	UFUNCTION(BlueprintCallable)
	UMaterialInstance* GetIcon();

	

	UFUNCTION()
	static void ApplyEffectToTarget(AActor* Target, TSubclassOf<UGameplayEffect> GameplayEffectClass);
	
	/** Information related to the item */
	UPROPERTY(BlueprintReadWrite, Category = "ItemInfo")
	FItemInformation ItemInfos;
	
	/** Indicates the current slot of the item */
	UPROPERTY(BlueprintReadWrite)
	ECurrentItemSlot SavedSlot;

	/** Indicates whether the item is rotated */
	UPROPERTY(BlueprintReadWrite)
	bool Rotated = false;

	/** Transform representing the item's position, rotation, and scale */
	UPROPERTY(BlueprintReadWrite)
	FTransform Transform;

	
};	
