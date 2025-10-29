// EquippableToolTip.cpp

#include "UI/ToolTip/EquippableToolTip.h"
#include "Library/PHItemFunctionLibrary.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "UI/ToolTip/RequirementsBox.h"

void UEquippableToolTip::NativeConstruct()
{
    Super::NativeConstruct();
}

void UEquippableToolTip::InitializeToolTip()
{
    Super::InitializeToolTip();

    if (!ItemDefinition)
    {
        UE_LOG(LogTemp, Warning, TEXT("EquippableToolTip: No item definition"));
        return;
    }
    
    if (LoreText)
    {
        LoreText->SetText(ItemDefinition->Base.ItemDescription);
        const FSlateFontInfo FontInfo = FCoreStyle::GetDefaultFontStyle("Regular", 32);
        LoreText->SetFont(FontInfo);
    }

    // Update stats display
    UpdateStatsDisplay();

    // Create affix boxes from instance data
    CreateAffixBox();
}

void UEquippableToolTip::SetItemInfo( UItemDefinitionAsset* Definition,  FItemInstanceData& InInstanceData)
{
    // Call parent to set definition and instance
    Super::SetItemInfo(Definition, InInstanceData);

    // Set equippable-specific data
    if (StatsBox && ItemDefinition)
    { 
        StatsBox->SetItemData(ItemDefinition, InInstanceData);
    }
}

void UEquippableToolTip::UpdateStatsDisplay() const
{
    if (!StatsBox || !ItemDefinition) return;

    StatsBox->SetItemData(ItemDefinition, InstanceData);
    StatsBox->CreateMinMaxBoxByDamageTypes();
    StatsBox->SetMinMaxForOtherStats();
    StatsBox->CreateResistanceBoxes();
    
    if (StatsBox->GetRequirementsBox())
    {
        StatsBox->GetRequirementsBox()->SetItemRequirements(ItemDefinition->Equip, OwnerCharacter);
    }
}

void UEquippableToolTip::CreateAffixBox()
{
    if (!AffixBoxContainer) return;

    AffixBoxContainer->ClearChildren();

    //  Get affixes from INSTANCE DATA
    const TArray<FPHAttributeData>& Implicits = InstanceData.Implicits;
    const TArray<FPHAttributeData>& Prefixes = InstanceData.Prefixes;
    const TArray<FPHAttributeData>& Suffixes = InstanceData.Suffixes;
    const TArray<FPHAttributeData>& Crafted = InstanceData.Crafted;

    //  Helper to create text blocks - all same color (white)
    auto AddAffixList = [this](const TArray<FPHAttributeData>& AffixList)
    {
        for (const FPHAttributeData& Attr : AffixList)
        {
            const FText FormattedText = UPHItemFunctionLibrary::FormatAttributeText(Attr);

            UTextBlock* TextBlock = NewObject<UTextBlock>(this);
            if (!TextBlock) continue;

            TextBlock->SetText(FormattedText);
            
            FSlateFontInfo FontInfo = FCoreStyle::GetDefaultFontStyle("Regular", 12);
            TextBlock->SetFont(FontInfo);
            
            // Keep all affix text white - the tooltip border/bg shows rarity
            TextBlock->SetColorAndOpacity(FSlateColor(FLinearColor::White));

            AffixBoxContainer->AddChildToVerticalBox(TextBlock);
        }
    };


    AddAffixList(Implicits);
    AddAffixList(Prefixes);
    AddAffixList(Suffixes);
    AddAffixList(Crafted);
}