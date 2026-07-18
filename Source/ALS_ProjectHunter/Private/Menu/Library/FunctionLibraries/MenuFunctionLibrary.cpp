#include "Menu/Library/FunctionLibraries/MenuFunctionLibrary.h"

TArray<EEquipmentSlot> UMenuFunctionLibrary::GetDefaultEquipmentSlotOrder()
{
	return
	{
		EEquipmentSlot::ES_MainHand,
		EEquipmentSlot::ES_OffHand,
		EEquipmentSlot::ES_TwoHand,
		EEquipmentSlot::ES_Head,
		EEquipmentSlot::ES_Chest,
		EEquipmentSlot::ES_Hands,
		EEquipmentSlot::ES_Legs,
		EEquipmentSlot::ES_Feet,
		EEquipmentSlot::ES_Amulet,
		EEquipmentSlot::ES_Belt,
		EEquipmentSlot::ES_Ring1,
		EEquipmentSlot::ES_Ring2,
		EEquipmentSlot::ES_Ring3,
		EEquipmentSlot::ES_Ring4,
		EEquipmentSlot::ES_Ring5,
		EEquipmentSlot::ES_Ring6,
		EEquipmentSlot::ES_Ring7,
		EEquipmentSlot::ES_Ring8,
		EEquipmentSlot::ES_Ring9,
		EEquipmentSlot::ES_Ring10
	};
}

FText UMenuFunctionLibrary::GetEquipmentSlotDisplayName(const EEquipmentSlot EquipmentSlot)
{
	if (const UEnum* EquipmentSlotEnum = StaticEnum<EEquipmentSlot>())
	{
		return EquipmentSlotEnum->GetDisplayNameTextByValue(static_cast<int64>(EquipmentSlot));
	}

	return FText::FromString(TEXT("None"));
}

FEquipmentMenuSlotViewData UMenuFunctionLibrary::MakeEquipmentSlotViewData(EEquipmentSlot EquipmentSlot, UItemInstance* Item)
{
	FEquipmentMenuSlotViewData SlotData;
	SlotData.Slot = EquipmentSlot;
	SlotData.DisplayName = GetEquipmentSlotDisplayName(EquipmentSlot);
	SlotData.Item = Item;
	SlotData.bOccupied = Item != nullptr;
	return SlotData;
}

FEquipmentMenuInventorySlotViewData UMenuFunctionLibrary::MakeInventorySlotViewData(
	const int32 SlotIndex,
	UItemInstance* Item,
	const EEquipmentSlot SuggestedEquipmentSlot)
{
	FEquipmentMenuInventorySlotViewData SlotData;
	SlotData.SlotIndex = SlotIndex;
	SlotData.Item = Item;
	SlotData.bOccupied = Item != nullptr;
	SlotData.SuggestedEquipmentSlot = SuggestedEquipmentSlot;
	SlotData.bCanEquip = SuggestedEquipmentSlot != EEquipmentSlot::ES_None;
	return SlotData;
}
