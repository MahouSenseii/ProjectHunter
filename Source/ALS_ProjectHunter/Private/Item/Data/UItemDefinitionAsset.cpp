// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/Data/UItemDefinitionAsset.h"

bool UItemDefinitionAsset::IsValidDefinition() const
{
	return Base.IsValid() || Equip.IsValid(); 
}

FItemInformation UItemDefinitionAsset::GetBaseItemInfo() const
{
	FItemInformation ItemInfo;
	
	// Combine the Base and Equip data into a single FItemInformation structure
	ItemInfo.ItemInfo = Base;   // FItemBase -> FItemInformation.ItemInfo
	ItemInfo.ItemData = Equip;  // FEquippableItemData -> FItemInformation.ItemData
	
	return ItemInfo;
}