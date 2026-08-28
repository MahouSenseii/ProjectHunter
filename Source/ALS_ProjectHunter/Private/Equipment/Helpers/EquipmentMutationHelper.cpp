#include "Equipment/Helpers/EquipmentMutationHelper.h"

#include "Core/Logging/ProjectHunterLogMacros.h"
#include "Equipment/Actors/EquippedItemRuntimeActor.h"
#include "Equipment/Components/EquipmentManager.h"
#include "Equipment/Components/EquipmentPresentationComponent.h"
#include "Equipment/Helpers/EquipmentHandSlotMutationHelper.h"
#include "Equipment/Helpers/EquipmentReplicationHelper.h"
#include "Equipment/Helpers/EquipmentSlotResolver.h"
#include "Equipment/Library/EquipmentLog.h"
#include "Equipment/Library/FunctionLibraries/EquipmentFunctionLibrary.h"
#include "Inventory/Components/InventoryManager.h"
#include "Item/ItemInstance.h"

UItemInstance* FEquipmentMutationHelper::EquipItem(UEquipmentManager& Manager, UItemInstance* Item, EEquipmentSlot Slot,
	bool bSwapToBag)
{
	if (!Item)
	{
		PH_LOG_WARNING(LogEquipmentManager, "EquipItem failed: Item was null.");
		return nullptr;
	}

	if (!Manager.GetOwner() || !Manager.GetOwner()->HasAuthority())
	{
		Manager.ServerEquipItem(Item, Slot, bSwapToBag);
		return nullptr;
	}

	return EquipItemInternal(Manager, Item, Slot, bSwapToBag, false);
}

UItemInstance* FEquipmentMutationHelper::UnequipItem(UEquipmentManager& Manager, EEquipmentSlot Slot, bool bMoveToBag)
{
	if (!Manager.GetOwner() || !Manager.GetOwner()->HasAuthority())
	{
		Manager.ServerUnequipItem(Slot, bMoveToBag);
		return nullptr;
	}

	UItemInstance* CurrentItem = Manager.GetEquippedItem(Slot);
	if (!CurrentItem)
	{
		UE_LOG(LogEquipmentManager, Verbose, TEXT("EquipmentManager::UnequipItem: Slot %d is already empty."),
			static_cast<int32>(Slot));
		return nullptr;
	}

	FEquipmentReplicationHelper::RemoveEquipment(Manager, Slot);

	if (bMoveToBag && Manager.InventoryManager && !Manager.InventoryManager->AddItem(CurrentItem))
	{
		PH_LOG_WARNING(LogEquipmentManager, "UnequipItem failed: Could not return Item=%s to inventory.",
			*GetNameSafe(CurrentItem));
	}

	Manager.OnEquipmentChanged.Broadcast(Slot, nullptr, CurrentItem);

	UE_LOG(LogEquipmentManager, Log, TEXT("EquipmentManager: Unequipped '%s' from slot %d."),
		*GetNameSafe(CurrentItem), static_cast<int32>(Slot));

	return CurrentItem;
}

void FEquipmentMutationHelper::UnequipAll(UEquipmentManager& Manager, bool bMoveToBag)
{
	if (!Manager.GetOwner() || !Manager.GetOwner()->HasAuthority())
	{
		PH_LOG_WARNING(LogEquipmentManager, "UnequipAll failed: Must be called on the server.");
		return;
	}

	TArray<EEquipmentSlot> Slots;
	Manager.EquippedItemsMap.GetKeys(Slots);

	for (const EEquipmentSlot Slot : Slots)
	{
		UnequipItem(Manager, Slot, bMoveToBag);
	}
}

AEquippedItemRuntimeActor* FEquipmentMutationHelper::GetActiveRuntimeItemActor(const UEquipmentManager& Manager,
	EEquipmentSlot Slot)
{
	if (Manager.EquipmentPresentation)
	{
		return Manager.EquipmentPresentation->GetActiveRuntimeItemActor(Slot);
	}

	return nullptr;
}

