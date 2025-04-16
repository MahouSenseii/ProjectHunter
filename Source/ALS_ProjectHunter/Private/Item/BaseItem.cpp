// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/BaseItem.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"


FIntPoint UBaseItem::GetDimensions()
{
	if (ItemInfos.ItemInfo.Rotated)
	{
		// Return reversed dimensions if Rotated is true
		return FIntPoint(ItemInfos.ItemInfo.Dimensions.Y, ItemInfos.ItemInfo.Dimensions.X);
	}

	// Return original dimensions if Rotated is false
	return ItemInfos.ItemInfo.Dimensions;
}


UMaterialInstance* UBaseItem::GetIcon() const
{
	if (ItemInfos.ItemInfo.Rotated)
	{
		// Return rotated image if available; otherwise, return original image
		return ItemInfos.ItemInfo.ItemImageRotated ? ItemInfos.ItemInfo.ItemImageRotated : ItemInfos.ItemInfo.ItemImage;
	}
	return ItemInfos.ItemInfo.ItemImage ? ItemInfos.ItemInfo.ItemImage : nullptr; // Return original image if available, or null if not
}

int32 UBaseItem::GetQuantity() const
{
	return ItemInfos.ItemInfo.Quantity;
}

void UBaseItem::AddQuantity(const int32 InQty)
{
	if (InQty <= 0) return;

	const int32 MaxStack = ItemInfos.ItemInfo.MaxStackSize;
	if (MaxStack > 0)
	{
		ItemInfos.ItemInfo.Quantity = FMath::Clamp(GetQuantity() + InQty, 0, MaxStack);
	}
	else
	{
		ItemInfos.ItemInfo.Quantity += InQty;
	}
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


