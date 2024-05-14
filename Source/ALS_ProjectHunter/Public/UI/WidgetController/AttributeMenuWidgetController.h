// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Library/AttributeStructsLibrary.h"
#include "UI/WidgetController/PHWidgetController.h"
#include "AttributeMenuWidgetController.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAttributeInfoSignature, const FPHAttributeInfo, Info);


class UAttributeInfo;

/**
 * 
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
