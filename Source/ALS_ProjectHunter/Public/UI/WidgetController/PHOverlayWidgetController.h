// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/PHAttributeSet.h"
#include "UI/WidgetController/PHWidgetController.h"
#include "PHOverlayWidgetController.generated.h"


/**
 * @class UPHOverlayWidgetController
 * @brief A controller responsible for managing overlay widgets in an application.
 *
 * The UPHOverlayWidgetController class is designed to provide functionality for
 * handling and managing overlay widgets within a given context. This class
 * facilitates the adding, removing, and updating of overlay widgets, as well
 * as managing their lifecycle and interaction with other widgets.
 *
 * This class is typically used in applications where overlay elements need to
 * be dynamically managed, such as contextual menus, notifications, or interactive
 * elements that overlay other UI components.
 *
 * Key Features:
 * - Dynamically manage the lifecycle of overlay widgets.
 * - Add and remove overlay elements in real-time.
 * - Maintain clean organization of widget layers and ensure interactions occur as intended.
 *
 * Thread-safety: This class is not thread-safe unless specified otherwise.
 */

UCLASS(BlueprintType, Blueprintable)
class ALS_PROJECTHUNTER_API UPHOverlayWidgetController : public UPHWidgetController
{
	GENERATED_BODY()

public:

	virtual void BroadcastInitialValues() override;
	virtual void BindCallbacksToDependencies() override;

protected:
	UFUNCTION()
	void OnXPChange(int32 NewXP);

	

public:
	
	UPROPERTY(BlueprintAssignable, Category = "GAS|Attributes")
	FOnAttributeChangeSignature OnHealthChange;

	UPROPERTY(BlueprintAssignable, Category = "GAS|Attributes")
	FOnAttributeChangeSignature OnMaxEffectiveHealthChange;

	UPROPERTY(BlueprintAssignable, Category = "GAS|Attributes")
	FOnAttributeChangeSignature OnMaxHealthChange;

	UPROPERTY(BlueprintAssignable, Category = "GAS|Attributes")
	FOnAttributeChangeSignature OnHealthReservedChange;

	UPROPERTY(BlueprintAssignable, Category = "GAS|Attributes")
	FOnAttributeChangeSignature OnStaminaChange;

	UPROPERTY(BlueprintAssignable, Category = "GAS|Attributes")
	FOnAttributeChangeSignature OnMaxStaminaChange;

	UPROPERTY(BlueprintAssignable, Category = "GAS|Attributes")
	FOnAttributeChangeSignature OnMaxEffectiveStaminaChange;

	UPROPERTY(BlueprintAssignable, Category = "GAS|Attributes")
	FOnAttributeChangeSignature OnStaminaReservedChange;

	UPROPERTY(BlueprintAssignable, Category = "GAS|Attributes")
	FOnAttributeChangeSignature OnManaChange;

	UPROPERTY(BlueprintAssignable, Category = "GAS|Attributes")
	FOnAttributeChangeSignature OnMaxManaChange;

	UPROPERTY(BlueprintAssignable, Category = "GAS|Attributes")
	FOnAttributeChangeSignature OnMaxEffectiveManaChange;

	UPROPERTY(BlueprintAssignable, Category = "GAS|Attributes")
	FOnAttributeChangeSignature OnManaReservedChange;

	UPROPERTY(BlueprintAssignable, Category = "GAS|XP")
	FOnAttributeChangeSignature OnXPPercentChangeDelegate;
	

};