// Author: Quentin Davis

#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "UI/Menu/Library/Structs/MenuStructs.h"

class IPHInventorySlotHost;
class UPanelWidget;
class UPHInventorySlotWidget;
class UUserWidget;

/**
 * Builds and refreshes the inventory cell grid.
 *
 * Shared by the inventory panel and the equipment page so the grid behaves
 * identically whichever one owns it, and so the "why is my grid empty" logging
 * lives in exactly one place.
 */
class FMenuInventoryGridBuilder
{
public:
	/**
	 * Refreshes cells in place when the grid size is unchanged, otherwise
	 * recreates them.
	 *
	 * @return true when widgets were actually recreated (callers broadcast their
	 *         "rebuilt" event only then).
	 */
	static bool Rebuild(
		UUserWidget& Owner,
		const TScriptInterface<IPHInventorySlotHost>& Host,
		UPanelWidget* Container,
		TSubclassOf<UPHInventorySlotWidget> SlotWidgetClass,
		int32 GridColumns,
		FVector2D CellSize,
		const TArray<FEquipmentMenuInventorySlotViewData>& SlotData,
		TArray<TObjectPtr<UPHInventorySlotWidget>>& InOutSlotWidgets);

private:
	static bool CanReuse(
		UPanelWidget* Container,
		TSubclassOf<UPHInventorySlotWidget> SlotWidgetClass,
		const TArray<FEquipmentMenuInventorySlotViewData>& SlotData,
		const TArray<TObjectPtr<UPHInventorySlotWidget>>& SlotWidgets);

	/** One-time-per-condition explanation of why nothing rendered. */
	static void WarnMissingSetup(
		const UUserWidget& Owner,
		const UPanelWidget* Container,
		const TSubclassOf<UPHInventorySlotWidget>& SlotWidgetClass);
};
