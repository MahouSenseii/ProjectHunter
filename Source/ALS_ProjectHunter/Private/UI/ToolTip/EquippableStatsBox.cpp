// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/ToolTip/EquippableStatsBox.h"

#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBoxSlot.h"
#include "Interactables/Pickups/WeaponPickup.h"
#include "UI/ToolTip/MinMaxBox.h"

void UEquippableStatsBox::NativeConstruct()
{
	Super::NativeConstruct();

	if(ItemData.ItemInfo.IsValid())
	{
		RequirementsBox->ItemData = ItemData.ItemData;
	}

	MinMaxBoxClass = UMinMaxBox::StaticClass();
}

void UEquippableStatsBox::CreateMinMaxBoxByDamageTypes()
{
	if (!ItemData.IsValid()) return;
	check(MinMaxBoxClass);

	const TMap<EDamageTypes, FDamageRange>& DamageMap = ItemData.ItemData.WeaponBaseStats.BaseDamage;
	bool bHasElementalDamage = false;

	// Handle Physical Damage
	if (const FDamageRange* PhysicalDamage = DamageMap.Find(EDamageTypes::DT_Physical))
	{
		if (!FMath::IsNearlyZero(PhysicalDamage->Min) || !FMath::IsNearlyZero(PhysicalDamage->Max))
		{
			const FString FormattedMin = FString::Printf(TEXT("(%d/"), FMath::RoundToInt(PhysicalDamage->Min));
			const FString FormattedMax = FString::Printf(TEXT("%d)"), FMath::RoundToInt(PhysicalDamage->Max));

			if (PhysicalDamageValueMin) PhysicalDamageValueMin->SetText(FText::FromString(FormattedMin));
			if (PhysicalDamageValueMax) PhysicalDamageValueMax->SetText(FText::FromString(FormattedMax));
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
	for (const TPair<EDamageTypes, FDamageRange>& Pair : DamageMap)
	{
		const EDamageTypes DamageType = Pair.Key;

		if (DamageType == EDamageTypes::DT_Physical) continue;

		const FDamageRange& DamageValues = Pair.Value;

		if (FMath::IsNearlyZero(DamageValues.Min) && FMath::IsNearlyZero(DamageValues.Max)) continue;

		bHasElementalDamage = true;

		// Create MinMaxBox
		UMinMaxBox* MinMaxBox = CreateWidget<UMinMaxBox>(this, MinMaxBoxClass);
		
		if (!MinMaxBox)
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to create MinMaxBox for damage type %d"), static_cast<int32>(DamageType));
			continue;
		}

		MinMaxBox->Init();
		MinMaxBox->SetMinMaxText(DamageValues.Min, DamageValues.Max);
		MinMaxBox->SetColorBaseOnType(DamageType);
		MinMaxBox->SetFontSize(10.0f);

		if (EDBox)
		{
			// Add with alignment
			if (UHorizontalBox* VBox = Cast<UHorizontalBox>(EDBox))
			{
				if (UHorizontalBoxSlot* HBSlot = VBox->AddChildToHorizontalBox(MinMaxBox))
				{
					HBSlot->SetHorizontalAlignment(HAlign_Right);  // Align to the right
					HBSlot->SetVerticalAlignment(VAlign_Fill);
				}
			}
			else
			{
				// fallback for non-vertical box type
				EDBox->AddChild(MinMaxBox);
				
			}
		}
	}

	// Remove EDBox if no elemental damage was found
	if (!bHasElementalDamage && EDBox)
	{
		EDBox->RemoveFromParent();
	}
}



void UEquippableStatsBox::SetMinMaxForOtherStats() const
{
	if (!ItemData.ItemInfo.PickupClass) return;

	// Helper: Show/hide box based on value, with optional percent display
	auto SetStatRow = [](UHorizontalBox* Box, UTextBlock* TextBlock, float Value, bool bAsPercent = false)
	{
		if (!Box || !TextBlock) return;

		if (FMath::Abs(Value) < 0.01f)
		{
			Box->SetVisibility(ESlateVisibility::Collapsed);
		}
		else
		{
			Box->SetVisibility(ESlateVisibility::Visible);
			if (bAsPercent)
			{
				// Show as percentage (rounded)
				const int32 RoundedPercent = FMath::RoundToInt(Value * 100.f);
				TextBlock->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), RoundedPercent)));
			}
			else
			{
				// Show as rounded whole number
				TextBlock->SetText(FText::AsNumber(FMath::RoundToInt(Value)));
			}
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

		const FPHItemStats& Affixes = ItemData.ItemData.Affixes;
		Accumulate(Affixes.Prefixes);
		Accumulate(Affixes.Suffixes);
		Accumulate(Affixes.Implicits);
		Accumulate(Affixes.Crafted);

		return Total;
	};

	// Final values = Base + Modifiers
	const float FinalArmor        = ItemData.ItemData.ArmorBaseStats.Armor + GetStatModifierFromAffixes(UPHAttributeSet::GetArmourAttribute());
	const float FinalPoise        = ItemData.ItemData.ArmorBaseStats.Poise + GetStatModifierFromAffixes(UPHAttributeSet::GetPoiseAttribute());
	const float FinalCritChance   = ItemData.ItemData.WeaponBaseStats.CritChance + GetStatModifierFromAffixes(UPHAttributeSet::GetCritChanceAttribute());
	const float FinalAttackSpeed  = ItemData.ItemData.WeaponBaseStats.AttackSpeed + GetStatModifierFromAffixes(UPHAttributeSet::GetAttackSpeedAttribute());
	const float FinalCastTime     = GetStatModifierFromAffixes(UPHAttributeSet::GetCastSpeedAttribute());
	const float FinalManaCost     = GetStatModifierFromAffixes(UPHAttributeSet::GetManaCostChangesAttribute());
	const float FinalStaminaCost  = GetStatModifierFromAffixes(UPHAttributeSet::GetStaminaCostChangesAttribute());
	const float FinalWeaponRange  = ItemData.ItemData.WeaponBaseStats.WeaponRange + GetStatModifierFromAffixes(UPHAttributeSet::GetAttackRangeAttribute());

	// Apply to UI
	SetStatRow(ArmourBox,      ArmourValue,      FinalArmor,false);
	SetStatRow(PoiseBox,       PoiseValue,       FinalPoise, false);
	SetStatRow(CritChanceBox,  CritChanceValue,  FinalCritChance, true);
	SetStatRow(AtkSpeedBox,    AtkSpeedValue,    FinalAttackSpeed, false);
	SetStatRow(CastTimeBox,    CastTimeValue,    FinalCastTime , false);
	SetStatRow(ManaCostBox,    ManaCostValue,    FinalManaCost, false);
	SetStatRow(StaminaCostBox, StaminaCostValue, FinalStaminaCost, false);
	SetStatRow(WeaponRangeBox, WeaponRangeValue, FinalWeaponRange, false);
}




void UEquippableStatsBox::CreateResistanceBoxes()
{
	if (!ResistanceBox || !ResistanceBoxContainer || !MinMaxBoxClass) return;

	ResistanceBoxContainer->ClearChildren();

	bool bHasAnyResistance = false;

	for (const TPair<EDefenseTypes, float>& Pair : ItemData.ItemData.ArmorBaseStats.Resistances)
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
