// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#include "MenuRowBuilder.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/PanelWidget.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Font.h"

namespace
{
	/** The menu's authored face. Resolved once; the engine default is the fallback. */
	UFont* ResolveMenuFont()
	{
		static TWeakObjectPtr<UFont> Cached;
		static bool bAttempted = false;
		if (!bAttempted)
		{
			bAttempted = true;
			Cached = LoadObject<UFont>(nullptr,
				TEXT("/Game/ProjectHunter/UI/Fonts/Rajdhani/Rajdhani.Rajdhani"));
		}
		return Cached.Get();
	}

	FSlateBrush FlatBrush(const FLinearColor& Fill, const FLinearColor& Outline, float Width = 1.0f)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.TintColor = FSlateColor(Fill);
		Brush.OutlineSettings.Color = FSlateColor(Outline);
		Brush.OutlineSettings.Width = Width;
		Brush.OutlineSettings.CornerRadii = FVector4(2.0f, 2.0f, 2.0f, 2.0f);
		Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
		return Brush;
	}
}

namespace PHMenuRowBuilder
{
	FSlateFontInfo MenuFont(const int32 Size, const FName& Typeface)
	{
		FSlateFontInfo Font;
		if (UFont* Resolved = ResolveMenuFont())
		{
			Font = FSlateFontInfo(Resolved, Size, Typeface);
		}
		else
		{
			Font = FCoreStyle::GetDefaultFontStyle("Regular", Size);
		}
		Font.Size = Size;
		return Font;
	}

	UTextBlock* MakeText(UWidgetTree& Tree, const FText& Text, const int32 Size,
		const FLinearColor& Color, const FName& Typeface, const float LetterSpacing)
	{
		UTextBlock* Block = Tree.ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Block->SetText(Text);
		Block->SetColorAndOpacity(FSlateColor(Color));

		FSlateFontInfo Font = MenuFont(Size, Typeface);
		Font.LetterSpacing = static_cast<int32>(LetterSpacing);
		Block->SetFont(Font);
		return Block;
	}

	UButton* MakeButton(UWidgetTree& Tree, const FText& Label, const int32 Size)
	{
		UButton* Button = Tree.ConstructWidget<UButton>(UButton::StaticClass());

		FButtonStyle Style = Button->GetStyle();
		Style.Normal = FlatBrush(Palette::Well, Palette::Line);
		Style.Hovered = FlatBrush(FLinearColor(0.10f, 0.35f, 0.62f, 0.85f), Palette::Text);
		Style.Pressed = FlatBrush(FLinearColor(0.55f, 0.83f, 0.98f, 0.95f), Palette::Text);
		Style.NormalPadding = FMargin(0.0f);
		Style.PressedPadding = FMargin(0.0f);
		Button->SetStyle(Style);

		if (!Label.IsEmpty())
		{
			UTextBlock* Text = MakeText(Tree, Label, Size, Palette::Text);
			Text->SetJustification(ETextJustify::Center);
			Button->AddChild(Text);
		}
		return Button;
	}

	UHorizontalBox* MakeStatRow(UWidgetTree& Tree, const FText& Label,
		UTextBlock*& OutValueText, const int32 Size)
	{
		UHorizontalBox* Row = Tree.ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

		UTextBlock* LabelText = MakeText(Tree, Label, Size, Palette::Dim, TEXT("Regular"));
		if (UHorizontalBoxSlot* LabelSlot = Cast<UHorizontalBoxSlot>(Row->AddChild(LabelText)))
		{
			// The label takes the slack so the value stays pinned right and the
			// numbers line up down the column.
			LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			LabelSlot->SetVerticalAlignment(VAlign_Center);
		}

		OutValueText = MakeText(Tree, FText::GetEmpty(), Size, Palette::Text);
		OutValueText->SetJustification(ETextJustify::Right);
		if (UHorizontalBoxSlot* ValueSlot = Cast<UHorizontalBoxSlot>(Row->AddChild(OutValueText)))
		{
			ValueSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
			ValueSlot->SetVerticalAlignment(VAlign_Center);
			ValueSlot->SetPadding(FMargin(12.0f, 0.0f, 0.0f, 0.0f));
		}
		return Row;
	}

	UButton* MakeSectionHeader(UWidgetTree& Tree, const FText& Title, UTextBlock*& OutCaret)
	{
		UButton* Header = MakeButton(Tree, FText::GetEmpty());
		Header->ClearChildren();

		UHorizontalBox* Row = Tree.ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

		OutCaret = MakeText(Tree, FText::FromString(TEXT("-")), 15, Palette::Header);
		if (UHorizontalBoxSlot* CaretSlot = Cast<UHorizontalBoxSlot>(Row->AddChild(OutCaret)))
		{
			CaretSlot->SetPadding(FMargin(10.0f, 4.0f, 10.0f, 4.0f));
			CaretSlot->SetVerticalAlignment(VAlign_Center);
		}

		UTextBlock* TitleText = MakeText(Tree, Title, 14, Palette::Header, TEXT("Bold"), 90.0f);
		if (UHorizontalBoxSlot* TitleSlot = Cast<UHorizontalBoxSlot>(Row->AddChild(TitleText)))
		{
			TitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			TitleSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 4.0f));
			TitleSlot->SetVerticalAlignment(VAlign_Center);
		}

		Header->AddChild(Row);
		return Header;
	}

	void AddRow(UVerticalBox& Column, UWidget& Child, const float Top, const float Bottom)
	{
		if (UVerticalBoxSlot* Slot = Cast<UVerticalBoxSlot>(Column.AddChild(&Child)))
		{
			Slot->SetPadding(FMargin(0.0f, Top, 0.0f, Bottom));
			Slot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		}
	}

	UPanelWidget* EnsureRowHost(UWidgetTree& Tree, UPanelWidget* AuthoredHost)
	{
		if (AuthoredHost)
		{
			return AuthoredHost;
		}

		UScrollBox* Scroll = Tree.ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
		if (!Tree.RootWidget)
		{
			Tree.RootWidget = Scroll;
			return Scroll;
		}

		UPanelWidget* Root = Cast<UPanelWidget>(Tree.RootWidget);
		if (!Root)
		{
			return nullptr;
		}

		UPanelSlot* AddedSlot = Root->AddChild(Scroll);
		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(AddedSlot))
		{
			CanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
			CanvasSlot->SetOffsets(FMargin(0.0f));
			CanvasSlot->SetAlignment(FVector2D(0.0f, 0.0f));
		}
		return Scroll;
	}

	UVerticalBox* AddPaddedColumn(UWidgetTree& Tree, UPanelWidget& Host)
	{
		UVerticalBox* Column = Tree.ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		UPanelSlot* AddedSlot = Host.AddChild(Column);
		if (UScrollBoxSlot* ScrollSlot = Cast<UScrollBoxSlot>(AddedSlot))
		{
			ScrollSlot->SetPadding(FMargin(4.0f, 8.0f, 16.0f, 16.0f));
			ScrollSlot->SetHorizontalAlignment(HAlign_Fill);
		}
		return Column;
	}
}
