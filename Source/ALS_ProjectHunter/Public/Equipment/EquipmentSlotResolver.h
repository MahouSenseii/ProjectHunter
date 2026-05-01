#pragma once

#include "CoreMinimal.h"

class UEquipmentManager;
class UItemInstance;
enum class EEquipmentSlot : uint8;

class ALS_PROJECTHUNTER_API FEquipmentSlotResolver
{
public:
	static EEquipmentSlot DetermineEquipmentSlot(const UEquipmentManager& Manager, UItemInstance* Item);
	static bool CanEquipToSlot(const UEquipmentManager& Manager, UItemInstance* Item, EEquipmentSlot Slot);
	static bool TryEquipGroundPickupItem(UEquipmentManager& Manager, UItemInstance* Item, EEquipmentSlot& OutEquippedSlot, bool bSwapToBag);
	static EEquipmentSlot GetNextAvailableRingSlot(const UEquipmentManager& Manager);
	static bool IsRingSlot(EEquipmentSlot Slot);
};
