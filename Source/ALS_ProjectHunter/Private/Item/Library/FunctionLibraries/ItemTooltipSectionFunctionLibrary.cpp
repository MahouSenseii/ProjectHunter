#include "Item/Library/FunctionLibraries/ItemTooltipSectionFunctionLibrary.h"

#include "Item/ItemInstance.h"
#include "Item/Library/FunctionLibraries/ItemAffixFunctionLibrary.h"
#include "Item/Library/FunctionLibraries/ItemTooltipLineFunctionLibrary.h"
#include "Item/Library/Structs/ItemAttributeStructs.h"
#include "Item/Library/Structs/ItemRequirementStructs.h"
#include "Item/Library/Structs/ItemStructs.h"

namespace ItemTooltipSection
{
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
			return UItemTooltipLineFunctionLibrary::GetCorruptedTextColor();
		}

		return Affix.RolledStatValue < 0.0f
			? UItemTooltipLineFunctionLibrary::GetNegativeTextColor()
			: UItemTooltipLineFunctionLibrary::GetAffixTextColor();
	}

	void AddAffixLines(FItemTooltipSection& Section, const TArray<FPHAttributeData>& Affixes)
	{
		for (const FPHAttributeData& Affix : Affixes)
		{
			if (!HasDisplayableAffixText(Affix))
			{
				continue;
			}

			const FString AffixText = UItemAffixFunctionLibrary::FormatAffixText(Affix);
			if (AffixText.IsEmpty())
			{
				continue;
			}

			const bool bCorrupted = Affix.IsCorruptedAffix() || Affix.GetRankPointValue() < 0;
			Section.Lines.Add(UItemTooltipLineFunctionLibrary::MakeTooltipTextLine(
				FText::FromString(AffixText),
				GetAffixColor(Affix),
				bCorrupted ? EItemTooltipLineStyle::Corrupted : EItemTooltipLineStyle::Affix,
				bCorrupted));
		}
	}

	void AddDurabilityLine(FItemTooltipSection& Section, const FItemDurability& Durability)
	{
		if (Durability.MaxDurability <= 0.0f)
		{
			return;
		}

		UItemTooltipLineFunctionLibrary::AddTooltipValueLine(
			Section.Lines,
			TEXT("Durability"),
			FText::FromString(FString::Printf(
				TEXT("%s / %s"),
				*UItemTooltipLineFunctionLibrary::FormatTooltipNumber(Durability.CurrentDurability),
				*UItemTooltipLineFunctionLibrary::FormatTooltipNumber(Durability.MaxDurability))),
			UItemTooltipLineFunctionLibrary::GetMutedTextColor());
	}

	const TCHAR* GetRequirementLabel(const EItemRequirementType RequirementType)
	{
		switch (RequirementType)
		{
		case EItemRequirementType::Level:        return TEXT("Level");
		case EItemRequirementType::Strength:     return TEXT("Strength");
		case EItemRequirementType::Dexterity:    return TEXT("Dexterity");
		case EItemRequirementType::Intelligence: return TEXT("Intelligence");
		case EItemRequirementType::Endurance:    return TEXT("Endurance");
		case EItemRequirementType::Affliction:   return TEXT("Affliction");
		case EItemRequirementType::Luck:         return TEXT("Luck");
		case EItemRequirementType::Covenant:     return TEXT("Covenant");
		default:                                 return TEXT("Requirement");
		}
	}

	bool IsDisplayableRequirement(const FItemRequirementStatus& Status)
	{
		return Status.RequiredValue > 0.0f
			&& (Status.RequirementType != EItemRequirementType::Level || Status.RequiredValue > 1.0f);
	}

	void AddEvaluatedRequirementLines(
		FItemTooltipSection& Section,
		const FItemRequirementCheckResult& RequirementResult)
	{
		for (const FItemRequirementStatus& Status : RequirementResult.Checks)
		{
			if (!IsDisplayableRequirement(Status))
			{
				continue;
			}

			const FString Current = UItemTooltipLineFunctionLibrary::FormatTooltipNumber(Status.CurrentValue);
			const FString Required = UItemTooltipLineFunctionLibrary::FormatTooltipNumber(Status.RequiredValue);
			const FString Value = Status.bMet
				? FString::Printf(TEXT("%s / %s"), *Current, *Required)
				: FString::Printf(
					TEXT("%s / %s (%s missing)"),
					*Current,
					*Required,
					*UItemTooltipLineFunctionLibrary::FormatTooltipNumber(Status.MissingValue));

			Section.Lines.Add(UItemTooltipLineFunctionLibrary::MakeTooltipLine(
				FText::FromString(GetRequirementLabel(Status.RequirementType)),
				FText::FromString(Value),
				Status.bMet
					? UItemTooltipLineFunctionLibrary::GetPositiveTextColor()
					: UItemTooltipLineFunctionLibrary::GetNegativeTextColor(),
				Status.bMet ? EItemTooltipLineStyle::Property : EItemTooltipLineStyle::Warning,
				true,
				!Status.bMet));
		}
	}
}

