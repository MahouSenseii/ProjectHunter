// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Library/WidgetEnumLibrary.h"
#include "UI/Widgets/PHUserWidget.h"
#include "MenuButton.generated.h"

class UTextBlock;
class UButton;
/**
 * 
 */
UCLASS()
class ALS_PROJECTHUNTER_API UMenuButton : public UPHUserWidget
{
	GENERATED_BODY()

public:
	// The low-level type of widget this button might open or switch to.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MenuButton")
	EWidgets WidgetType = EWidgets::AW_None;

	// The sub-tab or sub-switcher this button belongs to.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MenuButton")
	EWidgetsSwitcher WidgetSwitcher = EWidgetsSwitcher::WS_None;

	// Bound in UMG with the same variable name (BindWidget).
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UButton* MenuButton = nullptr;

	// Bound in UMG with the same variable name (BindWidget).
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UTextBlock* MenuTextBlock = nullptr;

public:
	// Called when the widget is constructed (i.e., after being created from C++).
	virtual void NativeConstruct() override;

	// Simple function to initialize or reset internal state.
	UFUNCTION(BlueprintCallable, Category="MenuButton")
	void PHInitialize() const;
};
