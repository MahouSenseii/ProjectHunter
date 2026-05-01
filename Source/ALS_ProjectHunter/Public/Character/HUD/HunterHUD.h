// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "HunterHUD.generated.h"

class UItemTooltipWidget;
class UItemInstance;
class APawn;

class UHunterHUDResourceWidget;
class UHunterHUD_XPWidget;
class UStatusEffectHUDWidget;
class APHBaseCharacter;

DECLARE_LOG_CATEGORY_EXTERN(LogHunterHUD, Log, All);

/**
 * AHunterHUD — root HUD actor for the player.
 *
 * Owns the resource widgets (Health, Stamina, Mana) and the XP widget, and
 * keeps them in sync with the locally controlled pawn. Pawn changes are
 * handled automatically via APlayerController::OnPossessedPawnChanged.
 *
 * Setup in Blueprint defaults:
 *   1. Set HealthWidgetClass, StaminaWidgetClass, ManaWidgetClass to BP children
 *      of UHunterHUDResourceWidget — each with ResourceType set to the matching pool.
 *   2. Set XPWidgetClass and StatusEffectWidgetClass.
 *   3. Assign this HUD class to the GameMode or PlayerController.
 */
UCLASS()
class ALS_PROJECTHUNTER_API AHunterHUD : public AHUD
{
	GENERATED_BODY()

public:
	// ─────────────────────────────────────────────────────────────────────────
	// Tooltip management
	// ─────────────────────────────────────────────────────────────────────────

	UFUNCTION(BlueprintCallable)
	void ShowItemTooltip(UItemInstance* Item, FVector2D ScreenPosition);

	UFUNCTION(BlueprintCallable)
	void HideItemTooltip();

	void ShowMashProgressWidget(const FText& Text, int32 INT32);
	void HideMashProgressWidget();

	// ─────────────────────────────────────────────────────────────────────────
	// Widget accessors
	// ─────────────────────────────────────────────────────────────────────────

	UFUNCTION(BlueprintPure, Category = "HUD|Widgets")
	UHunterHUDResourceWidget* GetHealthWidget()  const { return HealthWidget;  }

	UFUNCTION(BlueprintPure, Category = "HUD|Widgets")
	UHunterHUDResourceWidget* GetStaminaWidget() const { return StaminaWidget; }

	UFUNCTION(BlueprintPure, Category = "HUD|Widgets")
	UHunterHUDResourceWidget* GetManaWidget()    const { return ManaWidget;    }

	UFUNCTION(BlueprintPure, Category = "HUD|Widgets")
	UHunterHUD_XPWidget*      GetXPWidget()      const { return XPWidget;      }

	UFUNCTION(BlueprintPure, Category = "HUD|Widgets")
	UStatusEffectHUDWidget*   GetStatusEffectWidget() const { return StatusEffectWidget; }

protected:
	virtual void BeginPlay() override;

	// ─────────────────────────────────────────────────────────────────────────
	// Widget class references — set in Blueprint defaults
	// ─────────────────────────────────────────────────────────────────────────

	/** BP child of UHunterHUDResourceWidget with ResourceType = Health */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Widget Classes")
	TSubclassOf<UHunterHUDResourceWidget> HealthWidgetClass;

	/** BP child of UHunterHUDResourceWidget with ResourceType = Stamina */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Widget Classes")
	TSubclassOf<UHunterHUDResourceWidget> StaminaWidgetClass;

	/** BP child of UHunterHUDResourceWidget with ResourceType = Mana */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Widget Classes")
	TSubclassOf<UHunterHUDResourceWidget> ManaWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "HUD|Widget Classes")
	TSubclassOf<UHunterHUD_XPWidget> XPWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "HUD|Widget Classes")
	TSubclassOf<UStatusEffectHUDWidget> StatusEffectWidgetClass;

	// ─────────────────────────────────────────────────────────────────────────
	// Tooltip
	// ─────────────────────────────────────────────────────────────────────────

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UItemTooltipWidget> ItemTooltipWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TObjectPtr<UItemTooltipWidget> ItemTooltipWidget;

private:
	// ─────────────────────────────────────────────────────────────────────────
	// Live widget instances
	// ─────────────────────────────────────────────────────────────────────────

	UPROPERTY()
	TObjectPtr<UHunterHUDResourceWidget> HealthWidget;

	UPROPERTY()
	TObjectPtr<UHunterHUDResourceWidget> StaminaWidget;

	UPROPERTY()
	TObjectPtr<UHunterHUDResourceWidget> ManaWidget;

	UPROPERTY()
	TObjectPtr<UHunterHUD_XPWidget> XPWidget;

	UPROPERTY()
	TObjectPtr<UStatusEffectHUDWidget> StatusEffectWidget;

	// ─────────────────────────────────────────────────────────────────────────
	// Helpers
	// ─────────────────────────────────────────────────────────────────────────

	void CreateStatWidgets();
	void BindWidgetsToCharacter(APHBaseCharacter* Character);

	UFUNCTION()
	void HandlePawnChanged(APawn* OldPawn, APawn* NewPawn);
};
