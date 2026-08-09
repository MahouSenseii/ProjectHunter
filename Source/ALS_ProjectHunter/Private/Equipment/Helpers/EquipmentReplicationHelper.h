#pragma once

#include "CoreMinimal.h"

class UEquipmentManager;
class UItemInstance;
enum class EEquipmentSlot : uint8;

class ALS_PROJECTHUNTER_API FEquipmentReplicationHelper
{
public:
	static void CacheComponents(UEquipmentManager& Manager);
	static void OnRepEquippedItems(UEquipmentManager& Manager);
	static void ServerEquipItem(UEquipmentManager& Manager, UItemInstance* Item, EEquipmentSlot Slot, bool bSwapToBag);
	static void ServerUnequipItem(UEquipmentManager& Manager, EEquipmentSlot Slot, bool bMoveToBag);
	static void RebuildEquipmentMap(UEquipmentManager& Manager);
	static void AddEquipment(UEquipmentManager& Manager, EEquipmentSlot Slot, UItemInstance* Item);
	static void RemoveEquipment(UEquipmentManager& Manager, EEquipmentSlot Slot);
};
