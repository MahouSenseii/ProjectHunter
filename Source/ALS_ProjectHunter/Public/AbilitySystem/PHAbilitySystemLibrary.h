// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UI/WidgetController/AttributeMenuWidgetController.h"
#include "UI/WidgetController/PHOverlayWidgetController.h"
#include "PHAbilitySystemLibrary.generated.h"

/**
 * 
 */
UCLASS()
class ALS_PROJECTHUNTER_API UPHAbilitySystemLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintPure, Category = "PH AbilitySystemLibrary|WidgetController")
	static UPHOverlayWidgetController* GetOverlayWidgetController(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "PH AbilitySystemLibrary|WidgetController")
	static UAttributeMenuWidgetController* GetAttibuteMenuWidgetController(const UObject* WorldContextObject);
};