FItemTooltipSection UItemTooltipSectionFunctionLibrary::MakeTooltipSection(
	const EItemTooltipSectionType Type,
	const FText& Heading,
	const bool bShowHeading)
{
	FItemTooltipSection Section;
	Section.SectionType = Type;
	Section.Heading = Heading;
	Section.bShowHeading = bShowHeading;
	return Section;
}

void UItemTooltipSectionFunctionLibrary::AddSectionIfAny(FItemTooltipData& TooltipData, FItemTooltipSection& Section)
{
	if (Section.HasDisplayableLines())
	{
		TooltipData.Sections.Add(MoveTemp(Section));
	}
}

void UItemTooltipSectionFunctionLibrary::AddAffixSection(
	FItemTooltipData& TooltipData,
	const EItemTooltipSectionType Type,
	const FText& Heading,
	const TArray<FPHAttributeData>& Affixes)
{
	FItemTooltipSection Section = MakeTooltipSection(Type, Heading);
	ItemTooltipSection::AddAffixLines(Section, Affixes);
	AddSectionIfAny(TooltipData, Section);
}

void UItemTooltipSectionFunctionLibrary::AddWeaponStatsSection(
	FItemTooltipData& TooltipData,
	const FBaseWeaponStats& Stats,
	const FItemDurability& Durability)
{
	FItemTooltipSection Section = MakeTooltipSection(EItemTooltipSectionType::BaseStats, FText::FromString(TEXT("Weapon Stats")));

	if (!FMath::IsNearlyZero(Stats.MinPhysicalDamage) || !FMath::IsNearlyZero(Stats.MaxPhysicalDamage))
	{
		UItemTooltipLineFunctionLibrary::AddTooltipValueLine(
			Section.Lines,
			TEXT("Physical Damage"),
			UItemTooltipLineFunctionLibrary::FormatTooltipRangeText(Stats.MinPhysicalDamage, Stats.MaxPhysicalDamage),
			UItemTooltipLineFunctionLibrary::GetStatTextColor(),
			EItemTooltipLineStyle::Stat);
	}

	if (!FMath::IsNearlyZero(Stats.MinFireDamage) || !FMath::IsNearlyZero(Stats.MaxFireDamage))
	{
		UItemTooltipLineFunctionLibrary::AddTooltipValueLine(
			Section.Lines,
			TEXT("Fire Damage"),
			UItemTooltipLineFunctionLibrary::FormatTooltipRangeText(Stats.MinFireDamage, Stats.MaxFireDamage),
			UItemTooltipLineFunctionLibrary::GetStatTextColor(),
			EItemTooltipLineStyle::Stat);
	}

	if (!FMath::IsNearlyZero(Stats.MinIceDamage) || !FMath::IsNearlyZero(Stats.MaxIceDamage))
	{
		UItemTooltipLineFunctionLibrary::AddTooltipValueLine(
			Section.Lines,
			TEXT("Ice Damage"),
			UItemTooltipLineFunctionLibrary::FormatTooltipRangeText(Stats.MinIceDamage, Stats.MaxIceDamage),
			UItemTooltipLineFunctionLibrary::GetStatTextColor(),
			EItemTooltipLineStyle::Stat);
	}

	if (!FMath::IsNearlyZero(Stats.MinLightningDamage) || !FMath::IsNearlyZero(Stats.MaxLightningDamage))
	{
		UItemTooltipLineFunctionLibrary::AddTooltipValueLine(
			Section.Lines,
			TEXT("Lightning Damage"),
			UItemTooltipLineFunctionLibrary::FormatTooltipRangeText(Stats.MinLightningDamage, Stats.MaxLightningDamage),
			UItemTooltipLineFunctionLibrary::GetStatTextColor(),
			EItemTooltipLineStyle::Stat);
	}

	if (!FMath::IsNearlyZero(Stats.MinLightDamage) || !FMath::IsNearlyZero(Stats.MaxLightDamage))
	{
		UItemTooltipLineFunctionLibrary::AddTooltipValueLine(
			Section.Lines,
			TEXT("Light Damage"),
			UItemTooltipLineFunctionLibrary::FormatTooltipRangeText(Stats.MinLightDamage, Stats.MaxLightDamage),
			UItemTooltipLineFunctionLibrary::GetStatTextColor(),
			EItemTooltipLineStyle::Stat);
	}

	if (!FMath::IsNearlyZero(Stats.MinCorruptionDamage) || !FMath::IsNearlyZero(Stats.MaxCorruptionDamage))
	{
		UItemTooltipLineFunctionLibrary::AddTooltipValueLine(
			Section.Lines,
			TEXT("Corruption Damage"),
			UItemTooltipLineFunctionLibrary::FormatTooltipRangeText(Stats.MinCorruptionDamage, Stats.MaxCorruptionDamage),
			UItemTooltipLineFunctionLibrary::GetCorruptedTextColor(),
			EItemTooltipLineStyle::Corrupted);
	}

	if (Stats.AttackSpeed > 0.0f)
	{
		UItemTooltipLineFunctionLibrary::AddTooltipValueLine(
			Section.Lines,
			TEXT("Attack Speed"),
			UItemTooltipLineFunctionLibrary::FormatTooltipNumberText(Stats.AttackSpeed, 2),
			UItemTooltipLineFunctionLibrary::GetStatTextColor(),
			EItemTooltipLineStyle::Stat);
	}

	if (Stats.CriticalStrikeChance > 0.0f)
	{
		UItemTooltipLineFunctionLibrary::AddTooltipValueLine(
			Section.Lines,
			TEXT("Critical Strike Chance"),
			UItemTooltipLineFunctionLibrary::FormatTooltipPercentText(Stats.CriticalStrikeChance),
			UItemTooltipLineFunctionLibrary::GetStatTextColor(),
			EItemTooltipLineStyle::Stat);
	}

	if (Stats.Range > 0.0f)
	{
		UItemTooltipLineFunctionLibrary::AddTooltipValueLine(
			Section.Lines,
			TEXT("Range"),
			UItemTooltipLineFunctionLibrary::FormatTooltipNumberText(Stats.Range),
			UItemTooltipLineFunctionLibrary::GetStatTextColor(),
			EItemTooltipLineStyle::Stat);
	}

	ItemTooltipSection::AddDurabilityLine(Section, Durability);
	AddSectionIfAny(TooltipData, Section);
}

