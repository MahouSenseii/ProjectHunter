// Systems/Equipment/Library/EquipmentFunctionLibrary.h
// PH-1.1 — Equipment Function Library (skeleton)
//
// HELPER LAYER. Pure, stateless equipment rules. Anything that more than one
// gameplay system needs to ask about equipment slots, two-hand rules, ring
// rules, or item-to-slot compatibility belongs here, NOT inside
// UEquipmentManager.
//
// Roadmap tickets:
//   PH-1.1 add skeleton (this file)
//   PH-1.2 replace inline equipment rule logic in UEquipmentManager with calls into this library

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Systems/Equipment/Library/EquipmentEnums.h"
#include "EquipmentFunctionLibrary.generated.h"

class UItemInstance;
struct FItemBase;

/**
 * UEquipmentFunctionLibrary
 *
 * Stateless helpers for equipment rules. Keep these pure: no UWorld access,
 * no manager state, no replication side effects.
 */
UCLASS()
class ALS_PROJECTHUNTER_API UEquipmentFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Returns the canonical slot for an item, or ES_None if it cannot be equipped. */
	UFUNCTION(BlueprintPure, Category = "ProjectHunter|Equipment|Rules")
	static EEquipmentSlot DetermineSlotForItem(const UItemInstance* Item);

	/** True if Item is allowed in Slot under current rules (type, level, class, etc.). */
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

	/**
	 * Auto-slot resolution for rings. Given the two ring slots and which are
	 * already occupied, returns the slot the new ring should land in.
	 */
	UFUNCTION(BlueprintPure, Category = "ProjectHunter|Equipment|Rules")
	static EEquipmentSlot ResolveRingAutoSlot(bool bRing1Occupied, bool bRing2Occupied);

	/** Convenience: returns the off-hand slot if Slot is the main hand, else ES_None. */
	UFUNCTION(BlueprintPure, Category = "ProjectHunter|Equipment|Rules")
	static EEquipmentSlot GetConflictingSlot(EEquipmentSlot Slot, bool bTwoHanded);
};
