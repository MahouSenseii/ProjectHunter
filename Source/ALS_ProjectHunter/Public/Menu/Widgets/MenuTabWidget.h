// Copyright © 2025 MahouSensei
// Author: Quentin Davis

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Menu/Library/MenuEnumLibrary.h"
#include "Menu/Library/MenuStructLibrary.h"
#include "MenuTabWidget.generated.h"

class UButton;
class UImage;
class UTextBlock;
class UTexture2D;

/** Fired when the user clicks this tab. Carries its MenuType so the bar knows who was clicked. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTabClicked, EMenuType, MenuType);

/**
 * UMenuTabWidget — one tab button inside the tab bar.
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class ALS_PROJECTHUNTER_API UMenuTabWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	// ─────────────────────────────────────────────────────────────────────────
	// Public API — called by UMenuTabBarWidget
	// ─────────────────────────────────────────────────────────────────────────

	/** Initialize this tab from an entry. Call once after CreateWidget. */
	UFUNCTION(BlueprintCallable, Category = "Tab")
	void SetTabData(const FMenuEntry& Entry);

	/** Drive selected visual state from the bar — does not fire OnTabClicked. */
	UFUNCTION(BlueprintCallable, Category = "Tab")
	void SetSelected(bool bInSelected);

	UFUNCTION(BlueprintPure, Category = "Tab")
	EMenuType GetMenuType() const { return MenuType; }

	UFUNCTION(BlueprintPure, Category = "Tab")
	bool IsSelected() const { return bIsSelected; }

	// ─────────────────────────────────────────────────────────────────────────
	// Delegate — bar binds to this on spawn
	// ─────────────────────────────────────────────────────────────────────────

	UPROPERTY(BlueprintAssignable, Category = "Tab|Events")
	FOnTabClicked OnTabClicked;

protected:

	// ─────────────────────────────────────────────────────────────────────────
	// UUserWidget overrides
	// ─────────────────────────────────────────────────────────────────────────

	virtual void NativeConstruct() override;

	// ─────────────────────────────────────────────────────────────────────────
	// Blueprint visual hooks — implement the look in BP
	// ─────────────────────────────────────────────────────────────────────────

	/** Called when the mouse enters the tab. Drive hover visuals here. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Tab|Events")
	void OnTabHovered();

	/** Called when the mouse leaves the tab. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Tab|Events")
	void OnTabUnhovered();

	/** Called when this tab becomes the active selection. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Tab|Events")
	void OnTabSelected();

	/** Called when another tab is selected and this one is deactivated. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Tab|Events")
	void OnTabDeselected();

	// ─────────────────────────────────────────────────────────────────────────
	// Bound widgets — name these exactly in the BP designer
	// ─────────────────────────────────────────────────────────────────────────

	/** Root button — wrap your icon + label inside this. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> TabButton;

	/** Icon image inside the button. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> TabIcon;

	/** Label text inside the button. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TabLabel;

	// ─────────────────────────────────────────────────────────────────────────
	// State
	// ─────────────────────────────────────────────────────────────────────────

	UPROPERTY(BlueprintReadOnly, Category = "Tab")
	EMenuType MenuType = EMenuType::MT_None;

	UPROPERTY(BlueprintReadOnly, Category = "Tab")
	bool bIsSelected = false;

private:

	UFUNCTION()
	void HandleButtonClicked();

	UFUNCTION()
	void HandleButtonHovered();

	UFUNCTION()
	void HandleButtonUnhovered();
};
