// Fill out your copyright notice in the Description page of Project Settings.


#include "Menu/Widgets/MenuBaseWidget.h"

#include "Character/PHBaseCharacter.h"
#include "Inventory/Components/InventoryManager.h"

void UMenuBaseWidget::NativeInitializeForCharacter(APHBaseCharacter* Character)
{
	Super::NativeInitializeForCharacter(Character);

	EquipmentManager = Character->GetEquipmentManager();
	StatsManager     = Character->GetStatsManager();

	// InventoryManager is a Blueprint-added component (no C++ getter on the
	// character), so resolve it generically. This was declared + exposed via
	// GetInventoryManager() but never assigned — every menu page saw null.
	InventoryManager = Character->FindComponentByClass<UInventoryManager>();
}

void UMenuBaseWidget::NativeReleaseCharacter()
{
	Super::NativeReleaseCharacter();

	EquipmentManager = nullptr;
	StatsManager = nullptr;
	InventoryManager = nullptr;
}
