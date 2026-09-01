// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PHFloorBannerWidget.generated.h"

class UTextBlock;
class UWidgetAnimation;

/**
 * The "FLOOR N" announcement shown on arriving at a new floor.
 *
 * C++ decides when it appears and what floor it names; the Blueprint decides what it looks like and
 * how it animates. That split is the point: a banner is a piece of presentation, and the canvas
 * text it replaces could never be more than debug output.
 *
 * The widget is created once and reused, so a Blueprint may run an entry animation on ShowFloor and
 * is never rebuilt mid-run.
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class ALS_PROJECTHUNTER_API UPHFloorBannerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * Announces a floor. Called on arrival, and again on every later floor of the same run.
	 *
	 * Existing Blueprint implementations remain supported. The optional native bindings set the
	 * number and play the authored FloorOpen animation before this customization event fires.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD|Run")
	void OnShowFloor(int32 FloorNumber);

	/** Sets the floor and raises OnShowFloor. */
	UFUNCTION(BlueprintCallable, Category = "HUD|Run")
	void ShowFloor(int32 FloorNumber);

	UFUNCTION(BlueprintCallable, Category = "HUD|Run")
	void HideBanner();

	UFUNCTION(BlueprintPure, Category = "HUD|Run")
	UWidgetAnimation* GetEntryAnimation() const;

	/** The floor most recently announced, for a Blueprint that would rather bind than take the event. */
	UPROPERTY(BlueprintReadOnly, Category = "HUD|Run")
	int32 CurrentFloor = 0;

protected:
	virtual void OnAnimationFinished_Implementation(const UWidgetAnimation* Animation) override;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Run", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> FloorNumberText;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Run", meta = (BindWidgetOptional))
	TObjectPtr<UWidget> BannerPanel;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "HUD|Run", meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> FloorOpen;
};
