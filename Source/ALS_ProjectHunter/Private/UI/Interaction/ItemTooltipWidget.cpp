#include "UI/Interaction/ItemTooltipWidget.h"

#include "Animation/WidgetAnimation.h"
#include "Blueprint/WidgetTree.h"
#include "Character/PHBaseCharacter.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Equipment/Components/EquipmentManager.h"
#include "UI/Interaction/ItemTooltipSectionWidget.h"
#include "Item/Library/FunctionLibraries/ItemTooltipFunctionLibrary.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace ItemTooltipWidgetPrivate
{
	const FSlateColor PureWhiteText(FLinearColor::White);
	const FName SquareFlickerWidgetName(TEXT("Image_SquareFlicker"));
	const FName BackgroundWidgetName(TEXT("Image_Background"));
	const FName SquareTintParameterName(TEXT("SquareTint"));
	const FName ImageTintParameterName(TEXT("ImageTint"));

	void ApplyMaterialTint(
		UWidgetTree* WidgetTree,
		const FName WidgetName,
		const FName ParameterName,
		const FLinearColor& Color)
	{
		if (!WidgetTree)
		{
			return;
		}

		UImage* Image = WidgetTree->FindWidget<UImage>(WidgetName);
		if (!Image)
		{
			return;
		}

		// Keep the brush tint neutral so the material parameter supplies the exact grade color.
		Image->SetColorAndOpacity(FLinearColor::White);

		if (UMaterialInstanceDynamic* DynamicMaterial = Image->GetDynamicMaterial())
		{
			DynamicMaterial->SetVectorParameterValue(ParameterName, Color);
		}
	}

	FText MakeFallbackLineText(const FItemTooltipLine& Line)
	{
		if (Line.bUseValueColumn && !Line.Value.IsEmpty())
		{
			return FText::Format(
				FText::FromString(TEXT("{0}: {1}")),
				Line.Label,
				Line.Value);
		}

		return Line.Label;
	}
}

void UItemTooltipWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (OpenCloseAnimation)
	{
		UnbindAllFromAnimationFinished(OpenCloseAnimation);

		FWidgetAnimationDynamicEvent FinishedEvent;
		FinishedEvent.BindDynamic(this, &UItemTooltipWidget::HandleCloseAnimationFinished);
		BindToAnimationFinished(OpenCloseAnimation, FinishedEvent);
	}
}

void UItemTooltipWidget::NativeDestruct()
{
	if (OpenCloseAnimation)
	{
		UnbindAllFromAnimationFinished(OpenCloseAnimation);
	}

	bCloseRequested = false;
	Super::NativeDestruct();
}

void UItemTooltipWidget::UpdateTooltip(UItemInstance* Item)
{
	if (!Item)
	{
		ClearTooltip();
		return;
	}

	const APHBaseCharacter* ViewerCharacter = Cast<APHBaseCharacter>(GetOwningPlayerPawn());
	UEquipmentManager* ViewerEquipmentManager = ViewerCharacter
		? ViewerCharacter->GetEquipmentManager()
		: nullptr;

	if (!UItemTooltipFunctionLibrary::BuildItemTooltipDataForViewer(
		Item,
		ViewerEquipmentManager,
		TooltipData))
	{
		ClearTooltip();
		return;
	}

	DisplayedItem = Item;

	if (ItemNameText)
	{
		ItemNameText->SetText(TooltipData.DisplayName);
	}

	if (ItemTypeText)
	{
		ItemTypeText->SetText(FText::Format(
			FText::FromString(TEXT("{0} - {1}")),
			TooltipData.RarityName,
			TooltipData.ItemSubTypeName.IsEmpty() ? TooltipData.ItemTypeName : TooltipData.ItemSubTypeName));
		ItemTypeText->SetColorAndOpacity(ItemTooltipWidgetPrivate::PureWhiteText);
	}

	if (ItemIconImage && TooltipData.Icon)
	{
		// bMatchSize=false: the WBP controls the icon box, not the source texture.
		ItemIconImage->SetBrushFromTexture(TooltipData.Icon, /*bMatchSize=*/false);
	}

	SetGradeVisuals(TooltipData.Rarity);
	PopulateSections();
	OnTooltipDataUpdated(TooltipData);
	OnTooltipUpdated(Item);
}

void UItemTooltipWidget::ShowAnimated()
{
	const bool bWasHidden = GetVisibility() == ESlateVisibility::Collapsed
		|| GetVisibility() == ESlateVisibility::Hidden;
	const bool bWasClosing = bCloseRequested;

	bCloseRequested = false;

	SetVisibility(ESlateVisibility::HitTestInvisible);

	if ((bWasHidden || bWasClosing) && OpenCloseAnimation)
	{
		PlayAnimationForward(OpenCloseAnimation);
	}
}

void UItemTooltipWidget::HideAnimated()
{
	if (GetVisibility() == ESlateVisibility::Collapsed || bCloseRequested)
	{
		return;
	}

	bCloseRequested = true;

	if (OpenCloseAnimation)
	{
		PlayAnimationReverse(OpenCloseAnimation);
		return;
	}

	HandleCloseAnimationFinished();
}

