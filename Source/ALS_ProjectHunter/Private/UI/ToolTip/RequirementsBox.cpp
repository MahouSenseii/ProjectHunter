// RequirementsBox.cpp
#include "UI/ToolTip/RequirementsBox.h"
#include "AbilitySystem/PHAttributeSet.h"
#include "Character/PHBaseCharacter.h"
#include "Components/TextBlock.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"

void URequirementsBox::NativeConstruct()
{
    Super::NativeConstruct();

}

void URequirementsBox::SetItemRequirements(const FEquippableItemData& PassedItemData, APHBaseCharacter* Character)
{
    if (!Character)
    {
        UE_LOG(LogTemp, Warning, TEXT("Character is NULL!"));
        return;
    }

    const UPHAttributeSet* AttributeSet = Cast<UPHAttributeSet>(Character->GetAttributeSet());
    if (!AttributeSet)
    {
        UE_LOG(LogTemp, Warning, TEXT("Character does not have an AttributeSet!"));
        return;
    }

    if (!RequirementsBox)
    {
        UE_LOG(LogTemp, Warning, TEXT("RequirementsBox is NULL!"));
        return;
    }

    // Clear any existing children
    RequirementsBox->ClearChildren();

    const FItemStatRequirement& Requirements = PassedItemData.StatRequirements;

    // Define all possible requirements
    struct FStatInfo
    {
        FString Name;
        float Required;
        float Current;
    };

    TArray<FStatInfo> Stats = {
        {"STR", Requirements.RequiredStrength, AttributeSet->GetStrength()},
        {"END", Requirements.RequiredEndurance, AttributeSet->GetEndurance()},
        {"INT", Requirements.RequiredIntelligence, AttributeSet->GetIntelligence()},
        {"DEX", Requirements.RequiredDexterity, AttributeSet->GetDexterity()},
        {"AFF", Requirements.RequiredAffliction, AttributeSet->GetAffliction()},
        {"LUCK", Requirements.RequiredLuck, AttributeSet->GetLuck()},
        {"COV", Requirements.RequiredCovenant, AttributeSet->GetCovenant()}
    };

    bool bHasAnyRequirement = false;

    // Only add stats that have requirements
    for (const FStatInfo& Stat : Stats)
    {
        if (Stat.Required > 0)
        {
            bHasAnyRequirement = true;
            AddRequirementElement(Stat.Name, Stat.Required, Stat.Current);
        }
    }

    // Remove the widget if there are no requirements
    if (!bHasAnyRequirement)
    {
        this->RemoveFromParent();
    }
}

void URequirementsBox::AddRequirementElement(const FString& StatName, float RequiredValue, float CurrentValue)
{
    // Create stat name text block
    UTextBlock* StatNameText = NewObject<UTextBlock>(this);
    StatNameText->SetText(FText::FromString(StatName + ":"));
    StatNameText->SetFont(RequirementFont);
    StatNameText->SetColorAndOpacity(FSlateColor(FLinearColor::White));

    // Create stat value text block
    UTextBlock* StatValueText = NewObject<UTextBlock>(this);
    StatValueText->SetText(FText::AsNumber(FMath::RoundToInt(RequiredValue)));
    StatValueText->SetFont(RequirementFont);
    
    // Set color based on whether requirement is met
    FLinearColor TextColor = (CurrentValue >= RequiredValue) ? RequirementMetColor : RequirementNotMetColor;
    StatValueText->SetColorAndOpacity(FSlateColor(TextColor));

    // Add to horizontal box
    UHorizontalBoxSlot* NameSlot = RequirementsBox->AddChildToHorizontalBox(StatNameText);
    NameSlot->SetPadding(RequirementPadding);
    
    UHorizontalBoxSlot* ValueSlot = RequirementsBox->AddChildToHorizontalBox(StatValueText);
    ValueSlot->SetPadding(FMargin(5.0f, 0.0f, 15.0f, 0.0f)); // Small gap after value
}