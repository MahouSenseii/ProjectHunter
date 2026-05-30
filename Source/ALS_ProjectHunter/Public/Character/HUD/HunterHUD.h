// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "HunterHUD.generated.h"

class APHBaseCharacter;
class APawn;
class UHunterMainHUDWidget;
class UItemInstance;
class UItemTooltipWidget;

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

	void ShowMashProgressWidget(const FText& Text, int32 INT32);
	void HideMashProgressWidget();

	UFUNCTION(BlueprintPure, Category = "HUD|Widgets")
	UHunterMainHUDWidget* GetMainHUDWidget() const { return MainHUDWidget; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, Category = "HUD|Widget Classes")
	TSubclassOf<UHunterMainHUDWidget> MainHUDWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UItemTooltipWidget> ItemTooltipWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TObjectPtr<UItemTooltipWidget> ItemTooltipWidget;

private:
	UPROPERTY()
	TObjectPtr<UHunterMainHUDWidget> MainHUDWidget;

	void CreateMainHUDWidget();
	void BindWidgetsToCharacter(APHBaseCharacter* Character) const;

	UFUNCTION()
	void HandlePawnChanged(APawn* OldPawn, APawn* NewPawn);
};
