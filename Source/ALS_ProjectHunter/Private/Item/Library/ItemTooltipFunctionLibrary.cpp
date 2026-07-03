#include "Item/Library/ItemTooltipFunctionLibrary.h"

#include "Item/ItemInstance.h"
#include "Item/Library/ItemFunctionLibrary.h"
#include "Item/Library/ItemStructs.h"

namespace ItemTooltip
{
	const FLinearColor MutedText(0.72f, 0.72f, 0.72f, 1.0f);
	const FLinearColor StatText(0.78f, 0.90f, 1.0f, 1.0f);
	const FLinearColor AffixText(0.72f, 0.88f, 1.0f, 1.0f);
	const FLinearColor PositiveText(0.42f, 0.95f, 0.48f, 1.0f);
	const FLinearColor NegativeText(1.0f, 0.28f, 0.25f, 1.0f);
	const FLinearColor WarningText(1.0f, 0.78f, 0.28f, 1.0f);
	const FLinearColor CorruptedText(0.78f, 0.20f, 1.0f, 1.0f);
	const FLinearColor DescriptionText(0.82f, 0.82f, 0.82f, 1.0f);

	FString FormatNumber(const float Value, const int32 MaxDecimals = 1)
	{
		if (FMath::IsNearlyEqual(Value, FMath::RoundToFloat(Value), 0.01f))
		{
			return FString::Printf(TEXT("%d"), FMath::RoundToInt(Value));
		}

		switch (FMath::Clamp(MaxDecimals, 0, 3))
		{
		case 0:
			return FString::Printf(TEXT("%.0f"), Value);
		case 2:
			return FString::Printf(TEXT("%.2f"), Value);
		case 3:
			return FString::Printf(TEXT("%.3f"), Value);
		case 1:
		default:
			return FString::Printf(TEXT("%.1f"), Value);
		}
	}

	FText FormatNumberText(const float Value, const int32 MaxDecimals = 1)
	{
		return FText::FromString(FormatNumber(Value, MaxDecimals));
	}

	FText FormatRangeText(const float MinValue, const float MaxValue)
	{
		return FText::FromString(FString::Printf(TEXT("%s-%s"), *FormatNumber(MinValue), *FormatNumber(MaxValue)));
	}

	FText FormatPercentText(const float Value)
	{
		return FText::FromString(FString::Printf(TEXT("%s%%"), *FormatNumber(Value)));
	}

	FText FormatWeightText(const float Value)
	{
		return FText::FromString(FormatNumber(Value, 2));
	}

	FItemTooltipLine MakeLine(
		const FText& Label,
		const FText& Value,
		const FLinearColor& Color,
		const EItemTooltipLineStyle Style,
		const bool bUseValueColumn = true,
		const bool bEmphasized = false)
	{
		FItemTooltipLine Line;
		Line.Label = Label;
		Line.Value = Value;
		Line.TextColor = Color;
		Line.Style = Style;
		Line.bUseValueColumn = bUseValueColumn;
		Line.bEmphasized = bEmphasized;
		return Line;
	}

	FItemTooltipLine MakeTextLine(
		const FText& Text,
		const FLinearColor& Color,
		const EItemTooltipLineStyle Style,
		const bool bEmphasized = false)
	{
		return MakeLine(Text, FText::GetEmpty(), Color, Style, false, bEmphasized);
	}

	void AddValueLine(
		TArray<FItemTooltipLine>& Lines,
		const TCHAR* Label,
		const FText& Value,
		const FLinearColor& Color = MutedText,
		const EItemTooltipLineStyle Style = EItemTooltipLineStyle::Property)
	{
		if (!Value.IsEmpty())
		{
			Lines.Add(MakeLine(FText::FromString(Label), Value, Color, Style));
		}
	}

	void AddNonZeroLine(
		TArray<FItemTooltipLine>& Lines,
		const TCHAR* Label,
		const float Value,
		const bool bPercent = false,
		const FLinearColor& Color = StatText)
	{
		if (FMath::IsNearlyZero(Value))
		{
			return;
		}

		AddValueLine(Lines, Label, bPercent ? FormatPercentText(Value) : FormatNumberText(Value), Color, EItemTooltipLineStyle::Stat);
	}

