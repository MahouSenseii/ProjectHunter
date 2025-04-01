// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/CanvasPanel.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/ProgressBar.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "UI/Widgets/PHUserWidget.h"
#include "PHBaseProgressBar.generated.h"

/**
 * 
 */
UCLASS()
class ALS_PROJECTHUNTER_API UPHBaseProgressBar : public UPHUserWidget
{
	GENERATED_BODY()


public:

	virtual void NativeConstruct() override;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UPHOverlayWidgetController* OverlyWidgetController;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	EVitalType VitalType;

	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(BindWidget))
	UImage* ImageBackground;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(BindWidget))
	UScaleBox* ScaleBox;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(BindWidget))
	UOverlay* Overlay;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(BindWidget))
	USizeBox* SBHolder;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(BindWidget))
	UProgressBar* Ghost;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(BindWidget))
	UProgressBar* Main;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(BindWidget))
	UProgressBar* Reserved;
};
