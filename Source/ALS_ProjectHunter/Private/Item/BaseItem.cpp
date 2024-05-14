// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/BaseItem.h"
#include "AbilitySystemBlueprintLibrary.h"


FIntPoint UBaseItem::GetDimensions()
{
	if (Rotated)
	{
		// Return reversed dimensions if Rotated is true
		return FIntPoint(ItemInfo.Dimensions.Y, ItemInfo.Dimensions.X);
	}

	// Return original dimensions if Rotated is false
	return ItemInfo.Dimensions;
}


UMaterialInstance* UBaseItem::GetIcon()
{
	if (Rotated)
	{
		// Return rotated image if available; otherwise, return original image
		return ItemInfo.ItemImageRotated ? ItemInfo.ItemImageRotated : ItemInfo.ItemImage;
	}
	return ItemInfo.ItemImage ? ItemInfo.ItemImage : nullptr; // Return original image if available, or null if not
}


void UBaseItem::ApplyEffectToTarget(AActor* Target, TSubclassOf<UGameplayEffect> GameplayEffectClass)
{
	UAbilitySystemComponent*TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	if(TargetASC == nullptr) return;

	check(GameplayEffectClass);
	const FGameplayEffectContextHandle EffectGameplayContextHandle = TargetASC->MakeEffectContext();
	const FGameplayEffectSpecHandle EffectSpecHandle = TargetASC->MakeOutgoingSpec
	(GameplayEffectClass, 1.f, EffectGameplayContextHandle);
	
	TargetASC->ApplyGameplayEffectSpecToSelf(*EffectSpecHandle.Data.Get());
}


