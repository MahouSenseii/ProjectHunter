// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/PHUserWidget.h"

#include "UI/WidgetController/PHOverlayWidgetController.h"

void UPHUserWidget::SetWidgetController(UObject* InWidgetController)
{
	WidgetController = InWidgetController;
	WidgetControllerSet();
}
