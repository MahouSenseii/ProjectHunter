// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/ConsumableItem.h"

#include "Character/Player/PHPlayerCharacter.h"
#include "Components/InventoryManager.h"

void UConsumableItem::Initialize(const FItemInformation& ItemInfo)
{
	Super::Initialize(ItemInfo);
	// Initialize the consumable data from the item info
	ConsumableData.GameplayEffectClass = ItemInfo.GameplayEffectClass;

	// Default quantity is 1 unless set otherwise
	ConsumableData.Quantity = 1;
	
}

void UConsumableItem::UseItem(AActor* Target)
{
	if (ConsumableData.Quantity > 0 && ConsumableData.GameplayEffectClass)
	{
		// Apply the gameplay effect to the target actor
		ApplyEffectToTarget(Target, ConsumableData.GameplayEffectClass);

		// Decrease the quantity of the item
		ConsumableData.Quantity--;
	}
	else
	{
		// Attempt to cast the Target to APHPlayerCharacter

		if (APHPlayerCharacter* Player = Cast<APHPlayerCharacter>(Target))
		{
			Player->GetInventoryManager()->RemoveItemInInventory(this);
		}
		else
		{
			// Handle the case where the cast fails, if necessary
			UE_LOG(LogTemp, Warning, TEXT("UseItem: Target is not a valid APHPlayerCharacter."));
		}
	}
}
void UConsumableItem::SetQuantity(int NewQuantity)
{
	if (NewQuantity >= 0)
	{
		ConsumableData.Quantity = NewQuantity;
	}
}

int UConsumableItem::GetQuantity() const
{
	return ConsumableData.Quantity;
}