	void AddPositiveLine(
		TArray<FItemTooltipLine>& Lines,
		const TCHAR* Label,
		const int32 Value,
		const FLinearColor& Color = MutedText)
	{
		if (Value > 0)
		{
			AddValueLine(Lines, Label, FText::AsNumber(Value), Color);
		}
	}

	void AddSectionIfAny(FItemTooltipData& TooltipData, FItemTooltipSection& Section)
	{
		if (Section.HasDisplayableLines())
		{
			TooltipData.Sections.Add(MoveTemp(Section));
		}
	}

	FItemTooltipSection MakeSection(
		const EItemTooltipSectionType Type,
		const FText& Heading,
		const bool bShowHeading = true)
	{
		FItemTooltipSection Section;
		Section.SectionType = Type;
		Section.Heading = Heading;
		Section.bShowHeading = bShowHeading;
		return Section;
	}

	bool HasDisplayableAffixText(const FPHAttributeData& Affix)
	{
		return Affix.bIsIdentified
			&& (!Affix.AttributeName.IsNone()
				|| !Affix.DisplayText.IsEmpty()
				|| !FMath::IsNearlyZero(Affix.RolledStatValue)
				|| !FMath::IsNearlyZero(Affix.MinValue)
				|| !FMath::IsNearlyZero(Affix.MaxValue));
	}

	FLinearColor GetAffixColor(const FPHAttributeData& Affix)
	{
		if (Affix.IsCorruptedAffix() || Affix.GetRankPointValue() < 0)
		{
			return CorruptedText;
		}

		return Affix.RolledStatValue < 0.0f ? NegativeText : AffixText;
	}

	void AddAffixLines(FItemTooltipSection& Section, const TArray<FPHAttributeData>& Affixes)
	{
		for (const FPHAttributeData& Affix : Affixes)
		{
			if (!HasDisplayableAffixText(Affix))
			{
				continue;
			}

			const FString AffixTextString = UItemFunctionLibrary::FormatAffixText(Affix);
			if (AffixTextString.IsEmpty())
			{
				continue;
			}

			const bool bCorrupted = Affix.IsCorruptedAffix() || Affix.GetRankPointValue() < 0;
			Section.Lines.Add(MakeTextLine(
				FText::FromString(AffixTextString),
				GetAffixColor(Affix),
				bCorrupted ? EItemTooltipLineStyle::Corrupted : EItemTooltipLineStyle::Affix,
				bCorrupted));
		}
	}

	void AddAffixSection(
		FItemTooltipData& TooltipData,
		const EItemTooltipSectionType Type,
		const FText& Heading,
		const TArray<FPHAttributeData>& Affixes)
	{
		FItemTooltipSection Section = MakeSection(Type, Heading);
		AddAffixLines(Section, Affixes);
		AddSectionIfAny(TooltipData, Section);
	}

