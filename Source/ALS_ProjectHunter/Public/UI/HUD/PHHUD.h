// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/CanvasPanel.h"
#include "GameFramework/HUD.h"
#include "UI/WidgetController/AttributeMenuWidgetController.h"
#include "UI/WidgetController/PHWidgetController.h"
#include "UI/Widgets/PHUserWidget.h"
#include "PHHUD.generated.h"

/**
 * 
 */
UCLASS()
class ALS_PROJECTHUNTER_API APHHUD : public AHUD
{
	GENERATED_BODY()
public:

	virtual void BeginPlay() override;
	UCanvasPanel* GetOrCreateTooltipCanvas();
	UAttributeMenuWidgetController*  GetAttributeWidgetController(const FWidgetControllerParams& WCParams);
	void InitOverlay(APlayerController* PC, APlayerState* PS, UAbilitySystemComponent* ASC, UAttributeSet* AS);
	UPHOverlayWidgetController* GetOverlayWidgetController(const FWidgetControllerParams& WCParams);	
protected:


public:
	UPROPERTY()
	TObjectPtr<UPHUserWidget> OverlayWidget;

	UPROPERTY()
	TObjectPtr<UCanvasPanel> TooltipCanvas;


protected:

	UPROPERTY(EditAnywhere, Category = "Tooltip")
	TSubclassOf<UUserWidget> TooltipContainerClass;

private:

	UPROPERTY(EditAnywhere)
	TSubclassOf<UPHUserWidget> OverlayWidgetClass;

	UPROPERTY()
	TObjectPtr<UPHOverlayWidgetController> OverlayWidgetController;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UPHOverlayWidgetController> OverlayWidgetControllerClass;

	UPROPERTY()
	TObjectPtr<UAttributeMenuWidgetController> AttributeMenuWidgetController;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UAttributeMenuWidgetController> AttributeMenuWidgetControllerClass;
};
