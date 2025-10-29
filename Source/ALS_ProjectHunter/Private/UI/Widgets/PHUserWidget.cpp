// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/Widgets/PHUserWidget.h"
#include "Character/PHBaseCharacter.h"
#include "Character/ALSPlayerController.h"
#include "UI/WidgetController/PHOverlayWidgetController.h"

void UPHUserWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	ALSPlayerController =  Cast<AALSPlayerController>(GetOwningPlayer());
}

void UPHUserWidget::NativeConstruct()
{
	Super::NativeConstruct();
	OwnerCharacter = Cast<APHBaseCharacter>(ALSPlayerController->PossessedCharacter);
	if(OwnerCharacter)
	{
		PHAttributeSet = Cast<UPHAttributeSet>(OwnerCharacter->GetAttributeSet());
	}
}



void UPHUserWidget::SetWidgetOwner(APHBaseCharacter* InWidgetOwner)
{
	OwnerCharacter = InWidgetOwner;
}

void UPHUserWidget::SetWidgetController(UObject* InWidgetController)
{
	WidgetController = InWidgetController;
	WidgetControllerSet();
}
