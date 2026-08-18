// Author: Quentin Davis

#include "UI/Menu/Helpers/MenuInventoryGridBuilder.h"

#include "Blueprint/UserWidget.h"
#include "Components/PanelSlot.h"
#include "Components/PanelWidget.h"
#include "Components/SizeBox.h"
#include "Components/UniformGridSlot.h"
#include "Core/Logging/ProjectHunterLogMacros.h"
#include "UI/Menu/Interfaces/PHInventorySlotHost.h"
#include "UI/Menu/Library/MenuLog.h"
#include "UI/Menu/Widgets/PHInventorySlotWidget.h"

bool FMenuInventoryGridBuilder::Rebuild(
	UUserWidget& Owner,
	const TScriptInterface<IPHInventorySlotHost>& Host,
	UPanelWidget* Container,
	TSubclassOf<UPHInventorySlotWidget> SlotWidgetClass,
	const int32 GridColumns,
	const FVector2D CellSize,
	const TArray<FEquipmentMenuInventorySlotViewData>& SlotData,
	TArray<TObjectPtr<UPHInventorySlotWidget>>& InOutSlotWidgets)
{
	if (!Container || !SlotWidgetClass || !Host.GetInterface())
	{
		WarnMissingSetup(Owner, Container, SlotWidgetClass);
		InOutSlotWidgets.Reset();
		return false;
	}

	if (SlotData.Num() == 0 && InOutSlotWidgets.Num() > 0)
	{
		// A refresh that ran before the inventory component resolved would
		// otherwise wipe a grid that is already built and correct.
		UE_LOG(LogPHMenu, Verbose,
			TEXT("%s: skipping grid rebuild - no slot data, keeping %d existing cells."),
			*Owner.GetName(), InOutSlotWidgets.Num());
		return false;
	}

	// Refresh in place when the grid size has not changed, which is almost every
	// refresh. Recreating the cells tears down whatever the player is hovering or
	// dragging, mid-gesture.
	if (CanReuse(Container, SlotWidgetClass, SlotData, InOutSlotWidgets))
	{
		for (int32 Index = 0; Index < SlotData.Num(); ++Index)
		{
			InOutSlotWidgets[Index]->SetSlotData(SlotData[Index]);
		}

		return false;
	}

	InOutSlotWidgets.Reset();
	Container->ClearChildren();

	const int32 SafeColumnCount = FMath::Max(1, GridColumns);

	for (int32 VisualIndex = 0; VisualIndex < SlotData.Num(); ++VisualIndex)
	{
		UPHInventorySlotWidget* SlotWidget =
			CreateWidget<UPHInventorySlotWidget>(&Owner, SlotWidgetClass);
		if (!SlotWidget)
		{
			continue;
		}

		SlotWidget->InitializeInventorySlot(Host, SlotData[VisualIndex]);
		InOutSlotWidgets.Add(SlotWidget);

		// A cell whose root is a CanvasPanel reports a desired size of zero, and a
		// UniformGridPanel sizes cells from the largest child's desired size - so
		// every cell would collapse and the grid would render as nothing. Wrapping
		// in a SizeBox gives the cell a real footprint regardless of its internals.
		UWidget* ChildToAdd = SlotWidget;
		if (CellSize.X > 0.0 && CellSize.Y > 0.0)
		{
			if (USizeBox* CellBox = NewObject<USizeBox>(&Owner))
			{
				CellBox->SetWidthOverride(static_cast<float>(CellSize.X));
				CellBox->SetHeightOverride(static_cast<float>(CellSize.Y));
				CellBox->AddChild(SlotWidget);
				ChildToAdd = CellBox;
			}
		}

		if (UPanelSlot* PanelSlot = Container->AddChild(ChildToAdd))
		{
			// Only a uniform grid needs explicit row/column; wrap boxes and
			// vertical boxes lay themselves out.
			if (UUniformGridSlot* GridSlot = Cast<UUniformGridSlot>(PanelSlot))
			{
				GridSlot->SetRow(VisualIndex / SafeColumnCount);
				GridSlot->SetColumn(VisualIndex % SafeColumnCount);
			}
		}
	}

	UE_LOG(LogPHMenu, Log, TEXT("%s: built %d inventory cells (cell size %s)."),
		*Owner.GetName(), InOutSlotWidgets.Num(),
		(CellSize.X > 0.0 && CellSize.Y > 0.0) ? *CellSize.ToString() : TEXT("auto"));

	return true;
}

bool FMenuInventoryGridBuilder::CanReuse(
	UPanelWidget* Container,
	TSubclassOf<UPHInventorySlotWidget> SlotWidgetClass,
	const TArray<FEquipmentMenuInventorySlotViewData>& SlotData,
	const TArray<TObjectPtr<UPHInventorySlotWidget>>& SlotWidgets)
{
	if (SlotData.Num() == 0
		|| SlotWidgets.Num() != SlotData.Num()
		|| Container->GetChildrenCount() != SlotData.Num())
	{
		return false;
	}

	for (const TObjectPtr<UPHInventorySlotWidget>& SlotWidget : SlotWidgets)
	{
		// A changed class means the Blueprint was reconfigured since the build.
		if (!SlotWidget || SlotWidget->GetClass() != SlotWidgetClass.Get())
		{
			return false;
		}
	}

	return true;
}

void FMenuInventoryGridBuilder::WarnMissingSetup(
	const UUserWidget& Owner,
	const UPanelWidget* Container,
	const TSubclassOf<UPHInventorySlotWidget>& SlotWidgetClass)
{
	// A blank inventory grid is otherwise completely silent, so say exactly which
	// piece of Blueprint setup is missing.
	if (!Container)
	{
		PH_LOG_WARNING(LogPHMenu,
			"%s: no InventorySlotContainer. Add a panel (UniformGridPanel or WrapBox) "
			"named exactly 'InventorySlotContainer' to this widget's Blueprint.",
			*Owner.GetName());
	}

	if (!SlotWidgetClass)
	{
		PH_LOG_WARNING(LogPHMenu,
			"%s: InventorySlotWidgetClass is unset. Set it in the Blueprint class "
			"defaults to your inventory cell WBP (a UPHInventorySlotWidget child).",
			*Owner.GetName());
	}
}
