// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/Data/UItemDefinitionAsset.h"

bool UItemDefinitionAsset::IsValidDefinition() const
{
	return Base.IsValid() || Equip.IsValid(); 
}

UItemDefinitionAsset* UItemDefinitionAsset::GetBaseItemInfo() const
{
	UItemDefinitionAsset* ItemInfo = nullptr;
	
	// Combine the Base and Equip data into a single FItemInformation structure
	ItemInfo->Base = Base;   // FItemBase -> FItemInformation.ItemInfo
	ItemInfo->Equip = Equip;  // FEquippableItemData -> FItemInformation.ItemData
	
	return ItemInfo;
}