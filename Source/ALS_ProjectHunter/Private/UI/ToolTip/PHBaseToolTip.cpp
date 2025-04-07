// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/ToolTip/PHBaseToolTip.h"
#include "Library/PHItemEnumLibrary.h"

void UPHBaseToolTip::NativePreConstruct()
{
	Super::NativePreConstruct();

	RarityColors.Add(EItemRarity::IR_GradeF, FLinearColor(0.025f, 0.46f, 0.982f, 1.0f));
	RarityColors.Add(EItemRarity::IR_GradeD, FLinearColor(0.16f, 0.86f, 1.0f, 1.0f));
	RarityColors.Add(EItemRarity::IR_GradeC, FLinearColor(1.0f, 0.946f, 0.447f, 1.0f));
	RarityColors.Add(EItemRarity::IR_GradeB, FLinearColor(0.41f, 0.032f, 0.5f, 1.0f));
	RarityColors.Add(EItemRarity::IR_GradeA, FLinearColor(0.11f, 0.49f, 1.0f, 1.0f));
	RarityColors.Add(EItemRarity::IR_GradeS, FLinearColor(1.0f, 0.0f, 0.0f, 1.0f));
	RarityColors.Add(EItemRarity::IR_Corrupted, FLinearColor::Black);
	RarityColors.Add(EItemRarity::IR_Unkown, FLinearColor(0.0251f, 0.462f, 0.982f, 1.0f));
	RarityColors.Add(EItemRarity::IR_None, FLinearColor(0.f, 0.f, 0.f, 1.0f));
}

void UPHBaseToolTip::NativeConstruct()
{
	Super::NativeConstruct();
	
	InitializeToolTip();
}

void UPHBaseToolTip::InitializeToolTip()
{
	if (!ItemInfo.ItemData.ItemName.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("InitializeToolTip: ItemObj is null."));
		return;
	}

	ItemGrade =  ItemInfo.ItemInfo.ItemRarity;
	ChangeColorByRarity();

	if (ItemName)
	{
		ItemName->SetText( ItemInfo.ItemInfo.ItemName);
	}
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
	if (!ToolTipBGMaterial && !ToolTipBGMaterial2)
	{
		ToolTipBGMaterial = ToolTipBackgroundImage->GetDynamicMaterial();
		ToolTipBGMaterial2 = ToolTipBackgroundImageFlicker->GetDynamicMaterial();
	}

	if (ToolTipBGMaterial && ToolTipBGMaterial2)
	{
		const FLinearColor ColorToChange = GetColorBaseOnRarity();
		ToolTipBGMaterial->SetVectorParameterValue("ImageTint", ColorToChange);
		ToolTipBGMaterial2->SetVectorParameterValue("ImageTint", ColorToChange);
	}
}

FLinearColor UPHBaseToolTip::GetColorBaseOnRarity()
{
	// Make sure the color map contains the current item grade
	if (const FLinearColor* FoundColor = RarityColors.Find(ItemGrade))
	{
		return *FoundColor;
	}

	// Fallback color if not found
	return FLinearColor(0.f, 0.f, 0.f, 0.1f);
}

void UPHBaseToolTip::SetItemInfo(const FItemInformation& Item)
{
	 ItemInfo = Item;
	ItemGrade =  ItemInfo.ItemInfo.ItemRarity;
	ChangeColorByRarity();
}
