// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Library/PHItemEnumLibrary.h"
#include "UI/Widgets/PHUserWidget.h"
#include "MinMaxBox.generated.h"

/**
 * UMinMaxBox is a class that represents a user widget used in the ALS_PROJECTHUNTER game project.
 * It inherits from UPHUserWidget and provides functionality specific to the min-max box element.
 */
UCLASS()
class ALS_PROJECTHUNTER_API UMinMaxBox : public UPHUserWidget
{
	GENERATED_BODY()

public:
	

	UFUNCTION()
	void SetColorBaseOnType(EDamageTypes Type) const;

	UFUNCTION()
	void SetMinMaxText(float MaxValue, float MinValue) const;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UTextBlock* MinText;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UTextBlock* SpacerText;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UTextBlock* MaxText;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UVerticalBox* VerticalBox;
	
};
