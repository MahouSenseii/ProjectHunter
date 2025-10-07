// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/ToolTip/RequirementsBox.h"
#include "AbilitySystem/PHAttributeSet.h"
#include "Character/PHBaseCharacter.h"

void URequirementsBox::SetItemRequirements(const FEquippableItemData& PassedItemData, APHBaseCharacter* Character)
{
	if (!Character)
	{
		UE_LOG(LogTemp, Warning, TEXT("Character is NULL!"));
		return;
	}

	const FItemStatRequirement& Requirements = PassedItemData.StatRequirements;
	const UPHAttributeSet* AttributeSet = Cast<UPHAttributeSet>(Character->GetAttributeSet());

	if (!AttributeSet)
	{
		UE_LOG(LogTemp, Warning, TEXT("Character does not have an AttributeSet!"));
		return;
	}

	bool bHasAnyRequirement = false;

	auto ShowRequirement = [&bHasAnyRequirement](UHorizontalBox* Box, UTextBlock* TextBlock, float RequiredValue, float CurrentValue)
	{
		if (!Box || !TextBlock) return;

		if (RequiredValue > 0)
		{
			bHasAnyRequirement = true;

			Box->SetVisibility(ESlateVisibility::Visible);

			const FText DisplayText = FText::AsNumber(FMath::RoundToInt(RequiredValue));

			TextBlock->SetText(DisplayText);
			TextBlock->SetColorAndOpacity(
				(CurrentValue >= RequiredValue) ? FSlateColor(FLinearColor::Green)
												: FSlateColor(FLinearColor::Red));
		}
		else
		{
			Box->SetVisibility(ESlateVisibility::Collapsed);
		}
	};

	// Check each individual requirement stat
	ShowRequirement(STRBox, STRValue, Requirements.RequiredStrength, AttributeSet->GetStrength());
	ShowRequirement(INTBox, INTValue, Requirements.RequiredIntelligence, AttributeSet->GetIntelligence());
	ShowRequirement(DEXBox, DEXValue, Requirements.RequiredDexterity, AttributeSet->GetDexterity());
	ShowRequirement(ENDBox, ENDValue, Requirements.RequiredEndurance, AttributeSet->GetEndurance());
	ShowRequirement(AFFBox, AFFValue, Requirements.RequiredAffliction, AttributeSet->GetAffliction());
	ShowRequirement(LUCKBox, LUCKValue, Requirements.RequiredLuck, AttributeSet->GetLuck());
	ShowRequirement(COVBox, COVValue, Requirements.RequiredCovenant, AttributeSet->GetCovenant());

	// Remove the entire box if no requirement is present
	if (!bHasAnyRequirement)
	{
		this->RemoveFromParent();
	}
}



