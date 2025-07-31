// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Library/AttributeStructsLibrary.h"
#include "UI/WidgetController/PHWidgetController.h"
#include "AttributeMenuWidgetController.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAttributeInfoSignature, const FPHAttributeInfo, Info);


class UAttributeInfo;

/**
 * @class UAttributeMenuWidgetController
 * @brief Handles widget-related functionalities for attribute display in the UI.
 *
 * UAttributeMenuWidgetController extends UPHWidgetController to provide methods for
 * binding callbacks to attribute dependencies and broadcasting initial attribute values.
 * It uses the Gameplay Ability System to manage and respond to attribute changes.
 */
UCLASS(BlueprintType, Blueprintable)
class ALS_PROJECTHUNTER_API UAttributeMenuWidgetController : public UPHWidgetController
{
	GENERATED_BODY()

public:

	
	virtual void BindCallbacksToDependencies() override;
	
	virtual void BroadcastInitialValues() override;

	UPROPERTY(BlueprintAssignable, Category = "GAS|Attribute")
	FAttributeInfoSignature AttributeInfoDelegate;

protected:

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UAttributeInfo> AttributeInfo;

private:

	void BroadcastAttributeInfo(const FGameplayTag& AttributeTag, const FGameplayAttribute& Attribute) const;
};
