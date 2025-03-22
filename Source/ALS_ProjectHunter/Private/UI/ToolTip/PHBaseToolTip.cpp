// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/ToolTip/PHBaseToolTip.h"
#include "Library/PHItemEnumLibrary.h"

void UPHBaseToolTip::NativeConstruct()
{
	Super::NativeConstruct();

	InitializeToolTip();
}

void UPHBaseToolTip::InitializeToolTip()
{
	ItemGrade = ItemObj->GetItemInfo().ItemRarity;
}

FString UPHBaseToolTip::GetRarityText(const EItemRarity Rarity)
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

void UPHBaseToolTip::ChangeColorByRarity()
{
	// Get the color based on the item's rarity.
	const FLinearColor ColorToChange = GetColorBaseOnRarity();
	ToolTipBackgroundImage->SetColorAndOpacity(ColorToChange); // Apply original color to the background image
}

FLinearColor UPHBaseToolTip::GetColorBaseOnRarity() const
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
