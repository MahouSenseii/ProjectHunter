// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/Data/UItemDefinitionAsset.h"

bool UItemDefinitionAsset::IsValidDefinition() const
{
	return Base.IsValid() || Equip.IsValid(); 
}
