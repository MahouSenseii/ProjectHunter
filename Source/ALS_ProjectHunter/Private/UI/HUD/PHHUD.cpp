// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/HUD/PHHUD.h"

#include "Blueprint/UserWidget.h"
#include "Components/CanvasPanel.h"
#include "UI/WidgetController/PHOverlayWidgetController.h"

void APHHUD::BeginPlay()
{
	Super::BeginPlay();

	if (!TooltipCanvas && TooltipContainerClass)
	{
		if (UUserWidget* TooltipContainer = CreateWidget<UUserWidget>(GetWorld(), TooltipContainerClass))
		{
			TooltipCanvas = Cast<UCanvasPanel>(TooltipContainer->GetRootWidget());
			if (TooltipCanvas)
			{
				TooltipContainer->AddToViewport(100);
				UE_LOG(LogTemp, Log, TEXT("✅ TooltipCanvas created successfully"));
			}
		}
	}

	GetOrCreateTooltipCanvas();
}

UCanvasPanel* APHHUD::GetOrCreateTooltipCanvas()
{
	// If already created, return it
	if (TooltipCanvas)
	{
		return TooltipCanvas;
	}

	//Check if the class is set
	if (!TooltipContainerClass)
	{
		UE_LOG(LogTemp, Error, TEXT("❌ TooltipContainerClass is NOT SET in BP_PHHUD! Please assign WBP_TooltipContainer"));
		return nullptr;
	}

	// Create the container widget
	UUserWidget* TooltipContainer = CreateWidget<UUserWidget>(GetWorld(), TooltipContainerClass);
	if (!TooltipContainer)
	{
		UE_LOG(LogTemp, Error, TEXT("❌ Failed to create TooltipContainer widget"));
		return nullptr;
	}

	// Get the canvas panel from the root widget
	TooltipCanvas = Cast<UCanvasPanel>(TooltipContainer->GetRootWidget());
	if (!TooltipCanvas)
	{
		UE_LOG(LogTemp, Error, TEXT("❌ TooltipContainer root widget is not a Canvas Panel!"));
		return nullptr;
	}

	// Add to viewport with high Z-order
	TooltipContainer->AddToViewport(100);
	UE_LOG(LogTemp, Log, TEXT("✅ TooltipCanvas created and added to viewport"));

	return TooltipCanvas;
}


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