	void AddWeaponStats(FItemTooltipData& TooltipData, const FBaseWeaponStats& Stats)
	{
		FItemTooltipSection Section = MakeSection(EItemTooltipSectionType::BaseStats, FText::FromString(TEXT("Weapon Stats")));

		if (!FMath::IsNearlyZero(Stats.MinPhysicalDamage) || !FMath::IsNearlyZero(Stats.MaxPhysicalDamage))
		{
			AddValueLine(Section.Lines, TEXT("Physical Damage"), FormatRangeText(Stats.MinPhysicalDamage, Stats.MaxPhysicalDamage), StatText, EItemTooltipLineStyle::Stat);
		}

		if (!FMath::IsNearlyZero(Stats.MinFireDamage) || !FMath::IsNearlyZero(Stats.MaxFireDamage))
		{
			AddValueLine(Section.Lines, TEXT("Fire Damage"), FormatRangeText(Stats.MinFireDamage, Stats.MaxFireDamage), StatText, EItemTooltipLineStyle::Stat);
		}

		if (!FMath::IsNearlyZero(Stats.MinIceDamage) || !FMath::IsNearlyZero(Stats.MaxIceDamage))
		{
			AddValueLine(Section.Lines, TEXT("Ice Damage"), FormatRangeText(Stats.MinIceDamage, Stats.MaxIceDamage), StatText, EItemTooltipLineStyle::Stat);
		}

		if (!FMath::IsNearlyZero(Stats.MinLightningDamage) || !FMath::IsNearlyZero(Stats.MaxLightningDamage))
		{
			AddValueLine(Section.Lines, TEXT("Lightning Damage"), FormatRangeText(Stats.MinLightningDamage, Stats.MaxLightningDamage), StatText, EItemTooltipLineStyle::Stat);
		}

		if (!FMath::IsNearlyZero(Stats.MinLightDamage) || !FMath::IsNearlyZero(Stats.MaxLightDamage))
		{
			AddValueLine(Section.Lines, TEXT("Light Damage"), FormatRangeText(Stats.MinLightDamage, Stats.MaxLightDamage), StatText, EItemTooltipLineStyle::Stat);
		}

		if (!FMath::IsNearlyZero(Stats.MinCorruptionDamage) || !FMath::IsNearlyZero(Stats.MaxCorruptionDamage))
		{
			AddValueLine(Section.Lines, TEXT("Corruption Damage"), FormatRangeText(Stats.MinCorruptionDamage, Stats.MaxCorruptionDamage), CorruptedText, EItemTooltipLineStyle::Corrupted);
		}

		if (Stats.AttackSpeed > 0.0f)
		{
			AddValueLine(Section.Lines, TEXT("Attack Speed"), FormatNumberText(Stats.AttackSpeed, 2), StatText, EItemTooltipLineStyle::Stat);
		}

		if (Stats.CriticalStrikeChance > 0.0f)
		{
			AddValueLine(Section.Lines, TEXT("Critical Strike Chance"), FormatPercentText(Stats.CriticalStrikeChance), StatText, EItemTooltipLineStyle::Stat);
		}

		if (Stats.Range > 0.0f)
		{
			AddValueLine(Section.Lines, TEXT("Range"), FormatNumberText(Stats.Range, 1), StatText, EItemTooltipLineStyle::Stat);
		}

		AddSectionIfAny(TooltipData, Section);
	}

	void AddArmorStats(FItemTooltipData& TooltipData, const FBaseArmorStats& Stats)
	{
		FItemTooltipSection Section = MakeSection(EItemTooltipSectionType::BaseStats, FText::FromString(TEXT("Armor Stats")));

		AddNonZeroLine(Section.Lines, TEXT("Armor"), Stats.Armor);
		AddNonZeroLine(Section.Lines, TEXT("Fire Resistance"), Stats.FireResistance, true);
		AddNonZeroLine(Section.Lines, TEXT("Ice Resistance"), Stats.IceResistance, true);
		AddNonZeroLine(Section.Lines, TEXT("Lightning Resistance"), Stats.LightningResistance, true);
		AddNonZeroLine(Section.Lines, TEXT("Light Resistance"), Stats.LightResistance, true);
		AddNonZeroLine(Section.Lines, TEXT("Corruption Resistance"), Stats.CorruptionResistance, true, CorruptedText);

		AddSectionIfAny(TooltipData, Section);
	}

	void AddRequirements(FItemTooltipData& TooltipData, const FItemStatRequirement& Requirements)
	{
		FItemTooltipSection Section = MakeSection(EItemTooltipSectionType::Requirements, FText::FromString(TEXT("Requirements")));

		AddPositiveLine(Section.Lines, TEXT("Level"), Requirements.RequiredLevel > 1 ? Requirements.RequiredLevel : 0);
		AddPositiveLine(Section.Lines, TEXT("Strength"), Requirements.RequiredStrength);
		AddPositiveLine(Section.Lines, TEXT("Dexterity"), Requirements.RequiredDexterity);
		AddPositiveLine(Section.Lines, TEXT("Intelligence"), Requirements.RequiredIntelligence);
		AddPositiveLine(Section.Lines, TEXT("Endurance"), Requirements.RequiredEndurance);
		AddPositiveLine(Section.Lines, TEXT("Affliction"), Requirements.RequiredAffliction);
		AddPositiveLine(Section.Lines, TEXT("Luck"), Requirements.RequiredLuck);
		AddPositiveLine(Section.Lines, TEXT("Covenant"), Requirements.RequiredCovenant);

		AddSectionIfAny(TooltipData, Section);
	}

