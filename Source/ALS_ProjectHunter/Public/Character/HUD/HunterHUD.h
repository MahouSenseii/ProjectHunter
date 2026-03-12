// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "HunterHUD.generated.h"

class UItemTooltipWidget;
/**
 * 
 */
UCLASS()
class ALS_PROJECTHUNTER_API AHunterHUD : public AHUD
{
	GENERATED_BODY()
public:
	// Tooltip management
	UFUNCTION(BlueprintCallable)
	void ShowItemTooltip(UItemInstance* Item, FVector2D ScreenPosition);
    
	UFUNCTION(BlueprintCallable)
	void HideItemTooltip();
	void ShowMashProgressWidget(const FText& Text, int32 INT32);
	void HideMashProgressWidget();

protected:
	virtual void BeginPlay() override;
    
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UItemTooltipWidget> ItemTooltipWidgetClass;
    
	UPROPERTY()
	UItemTooltipWidget* ItemTooltipWidget;
};
