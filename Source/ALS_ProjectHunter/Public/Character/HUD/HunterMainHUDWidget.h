// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/HUD/HunterHUDBaseWidget.h"
#include "HunterMainHUDWidget.generated.h"

class APHBaseCharacter;
class UHunterHUDResourceWidget;
class UHunterHUD_XPWidget;
class UStatusEffectHUDWidget;

/**
 * Root player HUD widget. Create a Blueprint child and place the resource, XP,
 * and status widgets in its designer so the designer controls size and layout.
 */
UCLASS(BlueprintType, Blueprintable)
class ALS_PROJECTHUNTER_API UHunterMainHUDWidget : public UHunterHUDBaseWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void BindToCharacter(APHBaseCharacter* Character);

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void RemoveWidget();

	UFUNCTION(BlueprintPure, Category = "HUD|Widgets")
	UHunterHUDResourceWidget* GetHealthWidget() const;

	UFUNCTION(BlueprintPure, Category = "HUD|Widgets")
	UHunterHUDResourceWidget* GetStaminaWidget() const;

	UFUNCTION(BlueprintPure, Category = "HUD|Widgets")
	UHunterHUDResourceWidget* GetManaWidget() const { return ManaWidget; }

	UFUNCTION(BlueprintPure, Category = "HUD|Widgets")
	UHunterHUD_XPWidget* GetXPWidget() const { return XPWidget; }

	UFUNCTION(BlueprintPure, Category = "HUD|Widgets")
	UStatusEffectHUDWidget* GetStatusEffectWidget() const { return StatusEffectWidget; }

protected:
	virtual void NativeInitializeForCharacter(APHBaseCharacter* Character) override;
	virtual void NativeReleaseCharacter() override;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Widgets", meta = (BindWidgetOptional))
	TObjectPtr<UHunterHUDResourceWidget> HealthWidget;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Widgets", meta = (BindWidgetOptional))
	TObjectPtr<UHunterHUDResourceWidget> StaminaWidget;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Widgets", meta = (BindWidgetOptional))
	TObjectPtr<UHunterHUDResourceWidget> HealthBar;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Widgets", meta = (BindWidgetOptional))
	TObjectPtr<UHunterHUDResourceWidget> StaminaBar;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Widgets", meta = (BindWidgetOptional))
	TObjectPtr<UHunterHUDResourceWidget> WPB_HealthBar;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Widgets", meta = (BindWidgetOptional))
	TObjectPtr<UHunterHUDResourceWidget> WPB_StaminaBar;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Widgets", meta = (BindWidgetOptional))
	TObjectPtr<UHunterHUDResourceWidget> ManaWidget;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Widgets", meta = (BindWidgetOptional))
	TObjectPtr<UHunterHUD_XPWidget> XPWidget;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Widgets", meta = (BindWidgetOptional))
	TObjectPtr<UStatusEffectHUDWidget> StatusEffectWidget;
};
