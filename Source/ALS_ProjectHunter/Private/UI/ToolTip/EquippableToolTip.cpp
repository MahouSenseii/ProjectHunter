// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/ToolTip/EquippableToolTip.h"
#include "Library/PHItemFunctionLibrary.h"

void UEquippableToolTip::NativeConstruct()
{	
	InitializeToolTip();
}

void UEquippableToolTip::InitializeToolTip()
{
	Super::InitializeToolTip();

	if(ItemInfo->Base.IsValid())
	{
		StatsBox->SetItemData(ItemInfo);
	}
	StatsBox->CreateMinMaxBoxByDamageTypes();
	StatsBox->SetMinMaxForOtherStats();
	StatsBox->CreateResistanceBoxes();
	StatsBox->GetRequirementsBox()->SetItemRequirements(ItemInfo->Equip, OwnerCharacter);
	CreateAffixBox();
	LoreText->SetText(ItemInfo->Base.ItemDescription);
}

void UEquippableToolTip::SetItemInfo( UItemDefinitionAsset*& Item)
{
	Super::SetItemInfo(Item);

	if (StatsBox && ItemInfo->Base.IsValid())
	{
			StatsBox->SetEquippableItem(ItemInfo->Equip);
	}
}



const FItemInstanceData& UPHBaseToolTip::GetInstanceData() const
{
	static FItemInstanceData Empty;
	return ItemObject ? ItemObject->RuntimeData : Empty;
}


void UPHBaseToolTip::UpdateTooltipDisplay()
{
	if (!ItemInstance) return;

	const UItemDefinitionAsset* Def = ItemInstance->ItemDefinition;	
	const FItemInstanceData& Instance = ItemInstance->RuntimeData;
	
}

void UEquippableToolTip::CreateAffixBox()
{
	if (!AffixBoxContainer) return;

	AffixBoxContainer->ClearChildren();

	const FPHItemStats& Affixes = ItemInfo->Equip.Affixes;

	// Helper to create a text block for each attribute
	auto AddAffixList = [this](const TArray<FPHAttributeData>& AffixList)
	{
		for (const FPHAttributeData& Attr : AffixList)
		{
			const FText FormattedText = UPHItemFunctionLibrary::FormatAttributeText(Attr);

			UTextBlock* TextBlock = NewObject<UTextBlock>(this);
			if (!TextBlock) continue;

			TextBlock->SetText(FormattedText);
			TextBlock->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 12));
			TextBlock->SetColorAndOpacity(FSlateColor(FLinearColor::White));

			AffixBoxContainer->AddChildToVerticalBox(TextBlock);
		}
	};

	AddAffixList(Affixes.Implicits);
	AddAffixList(Affixes.Prefixes);
	AddAffixList(Affixes.Suffixes);
	AddAffixList(Affixes.Crafted);
}


void UEquippableToolTip::UpdateTooltipDisplay()
{
    Super::UpdateTooltipDisplay();

    if (!ItemObject) return;

    // ✅ Update name (from instance or definition)
    if (ItemNameText)
    {
        ItemNameText->SetText(GetDisplayName());
    }

    // ✅ Update rarity (from instance or definition)
    if (RarityText)
    {
        EItemRarity Rarity = GetRarity();
        FText RarityDisplayText = UEnum::GetDisplayValueAsText(Rarity);
        RarityText->SetText(RarityDisplayText);
        
        // Set color based on rarity
        FLinearColor RarityColor = GetRarityColor(Rarity);
        RarityText->SetColorAndOpacity(FSlateColor(RarityColor));
    }

    // ✅ Display affixes from instance data
    if (StatsContainer)
    {
        StatsContainer->ClearChildren();

        const FItemInstanceData& Instance = GetInstanceData();

        // Display prefixes
        for (const FPHAttributeData& Prefix : Instance.Prefixes)
        {
            AddStatToContainer(Prefix);
        }

        // Display suffixes
        for (const FPHAttributeData& Suffix : Instance.Suffixes)
        {
            AddStatToContainer(Suffix);
        }

        // Display implicits
        for (const FPHAttributeData& Implicit : Instance.Implicits)
        {
            AddStatToContainer(Implicit);
        }
    }

    // ✅ Compare with equipped item if owner character exists
    if (OwnerCharacter)
    {
        CompareWithEquippedItem();
    }
	
}

void UEquippableToolTip::AddStatToContainer(const FPHAttributeData& Stat)
{
    if (!StatsContainer) return;

    // Format the stat text
    FText StatText = UPHItemFunctionLibrary::FormatAttributeText(Stat);

    // Create a text widget for the stat
    UTextBlock* StatWidget = NewObject<UTextBlock>(this);
    StatWidget->SetText(StatText);
    
    // Color based on identification
    FLinearColor StatColor = Stat.bIsIdentified ? 
        FLinearColor::White : 
        FLinearColor(0.5f, 0.5f, 0.5f);
    StatWidget->SetColorAndOpacity(FSlateColor(StatColor));

    StatsContainer->AddChildToVerticalBox(StatWidget);
}

void UEquippableToolTip::CompareWithEquippedItem()
{
	//TODO::
    // Compare with currently equipped item in same slot
    // Show stat differences, etc.
}

FLinearColor UEquippableToolTip::GetRarityColor(EItemRarity Rarity)
{
    switch (Rarity)
    {
    case EItemRarity::IR_GradeS: return FLinearColor(1.0f, 0.5f, 0.0f);  // Orange
    case EItemRarity::IR_GradeA: return FLinearColor(1.0f, 0.0f, 1.0f);  // Magenta
    case EItemRarity::IR_GradeB: return FLinearColor(0.0f, 0.5f, 1.0f);  // Blue
    case EItemRarity::IR_GradeC: return FLinearColor(0.0f, 1.0f, 0.0f);  // Green
    case EItemRarity::IR_GradeD: return FLinearColor(1.0f, 1.0f, 0.0f);  // Yellow
    case EItemRarity::IR_GradeF: return FLinearColor(0.7f, 0.7f, 0.7f);  // Gray
    default: return FLinearColor::White;
    }
}
