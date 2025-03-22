// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "UI/Widgets/PHUserWidget.h"
#include "MenuVitalInfo.generated.h"

/**
 * 
 */
UCLASS()
class ALS_PROJECTHUNTER_API UMenuVitalInfo : public UPHUserWidget
{
	GENERATED_BODY()


public:
	UPROPERTY(EditAnywhere, meta = (BindWidget), Category = "VitalInfo" )
	UImage* VitalImage;

	UPROPERTY(EditAnywhere, meta = (BindWidget), Category = "VitalInfo" )
	UProgressBar* VitalProgressBar;

	UPROPERTY(EditAnywhere, meta = (BindWidget), Category = "VitalInfo" )
	UTextBlock* VitalType;

	UPROPERTY(EditAnywhere, meta = (BindWidget), Category = "VitalInfo" )
	UTextBlock* VitalMin;

	UPROPERTY(EditAnywhere, meta = (BindWidget), Category = "VitalInfo" )
	UTextBlock* VitalMax;
};
