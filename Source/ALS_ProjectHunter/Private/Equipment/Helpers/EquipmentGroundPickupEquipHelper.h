#pragma once

#include "CoreMinimal.h"

class UEquipmentManager;
class UItemInstance;
enum class EEquipmentSlot : uint8;

class ALS_PROJECTHUNTER_API FEquipmentGroundPickupEquipHelper
{
public:
	static bool TryEquipItem(UEquipmentManager& Manager, UItemInstance* Item, EEquipmentSlot& OutEquippedSlot,
	                         bool bSwapToBag);
};
