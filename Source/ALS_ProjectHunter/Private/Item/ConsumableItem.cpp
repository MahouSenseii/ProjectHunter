// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/ConsumableItem.h"

#include "Components/InventoryManager.h"

void UConsumableItem::Initialize(FItemInformation& ItemInfo)
{
	Super::Initialize(ItemInfo);
	// Initialize the consumable data from the item info
	ConsumableData.GameplayEffectClass = ItemInfo.ItemInfo.GameplayEffectClass;

	// Default quantity is 1 unless set otherwise
	ConsumableData.Quantity = 1;
	
}

void UConsumableItem::UseItem(AActor* Target)
{
	if (ConsumableData.Quantity > 0 && ConsumableData.GameplayEffectClass)
	{
		ApplyEffectToTarget(Target, ConsumableData.GameplayEffectClass);
		ConsumableData.Quantity--;
        
		// Remove item when fully consumed
		if (ConsumableData.Quantity == 0)
		{
			if (APHBaseCharacter* Player = Cast<APHBaseCharacter>(Target))
			{
				Player->GetInventoryManager()->RemoveItemFromInventory(this);
			}
		}
	}
}
