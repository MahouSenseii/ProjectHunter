// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PHUserWidget.generated.h"

class UPHOverlayWidgetController;
struct FWidgetControllerParams;

/**
 * 
 */
UCLASS()
class ALS_PROJECTHUNTER_API UPHUserWidget : public UUserWidget
{
	GENERATED_BODY()
public:

	UFUNCTION(BlueprintCallable)
	void SetWidgetController(UObject* InWidgetController);
	
	
	
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UObject> WidgetController;

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<UObject> OwnerWidget;

protected:

	UFUNCTION(BlueprintImplementableEvent)
	void WidgetControllerSet();


public:
	
		
};
