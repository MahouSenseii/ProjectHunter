// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/ToolTip/EquippableStatsBox.h"

#include "Interactables/Pickups/WeaponPickup.h"
#include "Item/EquippableItem.h"
#include "Library/WidgetFunctionLibrary.h"
#include "UI/ToolTip/MinMaxBox.h"

void UEquippableStatsBox::NativeConstruct()
{
	Super::NativeConstruct();

	if(EquippableItemData.PickupClass)
	{
		RequirementsBox->ItemData = EquippableItemData;
	}
}

void UEquippableStatsBox::CreateMinMaxBoxByDamageTypes()
{
	if (!EquippableItemData.PickupClass) return;

	const TMap<EDamageTypes, FDamageRange>& DamageMap = EquippableItemData.WeaponBaseStats.BaseDamage;

	bool bHasElementalDamage = false;

	// Handle Physical Damage
	if (const FDamageRange* PhysicalDamage = DamageMap.Find(EDamageTypes::DT_Physical))
	{
		if (!FMath::IsNearlyZero(PhysicalDamage->Min) || !FMath::IsNearlyZero(PhysicalDamage->Max))
		{
			if (PhysicalDamageValueMin) PhysicalDamageValueMin->SetText(FText::AsNumber(PhysicalDamage->Min));
			if (PhysicalDamageValueMax) PhysicalDamageValueMax->SetText(FText::AsNumber(PhysicalDamage->Max));
		}
		else if (PhysicalDamageBox)
		{
			PhysicalDamageBox->RemoveFromParent();
		}
	}
	else if (PhysicalDamageBox)
	{
		PhysicalDamageBox->RemoveFromParent(); // Not present at all
	}

	// Handle Elemental Damage
	if (EDBox)
	{
		EDBox->ClearChildren();
	}

	for (const TPair<EDamageTypes, FDamageRange>& Pair : DamageMap)
	{
		const EDamageTypes DamageType = Pair.Key;

		if (DamageType == EDamageTypes::DT_Physical) continue;

		const FDamageRange& DamageValues = Pair.Value;

		if (FMath::IsNearlyZero(DamageValues.Min) && FMath::IsNearlyZero(DamageValues.Max)) continue;

		bHasElementalDamage = true;

		UMinMaxBox* MinMaxBox = CreateWidget<UMinMaxBox>(this, MinMaxBoxClass);
		if (!MinMaxBox) continue;

		MinMaxBox->SetMinMaxText(DamageValues.Max, DamageValues.Min);
		MinMaxBox->SetColorBaseOnType(DamageType);

		if (EDBox)
		{
			EDBox->AddChild(MinMaxBox);
		}
	}

	// Remove EDBox if no elemental damage
	if (!bHasElementalDamage && EDBox)
	{
		EDBox->RemoveFromParent();
	}
}



void UEquippableStatsBox::SetMinMaxForOtherStats() const
{
	if (!EquippableItemData.PickupClass) return;

	// Helper: Show/hide box based on value
	auto SetStatRow = [](UHorizontalBox* Box, UTextBlock* TextBlock, float Value)
	{
		if (!Box || !TextBlock) return;

		if (FMath::IsNearlyZero(Value))
		{
			Box->SetVisibility(ESlateVisibility::Collapsed);
		}
		else
		{
			Box->SetVisibility(ESlateVisibility::Visible);
			TextBlock->SetText(FText::AsNumber(FMath::RoundToInt(Value)));
		}
	};

	// Helper: Sum affix contributions where bAffectsBaseWeaponStatsDirectly is true
	auto GetStatModifierFromAffixes = [this](const FGameplayAttribute& Attribute) -> float
	{
		float Total = 0.f;

		auto Accumulate = [&](const TArray<FPHAttributeData>& Stats)
		{
			for (const auto& Stat : Stats)
			{
				if (Stat.ModifiedAttribute == Attribute && Stat.bAffectsBaseWeaponStatsDirectly)
				{
					Total += Stat.RolledStatValue;
				}
			}
		};

		const FPHItemStats& Affixes = EquippableItemData.Affixes;
		Accumulate(Affixes.Prefixes);
		Accumulate(Affixes.Suffixes);
		Accumulate(Affixes.Implicits);
		Accumulate(Affixes.Crafted);

		return Total;
	};

	// Final values = Base + Modifiers
	const float FinalArmor        = EquippableItemData.ArmorBaseStats.Armor + GetStatModifierFromAffixes(UPHAttributeSet::GetArmourAttribute());
	const float FinalPoise        = EquippableItemData.ArmorBaseStats.Poise + GetStatModifierFromAffixes(UPHAttributeSet::GetPoiseAttribute());
	const float FinalCritChance   = EquippableItemData.WeaponBaseStats.CritChance + GetStatModifierFromAffixes(UPHAttributeSet::GetCritChanceAttribute());
	const float FinalAttackSpeed  = EquippableItemData.WeaponBaseStats.AttackSpeed + GetStatModifierFromAffixes(UPHAttributeSet::GetAttackSpeedAttribute());
	const float FinalCastTime     = GetStatModifierFromAffixes(UPHAttributeSet::GetCastSpeedAttribute());
	const float FinalManaCost     = GetStatModifierFromAffixes(UPHAttributeSet::GetManaCostChangesAttribute());
	const float FinalStaminaCost  = GetStatModifierFromAffixes(UPHAttributeSet::GetStaminaCostChangesAttribute());
	const float FinalWeaponRange  = EquippableItemData.WeaponBaseStats.WeaponRange + GetStatModifierFromAffixes(UPHAttributeSet::GetAttackRangeAttribute());

	// Apply to UI
	SetStatRow(ArmourBox,      ArmourValue,      FinalArmor);
	SetStatRow(PoiseBox,       PoiseValue,       FinalPoise);
	SetStatRow(CritChanceBox,  CritChanceValue,  FinalCritChance);
	SetStatRow(AtkSpeedBox,    AtkSpeedValue,    FinalAttackSpeed);
	SetStatRow(CastTimeBox,    CastTimeValue,    FinalCastTime);
	SetStatRow(ManaCostBox,    ManaCostValue,    FinalManaCost);
	SetStatRow(StaminaCostBox, StaminaCostValue, FinalStaminaCost);
	SetStatRow(WeaponRangeBox, WeaponRangeValue, FinalWeaponRange);
}



void UEquippableStatsBox::CreateResistanceBoxes()
{
	if (!ResistanceBox || !ResistanceBoxContainer || !MinMaxBoxClass) return;

	ResistanceBoxContainer->ClearChildren();

	bool bHasAnyResistance = false;

	for (const TPair<EDefenseTypes, float>& Pair : EquippableItemData.ArmorBaseStats.Resistances)
	{
		if (FMath::IsNearlyZero(Pair.Value)) continue;

		bHasAnyResistance = true;

		UMinMaxBox* MinMaxBox = CreateWidget<UMinMaxBox>(this, MinMaxBoxClass);
		if (!MinMaxBox) continue;

		// Resistances are just a flat value, so we can display it as (0 - Value)
		MinMaxBox->SetMinMaxText(Pair.Value, 0.0f);
		MinMaxBox->SetColorBaseOnType(static_cast<EDamageTypes>(Pair.Key)); // assuming EDefenseTypes maps to EDamageTypes visually

		ResistanceBoxContainer->AddChild(MinMaxBox);
	}

	if (!bHasAnyResistance)
	{
		ResistanceBox->SetVisibility(ESlateVisibility::Collapsed);
	}
	else
	{
		ResistanceBox->SetVisibility(ESlateVisibility::Visible);
	}
}
