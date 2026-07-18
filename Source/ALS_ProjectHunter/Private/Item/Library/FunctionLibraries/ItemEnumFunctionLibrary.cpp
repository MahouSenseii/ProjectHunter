#include "Item/Library/FunctionLibraries/ItemEnumFunctionLibrary.h"

bool UItemEnumFunctionLibrary::IsValidItemType(const EItemType Type)
{
	return Type != EItemType::IT_None;
}

bool UItemEnumFunctionLibrary::IsValidItemSubType(const EItemSubType SubType)
{
	return SubType != EItemSubType::IST_None;
}

bool UItemEnumFunctionLibrary::IsWeaponItemSubType(const EItemSubType SubType)
{
	switch (SubType)
	{
	case EItemSubType::IST_Sword:
	case EItemSubType::IST_Katana:
	case EItemSubType::IST_Greatsword:
	case EItemSubType::IST_Dagger:
	case EItemSubType::IST_Axe:
	case EItemSubType::IST_Mace:
	case EItemSubType::IST_Spear:
	case EItemSubType::IST_Bow:
	case EItemSubType::IST_Crossbow:
	case EItemSubType::IST_Staff:
	case EItemSubType::IST_Wand:
	case EItemSubType::IST_Shield:
		return true;
	default:
		return false;
	}
}

bool UItemEnumFunctionLibrary::IsArmorItemSubType(const EItemSubType SubType)
{
	switch (SubType)
	{
	case EItemSubType::IST_Helmet:
	case EItemSubType::IST_Chest:
	case EItemSubType::IST_Gloves:
	case EItemSubType::IST_Boots:
	case EItemSubType::IST_Legs:
		return true;
	default:
		return false;
	}
}

bool UItemEnumFunctionLibrary::IsAccessoryItemSubType(const EItemSubType SubType)
{
	switch (SubType)
	{
	case EItemSubType::IST_Ring:
	case EItemSubType::IST_Amulet:
	case EItemSubType::IST_Belt:
		return true;
	default:
		return false;
	}
}

bool UItemEnumFunctionLibrary::IsConsumableItemSubType(const EItemSubType SubType)
{
	switch (SubType)
	{
	case EItemSubType::IST_Potion:
	case EItemSubType::IST_Scroll:
	case EItemSubType::IST_Food:
		return true;
	default:
		return false;
	}
}

bool UItemEnumFunctionLibrary::IsItemSubTypeAllowedForItemType(const EItemType ItemType, const EItemSubType SubType)
{
	if (SubType == EItemSubType::IST_None)
	{
		return true;
	}

	switch (ItemType)
	{
	case EItemType::IT_Weapon:
		return IsWeaponItemSubType(SubType);
	case EItemType::IT_Armor:
		return IsArmorItemSubType(SubType);
	case EItemType::IT_Accessory:
		return IsAccessoryItemSubType(SubType);
	case EItemType::IT_Consumable:
		return IsConsumableItemSubType(SubType);
	default:
		return false;
	}
}

FLinearColor UItemEnumFunctionLibrary::GetItemRarityColor(const EItemRarity Rarity)
{
	switch (Rarity)
	{
	case EItemRarity::IR_GradeF:      return FLinearColor(0.5f, 0.5f, 0.5f);
	case EItemRarity::IR_GradeE:      return FLinearColor::White;
	case EItemRarity::IR_GradeD:      return FLinearColor(0.3f, 0.9f, 0.3f);
	case EItemRarity::IR_GradeC:      return FLinearColor(0.4f, 0.6f, 1.0f);
	case EItemRarity::IR_GradeB:      return FLinearColor(0.7f, 0.3f, 0.9f);
	case EItemRarity::IR_GradeA:      return FLinearColor(1.0f, 0.7f, 0.0f);
	case EItemRarity::IR_GradeS:      return FLinearColor(1.0f, 0.3f, 0.0f);
	case EItemRarity::IR_GradeSS:     return FLinearColor(1.0f, 0.0f, 0.0f);
	case EItemRarity::IR_Unknown:     return FLinearColor(0.3f, 0.3f, 0.3f);
	case EItemRarity::IR_Corrupted:   return FLinearColor(0.2f, 0.0f, 0.2f);
	default:                          return FLinearColor::White;
	}
}

FText UItemEnumFunctionLibrary::GetItemRarityDisplayName(EItemRarity Rarity)
{
	switch (Rarity)
	{
		case EItemRarity::IR_GradeF:    return FText::FromString("Grade F (Common)");
		case EItemRarity::IR_GradeE:    return FText::FromString("Grade E (Uncommon)");
		case EItemRarity::IR_GradeD:    return FText::FromString("Grade D (Rare)");
		case EItemRarity::IR_GradeC:    return FText::FromString("Grade C (Elite)");
		case EItemRarity::IR_GradeB:    return FText::FromString("Grade B (Named)");
		case EItemRarity::IR_GradeA:    return FText::FromString("Grade A (Legendary)");
		case EItemRarity::IR_GradeS:    return FText::FromString("Grade S (Mythic)");
		case EItemRarity::IR_GradeSS:   return FText::FromString("Grade SS (EX-Rank)");
		case EItemRarity::IR_Unknown:   return FText::FromString("Unknown");
		case EItemRarity::IR_Corrupted: return FText::FromString("Corrupted");
		default: return FText::FromString("None");
	}
}