void UItemTooltipWidget::HandleCloseAnimationFinished()
{
	if (!bCloseRequested)
	{
		return;
	}

	bCloseRequested = false;
	SetVisibility(ESlateVisibility::Collapsed);
}

void UItemTooltipWidget::ClearTooltip()
{
	DisplayedItem.Reset();
	TooltipData = FItemTooltipData();

	if (SectionsContainer)
	{
		SectionsContainer->ClearChildren();
	}

	if (ItemNameText)
	{
		ItemNameText->SetText(FText::GetEmpty());
	}

	if (ItemTypeText)
	{
		ItemTypeText->SetText(FText::GetEmpty());
	}

	OnTooltipCleared();
}

void UItemTooltipWidget::SetGradeVisuals(const EItemRarity Grade)
{
	const FLinearColor GradeColor = GetGradeColor(Grade);

	if (HeaderBorder)
	{
		HeaderBorder->SetBrushColor(GradeColor);
	}

	ItemTooltipWidgetPrivate::ApplyMaterialTint(
		WidgetTree,
		ItemTooltipWidgetPrivate::SquareFlickerWidgetName,
		ItemTooltipWidgetPrivate::SquareTintParameterName,
		GradeColor);
	ItemTooltipWidgetPrivate::ApplyMaterialTint(
		WidgetTree,
		ItemTooltipWidgetPrivate::BackgroundWidgetName,
		ItemTooltipWidgetPrivate::ImageTintParameterName,
		GradeColor);

	if (ItemNameText)
	{
		ItemNameText->SetColorAndOpacity(ItemTooltipWidgetPrivate::PureWhiteText);
	}
}

FLinearColor UItemTooltipWidget::GetGradeColor(const EItemRarity Grade) const
{
	switch (Grade)
	{
	case EItemRarity::IR_GradeF:    return Color_GradeF;
	case EItemRarity::IR_GradeE:    return Color_GradeE;
	case EItemRarity::IR_GradeD:    return Color_GradeD;
	case EItemRarity::IR_GradeC:    return Color_GradeC;
	case EItemRarity::IR_GradeB:    return Color_GradeB;
	case EItemRarity::IR_GradeA:    return Color_GradeA;
	case EItemRarity::IR_GradeS:    return Color_GradeS;
	case EItemRarity::IR_Unknown:   return Color_GradeUnkown;
	case EItemRarity::IR_GradeSS:   return Color_GradeSS;
	case EItemRarity::IR_Corrupted: return Color_GradeCorrupted;
	default:                        return FLinearColor::White;
	}
}

void UItemTooltipWidget::PopulateSections()
{
	if (!SectionsContainer)
	{
		return;
	}

	SectionsContainer->ClearChildren();
	const FLinearColor GradeColor = GetGradeColor(TooltipData.Rarity);

	for (const FItemTooltipSection& Section : TooltipData.Sections)
	{
		if (!Section.HasDisplayableLines())
		{
			continue;
		}

		if (SectionWidgetClass && GetWorld())
		{
			if (UItemTooltipSectionWidget* SectionWidget =
				CreateWidget<UItemTooltipSectionWidget>(GetWorld(), SectionWidgetClass))
			{
				SectionWidget->SetSectionData(Section, GradeColor);
				SectionsContainer->AddChildToVerticalBox(SectionWidget);
				continue;
			}
		}

		AddFallbackSection(Section);
	}
}

void UItemTooltipWidget::AddFallbackSection(const FItemTooltipSection& Section)
{
	if (!SectionsContainer)
	{
		return;
	}

	if (Section.bShowHeading && !Section.Heading.IsEmpty())
	{
		if (UTextBlock* HeadingBlock =
			CreateFallbackTextBlock(Section.Heading, FLinearColor::White, ETextJustify::Center))
		{
			SectionsContainer->AddChildToVerticalBox(HeadingBlock);
		}
	}

	for (const FItemTooltipLine& Line : Section.Lines)
	{
		const FText LineText = ItemTooltipWidgetPrivate::MakeFallbackLineText(Line);
		if (LineText.IsEmpty())
		{
			continue;
		}

		if (UTextBlock* TextBlock = CreateFallbackTextBlock(
			LineText,
			Line.TextColor,
			Line.bUseValueColumn ? ETextJustify::Left : ETextJustify::Center))
		{
			SectionsContainer->AddChildToVerticalBox(TextBlock);
		}
	}
}

UTextBlock* UItemTooltipWidget::CreateFallbackTextBlock(
	const FText& Text,
	const FLinearColor Color,
	const ETextJustify::Type Justification)
{
	if (!WidgetTree)
	{
		return nullptr;
	}

	UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	if (!TextBlock)
	{
		return nullptr;
	}

	TextBlock->SetText(Text);
	TextBlock->SetColorAndOpacity(FSlateColor(Color));
	TextBlock->SetJustification(Justification);
	TextBlock->SetAutoWrapText(true);
	return TextBlock;
}
