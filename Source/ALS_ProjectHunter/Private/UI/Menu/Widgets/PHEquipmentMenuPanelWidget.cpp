#include "UI/Menu/Widgets/PHEquipmentMenuPanelWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/PanelSlot.h"
#include "Components/PanelWidget.h"
#include "Components/UniformGridSlot.h"
#include "UI/Menu/Library/FunctionLibraries/MenuFunctionLibrary.h"
#include "UI/Menu/Widgets/PHEquipmentMenuPageWidget.h"
#include "UI/Menu/Widgets/PHEquipmentSlotWidget.h"

UPHEquipmentMenuPanelWidget::UPHEquipmentMenuPanelWidget()
{
	EquipmentSlotOrder = UMenuFunctionLibrary::GetDefaultEquipmentSlotOrder();
}

void UPHEquipmentMenuPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	RebuildEquipmentSlotWidgets();
}

void UPHEquipmentMenuPanelWidget::NativeInitializeForCharacter(APHBaseCharacter* Character)
{
	Super::NativeInitializeForCharacter(Character);

	if (EquipmentSlotWidgets.IsEmpty())
	{
		RebuildEquipmentSlotWidgets();
	}

	for (UPHEquipmentSlotWidget* SlotWidget : EquipmentSlotWidgets)
	{
		if (SlotWidget)
		{
			SlotWidget->InitializeForCharacter(Character);
		}
	}

	RefreshEquipmentSlotWidgets();
}

void UPHEquipmentMenuPanelWidget::NativeReleaseCharacter()
{
	for (UPHEquipmentSlotWidget* SlotWidget : EquipmentSlotWidgets)
	{
		if (SlotWidget)
		{
			SlotWidget->ReleaseCharacter();
		}
	}

	Super::NativeReleaseCharacter();
}

void UPHEquipmentMenuPanelWidget::RebuildEquipmentSlotWidgets()
{
	for (UPHEquipmentSlotWidget* SlotWidget : EquipmentSlotWidgets)
	{
		if (SlotWidget)
		{
			SlotWidget->ReleaseCharacter();
		}
	}

	EquipmentSlotWidgets.Reset();

	if (!bAutoBuildEquipmentSlotWidgets)
	{
		CacheDesignedSlotWidgets();
	}
	else if (EquipmentSlotContainer && EquipmentSlotWidgetClass)
	{
		EquipmentSlotContainer->ClearChildren();

		int32 VisualIndex = 0;
		for (const EEquipmentSlot EquipmentSlot : EquipmentSlotOrder)
		{
			if (EquipmentSlot == EEquipmentSlot::ES_None)
			{
				continue;
			}

			UPHEquipmentSlotWidget* SlotWidget = CreateWidget<UPHEquipmentSlotWidget>(this, EquipmentSlotWidgetClass);
			if (!SlotWidget)
			{
				continue;
			}

			ConfigureSlotWidget(SlotWidget, EquipmentSlot);

			if (UPanelSlot* PanelSlot = EquipmentSlotContainer->AddChild(SlotWidget))
			{
				if (UUniformGridSlot* GridSlot = Cast<UUniformGridSlot>(PanelSlot))
				{
					const int32 SafeColumnCount = FMath::Max(1, GridColumns);
					GridSlot->SetRow(VisualIndex / SafeColumnCount);
					GridSlot->SetColumn(VisualIndex % SafeColumnCount);
				}
			}

			++VisualIndex;
		}
	}

	OnEquipmentSlotWidgetsRebuilt();
	EquipmentSlotWidgetsRebuilt.Broadcast();
}

void UPHEquipmentMenuPanelWidget::RefreshEquipmentSlotWidgets()
{
	for (UPHEquipmentSlotWidget* SlotWidget : EquipmentSlotWidgets)
	{
		if (!SlotWidget)
		{
			continue;
		}

		SlotWidget->SetOwningEquipmentPage(OwningEquipmentPage);
		SlotWidget->RefreshSlot();
	}
}

void UPHEquipmentMenuPanelWidget::SetEquipmentSlotOrder(const TArray<EEquipmentSlot>& NewEquipmentSlotOrder)
{
	if (EquipmentSlotOrder == NewEquipmentSlotOrder)
	{
		return;
	}

	EquipmentSlotOrder = NewEquipmentSlotOrder;

	if (bAutoBuildEquipmentSlotWidgets)
	{
		RebuildEquipmentSlotWidgets();
	}
}

void UPHEquipmentMenuPanelWidget::SetOwningEquipmentPage(UPHEquipmentMenuPageWidget* EquipmentPage)
{
	OwningEquipmentPage = EquipmentPage;

	for (UPHEquipmentSlotWidget* SlotWidget : EquipmentSlotWidgets)
	{
		if (SlotWidget)
		{
			SlotWidget->SetOwningEquipmentPage(OwningEquipmentPage);
		}
	}
}

void UPHEquipmentMenuPanelWidget::CacheDesignedSlotWidgets()
{
	if (!WidgetTree)
	{
		return;
	}

	TArray<UWidget*> ChildWidgets;
	WidgetTree->GetAllWidgets(ChildWidgets);

	for (UWidget* ChildWidget : ChildWidgets)
	{
		if (UPHEquipmentSlotWidget* SlotWidget = Cast<UPHEquipmentSlotWidget>(ChildWidget))
		{
			ConfigureSlotWidget(SlotWidget, SlotWidget->GetConnectedEquipmentSlot());
		}
	}
}

void UPHEquipmentMenuPanelWidget::ConfigureSlotWidget(
	UPHEquipmentSlotWidget* SlotWidget,
	const EEquipmentSlot EquipmentSlot)
{
	if (!SlotWidget)
	{
		return;
	}

	SlotWidget->SetConnectedEquipmentSlot(EquipmentSlot);
	SlotWidget->SetOwningEquipmentPage(OwningEquipmentPage);
	EquipmentSlotWidgets.AddUnique(SlotWidget);

	if (APHBaseCharacter* Character = GetBoundCharacter())
	{
		SlotWidget->InitializeForCharacter(Character);
	}
}
