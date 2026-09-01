// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#pragma once

#include "CoreMinimal.h"
#include "UI/HUD/HunterHUDBaseWidget.h"
#include "UI/HUD/HunterHUDResourceWidget.h"
#include "HunterMainHUDWidget.generated.h"

class APHBaseCharacter;
class AActor;
class UCharacterProgressionManager;
class UHunterHUDResourceWidget;
class UHunterHUD_XPWidget;
class UStatusEffectHUDWidget;
class UTextBlock;
class UPHRunStatusWidget;
class UPHFloorBannerWidget;

/**
 * Root player HUD widget. Create a Blueprint child and place the resource, XP,
 * and status widgets in its designer so the designer controls size and layout.
 */
UCLASS(BlueprintType, Blueprintable)
class ALS_PROJECTHUNTER_API UHunterMainHUDWidget : public UHunterHUDBaseWidget
{
	GENERATED_BODY()

public:
	UHunterMainHUDWidget();

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void BindToCharacter(APHBaseCharacter* Character);

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void RemoveWidget();

	UFUNCTION(BlueprintPure, Category = "HUD|Widgets")
	UHunterHUDResourceWidget* GetHealthWidget() const;

	UFUNCTION(BlueprintPure, Category = "HUD|Widgets")
	UHunterHUDResourceWidget* GetStaminaWidget() const;

	UFUNCTION(BlueprintPure, Category = "HUD|Widgets")
	UHunterHUDResourceWidget* GetManaWidget() const;

	UFUNCTION(BlueprintPure, Category = "HUD|Widgets")
	UPHRunStatusWidget* GetRunStatusWidget() const;

	UFUNCTION(BlueprintPure, Category = "HUD|Widgets")
	UPHFloorBannerWidget* GetFloorBannerWidget() const;

	UFUNCTION(BlueprintPure, Category = "HUD|Widgets")
	UHunterHUD_XPWidget* GetXPWidget() const { return XPWidget; }

	UFUNCTION(BlueprintPure, Category = "HUD|Widgets")
	UStatusEffectHUDWidget* GetStatusEffectWidget() const { return StatusEffectWidget; }

protected:
	UPROPERTY(BlueprintReadOnly, Category = "HUD|Widgets", meta = (BindWidgetOptional))
	TObjectPtr<UPHRunStatusWidget> RunStatusWidget;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Widgets", meta = (BindWidgetOptional))
	TObjectPtr<UPHFloorBannerWidget> FloorBannerWidget;

	virtual void NativePreConstruct() override;
	virtual void NativeInitializeForCharacter(APHBaseCharacter* Character) override;
	virtual void NativeReleaseCharacter() override;
	virtual void NativeDestruct() override;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Widgets", meta = (BindWidgetOptional))
	TObjectPtr<UHunterHUDResourceWidget> HealthWidget;

	/** Legacy designer name used by the shipped player HUD asset. */
	UPROPERTY(BlueprintReadOnly, Category = "HUD|Widgets", meta = (BindWidgetOptional))
	TObjectPtr<UHunterHUDResourceWidget> Health;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Widgets", meta = (BindWidgetOptional))
	TObjectPtr<UHunterHUDResourceWidget> StaminaWidget;

	/** Legacy designer name used by the shipped player HUD asset. */
	UPROPERTY(BlueprintReadOnly, Category = "HUD|Widgets", meta = (BindWidgetOptional))
	TObjectPtr<UHunterHUDResourceWidget> Stamina;

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

	/** Short designer name for the mana resource child. */
	UPROPERTY(BlueprintReadOnly, Category = "HUD|Widgets", meta = (BindWidgetOptional))
	TObjectPtr<UHunterHUDResourceWidget> Mana;

	/** Legacy preset retained for Blueprint compatibility. Never applied automatically; style the Health child instead. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD|Resource Style")
	FPHHUDResourceVisualStyle HealthResourceStyle;

	/** Legacy preset retained for Blueprint compatibility. Never applied automatically; style the Mana child instead. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD|Resource Style")
	FPHHUDResourceVisualStyle ManaResourceStyle;

	/** Legacy preset retained for Blueprint compatibility. Never applied automatically; style the Stamina child instead. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD|Resource Style")
	FPHHUDResourceVisualStyle StaminaResourceStyle;

	/** Optional level label, updated from the existing progression owner. */
	UPROPERTY(BlueprintReadOnly, Category = "HUD|Widgets", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PlayerLevelText;

	/** Optional Current / Max label using the Health child's authoritative values. */
	UPROPERTY(BlueprintReadOnly, Category = "HUD|Widgets", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HealthValueText;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Widgets", meta = (BindWidgetOptional))
	TObjectPtr<UHunterHUD_XPWidget> XPWidget;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Widgets", meta = (BindWidgetOptional))
	TObjectPtr<UStatusEffectHUDWidget> StatusEffectWidget;

private:
	UHunterHUDResourceWidget* FindResourceWidgetByName(FName WidgetName) const;
	void UnbindPresentationSources();
	void ClearPresentationText() const;
	void RefreshHealthValueText();

	UFUNCTION()
	void RefreshPlayerLevelText();

	UFUNCTION()
	void HandlePresentationCharacterDestroyed(AActor* DestroyedActor);

	TWeakObjectPtr<APHBaseCharacter> PresentationCharacter;
	TWeakObjectPtr<UHunterHUDResourceWidget> PresentationHealthWidget;
	TWeakObjectPtr<UCharacterProgressionManager> PresentationProgressionManager;
	FDelegateHandle HealthStateChangedHandle;

	bool bLoggedMissingManaWidget = false;
};
