// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/ToolTip/ToolTip.h"
#include "Item/BaseItem.h"
#include "Item/WeaponItem.h"

void UToolTip::NativeConstruct()
{
	Super::NativeConstruct();

	InitializeToolTip();
}

void UToolTip::InitializeToolTip()
{
	SetItemGrade();
	ChangeColorByRarity();
	if (IsValid(ItemObj))
	{
		CheckDamageType();
		CheckStats();
		// Convert the enum to a string and then set it on the text block.
		const FString RarityText = GetRarityText(ItemObj->GetItemInfo().ItemRarity);
		Item_Rarity->SetText(FText::FromString(RarityText));
		SetDPSValue();
		SetTextDescription();
	}
}

// Function to check and calculate the damage types for an item.
void UToolTip::CheckDamageType() const
{
	// Verify ItemObj is not null to avoid potential crashes.
	if (ItemObj == nullptr) return;



	// Iterate through each element in the DamageStats to calculate total damages.
	if(ItemObj->GetItemInfo().ItemType == EItemType::IS_Weapon)
	{
		// Initialize variables to hold the total damages.
		float TotalElementalDamageMax = 0;
		float TotalElementalDamageMin = 0;
		float PhysicalDamageMax = 0;
		float PhysicalDamageMin = 0;
		const UWeaponItem* WeaponItem = Cast<UWeaponItem>(ItemObj);
		for (const auto& Elem : WeaponItem->GetWeaponData().DamageStats)
		{
			// Check the type of damage and accumulate the values accordingly.
			if (Elem.Key != EDamageTypes::DT_Physical) // Assuming non-physical damage is elemental.
			{
				TotalElementalDamageMax += Elem.Value.Max;
				TotalElementalDamageMin += Elem.Value.Min;
			}
			else // Physical damage accumulation.
			{
				PhysicalDamageMax += Elem.Value.Max;
				PhysicalDamageMin += Elem.Value.Min;
			}
		}

		// Update UI elements based on the calculated damages.
		UpdateUIDamageIndicators(TotalElementalDamageMax, TotalElementalDamageMin, PhysicalDamageMax, PhysicalDamageMin);
	}
}

// New function to handle UI updates based on damage values.
void UToolTip::UpdateUIDamageIndicators(float ElementalMax, float ElementalMin, float PhysicalMax, float PhysicalMin) const
{
	// Remove elemental damage box from parent if no elemental damage is present.
	if (ElementalMin == 0 && ElementalMax == 0)
	{
		EdBox->RemoveFromParent();
	}
	// Remove physical damage box from parent if no physical damage is present.
	if (PhysicalMax == 0 && PhysicalMin == 0)
	{
		PdBox->RemoveFromParent();
	}
}


// Defines a method to get a color based on the item's rarity.
FLinearColor UToolTip::GetColorBaseOnRarity() const
{
	FLinearColor colorToReturn; // Variable to hold the color that will be returned.

	// Switch statement based on the ItemGrade, which determines the color.
	switch (ItemGrade)
	{
	case EItemRarity::IR_GradeA: // Case for items of Grade A rarity.
		colorToReturn = FLinearColor(0.11f, 0.49f, 1.0f, 0.9f); // Sets color for Grade A items.
		break;
	case EItemRarity::IR_GradeB: // Case for items of Grade B rarity.
		colorToReturn = FLinearColor(0.41f, 0.032f, 0.5f, 0.9f); // Sets color for Grade B items.
		break;
	case EItemRarity::IR_GradeC: // Case for items of Grade C rarity.
		colorToReturn = FLinearColor(1.0f, 0.946f, 0.447f, 0.9f); // Sets color for Grade C items.
		break;
	case EItemRarity::IR_GradeD: // Case for items of Grade D rarity.
		colorToReturn = FLinearColor(0.16f, 0.86f, 1.0f, 0.9f); // Sets color for Grade D items.
		break;
	case EItemRarity::IR_GradeF: // Case for items of Grade F rarity.
		colorToReturn = FLinearColor(0.025f, 0.46f, 0.982f, 0.9f); // Sets color for Grade F items.
		break;
	case EItemRarity::IR_GradeS: // Case for items of Grade S rarity.
		colorToReturn = FLinearColor(1.0f, 0.0f, 0.0f, 0.9f); // Sets color for Grade S items.
		break;
	case EItemRarity::IR_Corrupted: // Case for corrupted items.
		colorToReturn = FLinearColor(0.0f, 0.0f, 0.0f, 0.9f); // Sets color for corrupted items.
		break;
	case EItemRarity::IR_None: // Case for items with no specified rarity.
		colorToReturn = FLinearColor(0.0f, 0.0f, 0.0f,0.9f); // Sets a default color for items with no rarity.
		break;
	case EItemRarity::IR_Unkown: // Case for items with unknown rarity.
		colorToReturn = FLinearColor(0.0251f, 0.462f, 0.982f, 0.6f); // Sets color for unknown rarity items.
		break;
	default: // Default case if none of the above cases match.
		colorToReturn = FLinearColor(0.0f, 0.0f, 0.0f, 0.1f); // Sets a default color if the item's rarity is not recognized.
		break;
	}
	return colorToReturn; // Returns the determined color.
}