void UItemTooltipSectionFunctionLibrary::AddArmorStatsSection(
	FItemTooltipData& TooltipData,
	const FBaseArmorStats& Stats,
	const FItemDurability& Durability)
{
	FItemTooltipSection Section = MakeTooltipSection(EItemTooltipSectionType::BaseStats, FText::FromString(TEXT("Armor Stats")));

	UItemTooltipLineFunctionLibrary::AddTooltipNonZeroLine(Section.Lines, TEXT("Armor"), Stats.Armor);
	UItemTooltipLineFunctionLibrary::AddTooltipNonZeroLine(Section.Lines, TEXT("Fire Resistance"), Stats.FireResistance, true);
	UItemTooltipLineFunctionLibrary::AddTooltipNonZeroLine(Section.Lines, TEXT("Ice Resistance"), Stats.IceResistance, true);
	UItemTooltipLineFunctionLibrary::AddTooltipNonZeroLine(Section.Lines, TEXT("Lightning Resistance"), Stats.LightningResistance, true);
	UItemTooltipLineFunctionLibrary::AddTooltipNonZeroLine(Section.Lines, TEXT("Light Resistance"), Stats.LightResistance, true);
	UItemTooltipLineFunctionLibrary::AddTooltipNonZeroLine(Section.Lines, TEXT("Corruption Resistance"), Stats.CorruptionResistance, true, UItemTooltipLineFunctionLibrary::GetCorruptedTextColor());

	ItemTooltipSection::AddDurabilityLine(Section, Durability);
	AddSectionIfAny(TooltipData, Section);
}

