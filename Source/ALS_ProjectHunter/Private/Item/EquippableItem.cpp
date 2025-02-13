// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/EquippableItem.h"

#include "Library/PHItemFunctionLibrary.h"

UEquippableItem::UEquippableItem(): StatsDataTable(nullptr)
{
}

void UEquippableItem::Initialize( FItemInformation& ItemInfo)
{
	Super::Initialize(ItemInfo);

	if(!ItemStats.bHasGenerated)
	{
		ItemStats = UPHItemFunctionLibrary::GenerateStats(StatsDataTable);
		ItemInfo = UPHItemFunctionLibrary::GenerateItemName(ItemStats, ItemInfo);



	}
}
