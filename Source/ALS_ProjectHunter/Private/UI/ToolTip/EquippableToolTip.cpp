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

	if(ItemInfo.IsValid())
	{
		StatsBox->SetItemData(ItemInfo);
	}
	StatsBox->CreateMinMaxBoxByDamageTypes();
	StatsBox->SetMinMaxForOtherStats();
	StatsBox->CreateResistanceBoxes();
	StatsBox->GetRequirementsBox()->SetItemRequirements(ItemInfo.ItemData, OwnerCharacter);
	CreateAffixBox();
	LoreText->SetText(ItemInfo.ItemInfo.ItemDescription);
}

void UEquippableToolTip::SetItemInfo(const FItemInformation& Item)
{
	Super::SetItemInfo(Item);

	if (StatsBox)
	{
			StatsBox->SetEquippableItem(ItemInfo.ItemData);
	}
}

void UEquippableToolTip::CreateAffixBox()
{
	if (!AffixBoxContainer) return;

	AffixBoxContainer->ClearChildren();

	const FPHItemStats& Affixes = ItemInfo.ItemData.Affixes;

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


