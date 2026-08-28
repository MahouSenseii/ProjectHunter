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

	/** True if the item is two-handed and therefore fills both hand slots. */
	UFUNCTION(BlueprintPure, Category = "ProjectHunter|Equipment|Rules")
	static bool IsTwoHanded(const UItemInstance* Item);

	/** True for the main-hand and off-hand slots. */
	UFUNCTION(BlueprintPure, Category = "ProjectHunter|Equipment|Rules")
	static bool IsHandSlot(EEquipmentSlot Slot);

	/**
	 * Where an item requested for RequestedSlot is actually stored. A two-handed
	 * weapon fills both hands, so either hand resolves to ES_TwoHand.
	 */
	UFUNCTION(BlueprintPure, Category = "ProjectHunter|Equipment|Rules")
	static EEquipmentSlot ResolveEquipSlot(const UItemInstance* Item, EEquipmentSlot RequestedSlot);

	/**
	 * Which slot backs what Slot shows. A two-handed weapon occupies both hands,
	 * so while one is equipped either hand reads from ES_TwoHand.
	 */
	UFUNCTION(BlueprintPure, Category = "ProjectHunter|Equipment|Rules")
	static EEquipmentSlot ResolveOccupyingSlot(EEquipmentSlot Slot, bool bTwoHandSlotOccupied);

	UFUNCTION(BlueprintPure, Category = "ProjectHunter|Equipment|Rules")
	static EEquipmentSlot ResolveRingAutoSlot(bool bRing1Occupied, bool bRing2Occupied);

	UFUNCTION(BlueprintPure, Category = "ProjectHunter|Equipment|Rules")
	static EEquipmentSlot GetConflictingSlot(EEquipmentSlot Slot, bool bTwoHanded);
};
