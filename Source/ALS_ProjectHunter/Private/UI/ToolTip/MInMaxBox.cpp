// Fill out your copyright notice in the Description page of Project Settings.


#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "UI/ToolTip/MinMaxBox.h"

void UMinMaxBox::Init()
{
	// Create Root HorizontalBox
	RootBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("RootBox"));
	WidgetTree->RootWidget = RootBox;

	// Create Min Text
	MinText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MinText"));
	MinText->SetText(FText::FromString(TEXT("Min")));
	UHorizontalBoxSlot* MinSlot = RootBox->AddChildToHorizontalBox(MinText);
	MinSlot->SetPadding(FMargin(5.f));
	MinSlot->SetHorizontalAlignment(HAlign_Right);


	SpacerText  = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	SpacerText->SetText(FText::FromString(TEXT("/")));
	
	// Create Max Text
	MaxText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MaxText"));
	MaxText->SetText(FText::FromString(TEXT("Max")));
	UHorizontalBoxSlot* MaxSlot = RootBox->AddChildToHorizontalBox(MaxText);
	MaxSlot->SetPadding(FMargin(5.f));
	MaxSlot->SetHorizontalAlignment(HAlign_Right);
}

void UMinMaxBox::SetColorBaseOnType(const EDamageTypes Type) const
{
	FLinearColor Color;

	switch (Type)
	{
	case EDamageTypes::DT_Physical:   Color = FLinearColor::White; break;
	case EDamageTypes::DT_Fire:       Color = FLinearColor::Red; break;

	// Light blue
	case EDamageTypes::DT_Ice:        Color = FLinearColor(0.6f, 0.8f, 1.0f, 1.0f); break;

	// Soft White/Goldish
	case EDamageTypes::DT_Light:      Color = FLinearColor(1.0f, 1.0f, 0.9f, 1.0f); break; 
	case EDamageTypes::DT_Lightning:  Color = FLinearColor::Yellow; break;
	case EDamageTypes::DT_Corruption: Color = FLinearColor::Black; break;
	default:                          Color = FLinearColor::White; break;
	}

	if (MinText)     MinText->SetColorAndOpacity(Color);
	if (MaxText)     MaxText->SetColorAndOpacity(Color);
	if (SpacerText)  SpacerText->SetColorAndOpacity(Color);
}

void UMinMaxBox::SetMinMaxText(const float MaxValue, const float MinValue) const
{
	if (MinText) MinText->SetText(FText::AsNumber(MinValue));
	if (MaxText) MaxText->SetText(FText::AsNumber(MaxValue));
}

void UMinMaxBox::SetFontSize(float InValue) const
{
	if (MinText)
	{
		FSlateFontInfo FontInfo = MinText->GetFont();
		FontInfo.Size = InValue;
		MinText->SetFont(FontInfo);
	}

	if(SpacerText)
	{
		FSlateFontInfo FontInfo = SpacerText->GetFont();
		FontInfo.Size = InValue;
		SpacerText->SetFont(FontInfo);
	}

	if (MaxText)
	{
		FSlateFontInfo FontInfo = MaxText->GetFont();
		FontInfo.Size = InValue;
		MaxText->SetFont(FontInfo);
	}
}
