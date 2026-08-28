// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "UI/Menu/Library/Enums/MenuEnums.h"
#include "HunterHUD.generated.h"

class APHBaseCharacter;
class APawn;
class UHunterMainHUDWidget;
class UItemInstance;
class UItemTooltipWidget;
class UPHMenuCameraComponent;
class UPHMenuRootWidget;

DECLARE_LOG_CATEGORY_EXTERN(LogHunterHUD, Log, All);

/**
 * Who asked for the shared item tooltip.
 *
 * The HUD owns exactly one tooltip widget and two unrelated systems drive it:
 * the interaction system (ground-item prompts, polled on a timer) and the menu
 * (slot hover). Without an owner, the interaction poll hides a slot tooltip a
 * fraction of a second after it opens.
 */
UENUM(BlueprintType)
enum class EItemTooltipSource : uint8
{
	/** Passed to HideItemTooltip to force a hide regardless of owner. */
	ITS_None			UMETA(DisplayName = "None"),
	ITS_Interaction		UMETA(DisplayName = "Interaction"),
	ITS_Menu			UMETA(DisplayName = "Menu")
};

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
	AHunterHUD();

	UFUNCTION(BlueprintCallable)
	void ShowItemTooltip(
		UItemInstance* Item,
		FVector2D ScreenPosition,
		EItemTooltipSource Source = EItemTooltipSource::ITS_Interaction);

	/**
	 * Shows the item tooltip beside the cursor, ignoring
	 * bPinItemTooltipToBottomRight. Used by the menu slots on hover.
	 * ViewportPosition is DPI-independent viewport space (the space
	 * UWidgetLayoutLibrary::GetMousePositionOnViewport returns).
	 */
	UFUNCTION(BlueprintCallable, Category = "HUD|Item Tooltip")
	void ShowItemTooltipAtViewportPosition(
		UItemInstance* Item,
		FVector2D ViewportPosition,
		EItemTooltipSource Source = EItemTooltipSource::ITS_Menu);

	/** Repositions an already-visible tooltip. No-op when it is hidden or owned by someone else. */
	UFUNCTION(BlueprintCallable, Category = "HUD|Item Tooltip")
	void UpdateItemTooltipPosition(
		FVector2D ViewportPosition,
		EItemTooltipSource Source = EItemTooltipSource::ITS_Menu);

	/**
	 * Hides the tooltip. A source only closes a tooltip it owns; pass ITS_None
	 * to force a hide (menu closing, HUD teardown).
	 */
	UFUNCTION(BlueprintCallable)
	void HideItemTooltip(EItemTooltipSource Source = EItemTooltipSource::ITS_None);

	/**
	 * Mash-progress HUD hooks. The C++ side only routes to the Blueprint
	 * events below - implement the actual widget in your HUD Blueprint
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

	// MENU (tabbed pause-less menu - Equipment / Stats / PassiveTree / Settings)

	/** Open the menu if closed, close it if open. Bind your Menu input to this. */
	UFUNCTION(BlueprintCallable, Category = "HUD|Menu")
	void ToggleMenu();

	/**
	 * Open the menu on a specific page.
	 * MT_None opens the root widget's configured default page (Equipment by default).
	 */
	UFUNCTION(BlueprintCallable, Category = "HUD|Menu")
	void OpenMenu(EMenuType MenuType = EMenuType::MT_Equipment);

	/** Hide the menu and restore game-only input. */
	UFUNCTION(BlueprintCallable, Category = "HUD|Menu")
	void CloseMenu();

	UFUNCTION(BlueprintPure, Category = "HUD|Menu")
	bool IsMenuOpen() const;

	/** Configures the full-screen menu Blueprint used by ToggleMenu/OpenMenu. */
	UFUNCTION(BlueprintCallable, Category = "HUD|Menu|Config")
	void SetMenuRootWidgetClass(TSubclassOf<UPHMenuRootWidget> InMenuRootWidgetClass)
	{
		MenuRootWidgetClass = InMenuRootWidgetClass;
	}

	/** Live menu root (null until first opened). */
	UFUNCTION(BlueprintPure, Category = "HUD|Menu")
	UPHMenuRootWidget* GetMenuRootWidget() const { return MenuRootWidget; }

	/**
	 * Swings the view onto the character while the menu is open.
	 * Lives here so the widget, the input mode, and the camera are switched by
	 * the same two functions and cannot drift out of step.
	 */
	UFUNCTION(BlueprintPure, Category = "HUD|Menu")
	UPHMenuCameraComponent* GetMenuCameraComponent() const { return MenuCamera; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, Category = "HUD|Widget Classes")
	TSubclassOf<UHunterMainHUDWidget> MainHUDWidgetClass;

	/** Blueprint child of UPHMenuRootWidget (tab bar + content switcher layout). */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Widget Classes")
	TSubclassOf<UPHMenuRootWidget> MenuRootWidgetClass;

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

	/**
	 * Freezes character movement and look while the menu is up.
	 *
	 * GameAndUI keeps feeding ALS the movement and look axes, so without this
	 * the character walks and the camera spins behind the open menu. Kept here
	 * rather than on the menu camera so it still applies when the camera is
	 * disabled or fails to find a character.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Menu")
	bool bBlockCharacterInputWhileMenuOpen = true;

	/**
	 * Switches the pawn's whole input component off, which is what actually
	 * stops attacks, rolls and abilities firing from clicks aimed at the menu.
	 * The ignore flags above only cover movement and look; a GAS ability bound
	 * through Enhanced Input goes straight past them.
	 *
	 * Safe to leave on: UPHMenuRootWidget owns the close key itself, so
	 * silencing every binding on the character cannot strand the menu open.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Menu")
	bool bDisablePawnInputWhileMenuOpen = true;

	/**
	 * UI-only input while the menu is up.
	 *
	 * Off by default: UI-only stops the player controller seeing input at all,
	 * which also kills the binding that opens and closes the menu. GameAndUI
	 * keeps controller bindings alive while bDisablePawnInputWhileMenuOpen
	 * silences the pawn's - which is where attacks and abilities live - so the
	 * combination blocks what should be blocked and keeps what must work.
	 *
	 * Turning this on also stops the menu camera's cursor and wheel polling,
	 * since the controller no longer receives that input. Drag and zoom survive
	 * because UPHCharacterPreviewWidget handles them through Slate directly.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Menu")
	bool bUseUIOnlyInputMode = false;

	/**
	 * Hides the gameplay HUD while the menu is up. The menu already shows
	 * vitals, so leaving it visible just bleeds a health bar through the
	 * bottom of the window.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Menu")
	bool bHideMainHUDWhileMenuOpen = true;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UItemTooltipWidget> ItemTooltipWidgetClass;

	/** Pin item tooltips to the viewport corner instead of positioning them beside the focused item. */
	UPROPERTY(EditDefaultsOnly, Category = "UI|Item Tooltip")
	bool bPinItemTooltipToBottomRight = true;

	/** Distance between the item tooltip and the bottom-right edge of the viewport. */
	UPROPERTY(EditDefaultsOnly, Category = "UI|Item Tooltip", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float ItemTooltipScreenPadding = 30.0f;

	/** Offset from the cursor for tooltips shown by ShowItemTooltipAtViewportPosition. */
	UPROPERTY(EditDefaultsOnly, Category = "UI|Item Tooltip")
	FVector2D ItemTooltipCursorOffset = FVector2D(20.0f, 20.0f);

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TObjectPtr<UItemTooltipWidget> ItemTooltipWidget;

private:
	/** Clamps a cursor-anchored tooltip so it never leaves the viewport. */
	FVector2D ClampTooltipToViewport(FVector2D DesiredPosition) const;

	/** False when a higher-priority system currently owns the tooltip. */
	bool CanShowItemTooltipFrom(EItemTooltipSource Source) const;

	/** Whoever the visible tooltip belongs to right now. */
	EItemTooltipSource ActiveItemTooltipSource = EItemTooltipSource::ITS_None;

	UPROPERTY()
	TObjectPtr<UHunterMainHUDWidget> MainHUDWidget;

	UPROPERTY()
	TObjectPtr<UPHMenuRootWidget> MenuRootWidget;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HUD|Menu", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPHMenuCameraComponent> MenuCamera;

	void CreateMainHUDWidget();
	void BindWidgetsToCharacter(APHBaseCharacter* Character) const;

	/** Lazily create the menu root and add it (hidden) to the player screen. */
	bool EnsureMenuRootWidget();

	/** Apply/restore input mode, cursor, and character input for menu open/close. */
	void ApplyMenuInputMode(bool bMenuOpen);

	/** SetIgnore*Input is reference counted, so track what we actually applied. */
	bool bCharacterInputBlocked = false;

	/** The pawn input was disabled by us, so only we re-enable it. */
	bool bPawnInputDisabled = false;

	UFUNCTION()
	void HandlePawnChanged(APawn* OldPawn, APawn* NewPawn);

	/** Re-frames the menu camera when the player switches tabs. */
	UFUNCTION()
	void HandleMenuPageChanged(EMenuType NewMenu, EMenuType OldMenu);
};
