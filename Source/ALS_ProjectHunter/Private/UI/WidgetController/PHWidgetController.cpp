// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/WidgetController/PHWidgetController.h"

void UPHWidgetController::SetWidgetControllerParams(const FWidgetControllerParams& WCParams)
{
	PlayerController = WCParams.PlayerController;
	PlayerState = WCParams.PlayerState;
	AbilitySystemComponent = WCParams.AbilitySystemComponent;
	AttributeSet = WCParams.AttributeSet;
}

void UPHWidgetController::BroadcastInitialValues()
{
}

void UPHWidgetController::BindCallbacksToDependencies()
{
	
}

