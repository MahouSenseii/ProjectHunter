// Fill out your copyright notice in the Description page of Project Settings.


#include "Menu/Widgets/MenuBaseWidget.h"

#include "Character/PHBaseCharacter.h"

void UMenuBaseWidget::NativeInitializeForCharacter(APHBaseCharacter* Character)
{
	Super::NativeInitializeForCharacter(Character);
	
	EquipmentManager = Character->GetEquipmentManager();
	StatsManager     = Character->GetStatsManager();
}

void UMenuBaseWidget::NativeReleaseCharacter()
{
	Super::NativeReleaseCharacter();
	
	EquipmentManager = nullptr;
	StatsManager = nullptr;
}
