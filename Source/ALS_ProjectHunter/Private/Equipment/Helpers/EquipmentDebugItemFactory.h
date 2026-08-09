#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Item/Library/Enums/ItemEnums.h"

class UEquipmentManager;
class UItemInstance;

class ALS_PROJECTHUNTER_API FEquipmentDebugItemFactory
{
public:
	static UItemInstance* GiveWeapon(UEquipmentManager& Manager, const FDataTableRowHandle& BaseItemHandle,
	                                 int32 ItemLevel, EItemRarity Rarity, bool bGenerateAffixes);
};
