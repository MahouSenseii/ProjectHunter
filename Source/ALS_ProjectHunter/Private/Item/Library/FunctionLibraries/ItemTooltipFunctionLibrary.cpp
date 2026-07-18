#include "Item/Library/FunctionLibraries/ItemTooltipFunctionLibrary.h"

#include "Item/ItemInstance.h"
#include "Item/Library/FunctionLibraries/ItemEnumFunctionLibrary.h"
#include "Item/Library/FunctionLibraries/ItemTooltipSectionFunctionLibrary.h"
#include "Item/Library/Structs/ItemStructs.h"

bool UItemTooltipFunctionLibrary::BuildItemTooltipData(UItemInstance* Item, FItemTooltipData& OutTooltipData)
{
	OutTooltipData = FItemTooltipData();

	if (!Item)
	{
		return false;
	}

	FItemBase* Base = Item->GetBaseData();
	if (!Base)
	{
		return false;
	}

	OutTooltipData.bHasItem = true;
	OutTooltipData.DisplayName = Item->GetDisplayName();
	OutTooltipData.BaseItemName = Item->GetBaseItemName();
	OutTooltipData.RarityName = UItemEnumFunctionLibrary::GetItemRarityDisplayName(Item->Rarity);
	OutTooltipData.ItemTypeName = UItemEnumFunctionLibrary::GetItemTypeName(Base->ItemType);
	OutTooltipData.ItemSubTypeName = UItemEnumFunctionLibrary::GetItemSubTypeName(Base->ItemSubType);
	OutTooltipData.Rarity = Item->Rarity;
	OutTooltipData.RarityColor = Item->GetRarityColor();
	OutTooltipData.BorderColor = OutTooltipData.RarityColor;
	OutTooltipData.HeaderColor = OutTooltipData.RarityColor;
	OutTooltipData.IconMaterial = Item->GetInventoryIcon();
	OutTooltipData.ItemLevel = Item->ItemLevel;
	OutTooltipData.Quantity = Item->Quantity;
	OutTooltipData.ItemValue = Item->GetCalculatedValue();
	OutTooltipData.TotalWeight = Item->GetTotalWeight();
	OutTooltipData.bIdentified = Item->IsIdentified();
	OutTooltipData.bStackable = Item->IsStackable();
	OutTooltipData.bCorrupted = Item->bHasCorruptedAffixes || Item->Rarity == EItemRarity::IR_Corrupted;

	UItemTooltipSectionFunctionLibrary::AddDetailsSection(OutTooltipData, Item);

	if (Base->ItemType == EItemType::IT_Weapon)
	{
		UItemTooltipSectionFunctionLibrary::AddWeaponStatsSection(OutTooltipData, Base->WeaponStats);
	}
	else if (Base->ItemType == EItemType::IT_Armor)
	{
		UItemTooltipSectionFunctionLibrary::AddArmorStatsSection(OutTooltipData, Base->ArmorStats);
	}

	if (Item->IsEquipment())
	{
		UItemTooltipSectionFunctionLibrary::AddRequirementsSection(OutTooltipData, Base->StatRequirements);
		UItemTooltipSectionFunctionLibrary::AddDurabilitySection(OutTooltipData, Item->Durability);
		UItemTooltipSectionFunctionLibrary::AddRunesSection(OutTooltipData, Item, *Base);
	}

	if (Base->ItemType == EItemType::IT_Consumable)
	{
		UItemTooltipSectionFunctionLibrary::AddConsumableSection(OutTooltipData, Item, Base->ConsumableData);
	}

	if (Item->IsEquipment())
	{
		const TArray<FPHAttributeData>& Implicits = Item->Stats.Implicits.Num() > 0 ? Item->Stats.Implicits : Base->ImplicitMods;
		UItemTooltipSectionFunctionLibrary::AddAffixSection(OutTooltipData, EItemTooltipSectionType::Implicits, FText::FromString(TEXT("Implicit")), Implicits);
		UItemTooltipSectionFunctionLibrary::AddAffixSection(OutTooltipData, EItemTooltipSectionType::Prefixes, FText::FromString(TEXT("Prefixes")), Item->Stats.Prefixes);
		UItemTooltipSectionFunctionLibrary::AddAffixSection(OutTooltipData, EItemTooltipSectionType::Suffixes, FText::FromString(TEXT("Suffixes")), Item->Stats.Suffixes);
		UItemTooltipSectionFunctionLibrary::AddAffixSection(OutTooltipData, EItemTooltipSectionType::Crafted, FText::FromString(TEXT("Crafted")), Item->Stats.Crafted);
		UItemTooltipSectionFunctionLibrary::AddAffixSection(OutTooltipData, EItemTooltipSectionType::Enchants, FText::FromString(TEXT("Enchants")), Item->Stats.Enchants);

		if (Base->bIsUnique || Item->Rarity == EItemRarity::IR_GradeSS)
		{
			UItemTooltipSectionFunctionLibrary::AddAffixSection(OutTooltipData, EItemTooltipSectionType::Unique, FText::FromString(TEXT("Unique")), Base->UniqueAffixes);
		}
	}

	UItemTooltipSectionFunctionLibrary::AddDescriptionSection(OutTooltipData, *Base);

	return true;
}

FItemTooltipData UItemTooltipFunctionLibrary::GetItemTooltipData(UItemInstance* Item)
{
	FItemTooltipData TooltipData;
	BuildItemTooltipData(Item, TooltipData);
	return TooltipData;
}
