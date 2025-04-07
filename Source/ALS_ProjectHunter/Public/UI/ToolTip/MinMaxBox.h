// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/TextBlock.h"
#include "Components/HorizontalBox.h"
#include "Library/PHItemEnumLibrary.h"
#include "UI/Widgets/PHUserWidget.h"
#include "MinMaxBox.generated.h"

/**
 * UMinMaxBox is a class that represents a user widget used in the ALS_PROJECTHUNTER game project.
 * It inherits from UPHUserWidget and provides functionality specific to the min-max box element.
 */
UCLASS(BlueprintType)
class ALS_PROJECTHUNTER_API UMinMaxBox : public UPHUserWidget
{
	GENERATED_BODY()

public:

	void Init();

	UFUNCTION()
	void SetColorBaseOnType(EDamageTypes Type) const;

	UFUNCTION()
	void SetMinMaxText(float MaxValue, float MinValue) const;

	UFUNCTION()
	void SetFontSize(float InValue) const;
	
	UPROPERTY()
	UTextBlock* MinText;

	UPROPERTY()
	UTextBlock* SpacerText;

	UPROPERTY()
	UTextBlock* MaxText;

	UPROPERTY()
	UHorizontalBox* RootBox;
	
};
