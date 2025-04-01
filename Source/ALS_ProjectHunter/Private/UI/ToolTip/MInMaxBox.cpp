// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/ToolTip/MinMaxBox.h"

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
