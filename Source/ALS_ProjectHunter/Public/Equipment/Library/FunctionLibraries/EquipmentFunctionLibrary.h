#pragma once

#include "CoreMinimal.h"
#include "Equipment/Library/Enums/EquipmentEnums.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "EquipmentFunctionLibrary.generated.h"

class UItemInstance;

UCLASS()
class ALS_PROJECTHUNTER_API UEquipmentFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Returns the canonical slot for an item, or ES_None if it cannot be equipped. */
	UFUNCTION(BlueprintPure, Category = "ProjectHunter|Equipment|Rules")
	static EEquipmentSlot DetermineSlotForItem(const UItemInstance* Item);

	/** True if Item is allowed in Slot under current rules. */
	UFUNCTION(BlueprintPure, Category = "ProjectHunter|Equipment|Rules")
	static bool IsItemCompatibleWithSlot(const UItemInstance* Item, EEquipmentSlot Slot);

	/** True if the item is a one-handed weapon and may occupy either hand. */
	UFUNCTION(BlueprintPure, Category = "ProjectHunter|Equipment|Rules")
	static bool IsOneHandedWeapon(const UItemInstance* Item);

	/**
	 * Ground-pickup equip compatibility.
	 * Allows one-handed weapons to fall back into the off-hand without widening
	 * the default manual-equip rules.
	 */
	UFUNCTION(BlueprintPure, Category = "ProjectHunter|Equipment|Rules")
	static bool CanGroundPickupEquipToSlot(const UItemInstance* Item, EEquipmentSlot Slot);

	/** True if the item is two-handed and therefore blocks the off-hand slot. */
	UFUNCTION(BlueprintPure, Category = "ProjectHunter|Equipment|Rules")
	static bool IsTwoHanded(const UItemInstance* Item);

	UFUNCTION(BlueprintPure, Category = "ProjectHunter|Equipment|Rules")
	static EEquipmentSlot ResolveRingAutoSlot(bool bRing1Occupied, bool bRing2Occupied);

	UFUNCTION(BlueprintPure, Category = "ProjectHunter|Equipment|Rules")
	static EEquipmentSlot GetConflictingSlot(EEquipmentSlot Slot, bool bTwoHanded);
};
