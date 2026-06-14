// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Menu/Library/MenuEnumLibrary.h"
#include "HunterHUD.generated.h"

class APHBaseCharacter;
class APawn;
class UHunterMainHUDWidget;
class UItemInstance;
class UItemTooltipWidget;
class UMenuRootWidget;

DECLARE_LOG_CATEGORY_EXTERN(LogHunterHUD, Log, All);

/**
 * Root HUD actor for the player.
 *
 * Creates the top-level HUD widget. The widget Blueprint owns the resource,
 * XP, and status child widgets so their screen layout is controlled in UMG.
 */
UCLASS()
class ALS_PROJECTHUNTER_API AHunterHUD : public AHUD
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void ShowItemTooltip(UItemInstance* Item, FVector2D ScreenPosition);

	UFUNCTION(BlueprintCallable)
	void HideItemTooltip();

	/**
	 * Mash-progress HUD hooks. The C++ side only routes to the Blueprint
	 * events below — implement the actual widget in your HUD Blueprint
	 * (these were silent empty stubs before).
	 */
	void ShowMashProgressWidget(const FText& Text, int32 RequiredCount);
	void HideMashProgressWidget();

	/** Implement in the HUD Blueprint: show a mash-progress widget. */
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD|Mash")
	void BP_OnShowMashProgress(const FText& Text, int32 RequiredCount);

	/** Implement in the HUD Blueprint: hide the mash-progress widget. */
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD|Mash")
	void BP_OnHideMashProgress();

	UFUNCTION(BlueprintPure, Category = "HUD|Widgets")
	UHunterMainHUDWidget* GetMainHUDWidget() const { return MainHUDWidget; }

	// ─────────────────────────────────────────────────────────────────────────
	// MENU (tabbed pause-less menu — Equipment / Stats / PassiveTree / Settings)
	// ─────────────────────────────────────────────────────────────────────────

	/** Open the menu if closed, close it if open. Bind your Menu input to this. */
	UFUNCTION(BlueprintCallable, Category = "HUD|Menu")
	void ToggleMenu();

	/**
	 * Open the menu on a specific page.
	 * MT_None restores the last page (or the first configured one).
	 */
	UFUNCTION(BlueprintCallable, Category = "HUD|Menu")
	void OpenMenu(EMenuType MenuType = EMenuType::MT_None);

	/** Hide the menu and restore game-only input. */
	UFUNCTION(BlueprintCallable, Category = "HUD|Menu")
	void CloseMenu();

	UFUNCTION(BlueprintPure, Category = "HUD|Menu")
	bool IsMenuOpen() const;

	/** Live menu root (null until first opened). */
	UFUNCTION(BlueprintPure, Category = "HUD|Menu")
	UMenuRootWidget* GetMenuRootWidget() const { return MenuRootWidget; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, Category = "HUD|Widget Classes")
	TSubclassOf<UHunterMainHUDWidget> MainHUDWidgetClass;

	/** Blueprint child of UMenuRootWidget (tab bar + content switcher layout). */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Widget Classes")
	TSubclassOf<UMenuRootWidget> MenuRootWidgetClass;

	/** Viewport Z-order for the menu overlay (main HUD is added at 10). */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Menu")
	int32 MenuZOrder = 50;

	/**
	 * When true, opening the menu switches to GameAndUI input with a visible
	 * cursor and CloseMenu restores GameOnly. Disable if your Blueprint wants
	 * to drive input modes itself.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Menu")
	bool bManageInputMode = true;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UItemTooltipWidget> ItemTooltipWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TObjectPtr<UItemTooltipWidget> ItemTooltipWidget;

private:
	UPROPERTY()
	TObjectPtr<UHunterMainHUDWidget> MainHUDWidget;

	UPROPERTY()
	TObjectPtr<UMenuRootWidget> MenuRootWidget;

	void CreateMainHUDWidget();
	void BindWidgetsToCharacter(APHBaseCharacter* Character) const;

	/** Lazily create the menu root and add it (hidden) to the player screen. */
	bool EnsureMenuRootWidget();

	/** Apply/restore input mode + cursor for menu open/close. */
	void ApplyMenuInputMode(bool bMenuOpen) const;

	UFUNCTION()
	void HandlePawnChanged(APawn* OldPawn, APawn* NewPawn);
};
