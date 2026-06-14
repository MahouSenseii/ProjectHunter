// Copyright © 2025 MahouSensei
// Author: Quentin Davis

#include "Interactable/Widget/ItemTooltipWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Item/Library/ItemStructs.h"

namespace ItemTooltipWidgetPrivate
{
	/** "MaxFireDamage" -> "Max Fire Damage". */
	FString PrettyAttributeName(const FName AttributeName)
	{
		return FName::NameToDisplayString(AttributeName.ToString(), /*bIsBool*/ false);
	}

	/** Trim trailing zeros: 10 -> "10", 7.5 -> "7.5". */
	FString FormatStatValue(const float Value)
	{
		if (FMath::IsNearlyEqual(Value, FMath::RoundToFloat(Value), 0.01f))
		{
			return FString::Printf(TEXT("%.0f"), Value);
		}
		return FString::Printf(TEXT("%.1f"), Value);
	}

	/** Compose one affix line from its display format. */
	FString FormatAffixLine(const FPHAttributeData& Stat)
	{
		// Designer-supplied override wins. Supports an optional {0} value slot.
		if (!Stat.DisplayText.IsEmpty())
		{
			return FText::Format(
				FTextFormat(Stat.DisplayText),
				FText::FromString(FormatStatValue(Stat.RolledStatValue))).ToString();
		}

		const FString Name  = PrettyAttributeName(Stat.AttributeName);
		const FString Value = FormatStatValue(Stat.RolledStatValue);

		switch (Stat.DisplayFormat)
		{
		case EAttributeDisplayFormat::ADF_Additive:     return FString::Printf(TEXT("+%s %s"), *Value, *Name);
		case EAttributeDisplayFormat::ADF_FlatNegative: return FString::Printf(TEXT("-%s %s"), *Value, *Name);
		case EAttributeDisplayFormat::ADF_Percent:      return FString::Printf(TEXT("+%s%% %s"), *Value, *Name);
		case EAttributeDisplayFormat::ADF_Increase:     return FString::Printf(TEXT("%s%% increased %s"), *Value, *Name);
		case EAttributeDisplayFormat::ADF_More:         return FString::Printf(TEXT("%s%% more %s"), *Value, *Name);
		case EAttributeDisplayFormat::ADF_Less:         return FString::Printf(TEXT("%s%% less %s"), *Value, *Name);
		case EAttributeDisplayFormat::ADF_Chance:       return FString::Printf(TEXT("%s%% chance to %s"), *Value, *Name);
		case EAttributeDisplayFormat::ADF_Duration:     return FString::Printf(TEXT("+%ss %s"), *Value, *Name);
		case EAttributeDisplayFormat::ADF_MinMax:       return FString::Printf(TEXT("Adds %s %s"), *Value, *Name);
		default:                                        return FString::Printf(TEXT("%s: %s"), *Name, *Value);
		}
	}

	/** Append a "Label: Min-Max" damage row when the range is non-zero. */
	void AppendDamageRange(TArray<FString>& OutRows, const TCHAR* Label, const float Min, const float Max)
	{
		if (Max <= 0.0f && Min <= 0.0f)
		{
			return;
		}
		OutRows.Add(FString::Printf(TEXT("%s: %s-%s"),
			Label, *FormatStatValue(Min), *FormatStatValue(Max)));
	}
}

void UItemTooltipWidget::UpdateTooltip(UItemInstance* Item)
{
	if (!Item)
	{
		return;
	}

	FItemBase* Base = Item->GetBaseData();
	if (!Base)
	{
		return;
	}

	if (ItemNameText)
	{
		ItemNameText->SetText(Item->GetDisplayName());
	}

	if (ItemTypeText)
	{
		ItemTypeText->SetText(UEnum::GetDisplayValueAsText(Item->GetItemType()));
	}

	if (ItemIconImage && Base->ItemImage)
	{
		ItemIconImage->SetBrushFromMaterial(Base->ItemImage);
	}

	SetGradeVisuals(Item->Rarity);
	PopulateBaseStats(Item);
	PopulateAffixes(Item);
	PopulateLore(Item);

	// Blueprint extension point — runs after the base population pass.
	OnTooltipUpdated(Item);
}