void UToolTip::ChangeColorByRarity()
{
	// Get the color based on the item's rarity.
	const FLinearColor ColorToChange = GetColorBaseOnRarity();

	// Set the alpha for Spacer1 to 0.1 as requested, maintaining its original RGB values.
	FLinearColor SpacerColor = ColorToChange;
	SpacerColor.A = 0.8f; // Set alpha to 0.1

	// Apply colors and opacities. Spacer1 has reduced opacity, while others retain original.
	Spacer1->SetColorAndOpacity(SpacerColor); // Apply modified color to Spacer1
	Spacer2->SetColorAndOpacity(SpacerColor); // Apply original color to Spacer2
	Spacer3->SetColorAndOpacity(SpacerColor); // Apply original color to Spacer3
	ToolTipBackgroundImage->SetColorAndOpacity(ColorToChange); // Apply original color to the background image

}

void UToolTip::SetItemGrade()
{
	if (ItemObj)
	{
		ItemGrade = ItemObj->GetItemInfo().ItemRarity;
	}
}

void UToolTip::CheckStats() const
{
	if (ItemObj == nullptr) return; // Ensure the Item object is valid.

	if(ItemObj->GetItemInfo().ItemType == EItemType::IS_Armor || ItemObj->GetItemInfo().ItemType == EItemType::IS_Shield ||
		ItemObj->GetItemInfo().ItemType == EItemType::IS_Weapon)
	{
		const UEquippableItem* EquippableItem  = Cast<UEquippableItem>(ItemObj);
		// Check and conditionally remove the Armor UI element.
		CheckAndRemoveUIElement(EquippableItem->GetEquippableData().DefenseStats, EDefenseTypes::DT_Armor, ArmorSizeBox);

		// Combined stat checks for both required and general item stats.
		CheckAndRemoveStatUIElements();

		// Optionally, hide the requirements section if all related UI elements are not visible.
		UpdateRequirementsVisibility();
	}
}

void UToolTip::CheckAndRemoveUIElement(const TMap<EDefenseTypes, float>& StatsMap, EDefenseTypes StatType, USizeBox* UIElement)
{
	if (UIElement && StatsMap.Find(StatType) == nullptr || *StatsMap.Find(StatType) == 0.0f)
	{
		UIElement->RemoveFromParent();
	}
}

