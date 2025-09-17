// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/ItemInstance.h"

#include "Item/Data/UItemDefinitionAsset.h"
#include "Library/PHItemStructLibrary.h"


void UItemInstanceObject::RebuildItemInfoView()
{
	ItemInfoView = FItemInformation{};            
	if (const UItemDefinitionAsset* Def = BaseDef.LoadSynchronous())
	{
		// Copy static author-time data
		ItemInfoView.ItemInfo = Def->Base;         
		ItemInfoView.ItemData = Def->Equip;      

		// Merge implicit + rolled affixes into the container the rest of your code expects
		ItemInfoView.ItemData.Affixes.Implicits     = Instance.Implicits;
		ItemInfoView.ItemData.Affixes.Prefixes      = Instance.Prefixes;
		ItemInfoView.ItemData.Affixes.Suffixes      = Instance.Suffixes;
		ItemInfoView.ItemData.Affixes.Crafted       = Instance.Crafted;
		ItemInfoView.ItemData.Affixes.bAffixesGenerated = ItemInfoView.ItemData.Affixes.GetTotalAffixCount() > 0;             

		// If you want durability to influence value/weight, you can also copy or apply it here
	}
}