EDefenseType UItemEnumFunctionLibrary::DamageTypeToResistance(EDamageType DamageType)
{
	switch (DamageType)
	{
		case EDamageType::DT_Fire:       return EDefenseType::DFT_FireResistance;
		case EDamageType::DT_Ice:        return EDefenseType::DFT_IceResistance;
		case EDamageType::DT_Lightning:  return EDefenseType::DFT_LightningResistance;
		case EDamageType::DT_Light:      return EDefenseType::DFT_LightResistance;
		case EDamageType::DT_Corruption: return EDefenseType::DFT_CorruptionResistance;
		default: return EDefenseType::DFT_None;
	}
}

FText UItemEnumFunctionLibrary::GetItemTypeName(EItemType ItemType)
{
	return UEnum::GetDisplayValueAsText(ItemType);
}

FText UItemEnumFunctionLibrary::GetItemSubTypeName(EItemSubType SubType)
{
	return UEnum::GetDisplayValueAsText(SubType);
}

EAttachmentRule UItemEnumFunctionLibrary::ToEngineRule(const EPHAttachmentRule Rule)
{
	switch (Rule)
	{
	case EPHAttachmentRule::AR_KeepRelative: return EAttachmentRule::KeepRelative;
	case EPHAttachmentRule::AR_KeepWorld:    return EAttachmentRule::KeepWorld;
	case EPHAttachmentRule::AR_SnapToTarget: return EAttachmentRule::SnapToTarget;
	default: return EAttachmentRule::KeepRelative;
	}
}

int32 UItemEnumFunctionLibrary::GetRankPointsValue(const ERankPoints Points)
{
	switch (Points)
	{
	case ERankPoints::RP_Minus10: return -10;
	case ERankPoints::RP_Minus9:  return -9;
	case ERankPoints::RP_Minus8:  return -8;
	case ERankPoints::RP_Minus7:  return -7;
	case ERankPoints::RP_Minus6:  return -6;
	case ERankPoints::RP_Minus5:  return -5;
	case ERankPoints::RP_Minus4:  return -4;
	case ERankPoints::RP_Minus3:  return -3;
	case ERankPoints::RP_Minus2:  return -2;
	case ERankPoints::RP_Minus1:  return -1;
	case ERankPoints::RP_0:       return 0;
	case ERankPoints::RP_1:       return 1;
	case ERankPoints::RP_2:       return 2;
	case ERankPoints::RP_3:       return 3;
	case ERankPoints::RP_4:       return 4;
	case ERankPoints::RP_5:       return 5;
	case ERankPoints::RP_6:       return 6;
	case ERankPoints::RP_7:       return 7;
	case ERankPoints::RP_8:       return 8;
	case ERankPoints::RP_9:       return 9;
	case ERankPoints::RP_10:      return 10;
	default: return 0;
	}
}

int32 UItemEnumFunctionLibrary::GetAffixRarityWeight(const EAffixRarity Rarity)
{
	switch (Rarity)
	{
	case EAffixRarity::AR_Common:    return 125;
	case EAffixRarity::AR_Uncommon:  return 75;
	case EAffixRarity::AR_Rare:      return 35;
	case EAffixRarity::AR_VeryRare:  return 12;
	case EAffixRarity::AR_Unique:    return 3;
	case EAffixRarity::AR_Mythic:    return 1;
	default:                         return 100;
	}
}

FLinearColor UItemEnumFunctionLibrary::GetAffixTierColor(const EAffixColorTier Tier)
{
	switch (Tier)
	{
	case EAffixColorTier::ACT_Normal:    return FLinearColor::White;
	case EAffixColorTier::ACT_Uncommon:  return FLinearColor(0.3f, 0.9f, 0.3f);
	case EAffixColorTier::ACT_Rare:      return FLinearColor(0.4f, 0.4f, 1.0f);
	case EAffixColorTier::ACT_Elite:     return FLinearColor(0.7f, 0.3f, 0.9f);
	case EAffixColorTier::ACT_Legendary: return FLinearColor(1.0f, 0.85f, 0.0f);
	case EAffixColorTier::ACT_Mythic:    return FLinearColor(1.0f, 0.2f, 0.2f);
	case EAffixColorTier::ACT_Corrupted: return FLinearColor(0.2f, 0.0f, 0.2f);
	default:                             return FLinearColor::White;
	}
}

FString UItemEnumFunctionLibrary::GetModifyTypeSymbol(const EModifyType ModifyType)
{
	switch (ModifyType)
	{
	case EModifyType::MT_Add:            return TEXT("+");
	case EModifyType::MT_Multiply:       return TEXT("+% ");
	case EModifyType::MT_Override:       return TEXT("= ");
	case EModifyType::MT_More:           return TEXT("% More ");
	case EModifyType::MT_Increased:      return TEXT("% Increased ");
	case EModifyType::MT_Reduced:        return TEXT("% Reduced ");
	case EModifyType::MT_Less:           return TEXT("% Less ");
	case EModifyType::MT_ConvertTo:      return TEXT("% Converted to ");
	case EModifyType::MT_AddRange:       return TEXT("Adds ");
	case EModifyType::MT_MultiplyRange:  return TEXT("% Increased ");
	case EModifyType::MT_GrantSkill:     return TEXT("Grants ");
	case EModifyType::MT_SetRank:        return TEXT("Level ");
	default:                             return TEXT("");
	}
}
