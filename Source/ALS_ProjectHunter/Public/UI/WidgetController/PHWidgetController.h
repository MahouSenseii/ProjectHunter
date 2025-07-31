// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "UObject/NoExportTypes.h"
#include "PHWidgetController.generated.h"

USTRUCT(BlueprintType)
struct FWidgetControllerParams
{
	GENERATED_BODY()

	FWidgetControllerParams(){}
	FWidgetControllerParams(APlayerController* PC, APlayerState* PS, UAbilitySystemComponent* ASC, UAttributeSet* AS)
	:PlayerController(PC),
	PlayerState(PS),
	AbilitySystemComponent(ASC),
	AttributeSet(AS) {}

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<APlayerController> PlayerController = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<APlayerState> PlayerState  = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent  = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UAttributeSet> AttributeSet  = nullptr;
	
};

/**
 * @class UPHWidgetController
 * @brief A controller class for managing and coordinating widgets within the application.
 *
 * The UPHWidgetController class is responsible for handling the lifecycle, state,
 * and behavior of widgets. This includes creating, updating, and removing widgets,
 * as well as managing interactions between widgets and other components of the
 * application.
 *
 * The class provides a centralized interface for widget operations, ensuring
 * consistency and reusability across the application.
 */
UCLASS()
class ALS_PROJECTHUNTER_API UPHWidgetController : public UObject
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable)
	void SetWidgetControllerParams(const FWidgetControllerParams& WCParams);

	UFUNCTION(BlueprintCallable)
	virtual void BroadcastInitialValues();

	UFUNCTION()
	virtual void BindCallbacksToDependencies();

	UFUNCTION()
	APlayerController* GetPlayerController() const { return PlayerController; }

protected:

	UPROPERTY(BlueprintReadOnly, Category = " WidgetController")
	TObjectPtr<APlayerController> PlayerController;

	UPROPERTY(BlueprintReadOnly, Category = " WidgetController")
	TObjectPtr<APlayerState> PlayerState;

	

	UPROPERTY(BlueprintReadOnly, Category = " WidgetController")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(BlueprintReadOnly, Category = " WidgetController")
	TObjectPtr<UAttributeSet> AttributeSet;

	
};
