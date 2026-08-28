#include "UI/Menu/Library/FunctionLibraries/MenuFunctionLibrary.h"
#include "UI/Library/PHUIStyle.h"

TArray<EEquipmentSlot> UMenuFunctionLibrary::GetDefaultEquipmentSlotOrder()
{
	// No two-hand slot: a two-handed weapon is stored in ES_TwoHand but the menu
	// shows it filling both the main hand and the off hand.
	return
	{
		EEquipmentSlot::ES_MainHand,
		EEquipmentSlot::ES_OffHand,
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

FLinearColor UMenuFunctionLibrary::GetItemGradeColor(const EItemRarity Grade)
{
	switch (Grade)
	{
	case EItemRarity::IR_GradeF:    return PHUIStyle::GradeF;
	case EItemRarity::IR_GradeE:    return PHUIStyle::GradeE;
	case EItemRarity::IR_GradeD:    return PHUIStyle::GradeD;
	case EItemRarity::IR_GradeC:    return PHUIStyle::GradeC;
	case EItemRarity::IR_GradeB:    return PHUIStyle::GradeB;
	case EItemRarity::IR_GradeA:    return PHUIStyle::GradeA;
	case EItemRarity::IR_GradeS:    return PHUIStyle::GradeS;
	case EItemRarity::IR_GradeSS:   return PHUIStyle::GradeSS;
	case EItemRarity::IR_Unknown:   return PHUIStyle::GradeUnknown;
	case EItemRarity::IR_Corrupted: return PHUIStyle::GradeCorrupted;
	default:                        return PHUIStyle::TextPrimary;
	}
}

FText UMenuFunctionLibrary::GetItemGradeGlyph(const EItemRarity Grade)
{
	switch (Grade)
	{
	case EItemRarity::IR_GradeF:    return NSLOCTEXT("PHMenu", "GradeF", "[F]");
	case EItemRarity::IR_GradeE:    return NSLOCTEXT("PHMenu", "GradeE", "[E]");
	case EItemRarity::IR_GradeD:    return NSLOCTEXT("PHMenu", "GradeD", "[D]");
	case EItemRarity::IR_GradeC:    return NSLOCTEXT("PHMenu", "GradeC", "[C]");
	case EItemRarity::IR_GradeB:    return NSLOCTEXT("PHMenu", "GradeB", "[B]");
	case EItemRarity::IR_GradeA:    return NSLOCTEXT("PHMenu", "GradeA", "[A]");
	case EItemRarity::IR_GradeS:    return NSLOCTEXT("PHMenu", "GradeS", "[S]");
	case EItemRarity::IR_GradeSS:   return NSLOCTEXT("PHMenu", "GradeSS", "[SS]");
	// The system will not resolve either of these, and says so.
	case EItemRarity::IR_Unknown:   return NSLOCTEXT("PHMenu", "GradeUnknown", "[?]");
	case EItemRarity::IR_Corrupted: return NSLOCTEXT("PHMenu", "GradeCorrupted", "[X]");
	default:                        return FText::GetEmpty();
	}
}
