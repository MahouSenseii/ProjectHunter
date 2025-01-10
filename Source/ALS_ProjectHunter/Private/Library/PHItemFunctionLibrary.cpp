// Fill out your copyright notice in the Description page of Project Settings.


#include "Library/PHItemFunctionLibrary.h"

#include "Item/ConsumableItem.h"
#include "Item/WeaponItem.h"

bool UPHItemFunctionLibrary::AreItemSlotsEqual(const FItemInformation FirstItem, const FItemInformation SecondItem)
{
	if((FirstItem.EquipmentSlot == SecondItem.EquipmentSlot)|| (FirstItem.EquipmentSlot == EEquipmentSlot::ES_MainHand && SecondItem.EquipmentSlot == EEquipmentSlot::ES_OffHand)
		|| (SecondItem.EquipmentSlot == EEquipmentSlot::ES_MainHand && FirstItem.EquipmentSlot == EEquipmentSlot::ES_OffHand))
	{

		return  true;
	}


	return false;
}


UBaseItem* UPHItemFunctionLibrary::GetItemInformation(FItemInformation ItemInfo, FEquippableItemData EquippableItemData,
                                                      FWeaponItemData WeaponItemData, FConsumableItemData ConsumableItemData)
{
    switch (ItemInfo.EquipmentSlot)
    {
    case EEquipmentSlot::ES_Belt:
    case EEquipmentSlot::ES_Boots:
    case EEquipmentSlot::ES_Chestplate:
    case EEquipmentSlot::ES_Head:
    case EEquipmentSlot::ES_Neck:
    case EEquipmentSlot::ES_Legs:
    case EEquipmentSlot::ES_Cloak:
    case EEquipmentSlot::ES_Gloves:
    case EEquipmentSlot::ES_Ring:
        return CreateEquippableItem(ItemInfo, EquippableItemData);

    case EEquipmentSlot::ES_Flask:
        return CreateConsumableItem(ItemInfo, ConsumableItemData);

    case EEquipmentSlot::ES_MainHand:
    case EEquipmentSlot::ES_OffHand:
        return CreateWeaponItem(ItemInfo, EquippableItemData, WeaponItemData);

    case EEquipmentSlot::ES_None:
    default:
        UE_LOG(LogTemp, Warning, TEXT("Unknown Equipment Slot: %d"), static_cast<int32>(ItemInfo.EquipmentSlot));
        return nullptr;
    }
}

UEquippableItem* UPHItemFunctionLibrary::CreateEquippableItem(const FItemInformation& ItemInfo, const FEquippableItemData& EquippableItemData)
{
    UEquippableItem* NewEquipItem = NewObject<UEquippableItem>();
    NewEquipItem->SetItemInfo(ItemInfo);
    NewEquipItem->SetEquippableData(EquippableItemData);
    return NewEquipItem;
}

UConsumableItem* UPHItemFunctionLibrary::CreateConsumableItem(const FItemInformation& ItemInfo, const FConsumableItemData ConsumableItemData)
{
    UConsumableItem* NewConsumableItem = NewObject<UConsumableItem>();
    NewConsumableItem->SetItemInfo(ItemInfo);
    NewConsumableItem->SetConsumableData(ConsumableItemData);
    return NewConsumableItem;
}

UWeaponItem* UPHItemFunctionLibrary::CreateWeaponItem( const FItemInformation ItemInfo, const FEquippableItemData EquippableItemData, const FWeaponItemData WeaponItemData)
{
    UWeaponItem* NewWeaponItem = NewObject<UWeaponItem>();
    NewWeaponItem->SetItemInfo(ItemInfo);
    NewWeaponItem->SetEquippableData(EquippableItemData);
    NewWeaponItem->SetWeaponData(WeaponItemData);
    return NewWeaponItem;
}