void UItemTooltipWidget::SetGradeVisuals(EItemRarity Grade)
{
	const FLinearColor GradeColor = GetGradeColor(Grade);

	if (HeaderBorder)
	{
		HeaderBorder->SetBrushColor(GradeColor);
	}

	if (ItemNameText)
	{
		ItemNameText->SetColorAndOpacity(FSlateColor(GradeColor));
	}
}

FLinearColor UItemTooltipWidget::GetGradeColor(EItemRarity Grade) const
{
	switch (Grade)
	{
	case EItemRarity::IR_GradeF:    return Color_GradeF;
	case EItemRarity::IR_GradeE:    return Color_GradeE;
	case EItemRarity::IR_GradeD:    return Color_GradeD;
	case EItemRarity::IR_GradeC:    return Color_GradeC;
	case EItemRarity::IR_GradeB:    return Color_GradeB;
	case EItemRarity::IR_GradeA:    return Color_GradeA;
	case EItemRarity::IR_GradeS:    return Color_GradeS;
	case EItemRarity::IR_GradeSS:   return Color_GradeS;
	case EItemRarity::IR_Unknown:   return Color_GradeUnkown;
	case EItemRarity::IR_Corrupted: return Color_GradeCorrupted;
	default:                        return FLinearColor::White;
	}
}

void UItemTooltipWidget::PopulateBaseStats(UItemInstance* Item)
{
	using namespace ItemTooltipWidgetPrivate;

	if (!BaseStatsContainer)
	{
		return;
	}

	BaseStatsContainer->ClearChildren();

	FItemBase* Base = Item ? Item->GetBaseData() : nullptr;
	if (!Base)
	{
		if (BaseStatsBox) { BaseStatsBox->SetVisibility(ESlateVisibility::Collapsed); }
		return;
	}

	TArray<FString> Rows;

	if (Base->IsWeapon())
	{
		const FBaseWeaponStats& W = Base->WeaponStats;
		AppendDamageRange(Rows, TEXT("Physical Damage"),   W.MinPhysicalDamage,   W.MaxPhysicalDamage);
		AppendDamageRange(Rows, TEXT("Fire Damage"),       W.MinFireDamage,       W.MaxFireDamage);
		AppendDamageRange(Rows, TEXT("Ice Damage"),        W.MinIceDamage,        W.MaxIceDamage);
		AppendDamageRange(Rows, TEXT("Lightning Damage"),  W.MinLightningDamage,  W.MaxLightningDamage);
		AppendDamageRange(Rows, TEXT("Light Damage"),      W.MinLightDamage,      W.MaxLightDamage);
		AppendDamageRange(Rows, TEXT("Corruption Damage"), W.MinCorruptionDamage, W.MaxCorruptionDamage);

		Rows.Add(FString::Printf(TEXT("Attack Speed: %s"),      *FormatStatValue(W.AttackSpeed)));
		Rows.Add(FString::Printf(TEXT("Critical Chance: %s%%"), *FormatStatValue(W.CriticalStrikeChance)));
	}
	else if (Base->IsArmor())
	{
		const FBaseArmorStats& A = Base->ArmorStats;
		if (!FMath::IsNearlyZero(A.Armor))
		{
			Rows.Add(FString::Printf(TEXT("Armor: %s"), *FormatStatValue(A.Armor)));
		}
		if (!FMath::IsNearlyZero(A.FireResistance))       { Rows.Add(FString::Printf(TEXT("+%s Fire Resistance"),       *FormatStatValue(A.FireResistance))); }
		if (!FMath::IsNearlyZero(A.IceResistance))        { Rows.Add(FString::Printf(TEXT("+%s Ice Resistance"),        *FormatStatValue(A.IceResistance))); }
		if (!FMath::IsNearlyZero(A.LightningResistance))  { Rows.Add(FString::Printf(TEXT("+%s Lightning Resistance"),  *FormatStatValue(A.LightningResistance))); }
		if (!FMath::IsNearlyZero(A.LightResistance))      { Rows.Add(FString::Printf(TEXT("+%s Light Resistance"),      *FormatStatValue(A.LightResistance))); }
		if (!FMath::IsNearlyZero(A.CorruptionResistance)) { Rows.Add(FString::Printf(TEXT("+%s Corruption Resistance"), *FormatStatValue(A.CorruptionResistance))); }
	}

	if (Base->IsEquippable())
	{
		Rows.Add(FString::Printf(TEXT("Durability: %s/%s"),
			*FormatStatValue(Item->Durability.CurrentDurability),
			*FormatStatValue(Item->Durability.MaxDurability)));
	}

	for (const FString& Row : Rows)
	{
		if (UTextBlock* TextBlock = CreateStatTextBlock(Row, BaseStatColor))
		{
			BaseStatsContainer->AddChildToVerticalBox(TextBlock);
		}
	}

	if (BaseStatsBox)
	{
		BaseStatsBox->SetVisibility(Rows.Num() > 0
			? ESlateVisibility::Visible
			: ESlateVisibility::Collapsed);
	}
}

