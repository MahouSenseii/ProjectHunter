#pragma once

#include "CoreMinimal.h"

class AEquippedItemRuntimeActor;
class UEquipmentManager;
class UItemInstance;
enum class EEquipmentSlot : uint8;

class ALS_PROJECTHUNTER_API FEquipmentMutationHelper
{
public:
	static UItemInstance* EquipItem(UEquipmentManager& Manager, UItemInstance* Item, EEquipmentSlot Slot, bool bSwapToBag);
	static UItemInstance* UnequipItem(UEquipmentManager& Manager, EEquipmentSlot Slot, bool bMoveToBag);
	static void UnequipAll(UEquipmentManager& Manager, bool bMoveToBag);
	static AEquippedItemRuntimeActor* GetActiveRuntimeItemActor(const UEquipmentManager& Manager, EEquipmentSlot Slot);
	static UItemInstance* EquipItemInternal(UEquipmentManager& Manager, UItemInstance* Item, EEquipmentSlot Slot, bool bSwapToBag, bool bUseGroundPickupRules = false);
	static bool HandleTwoHandedWeapon(UEquipmentManager& Manager, UItemInstance* Item, bool bSwapToBag, UItemInstance*& OutOldMainHand, UItemInstance*& OutOldOffHand, UItemInstance*& OutOldTwoHand);
};
