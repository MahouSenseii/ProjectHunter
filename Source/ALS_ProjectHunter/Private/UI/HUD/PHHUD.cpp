// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/HUD/PHHUD.h"

#include "Blueprint/UserWidget.h"
#include "UI/WidgetController/PHOverlayWidgetController.h"

UAttributeMenuWidgetController* APHHUD::GetAttributeWidgetController(const FWidgetControllerParams& WCParams)
{
	if(AttributeMenuWidgetController == nullptr)
	{
		AttributeMenuWidgetController = NewObject<UAttributeMenuWidgetController>(this, AttributeMenuWidgetControllerClass);
		AttributeMenuWidgetController->SetWidgetControllerParams(WCParams);
		AttributeMenuWidgetController->BindCallbacksToDependencies();
	}
	return AttributeMenuWidgetController;
}

void APHHUD::InitOverlay(APlayerController* PC, APlayerState* PS, UAbilitySystemComponent* ASC, UAttributeSet* AS)
{
	checkf(OverlayWidgetClass, TEXT("OverlayWidgetClass uninitialized, please fill out BP_PHHUD."))
	checkf(OverlayWidgetControllerClass, TEXT("OverlayWidgetClass uninitialized, please fill out BP_PHHUD."))
	
	UUserWidget* CreatedWidget = CreateWidget<UPHUserWidget>(GetWorld(), OverlayWidgetClass );
	OverlayWidget = Cast<UPHUserWidget>(CreatedWidget);
	const FWidgetControllerParams WidgetControllerParams(PC,PS,ASC,AS);
	UPHOverlayWidgetController* WidgetController = GetOverlayWidgetController(WidgetControllerParams);

	OverlayWidget->SetWidgetController(WidgetController);
	WidgetController->BroadcastInitialValues();
	CreatedWidget->AddToViewport();
	
}



UPHOverlayWidgetController* APHHUD::GetOverlayWidgetController(const FWidgetControllerParams& WCParams)
{
	if(OverlayWidgetController == nullptr)
	{
		OverlayWidgetController = NewObject<UPHOverlayWidgetController>(this, OverlayWidgetControllerClass);
		OverlayWidgetController->SetWidgetControllerParams(WCParams);
		OverlayWidgetController->BindCallbacksToDependencies();
	}
	return  OverlayWidgetController;
}