	void AddDurability(FItemTooltipData& TooltipData, const FItemDurability& Durability)
	{
		if (Durability.MaxDurability <= 0.0f)
		{
			return;
		}

		FItemTooltipSection Section = MakeSection(EItemTooltipSectionType::Durability, FText::FromString(TEXT("Durability")));
		AddValueLine(
			Section.Lines,
			TEXT("Durability"),
			FText::FromString(FString::Printf(TEXT("%s / %s"), *FormatNumber(Durability.CurrentDurability), *FormatNumber(Durability.MaxDurability))),
			MutedText);

		AddSectionIfAny(TooltipData, Section);
	}

	void AddRunes(FItemTooltipData& TooltipData, const UItemInstance* Item, const FItemBase& Base)
	{
		FItemTooltipSection Section = MakeSection(EItemTooltipSectionType::Runes, FText::FromString(TEXT("Runes")));

		const int32 SocketCount = Item->RuneCraftingData.GetSocketCount();
		const int32 MaxSockets = FMath::Max(Base.MaxRuneSockets, SocketCount);
		if (MaxSockets > 0)
		{
			AddValueLine(
				Section.Lines,
				TEXT("Rune Sockets"),
				FText::FromString(FString::Printf(TEXT("%d / %d"), Item->RuneCraftingData.GetSocketedRuneCount(), MaxSockets)),
				MutedText);
		}

		if (Item->RuneCraftingData.EnhancementLevel > 0)
		{
			AddValueLine(
				Section.Lines,
				TEXT("Enhancement"),
				FText::FromString(FString::Printf(TEXT("+%d / +%d"), Item->RuneCraftingData.EnhancementLevel, FMath::Max(Base.MaxEnhancementLevel, Item->RuneCraftingData.MaxEnhancementLevel))),
				PositiveText);
		}

		if (Item->Quality > 0)
		{
			AddValueLine(
				Section.Lines,
				TEXT("Reinforcement"),
				FText::FromString(FString::Printf(TEXT("+%d"), Item->Quality)),
				PositiveText);
		}

		AddSectionIfAny(TooltipData, Section);
	}

	void AddConsumable(FItemTooltipData& TooltipData, const UItemInstance* Item, const FConsumableData& ConsumableData)
	{
		FItemTooltipSection Section = MakeSection(EItemTooltipSectionType::Consumable, FText::FromString(TEXT("Consumable")));

		if (ConsumableData.MaxUses > 1)
		{
			AddValueLine(
				Section.Lines,
				TEXT("Uses"),
				FText::FromString(FString::Printf(TEXT("%d / %d"), Item->RemainingUses, ConsumableData.MaxUses)),
				MutedText);
		}

		if (ConsumableData.Cooldown > 0.0f)
		{
			AddValueLine(Section.Lines, TEXT("Cooldown"), FText::FromString(FString::Printf(TEXT("%ss"), *FormatNumber(ConsumableData.Cooldown, 1))), MutedText);
		}

		AddSectionIfAny(TooltipData, Section);
	}

	void AddDetails(FItemTooltipData& TooltipData, const UItemInstance* Item)
	{
		FItemTooltipSection Section = MakeSection(EItemTooltipSectionType::Details, FText::FromString(TEXT("Item")), false);

		if (!TooltipData.ItemTypeName.IsEmpty())
		{
			AddValueLine(Section.Lines, TEXT("Type"), TooltipData.ItemTypeName, MutedText);
		}

		if (!TooltipData.ItemSubTypeName.IsEmpty() && TooltipData.ItemSubTypeName.ToString() != TEXT("None"))
		{
			AddValueLine(Section.Lines, TEXT("Subtype"), TooltipData.ItemSubTypeName, MutedText);
		}

		if (TooltipData.ItemLevel > 0)
		{
			AddValueLine(Section.Lines, TEXT("Item Level"), FText::AsNumber(TooltipData.ItemLevel), MutedText);
		}

		if (TooltipData.bStackable || TooltipData.Quantity > 1)
		{
			AddValueLine(Section.Lines, TEXT("Quantity"), FText::AsNumber(TooltipData.Quantity), MutedText);
			AddValueLine(Section.Lines, TEXT("Max Stack"), FText::AsNumber(Item->GetMaxStackSize()), MutedText);
		}

		if (TooltipData.ItemValue > 0)
		{
			AddValueLine(Section.Lines, TEXT("Value"), FText::AsNumber(TooltipData.ItemValue), MutedText);
		}

		if (TooltipData.TotalWeight > 0.0f)
		{
			AddValueLine(Section.Lines, TEXT("Weight"), FormatWeightText(TooltipData.TotalWeight), MutedText);
		}

		if (!TooltipData.bIdentified)
		{
			Section.Lines.Add(MakeTextLine(FText::FromString(TEXT("Unidentified")), WarningText, EItemTooltipLineStyle::Warning, true));
		}

		if (TooltipData.bCorrupted)
		{
			Section.Lines.Add(MakeTextLine(FText::FromString(TEXT("Corrupted")), CorruptedText, EItemTooltipLineStyle::Corrupted, true));
		}

		AddSectionIfAny(TooltipData, Section);
	}