void UItemTooltipWidget::PopulateAffixes(UItemInstance* Item)
{
	using namespace ItemTooltipWidgetPrivate;

	if (!AffixesContainer)
	{
		return;
	}

	AffixesContainer->ClearChildren();

	int32 RowCount = 0;

	if (Item)
	{
		Item->Stats.ForEachStat([this, &RowCount](const FPHAttributeData& Stat)
		{
			if (!Stat.bIsIdentified)
			{
				return;
			}

			const FLinearColor RowColor = Stat.IsCorruptedAffix()
				? Color_GradeCorrupted
				: AffixColor;

			if (UTextBlock* TextBlock = CreateStatTextBlock(FormatAffixLine(Stat), RowColor))
			{
				AffixesContainer->AddChildToVerticalBox(TextBlock);
				++RowCount;
			}
		});

		// One hint row for anything still hidden on the item.
		if (Item->Stats.HasUnidentifiedStats() || !Item->IsIdentified())
		{
			if (UTextBlock* TextBlock = CreateStatTextBlock(
				TEXT("??? (Unidentified)"), Color_GradeUnkown))
			{
				AffixesContainer->AddChildToVerticalBox(TextBlock);
				++RowCount;
			}
		}
	}

	if (AffixesBox)
	{
		AffixesBox->SetVisibility(RowCount > 0
			? ESlateVisibility::Visible
			: ESlateVisibility::Collapsed);
	}
}

void UItemTooltipWidget::PopulateLore(UItemInstance* Item)
{
	FItemBase* Base = Item ? Item->GetBaseData() : nullptr;
	const bool bHasLore = Base && !Base->ItemDescription.IsEmpty();

	if (LoreText)
	{
		LoreText->SetText(bHasLore ? Base->ItemDescription : FText::GetEmpty());
		LoreText->SetColorAndOpacity(FSlateColor(LoreColor));
	}

	if (LoreBox)
	{
		LoreBox->SetVisibility(bHasLore
			? ESlateVisibility::Visible
			: ESlateVisibility::Collapsed);
	}
}

UTextBlock* UItemTooltipWidget::CreateStatTextBlock(
	const FString& Text,
	FLinearColor Color)
{
	if (!WidgetTree)
	{
		return nullptr;
	}

	UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());

	if (!TextBlock)
	{
		return nullptr;
	}

	TextBlock->SetText(FText::FromString(Text));
	TextBlock->SetColorAndOpacity(FSlateColor(Color));

	return TextBlock;
}
