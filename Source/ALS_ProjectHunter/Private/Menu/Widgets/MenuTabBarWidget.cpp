// Copyright © 2025 MahouSensei
// Author: Quentin Davis

#include "Menu/Widgets/MenuTabBarWidget.h"
#include "Menu/Widgets/MenuTabWidget.h"
#include "Components/PanelWidget.h"

void UMenuTabBarWidget::InitializeTabs(const TArray<FMenuEntry>& Entries)
{
	if (!TabWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("UMenuTabBarWidget::InitializeTabs — TabWidgetClass is not set on %s."), *GetName());
		return;
	}

	if (!TabContainer)
	{
		UE_LOG(LogTemp, Warning, TEXT("UMenuTabBarWidget::InitializeTabs — TabContainer widget is missing on %s. Name it 'TabContainer' in the BP designer."), *GetName());
		return;
	}

	// Clear any previously spawned tabs
	TabContainer->ClearChildren();
	SpawnedTabs.Empty();

	for (const FMenuEntry& Entry : Entries)
	{
		UMenuTabWidget* Tab = CreateWidget<UMenuTabWidget>(this, TabWidgetClass);
		if (!Tab)
		{
			continue;
		}

		Tab->SetTabData(Entry);
		Tab->OnTabClicked.AddDynamic(this, &UMenuTabBarWidget::HandleTabClicked);

		TabContainer->AddChild(Tab);
		SpawnedTabs.Add(Tab);
	}

	// Auto-select the first tab if available
	if (SpawnedTabs.Num() > 0 && SpawnedTabs[0])
	{
		SelectTab(SpawnedTabs[0]->GetMenuType());
	}
}

void UMenuTabBarWidget::SelectTab(EMenuType MenuType)
{
	if (ActiveMenuType == MenuType)
	{
		return;
	}

	const EMenuType OldMenu = ActiveMenuType;
	ActiveMenuType = MenuType;

	for (UMenuTabWidget* Tab : SpawnedTabs)
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

void UMenuTabBarWidget::HandleTabClicked(EMenuType ClickedType)
{
	SelectTab(ClickedType);
}
