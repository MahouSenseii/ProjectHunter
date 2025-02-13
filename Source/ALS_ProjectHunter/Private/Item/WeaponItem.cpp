// Fill out your copyright notice in the Description page of Project Settings.


 #include "Item/WeaponItem.h"

void UWeaponItem::SetWeaponData(const FWeaponItemData& NewWeaponItemData)
{
 WeaponItemData = NewWeaponItemData;
}

FWeaponItemData UWeaponItem::GetWeaponData() const
{
 return WeaponItemData;
}