void UToolTip::CheckAndRemoveStatUIElements() const
{
	// Define stat checks for dynamic UI element visibility based on EItemStats.
		// In your class


	// Process general stats.
// Process general stats.
	for (TArray<FStatCheckInfo> GeneralStatChecks = {
		     {EItemStats::IS_CritChance, CritChanceSizeBox, CritChanceValue},
		     {EItemStats::IS_AttackSpeed, AttackSpeedSizeBox, AttackSpeedValue},
		     {EItemStats::IS_StaminaCost, StaminaCostSizeBox, StaminaCostValue},
		     {EItemStats::IS_WeaponRange, WeaponRangeSizeBox, WeaponRangeValue},
		     {EItemStats::IS_CastTime, CastTimeSizeBox, CastTimeValue},
		     {EItemStats::IS_ManaCost, ManaCostSizeBox, ManaCostValue},
	     }; const auto& [StatType, SizeBox, TextBlock] : GeneralStatChecks)
	{
		// Check if the corresponding SizeBox and TextBlock are valid pointers before using them.
		if (SizeBox && TextBlock)
		{
			// If the stat value is found, handle special formatting for certain stats like Crit Chance.
			if(ItemObj->GetItemInfo().ItemType == EItemType::IS_Armor || ItemObj->GetItemInfo().ItemType == EItemType::IS_Shield ||
			ItemObj->GetItemInfo().ItemType == EItemType::IS_Weapon)
			{
				const UEquippableItem* EquippableItem  = Cast<UEquippableItem>(ItemObj);
				if (const float* StatValue = EquippableItem->GetEquippableData().OverallStats.Find(StatType); StatValue != nullptr  && *StatValue != 0.0f)  // Ensure StatValue is not null before dereferencing it.
				{
					// Special handling for Crit Chance: convert it to a percentage and format with two decimal places.
					if (StatType == EItemStats::IS_CritChance)
					{
						const float LocalValue = *StatValue * 100;  // Convert the decimal to a percentage.
						TextBlock->SetText(FText::Format(FText::FromString(TEXT("{0:.2f}%")), LocalValue));
					}
					else
					{
						// For other stats, just display the value as is.
						TextBlock->SetText(FText::AsNumber(*StatValue));
					}

					// Ensure both UI elements are visible since we have a valid stat value.
					SizeBox->SetVisibility(ESlateVisibility::Visible);
					TextBlock->SetVisibility(ESlateVisibility::Visible);
				}
				else
				{
					// If the stat value is not found or is zero, remove the SizeBox from its parent and hide the TextBlock.
					// Note: We don't need to check *StatValue == 0.0f here because nullptr check is enough for not found.
					SizeBox->RemoveFromParent();
					TextBlock->SetVisibility(ESlateVisibility::Collapsed);
				}
			}
			else
			{
				// If the stat value is not found or is zero, remove the SizeBox from its parent and hide the TextBlock.
				// Note: We don't need to check *StatValue == 0.0f here because nullptr check is enough for not found.
				SizeBox->RemoveFromParent();
				TextBlock->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
	}

	// Define stat checks for required stats categories.
	TArray<TPair<EItemRequiredStatsCategory, USizeBox*>> RequiredStatChecks = {
		{EItemRequiredStatsCategory::ISC_RequiredStrength, StrSizeBox},
		{EItemRequiredStatsCategory::ISC_RequiredIntelligence, IntSizeBox},
		{EItemRequiredStatsCategory::ISC_RequiredDexterity, DexSizeBox},
		{EItemRequiredStatsCategory::ISC_RequiredAffliction, AffSizeBox},
		{EItemRequiredStatsCategory::ISC_RequiredCovenant, CovSizeBox},
		{EItemRequiredStatsCategory::ISC_RequiredLuck, LuckSizeBox},
		{EItemRequiredStatsCategory::ISC_RequiredEndurance, EndSizeBox}
	};

	// Process required stats.
	if(ItemObj->GetItemInfo().ItemType == EItemType::IS_Weapon)
	{
		const UWeaponItem* WeaponItem  = Cast<UWeaponItem>(ItemObj);
		for (const auto& StatCheck : RequiredStatChecks)
		{
			if (StatCheck.Value && WeaponItem->GetWeaponData().WeaponRequirementStats.Find(StatCheck.Key) == nullptr) 
			{
				StatCheck.Value->RemoveFromParent();
			}
		}
	}
}


void UToolTip::UpdateRequirementsVisibility() const
{
	bool bShouldHideRequirements = true;
	TArray<FStatRequirementInfo> StatBoxes = {
	{EItemRequiredStatsCategory::ISC_RequiredStrength, StrSizeBox, StrValue},
	{EItemRequiredStatsCategory::ISC_RequiredDexterity, DexSizeBox, DexValue},
	{EItemRequiredStatsCategory::ISC_RequiredIntelligence, IntSizeBox, INTValue},
	{EItemRequiredStatsCategory::ISC_RequiredAffliction, AffSizeBox, AffValue},
	{EItemRequiredStatsCategory::ISC_RequiredCovenant, CovSizeBox, CovValue},
	{EItemRequiredStatsCategory::ISC_RequiredEndurance, EndSizeBox, EndValue},
	{EItemRequiredStatsCategory::ISC_RequiredLuck, LuckSizeBox, LuckValue}
	};


	for (const FStatRequirementInfo& StatCheck : StatBoxes)
	{
		if (StatCheck.SizeBox && StatCheck.TextBlock)
		{
			if(ItemObj->GetItemInfo().ItemType == EItemType::IS_Weapon)
			{
				const UWeaponItem* WeaponItem  = Cast<UWeaponItem>(ItemObj);
				const float* StatValue = WeaponItem->GetWeaponData().WeaponRequirementStats.Find(StatCheck.StatType);
				if (StatValue != nullptr && *StatValue != 0.0f)
				{
					bShouldHideRequirements = false;
				}
				// If the stat value is not found, remove the SizeBox from its parent and hide the TextBlock.
				if (StatValue == nullptr || *StatValue == 0.0f)
				{
					StatCheck.SizeBox->RemoveFromParent();  // Remove the size box if the stat does not exist.
					StatCheck.TextBlock->SetVisibility(ESlateVisibility::Collapsed);  // Hide the text block if the stat does not exist.
				}
				else
				{
					// If the stat value is found, update the TextBlock with the stat value and ensure both UI elements are visible.

					StatCheck.TextBlock->SetText(FText::AsNumber(*StatValue));  // Set the text to the stat value.
					StatCheck.SizeBox->SetVisibility(ESlateVisibility::Visible);  // Ensure the size box is visible.
					StatCheck.TextBlock->SetVisibility(ESlateVisibility::Visible);  // Ensure the text block is visible.
				}
			}
			
		}
	}

	if (bShouldHideRequirements && RequirementsBox)
	{
		RequirementsBox->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UToolTip::SetTextDescription() const
{
	DescriptionText->SetText(ItemObj->GetItemInfo().ItemDescription);

	// Enable text wrapping and set a specific wrap width.
	DescriptionText->SetAutoWrapText(true); // Enable automatic text wrapping.
}

FString UToolTip::GetRarityText(const EItemRarity Rarity)
{
	switch (Rarity)
	{
	case EItemRarity::IR_GradeF:
		return TEXT("Grade F");
	case EItemRarity::IR_GradeD:
		return TEXT("Grade D");
	case EItemRarity::IR_GradeC:
		return TEXT("Grade C");
	case EItemRarity::IR_GradeB:
		return TEXT("Grade B");
	case EItemRarity::IR_GradeA:
		return TEXT("Grade A");
	case EItemRarity::IR_GradeS:
		return TEXT("Unknown");
	case EItemRarity::IR_Corrupted:
		return TEXT("!@#$%@%");
	case EItemRarity::IR_Unkown:
		return TEXT("########");
	default: ;
	}
	return {};
}

void UToolTip::UpdateRarity() const
{
	if (ItemObj && Item_Rarity)
	{
		// Convert the enum to a string and then set it on the text block.
		const FString RarityText = GetRarityText(ItemObj->GetItemInfo().ItemRarity);
		Item_Rarity->SetText(FText::FromString(RarityText));
	}
}

void UToolTip::SetDPSValue() const
{
	if (ItemObj)
	{
		if(ItemObj->GetItemInfo().ItemType == EItemType::IS_Weapon)
		{
			const UWeaponItem* WeaponItem  = Cast<UWeaponItem>(ItemObj);
			// Attempt to find the attack speed from the weapon stats.
			const float* AttackSpeedPtr = WeaponItem->GetEquippableData().OverallStats.Find(EItemStats::IS_AttackSpeed);
			const float AttackSpeed = AttackSpeedPtr ? *AttackSpeedPtr : 1.0f;

			// Initialize total DPS value.
			float TotalDPS = 0.0f;
			float TotalEDMin = 0.0f;
			float TotalEDMax = 0.0f;
			float TotalPDMin = 0.0f;
			float TotalPDMax = 0.0f;
			// Iterate through each element in DamageStats.
			for (const auto& Elem : WeaponItem->GetWeaponData().DamageStats)
			{
				// Each 'Elem' here is assumed to be a pair with a damage type and a struct containing min and max values.
				// Calculate the average damage for this type.
				const float AverageDamage = (Elem.Value.Min + Elem.Value.Max) / 2.0f;

				// Add the DPS for this damage type to the total DPS.0
				// DPS is average damage multiplied by attacks per second.
				TotalDPS += AverageDamage * AttackSpeed;
				if(Elem.Key != EDamageTypes::DT_Physical)
				{
					TotalEDMax += Elem.Value.Max;
					TotalEDMin += Elem.Value.Min;
				}
				else
				{
					TotalPDMax += Elem.Value.Max;
					TotalPDMin += Elem.Value.Min;
				}
			
			}

			// Now, 'TotalDPS' holds the combined DPS for all damage types, adjusted by attack speed.
			// Set this value to the corresponding UI element. Assuming you have a UTextBlock* for DPS display:
			if (DPSValue) 
			{
				// Convert the float DPS value to text and set it.
				DPSValue->SetText(FText::AsNumber(TotalDPS));
			}
			const FText PDText = FText::Format(NSLOCTEXT("Total Physical Damage", "PDRange", "{0} - {1}"),
											   FText::AsNumber(TotalPDMin),
											   FText::AsNumber(TotalPDMax));

			const FText EDText = FText::Format(NSLOCTEXT("Total Elemental Damage", "EDRange", "{0} - {1}"),
											   FText::AsNumber(TotalEDMin),
											   FText::AsNumber(TotalEDMax));

			PdValue->SetText(PDText);
			EdValue->SetText(EDText);
		}
	}
}