// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/ToolTip/RequirementsBox.h"

#include "AbilitySystem/PHAttributeSet.h"
#include "Character/PHBaseCharacter.h"

void URequirementsBox::GetItemRequirements(const UEquippableItem* Item,APHBaseCharacter* Character) const
{
	if (!Item || !Character)
    {
        UE_LOG(LogTemp, Warning, TEXT("Item or Character is NULL!"));
        return;
    }

    const FItemStatRequirement& Requirements = Item->GetStatRequirements();
    const UPHAttributeSet* AttributeSet = Cast<UPHAttributeSet>(Character->GetAttributeSet());

    if (!AttributeSet)
    {
        UE_LOG(LogTemp, Warning, TEXT("Character does not have an AttributeSet!"));
        return;
    }

    // Lambda function to handle showing/hiding requirement UI elements
    auto ShowRequirement = [](UHorizontalBox* Box, UTextBlock* TextBlock, float RequiredValue, float CurrentValue)
    {
        if (RequiredValue > 0)
        {
            if (Box) Box->SetVisibility(ESlateVisibility::Visible);
            if (TextBlock) 
            {
                const FText DisplayText = FText::Format(
                    FText::FromString(TEXT("{0} / {1}")),
                    FText::AsNumber(FMath::RoundToInt(CurrentValue)),
                    FText::AsNumber(FMath::RoundToInt(RequiredValue))
                );

                TextBlock->SetText(DisplayText);

                // Change color based on whether requirement is met
                if (CurrentValue >= RequiredValue)
                {
                    TextBlock->SetColorAndOpacity(FSlateColor(FLinearColor::Green));
                }
                else
                {
                    TextBlock->SetColorAndOpacity(FSlateColor(FLinearColor::Red));
                }
            }
        }
        else
        {
            if (Box) Box->SetVisibility(ESlateVisibility::Collapsed);
        }
    };

    // Apply requirements to the UI
    ShowRequirement(STRBox, STRValue, Requirements.RequiredStrength, AttributeSet->GetStrength());
    ShowRequirement(INTBox, INTValue, Requirements.RequiredIntelligence, AttributeSet->GetIntelligence());
    ShowRequirement(DEXBox, DEXValue, Requirements.RequiredDexterity, AttributeSet->GetDexterity());
    ShowRequirement(ENDBox, ENDValue, Requirements.RequiredEndurance, AttributeSet->GetEndurance());
    ShowRequirement(AFFBox, AFFValue, Requirements.RequiredAffliction, AttributeSet->GetAffliction());
    ShowRequirement(LUCKBox, LUCKValue, Requirements.RequiredLuck, AttributeSet->GetLuck());
    ShowRequirement(COVBox, COVValue, Requirements.RequiredCovenant, AttributeSet->GetCovenant());
    ShowRequirement(LVLBox, LVLValue, Requirements.RequiredLevel, Character->GetPlayerLevel());
}
