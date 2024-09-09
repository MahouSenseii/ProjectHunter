// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/BaseItem.h"
#include "AbilitySystemBlueprintLibrary.h"



FIntPoint UBaseItem::GetDimensions()
{
	if (Rotated)
	{
		// Return reversed dimensions if Rotated is true
		return FIntPoint(ItemInfos.Dimensions.Y, ItemInfos.Dimensions.X);
	}

	// Return original dimensions if Rotated is false
	return ItemInfos.Dimensions;
}


UMaterialInstance* UBaseItem::GetIcon()
{
	if (Rotated)
	{
		// Return rotated image if available; otherwise, return original image
		return ItemInfos.ItemImageRotated ? ItemInfos.ItemImageRotated : ItemInfos.ItemImage;
	}
	return ItemInfos.ItemImage ? ItemInfos.ItemImage : nullptr; // Return original image if available, or null if not
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


