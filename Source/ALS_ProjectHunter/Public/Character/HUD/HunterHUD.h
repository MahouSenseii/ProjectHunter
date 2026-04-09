// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "HunterHUD.generated.h"

class UItemTooltipWidget;
class UItemInstance;
class APawn;

class UHunterHUD_HealthWidget;
class UHunterHUD_StaminaWidget;
class UHunterHUD_ManaWidget;
class UHunterHUD_XPWidget;
class UStatusEffectHUDWidget;
class APHBaseCharacter;

DECLARE_LOG_CATEGORY_EXTERN(LogHunterHUD, Log, All);

/**
 * AHunterHUD — root HUD actor for the player.
 *
 * Owns the four stat widgets (Health, Stamina, Mana, XP) and keeps them
 * in sync with the locally controlled pawn.  Pawn changes (respawn /
 * repossession) are handled automatically via the APlayerController
 * OnPossessedPawnChanged delegate.
 *
 * Setup in editor (Blueprint subclass):
 *   1. Set HealthWidgetClass, StaminaWidgetClass, ManaWidgetClass, XPWidgetClass
 *      to your Blueprint widget children.
 *   2. Assign the HUD class to the GameMode or PlayerController.
 */
UCLASS()
class ALS_PROJECTHUNTER_API AHunterHUD : public AHUD
{
	GENERATED_BODY()

public:
	// ─────────────────────────────────────────────────────────────────────────
	// Tooltip management (existing)
	// ─────────────────────────────────────────────────────────────────────────

	UFUNCTION(BlueprintCallable)
	void ShowItemTooltip(UItemInstance* Item, FVector2D ScreenPosition);

	UFUNCTION(BlueprintCallable)
	void HideItemTooltip();

	void ShowMashProgressWidget(const FText& Text, int32 INT32);
	void HideMashProgressWidget();

	// ─────────────────────────────────────────────────────────────────────────
	// Stat widget accessors — useful for Blueprint-side layout code
	// ─────────────────────────────────────────────────────────────────────────

	UFUNCTION(BlueprintPure, Category = "HUD|Widgets")
	UHunterHUD_HealthWidget*  GetHealthWidget()  const { return HealthWidget;  }

	UFUNCTION(BlueprintPure, Category = "HUD|Widgets")
	UHunterHUD_StaminaWidget* GetStaminaWidget() const { return StaminaWidget; }

	UFUNCTION(BlueprintPure, Category = "HUD|Widgets")
	UHunterHUD_ManaWidget*    GetManaWidget()    const { return ManaWidget;    }

	UFUNCTION(BlueprintPure, Category = "HUD|Widgets")
	UHunterHUD_XPWidget*      GetXPWidget()      const { return XPWidget;      }

	UFUNCTION(BlueprintPure, Category = "HUD|Widgets")
	UStatusEffectHUDWidget*   GetStatusEffectWidget() const { return StatusEffectWidget; }

protected:
	virtual void BeginPlay() override;

	// ─────────────────────────────────────────────────────────────────────────
	// Widget class references — set in Blueprint defaults
	// ─────────────────────────────────────────────────────────────────────────

	UPROPERTY(EditDefaultsOnly, Category = "HUD|Widget Classes")
	TSubclassOf<UHunterHUD_HealthWidget> HealthWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "HUD|Widget Classes")
	TSubclassOf<UHunterHUD_StaminaWidget> StaminaWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "HUD|Widget Classes")
	TSubclassOf<UHunterHUD_ManaWidget> ManaWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "HUD|Widget Classes")
	TSubclassOf<UHunterHUD_XPWidget> XPWidgetClass;

	/**
	 * Widget class for the status effect icon strip.
	 * Must be a Blueprint child of UStatusEffectHUDWidget.
	 * Assign in the HUD Blueprint defaults.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Widget Classes")
	TSubclassOf<UStatusEffectHUDWidget> StatusEffectWidgetClass;

	// ─────────────────────────────────────────────────────────────────────────
	// Tooltip (existing)
	// ─────────────────────────────────────────────────────────────────────────

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UItemTooltipWidget> ItemTooltipWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TObjectPtr<UItemTooltipWidget> ItemTooltipWidget;

private:
	// ─────────────────────────────────────────────────────────────────────────
	// Live widget instances
	// ─────────────────────────────────────────────────────────────────────────

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TObjectPtr<UHunterHUD_HealthWidget>  HealthWidget;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TObjectPtr<UHunterHUD_StaminaWidget> StaminaWidget;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TObjectPtr<UHunterHUD_ManaWidget>    ManaWidget;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TObjectPtr<UHunterHUD_XPWidget>      XPWidget;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TObjectPtr<UStatusEffectHUDWidget>   StatusEffectWidget;

	// ─────────────────────────────────────────────────────────────────────────
	// Pawn-change wiring
	// ─────────────────────────────────────────────────────────────────────────

	/** Create all stat widgets and add them to the viewport. */
	void CreateStatWidgets();

	/**
	 * Bind or rebind all stat widgets to the given character.
	 * Safely releases previous bindings before rebinding.
	 */
	void BindWidgetsToCharacter(APHBaseCharacter* Character);

	/** Callback for APlayerController::OnPossessedPawnChanged — handles respawn. */
	UFUNCTION()
	void HandlePawnChanged(APawn* OldPawn, APawn* NewPawn);
};