	void AddDescription(FItemTooltipData& TooltipData, const FItemBase& Base)
	{
		if (Base.ItemDescription.IsEmpty())
		{
			return;
		}

		FItemTooltipSection Section = MakeSection(EItemTooltipSectionType::Description, FText::FromString(TEXT("Description")));
		Section.Lines.Add(MakeTextLine(Base.ItemDescription, DescriptionText, EItemTooltipLineStyle::Description));
		AddSectionIfAny(TooltipData, Section);
	}
}

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
	OutTooltipData.RarityName = UItemFunctionLibrary::GetRarityDisplayName(Item->Rarity);
	OutTooltipData.ItemTypeName = UItemFunctionLibrary::GetItemTypeName(Base->ItemType);
	OutTooltipData.ItemSubTypeName = UItemFunctionLibrary::GetItemSubTypeName(Base->ItemSubType);
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

	ItemTooltip::AddDetails(OutTooltipData, Item);

	if (Base->ItemType == EItemType::IT_Weapon)
	{
		ItemTooltip::AddWeaponStats(OutTooltipData, Base->WeaponStats);
	}
	else if (Base->ItemType == EItemType::IT_Armor)
	{
		ItemTooltip::AddArmorStats(OutTooltipData, Base->ArmorStats);
	}

	if (Item->IsEquipment())
	{
		ItemTooltip::AddRequirements(OutTooltipData, Base->StatRequirements);
		ItemTooltip::AddDurability(OutTooltipData, Item->Durability);
		ItemTooltip::AddRunes(OutTooltipData, Item, *Base);
	}

	if (Base->ItemType == EItemType::IT_Consumable)
	{
		ItemTooltip::AddConsumable(OutTooltipData, Item, Base->ConsumableData);
	}

	if (Item->IsIdentified())
	{
		const TArray<FPHAttributeData>& Implicits = Item->Stats.Implicits.Num() > 0 ? Item->Stats.Implicits : Base->ImplicitMods;
		ItemTooltip::AddAffixSection(OutTooltipData, EItemTooltipSectionType::Implicits, FText::FromString(TEXT("Implicit")), Implicits);
		ItemTooltip::AddAffixSection(OutTooltipData, EItemTooltipSectionType::Prefixes, FText::FromString(TEXT("Prefixes")), Item->Stats.Prefixes);
		ItemTooltip::AddAffixSection(OutTooltipData, EItemTooltipSectionType::Suffixes, FText::FromString(TEXT("Suffixes")), Item->Stats.Suffixes);
		ItemTooltip::AddAffixSection(OutTooltipData, EItemTooltipSectionType::Crafted, FText::FromString(TEXT("Crafted")), Item->Stats.Crafted);
		ItemTooltip::AddAffixSection(OutTooltipData, EItemTooltipSectionType::Enchants, FText::FromString(TEXT("Enchants")), Item->Stats.Enchants);

		if (Base->bIsUnique || Item->Rarity == EItemRarity::IR_GradeSS)
		{
			ItemTooltip::AddAffixSection(OutTooltipData, EItemTooltipSectionType::Unique, FText::FromString(TEXT("Unique")), Base->UniqueAffixes);
		}
	}

	ItemTooltip::AddDescription(OutTooltipData, *Base);

	return true;
}

FItemTooltipData UItemTooltipFunctionLibrary::GetItemTooltipData(UItemInstance* Item)
{
	FItemTooltipData TooltipData;
	BuildItemTooltipData(Item, TooltipData);
	return TooltipData;
}
