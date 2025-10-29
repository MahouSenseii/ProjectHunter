// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HairStrandsDefinitions.h"
#include "AbilitySystem/PHAttributeSet.h"
#include "Blueprint/UserWidget.h"
#include "PHUserWidget.generated.h"

class AALSPlayerController;
class APHBaseCharacter;
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

	virtual void NativePreConstruct() override;

	virtual void NativeConstruct() override;
	void SetWidgetOwner(APHBaseCharacter* InWidgetOwner);

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<APHBaseCharacter> OwnerCharacter;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UPHAttributeSet> PHAttributeSet;
	
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<AALSPlayerController> ALSPlayerController;
	
	UFUNCTION(BlueprintCallable)
	void SetWidgetController(UObject* InWidgetController);
	
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UObject> WidgetController;

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<UObject> OwnerWidget;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<APHBaseCharacter> Vendor;
	
protected:
	

	UFUNCTION(BlueprintImplementableEvent)
	void WidgetControllerSet();
};