UItemInstance* FEquipmentMutationHelper::EquipItemInternal(UEquipmentManager& Manager, UItemInstance* Item,
	EEquipmentSlot Slot, bool bSwapToBag, bool bUseGroundPickupRules)
{
	if (!Item)
	{
		return nullptr;
	}

	FItemBase* BaseData = Item->GetBaseData();
	if (!BaseData)
	{
		PH_LOG_WARNING(LogEquipmentManager, "EquipItemInternal failed: Item=%s had no base data.",
			*GetNameSafe(Item));
		return nullptr;
	}

	if (!BaseData->IsEquippable())
	{
		PH_LOG_WARNING(LogEquipmentManager, "EquipItemInternal rejected Item=%s because it is not equippable.",
			*GetNameSafe(Item));
		return nullptr;
	}

	const FItemRequirementCheckResult RequirementResult = Manager.EvaluateItemRequirements(Item);
	if (!RequirementResult.bMeetsRequirements)
	{
		if (!RequirementResult.bStatsAvailable)
		{
			PH_LOG_WARNING(LogEquipmentManager,
				"EquipItemInternal rejected Item=%s because the owner's live stats were unavailable.",
				*GetNameSafe(Item));
		}
		else if (!RequirementResult.Failures.IsEmpty())
		{
			const FItemRequirementFailure& Failure = RequirementResult.Failures[0];
			PH_LOG_WARNING(LogEquipmentManager,
				"EquipItemInternal rejected Item=%s: Requirement=%d Current=%.2f Required=%.2f Missing=%.2f (%d total failures).",
				*GetNameSafe(Item),
				static_cast<int32>(Failure.RequirementType),
				Failure.CurrentValue,
				Failure.RequiredValue,
				Failure.MissingValue,
				RequirementResult.Failures.Num());
		}
		return nullptr;
	}

	if (Slot == EEquipmentSlot::ES_None && Manager.bAutoSlotSelection)
	{
		Slot = FEquipmentSlotResolver::DetermineEquipmentSlot(Manager, Item);
	}

	if (Slot == EEquipmentSlot::ES_None)
	{
		PH_LOG_WARNING(LogEquipmentManager, "EquipItemInternal failed: Could not determine a slot for Item=%s.",
			*GetNameSafe(Item));
		return nullptr;
	}

	// A two-handed weapon fills both hands, so aiming at either one stores it in
	// the shared ES_TwoHand entry that both hands read from.
	Slot = UEquipmentFunctionLibrary::ResolveEquipSlot(Item, Slot);

	const bool bCanEquip = bUseGroundPickupRules
		? UEquipmentFunctionLibrary::CanGroundPickupEquipToSlot(Item, Slot)
		: FEquipmentSlotResolver::CanEquipToSlot(Manager, Item, Slot);
	if (!bCanEquip)
	{
		PH_LOG_WARNING(LogEquipmentManager, "EquipItemInternal rejected Item=%s for Slot=%d.", *GetNameSafe(Item),
			static_cast<int32>(Slot));
		return nullptr;
	}

	if (Slot == EEquipmentSlot::ES_TwoHand && UEquipmentFunctionLibrary::IsTwoHanded(Item))
	{
		UItemInstance* DisplacedItem = FEquipmentHandSlotMutationHelper::EquipTwoHandedItem(Manager, Item, bSwapToBag);
		UE_LOG(LogEquipmentManager, Log, TEXT("EquipmentManager: Equipped two-handed '%s'."), *GetNameSafe(Item));
		return DisplacedItem;
	}

	UItemInstance* OldTwoHandItem = FEquipmentHandSlotMutationHelper::UnequipConflictingTwoHandedItem(Manager, Slot,
		bSwapToBag);
	UItemInstance* OldItem = Manager.GetEquippedItem(Slot);

	FEquipmentReplicationHelper::AddEquipment(Manager, Slot, Item);

	if (Manager.InventoryManager)
	{
		Manager.InventoryManager->RemoveItem(Item);
	}

	if (OldItem && bSwapToBag && Manager.InventoryManager && !Manager.InventoryManager->AddItem(OldItem))
	{
		PH_LOG_WARNING(LogEquipmentManager, "EquipItemInternal failed: Could not return displaced Item=%s to inventory.",
			*GetNameSafe(OldItem));
	}

	if (OldTwoHandItem)
	{
		Manager.OnEquipmentChanged.Broadcast(EEquipmentSlot::ES_TwoHand, nullptr, OldTwoHandItem);
	}
	Manager.OnEquipmentChanged.Broadcast(Slot, Item, OldItem);

	UE_LOG(LogEquipmentManager, Log, TEXT("EquipmentManager: Equipped '%s' to slot %d."),
		*GetNameSafe(Item), static_cast<int32>(Slot));

	return OldItem;
}
