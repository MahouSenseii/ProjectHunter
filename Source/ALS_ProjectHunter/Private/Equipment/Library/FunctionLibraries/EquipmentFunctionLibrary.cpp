#include "Equipment/Library/FunctionLibraries/EquipmentFunctionLibrary.h"

#include "Item/ItemInstance.h"
#include "Item/Library/FunctionLibraries/ItemEnumFunctionLibrary.h"

namespace EquipmentFunctionLibraryPrivate
{
	bool IsRingSlot(const EEquipmentSlot Slot)
	{
		return Slot >= EEquipmentSlot::ES_Ring1 && Slot <= EEquipmentSlot::ES_Ring10;
	}
}

EEquipmentSlot UEquipmentFunctionLibrary::DetermineSlotForItem(const UItemInstance* Item)
{
	if (!Item)
	{
		return EEquipmentSlot::ES_None;
	}

	const FItemBase* BaseData = Item->GetBaseData();
	if (!BaseData || !BaseData->IsEquippable())
	{
		return EEquipmentSlot::ES_None;
	}

	if (!UItemEnumFunctionLibrary::IsItemSubTypeAllowedForItemType(BaseData->ItemType, BaseData->ItemSubType))
	{
		return EEquipmentSlot::ES_None;
	}

	switch (BaseData->ItemSubType)
	{
	case EItemSubType::IST_Helmet:
		return EEquipmentSlot::ES_Head;
	case EItemSubType::IST_Chest:
		return EEquipmentSlot::ES_Chest;
	case EItemSubType::IST_Gloves:
		return EEquipmentSlot::ES_Hands;
	case EItemSubType::IST_Legs:
		return EEquipmentSlot::ES_Legs;
	case EItemSubType::IST_Boots:
		return EEquipmentSlot::ES_Feet;
	case EItemSubType::IST_Belt:
		return EEquipmentSlot::ES_Belt;
	case EItemSubType::IST_Amulet:
		return EEquipmentSlot::ES_Amulet;
	case EItemSubType::IST_Ring:
		return EEquipmentSlot::ES_Ring1;
	case EItemSubType::IST_Shield:
		return EEquipmentSlot::ES_OffHand;
	case EItemSubType::IST_Bow:
	case EItemSubType::IST_Crossbow:
	case EItemSubType::IST_Staff:
	case EItemSubType::IST_Greatsword:
	case EItemSubType::IST_Spear:
		return EEquipmentSlot::ES_TwoHand;
	case EItemSubType::IST_Sword:
	case EItemSubType::IST_Katana:
	case EItemSubType::IST_Axe:
	case EItemSubType::IST_Mace:
	case EItemSubType::IST_Dagger:
	case EItemSubType::IST_Wand:
		return IsTwoHanded(Item) ? EEquipmentSlot::ES_TwoHand : EEquipmentSlot::ES_MainHand;
	default:
		return EEquipmentSlot::ES_None;
	}
}

bool UEquipmentFunctionLibrary::IsItemCompatibleWithSlot(const UItemInstance* Item, EEquipmentSlot Slot)
{
	if (!Item || Slot == EEquipmentSlot::ES_None)
	{
		return false;
	}

	const EEquipmentSlot CanonicalSlot = DetermineSlotForItem(Item);
	if (CanonicalSlot == EEquipmentSlot::ES_None)
	{
		return false;
	}

	if (EquipmentFunctionLibraryPrivate::IsRingSlot(CanonicalSlot) &&
		EquipmentFunctionLibraryPrivate::IsRingSlot(Slot))
	{
		return true;
	}

	return CanonicalSlot == Slot;
}

bool UEquipmentFunctionLibrary::IsOneHandedWeapon(const UItemInstance* Item)
{
	if (!Item)
	{
		return false;
	}

	const FItemBase* BaseData = Item->GetBaseData();
	return BaseData != nullptr
		&& BaseData->IsWeapon()
		&& !IsTwoHanded(Item);
}

bool UEquipmentFunctionLibrary::CanGroundPickupEquipToSlot(const UItemInstance* Item, EEquipmentSlot Slot)
{
	if (IsItemCompatibleWithSlot(Item, Slot))
	{
		return true;
	}

	return Slot == EEquipmentSlot::ES_OffHand && IsOneHandedWeapon(Item);
}

bool UEquipmentFunctionLibrary::IsTwoHanded(const UItemInstance* Item)
{
	if (!Item)
	{
		return false;
	}

	if (Item->bIsTwoHanded())
	{
		return true;
	}

	const FItemBase* BaseData = Item->GetBaseData();
	if (!BaseData)
	{
		return false;
	}

	if (!BaseData->IsWeapon())
	{
		return false;
	}

	switch (BaseData->ItemSubType)
	{
	case EItemSubType::IST_Bow:
	case EItemSubType::IST_Crossbow:
	case EItemSubType::IST_Staff:
	case EItemSubType::IST_Greatsword:
	case EItemSubType::IST_Spear:
		return true;
	default:
		return false;
	}
}

bool UEquipmentFunctionLibrary::IsHandSlot(EEquipmentSlot Slot)
{
	return Slot == EEquipmentSlot::ES_MainHand || Slot == EEquipmentSlot::ES_OffHand;
}

EEquipmentSlot UEquipmentFunctionLibrary::ResolveEquipSlot(const UItemInstance* Item, EEquipmentSlot RequestedSlot)
{
	// A two-handed weapon aimed at either hand fills both, and both hands are
	// backed by the single ES_TwoHand entry.
	if (IsHandSlot(RequestedSlot) && IsTwoHanded(Item))
	{
		return EEquipmentSlot::ES_TwoHand;
	}

	return RequestedSlot;
}

EEquipmentSlot UEquipmentFunctionLibrary::ResolveOccupyingSlot(EEquipmentSlot Slot, bool bTwoHandSlotOccupied)
{
	if (bTwoHandSlotOccupied && IsHandSlot(Slot))
	{
		return EEquipmentSlot::ES_TwoHand;
	}

	return Slot;
}

EEquipmentSlot UEquipmentFunctionLibrary::ResolveRingAutoSlot(bool bRing1Occupied, bool bRing2Occupied)
{
	if (!bRing1Occupied)
	{
		return EEquipmentSlot::ES_Ring1;
	}
	if (!bRing2Occupied)
	{
		return EEquipmentSlot::ES_Ring2;
	}
	return EEquipmentSlot::ES_None;
}

EEquipmentSlot UEquipmentFunctionLibrary::GetConflictingSlot(EEquipmentSlot Slot, bool bTwoHanded)
{
	if (!bTwoHanded)
	{
		return EEquipmentSlot::ES_None;
	}

	switch (Slot)
	{
	case EEquipmentSlot::ES_MainHand:
	case EEquipmentSlot::ES_TwoHand:
		return EEquipmentSlot::ES_OffHand;
	case EEquipmentSlot::ES_OffHand:
		return EEquipmentSlot::ES_TwoHand;
	default:
		return EEquipmentSlot::ES_None;
	}
}
