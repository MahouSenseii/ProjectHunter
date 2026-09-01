// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateTypes.h"

class UButton;
class UHorizontalBox;
class UTextBlock;
class UWidget;
class UPanelWidget;
class UVerticalBox;
class UWidgetTree;

/**
 * Builds the label/value rows the Stats and Settings pages are made of.
 *
 * These pages have no authored UMG tree - unlike the equipment page, whose
 * layout is a Blueprint - so their content is constructed here instead. The
 * point of sharing it is that both pages produce visually identical rows and
 * pick up the System-window palette from one place.
 *
 * Presentation only. Nothing here reads or writes gameplay state.
 */
namespace PHMenuRowBuilder
{
	/** The System-window palette, matching PHUIStyle.h and the menu textures. */
	namespace Palette
	{
		inline constexpr FLinearColor Text{1.0f, 1.0f, 1.0f, 1.0f};
		inline constexpr FLinearColor Dim{0.62f, 0.83f, 0.96f, 1.0f};
		inline constexpr FLinearColor Well{0.027f, 0.106f, 0.20f, 0.55f};
		inline constexpr FLinearColor Line{1.0f, 1.0f, 1.0f, 0.55f};
		inline constexpr FLinearColor Accent{0.61f, 0.88f, 1.0f, 1.0f};

		/**
		 * Section headers: #8FE3DC, a teal-leaning cyan.
		 *
		 * Deliberately not Dim or Accent - both are blue-leaning and read as a
		 * paler shade of the azure body rather than as an accent against it.
		 * Must match PHUIStyle::HeaderTeal and PolishSystemMenu's "header".
		 */
		inline constexpr FLinearColor Header{0.2747f, 0.7682f, 0.7157f, 1.0f};
	}

	/** Rajdhani at the requested size/typeface, or the engine default if absent. */
	FSlateFontInfo MenuFont(int32 Size, const FName& Typeface = FName(TEXT("SemiBold")));

	UTextBlock* MakeText(UWidgetTree& Tree, const FText& Text, int32 Size,
		const FLinearColor& Color, const FName& Typeface = FName(TEXT("SemiBold")),
		float LetterSpacing = 0.0f);

	/** A flat button carrying the System line language. */
	UButton* MakeButton(UWidgetTree& Tree, const FText& Label, int32 Size = 13);

	/** Label on the left, value on the right, with the value right-aligned. */
	UHorizontalBox* MakeStatRow(UWidgetTree& Tree, const FText& Label,
		UTextBlock*& OutValueText, int32 Size = 14);

	/**
	 * A group header that toggles its content.
	 *
	 * Returns the button; the caller owns what expanding it reveals, because the
	 * builder deliberately knows nothing about stats or settings.
	 */
	UButton* MakeSectionHeader(UWidgetTree& Tree, const FText& Title, UTextBlock*& OutCaret);

	/** Adds a child to a vertical box with uniform padding. */
	void AddRow(UVerticalBox& Column, UWidget& Child, float Top = 3.0f, float Bottom = 3.0f);

	/**
	 * Returns the panel a page should fill with rows.
	 *
	 * When the Blueprint supplies a host, that wins. Otherwise a ScrollBox is
	 * created and attached to the widget's root - anchored to fill if that root
	 * is a CanvasPanel, which a newly created Widget Blueprint always has and
	 * which would otherwise collapse the child to zero size in the corner.
	 * Returns null when there is nowhere valid to attach.
	 */
	UPanelWidget* EnsureRowHost(UWidgetTree& Tree, UPanelWidget* AuthoredHost);

	/** The column rows go into, inset from the window frame so text does not touch it. */
	UVerticalBox* AddPaddedColumn(UWidgetTree& Tree, UPanelWidget& Host);
}
