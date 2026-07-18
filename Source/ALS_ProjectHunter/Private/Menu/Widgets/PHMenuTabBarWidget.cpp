#include "Menu/Widgets/PHMenuTabBarWidget.h"

#include "Components/PanelWidget.h"
#include "Menu/Widgets/PHMenuTabButtonWidget.h"

void UPHMenuTabBarWidget::InitializeTabs(const TArray<FMenuEntry>& Entries)
{
	if (!TabWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("UPHMenuTabBarWidget::InitializeTabs - TabWidgetClass is not set on %s."), *GetName());
		return;
	}

	if (!TabContainer)
	{
		UE_LOG(LogTemp, Warning, TEXT("UPHMenuTabBarWidget::InitializeTabs - TabContainer widget is missing on %s. Name it 'TabContainer' in the BP designer."), *GetName());
		return;
	}

	TabContainer->ClearChildren();
	SpawnedTabs.Empty();

	for (const FMenuEntry& Entry : Entries)
	{
		UPHMenuTabButtonWidget* Tab = CreateWidget<UPHMenuTabButtonWidget>(this, TabWidgetClass);
		if (!Tab)
		{
			continue;
		}

		Tab->SetTabData(Entry);
		Tab->OnTabClicked.AddDynamic(this, &UPHMenuTabBarWidget::HandleTabClicked);

		TabContainer->AddChild(Tab);
		SpawnedTabs.Add(Tab);
	}

	if (SpawnedTabs.Num() > 0 && SpawnedTabs[0])
	{
		SelectTab(SpawnedTabs[0]->GetMenuType());
	}
}

void UPHMenuTabBarWidget::SelectTab(EMenuType MenuType)
{
	if (ActiveMenuType == MenuType)
	{
		return;
	}

	const EMenuType OldMenu = ActiveMenuType;
	ActiveMenuType = MenuType;

	for (UPHMenuTabButtonWidget* Tab : SpawnedTabs)
	{
		if (!Tab)
		{
			continue;
		}

		Tab->SetSelected(Tab->GetMenuType() == ActiveMenuType);
	}

	OnActiveTabChanged(ActiveMenuType, OldMenu);
	OnMenuTabSelected.Broadcast(ActiveMenuType, OldMenu);
}

void UPHMenuTabBarWidget::HandleTabClicked(EMenuType ClickedType)
{
	SelectTab(ClickedType);
}
