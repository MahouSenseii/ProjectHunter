// Copyright © 2025 MahouSensei
// Author: Quentin Davis

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Menu/Library/MenuEnumLibrary.h"
#include "Menu/Library/MenuStructLibrary.h"
#include "MenuTabBarWidget.generated.h"

class UMenuTabWidget;
class UPanelWidget;

/** Fired when the active tab changes. Bind this in the parent menu widget. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMenuTabSelected, EMenuType, NewMenu, EMenuType, OldMenu);

/**
 * UMenuTabBarWidget — the horizontal tab header bar.
 *
 * Create a Blueprint child and add a panel (HorizontalBox or WrapBox)
 * named "TabContainer". Call InitializeTabs() with your FMenuEntry array
 * and bind OnMenuTabSelected to know when the player switches tabs.
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class ALS_PROJECTHUNTER_API UMenuTabBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	// ─────────────────────────────────────────────────────────────────────────
	// Public API
	// ─────────────────────────────────────────────────────────────────────────

	/**
	 * Spawn one tab per entry and add them to TabContainer.
	 * Call this once after the bar is added to the viewport.
	 */
	UFUNCTION(BlueprintCallable, Category = "TabBar")
	void InitializeTabs(const TArray<FMenuEntry>& Entries);

	/**
	 * Programmatically select a tab by type.
	 * Fires OnMenuTabSelected if the type actually changes.
	 */
	UFUNCTION(BlueprintCallable, Category = "TabBar")
	void SelectTab(EMenuType MenuType);

	UFUNCTION(BlueprintPure, Category = "TabBar")
	EMenuType GetActiveMenuType() const { return ActiveMenuType; }

	// ─────────────────────────────────────────────────────────────────────────
	// Delegate — bind in the parent menu widget
	// ─────────────────────────────────────────────────────────────────────────

	UPROPERTY(BlueprintAssignable, Category = "TabBar|Events")
	FOnMenuTabSelected OnMenuTabSelected;

protected:

	// ─────────────────────────────────────────────────────────────────────────
	// Blueprint visual hook
	// ─────────────────────────────────────────────────────────────────────────

	/** Called after SelectTab completes. Drive any bar-level animations here. */
	UFUNCTION(BlueprintImplementableEvent, Category = "TabBar|Events")
	void OnActiveTabChanged(EMenuType NewMenu, EMenuType OldMenu);

	// ─────────────────────────────────────────────────────────────────────────
	// Configuration — set in BP defaults
	// ─────────────────────────────────────────────────────────────────────────

	/** BP child class used to spawn each tab. Must be set in BP defaults. */
	UPROPERTY(EditDefaultsOnly, Category = "TabBar|Config")
	TSubclassOf<UMenuTabWidget> TabWidgetClass;

	// ─────────────────────────────────────────────────────────────────────────
	// Bound widgets — name these exactly in the BP designer
	// ─────────────────────────────────────────────────────────────────────────

	/** Panel that holds the spawned tab widgets (HorizontalBox or WrapBox). */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> TabContainer;

	// ─────────────────────────────────────────────────────────────────────────
	// State
	// ─────────────────────────────────────────────────────────────────────────

	UPROPERTY(BlueprintReadOnly, Category = "TabBar")
	EMenuType ActiveMenuType = EMenuType::MT_None;

	UPROPERTY()
	TArray<TObjectPtr<UMenuTabWidget>> SpawnedTabs;

private:

	UFUNCTION()
	void HandleTabClicked(EMenuType ClickedType);
};
