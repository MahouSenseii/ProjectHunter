// Copyright © 2025 MahouSensei
// Author: Quentin Davis

#pragma once

#include "CoreMinimal.h"
#include "Character/HUD/HunterHUDBaseWidget.h"
#include "Menu/Library/MenuEnumLibrary.h"
#include "Menu/Library/MenuStructLibrary.h"
#include "MenuRootWidget.generated.h"

class UMenuBaseWidget;
class UMenuTabBarWidget;
class UWidgetSwitcher;

/** Fired after the visible page changes. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMenuPageChanged, EMenuType, NewMenu, EMenuType, OldMenu);

/**
 * UMenuRootWidget — the full-screen menu container.
 *
 * Owns the tab bar and the page area; lazily spawns one UMenuBaseWidget page
 * per FMenuEntry (cached in Entry.CachedInstance and reused), and keeps every
 * page bound to the current character so Equipment/Stats pages can read their
 * managers.
 *
 * BP setup (create a Blueprint child):
 *   1. Add your UMenuTabBarWidget child named exactly  "TabBar".
 *   2. Add a WidgetSwitcher named exactly              "ContentSwitcher".
 *   3. Fill "Menu Entries" in class defaults (type, label, icon, page class).
 *   4. Assign this BP as MenuRootWidgetClass on your AHunterHUD Blueprint.
 *
 * AHunterHUD owns open/close/visibility — this widget only manages pages.
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class ALS_PROJECTHUNTER_API UMenuRootWidget : public UHunterHUDBaseWidget
{
	GENERATED_BODY()

public:

	// ─────────────────────────────────────────────────────────────────────────
	// Public API — called by AHunterHUD / Blueprint
	// ─────────────────────────────────────────────────────────────────────────

	/**
	 * Switch to a menu page. MT_None selects the first configured entry.
	 * Routes through the tab bar when one is bound so tab visuals stay in sync.
	 */
	UFUNCTION(BlueprintCallable, Category = "Menu")
	void OpenMenu(EMenuType MenuType);

	UFUNCTION(BlueprintPure, Category = "Menu")
	EMenuType GetActiveMenuType() const { return ActiveMenuType; }

	/** Live page widget for the active menu (null if none shown yet). */
	UFUNCTION(BlueprintPure, Category = "Menu")
	UMenuBaseWidget* GetActivePage() const;

	/** Live page widget for a specific menu (null until first opened). */
	UFUNCTION(BlueprintPure, Category = "Menu")
	UMenuBaseWidget* GetPageForMenu(EMenuType MenuType) const;

	// ─────────────────────────────────────────────────────────────────────────
	// Events
	// ─────────────────────────────────────────────────────────────────────────

	UPROPERTY(BlueprintAssignable, Category = "Menu|Events")
	FOnMenuPageChanged OnMenuPageChanged;

protected:

	// ─────────────────────────────────────────────────────────────────────────
	// UUserWidget / UHunterHUDBaseWidget overrides
	// ─────────────────────────────────────────────────────────────────────────

	virtual void NativeConstruct() override;
	virtual void NativeInitializeForCharacter(APHBaseCharacter* Character) override;
	virtual void NativeReleaseCharacter() override;

	// ─────────────────────────────────────────────────────────────────────────
	// Blueprint visual hook
	// ─────────────────────────────────────────────────────────────────────────

	/** Called after the visible page changes. Drive open/switch animations here. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Menu|Events")
	void OnPageChanged(EMenuType NewMenu, EMenuType OldMenu);

	// ─────────────────────────────────────────────────────────────────────────
	// Configuration — fill in BP defaults
	// ─────────────────────────────────────────────────────────────────────────

	/** One entry per menu page (tab label/icon + page widget class). */
	UPROPERTY(EditDefaultsOnly, Category = "Menu|Config")
	TArray<FMenuEntry> MenuEntries;

	// ─────────────────────────────────────────────────────────────────────────
	// Bound widgets — name these exactly in the BP designer
	// ─────────────────────────────────────────────────────────────────────────

	/** Tab header bar. Optional — pages still work without one. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UMenuTabBarWidget> TabBar;

	/** Switcher that holds the spawned page widgets. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UWidgetSwitcher> ContentSwitcher;

	// ─────────────────────────────────────────────────────────────────────────
	// State
	// ─────────────────────────────────────────────────────────────────────────

	UPROPERTY(BlueprintReadOnly, Category = "Menu")
	EMenuType ActiveMenuType = EMenuType::MT_None;

private:

	UFUNCTION()
	void HandleTabSelected(EMenuType NewMenu, EMenuType OldMenu);

	/** Resolve + show the page for MenuType. Core switch logic. */
	void ShowPage(EMenuType MenuType, EMenuType OldMenu);

	/** Lazily create (and cache) the page widget for an entry. */
	UMenuBaseWidget* GetOrCreatePage(FMenuEntry& Entry);

	FMenuEntry* FindEntry(EMenuType MenuType);
	const FMenuEntry* FindEntry(EMenuType MenuType) const;

	/** First entry with a valid type/class — fallback for MT_None requests. */
	EMenuType GetFirstValidMenuType() const;
};
