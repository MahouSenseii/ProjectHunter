// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/TextBlock.h"
#include "UI/Widgets/PHUserWidget.h"
#include "VitalToText.generated.h"

/**
 * 
 */
UCLASS()
class ALS_PROJECTHUNTER_API UVitalToText : public UPHUserWidget
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vitals")
	EVitalType VitalType;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(BindWidget))
	UTextBlock* CurrentValueAsText;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(BindWidget))
	UTextBlock* MaxValueAsText;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(BindWidget))
	UTextBlock* ReservedAmountAsText;

	
	
};