void UItemTooltipSectionFunctionLibrary::AddRequirementsSection(
	FItemTooltipData& TooltipData,
	const FItemStatRequirement& Requirements,
	const FItemRequirementCheckResult* RequirementResult)
{
	FItemTooltipSection Section = MakeTooltipSection(EItemTooltipSectionType::Requirements, FText::FromString(TEXT("Requirements")));

	if (RequirementResult && RequirementResult->bStatsAvailable)
	{
		ItemTooltipSection::AddEvaluatedRequirementLines(Section, *RequirementResult);
	}
	else
	{
		UItemTooltipLineFunctionLibrary::AddTooltipPositiveLine(Section.Lines, TEXT("Level"), Requirements.RequiredLevel > 1 ? Requirements.RequiredLevel : 0);
		UItemTooltipLineFunctionLibrary::AddTooltipPositiveLine(Section.Lines, TEXT("Strength"), Requirements.RequiredStrength);
		UItemTooltipLineFunctionLibrary::AddTooltipPositiveLine(Section.Lines, TEXT("Dexterity"), Requirements.RequiredDexterity);
		UItemTooltipLineFunctionLibrary::AddTooltipPositiveLine(Section.Lines, TEXT("Intelligence"), Requirements.RequiredIntelligence);
		UItemTooltipLineFunctionLibrary::AddTooltipPositiveLine(Section.Lines, TEXT("Endurance"), Requirements.RequiredEndurance);
		UItemTooltipLineFunctionLibrary::AddTooltipPositiveLine(Section.Lines, TEXT("Affliction"), Requirements.RequiredAffliction);
		UItemTooltipLineFunctionLibrary::AddTooltipPositiveLine(Section.Lines, TEXT("Luck"), Requirements.RequiredLuck);
		UItemTooltipLineFunctionLibrary::AddTooltipPositiveLine(Section.Lines, TEXT("Covenant"), Requirements.RequiredCovenant);
	}

	AddSectionIfAny(TooltipData, Section);
}

void UItemTooltipSectionFunctionLibrary::AddRunesSection(FItemTooltipData& TooltipData, const UItemInstance* Item, const FItemBase& Base)
{
	if (!Item)
	{
		return;
	}

	FItemTooltipSection Section = MakeTooltipSection(EItemTooltipSectionType::Runes, FText::FromString(TEXT("Runes")));

	const int32 SocketCount = Item->RuneCraftingData.GetSocketCount();
	const int32 MaxSockets = FMath::Max(Base.MaxRuneSockets, SocketCount);
	if (MaxSockets > 0)
	{
		UItemTooltipLineFunctionLibrary::AddTooltipValueLine(
			Section.Lines,
			TEXT("Rune Sockets"),
			FText::FromString(FString::Printf(TEXT("%d / %d"), Item->RuneCraftingData.GetSocketedRuneCount(), MaxSockets)),
			UItemTooltipLineFunctionLibrary::GetMutedTextColor());
	}

	if (Item->RuneCraftingData.EnhancementLevel > 0)
	{
		UItemTooltipLineFunctionLibrary::AddTooltipValueLine(
			Section.Lines,
			TEXT("Enhancement"),
			FText::FromString(FString::Printf(
				TEXT("+%d / +%d"),
				Item->RuneCraftingData.EnhancementLevel,
				FMath::Max(Base.MaxEnhancementLevel, Item->RuneCraftingData.MaxEnhancementLevel))),
			UItemTooltipLineFunctionLibrary::GetPositiveTextColor());
	}

	if (Item->Quality > 0)
	{
		UItemTooltipLineFunctionLibrary::AddTooltipValueLine(
			Section.Lines,
			TEXT("Reinforcement"),
			FText::FromString(FString::Printf(TEXT("+%d"), Item->Quality)),
			UItemTooltipLineFunctionLibrary::GetPositiveTextColor());
	}

	AddSectionIfAny(TooltipData, Section);
}

