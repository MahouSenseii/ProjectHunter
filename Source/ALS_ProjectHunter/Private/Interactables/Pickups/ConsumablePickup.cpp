// Fill out your copyright notice in the Description page of Project Settings.


#include "..\..\..\Public\Interactables\Pickups\ConsumablePickup.h"

void AConsumablePickup::OnUse_Implementation()
{
}

void AConsumablePickup::BeginPlay()
{
	Super::BeginPlay();

	ConsumableData.GameplayEffectClass = OnUseEffect;
}

bool AConsumablePickup::HandleInteraction(AActor* Actor, bool WasHeld,  UItemDefinitionAsset*& PassedItemInfo, FConsumableItemData ConsumableItemData) const
{
	Super::InteractionHandle(Actor, WasHeld);

	UItemDefinitionAsset*  PassedItemInformation = ItemInfo;
	const FConsumableItemData PassedConsumableItemData = ConsumableData;

	return Super::HandleInteraction(Actor, WasHeld,  PassedItemInformation, PassedConsumableItemData);
}
