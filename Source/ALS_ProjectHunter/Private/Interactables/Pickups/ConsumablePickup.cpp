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