void UItemTooltipSectionFunctionLibrary::AddConsumableSection(
	FItemTooltipData& TooltipData,
	const UItemInstance* Item,
	const FConsumableData& ConsumableData)
{
	if (!Item)
	{
		return;
	}

	FItemTooltipSection Section = MakeTooltipSection(EItemTooltipSectionType::Consumable, FText::FromString(TEXT("Consumable")));

	if (ConsumableData.MaxUses > 1)
	{
		UItemTooltipLineFunctionLibrary::AddTooltipValueLine(
			Section.Lines,
			TEXT("Uses"),
			FText::FromString(FString::Printf(TEXT("%d / %d"), Item->RemainingUses, ConsumableData.MaxUses)),
			UItemTooltipLineFunctionLibrary::GetMutedTextColor());
	}

	if (ConsumableData.Cooldown > 0.0f)
	{
		UItemTooltipLineFunctionLibrary::AddTooltipValueLine(
			Section.Lines,
			TEXT("Cooldown"),
			FText::FromString(FString::Printf(TEXT("%ss"), *UItemTooltipLineFunctionLibrary::FormatTooltipNumber(ConsumableData.Cooldown))),
			UItemTooltipLineFunctionLibrary::GetMutedTextColor());
	}

	AddSectionIfAny(TooltipData, Section);
}

void UItemTooltipSectionFunctionLibrary::AddDetailsSection(FItemTooltipData& TooltipData, const UItemInstance* Item)
{
	if (!Item)
	{
		return;
	}

	FItemTooltipSection Section = MakeTooltipSection(
		EItemTooltipSectionType::Details,
		FText::FromString(TEXT("Item Details")));

	if (TooltipData.ItemLevel > 0)
	{
		UItemTooltipLineFunctionLibrary::AddTooltipValueLine(Section.Lines, TEXT("Item Level"), FText::AsNumber(TooltipData.ItemLevel), UItemTooltipLineFunctionLibrary::GetMutedTextColor());
	}

	if (TooltipData.bStackable || TooltipData.Quantity > 1)
	{
		UItemTooltipLineFunctionLibrary::AddTooltipValueLine(Section.Lines, TEXT("Quantity"), FText::AsNumber(TooltipData.Quantity), UItemTooltipLineFunctionLibrary::GetMutedTextColor());
		UItemTooltipLineFunctionLibrary::AddTooltipValueLine(Section.Lines, TEXT("Max Stack"), FText::AsNumber(Item->GetMaxStackSize()), UItemTooltipLineFunctionLibrary::GetMutedTextColor());
	}

	if (TooltipData.ItemValue > 0)
	{
		UItemTooltipLineFunctionLibrary::AddTooltipValueLine(Section.Lines, TEXT("Value"), FText::AsNumber(TooltipData.ItemValue), UItemTooltipLineFunctionLibrary::GetMutedTextColor());
	}

	if (TooltipData.TotalWeight > 0.0f)
	{
		UItemTooltipLineFunctionLibrary::AddTooltipValueLine(Section.Lines, TEXT("Weight"), UItemTooltipLineFunctionLibrary::FormatTooltipWeightText(TooltipData.TotalWeight), UItemTooltipLineFunctionLibrary::GetMutedTextColor());
	}

	if (!TooltipData.bIdentified)
	{
		Section.Lines.Add(UItemTooltipLineFunctionLibrary::MakeTooltipTextLine(FText::FromString(TEXT("Unidentified")), UItemTooltipLineFunctionLibrary::GetWarningTextColor(), EItemTooltipLineStyle::Warning, true));
	}

	if (TooltipData.bCorrupted)
	{
		Section.Lines.Add(UItemTooltipLineFunctionLibrary::MakeTooltipTextLine(FText::FromString(TEXT("Corrupted")), UItemTooltipLineFunctionLibrary::GetCorruptedTextColor(), EItemTooltipLineStyle::Corrupted, true));
	}

	AddSectionIfAny(TooltipData, Section);
}

void UItemTooltipSectionFunctionLibrary::AddDescriptionSection(FItemTooltipData& TooltipData, const FItemBase& Base)
{
	if (Base.ItemDescription.IsEmpty())
	{
		return;
	}

	FItemTooltipSection Section = MakeTooltipSection(EItemTooltipSectionType::Description, FText::FromString(TEXT("Description")));
	Section.Lines.Add(UItemTooltipLineFunctionLibrary::MakeTooltipTextLine(Base.ItemDescription, UItemTooltipLineFunctionLibrary::GetDescriptionTextColor(), EItemTooltipLineStyle::Description));
	AddSectionIfAny(TooltipData, Section);
}
