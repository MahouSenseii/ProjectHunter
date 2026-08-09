#pragma once

#include "CoreMinimal.h"

class UEquipmentManager;
class UItemInstance;
enum class EEquipmentSlot : uint8;

class ALS_PROJECTHUNTER_API FEquipmentHandSlotMutationHelper
{
public:
	static UItemInstance* EquipTwoHandedItem(UEquipmentManager& Manager, UItemInstance* Item, bool bSwapToBag);
	static UItemInstance* UnequipConflictingTwoHandedItem(UEquipmentManager& Manager, EEquipmentSlot IncomingSlot,
	                                                      bool bSwapToBag);
};
