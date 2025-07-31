// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/DamagePopup.h"

void UDamagePopup::SetDamageData(int32 InAmount, EDamageTypes InType, bool bIsCrit)
{
	DamageAmount = InAmount;
	DamageType = InType;
	bIsCritical = bIsCrit;

	if (DamageText)
	{
		DamageText->SetText(FText::AsNumber(DamageAmount));
		DamageText->SetColorAndOpacity(FSlateColor(GetColorForDamageType(DamageType)));

		// Apply outline if crit
		FSlateFontInfo FontInfo = DamageText->Font;

		if (bIsCritical)
		{
			FontInfo.OutlineSettings.OutlineSize = 2; // Thickness of outline
			FontInfo.OutlineSettings.OutlineColor = FLinearColor::Yellow; // Or any color
		}
		else
		{
			FontInfo.OutlineSettings.OutlineSize = 0; // No outline for normal hits
		}

		DamageText->SetFont(FontInfo);
	}

	if (CritImage)
	{
		CritImage->SetVisibility(bIsCritical ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	if (PopupAnimation)
	{
		PlayAnimation(PopupAnimation);
	}
}

FLinearColor UDamagePopup::GetColorForDamageType(EDamageTypes InType)
{
	switch (InType)
	{
	case EDamageTypes::DT_Fire:       return FLinearColor::Red;
	case EDamageTypes::DT_Ice:        return FLinearColor::Blue;
	case EDamageTypes::DT_Lightning:  return FLinearColor(0.3f, 0.8f, 1.f);
	case EDamageTypes::DT_Light:      return FLinearColor::Yellow;
	case EDamageTypes::DT_Corruption: return FLinearColor(0.5f, 0.f, 0.5f);
	default:                          return FLinearColor::White;
	}
}