// Author: Quentin Davis

#pragma once

#include "CoreMinimal.h"

class AHunterHUD;
class UItemInstance;
class UUserWidget;

/**
 * Routes menu slot hover to the shared item tooltip owned by AHunterHUD.
 *
 * The HUD keeps a single tooltip widget, so slots never create their own.
 */
class FMenuSlotTooltipHelper
{
public:
	/** Resolves the local player's HUD from any widget. Null when not local. */
	static AHunterHUD* GetHunterHUD(const UUserWidget& Widget);

	/**
	 * Shows the tooltip for Item at the current cursor position.
	 * No-op when Item is null - callers don't need to branch.
	 */
	static void ShowForItem(const UUserWidget& Widget, UItemInstance* Item);

	/** Moves an already-visible tooltip to the current cursor position. */
	static void UpdatePosition(const UUserWidget& Widget);

	/** Hides the shared tooltip. */
	static void Hide(const UUserWidget& Widget);

	/** Cursor position in the same viewport space SetPositionInViewport expects. */
	static FVector2D GetCursorViewportPosition(const UUserWidget& Widget);
};
