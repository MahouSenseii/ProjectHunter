// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/HorizontalBox.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Library/PHItemStructLibrary.h"
#include "UI/Widgets/PHUserWidget.h"
#include "RequirementsBox.generated.h"

/**
 * 
 */
UCLASS()
class ALS_PROJECTHUNTER_API URequirementsBox : public UPHUserWidget
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, Category = "Item")
	FEquippableItemData ItemData;

	

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget)) UHorizontalBox* RequirementsBox;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget)) UScaleBox* RequirementsScaleBox;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget)) USizeBox* RequirementsSizeBox;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget)) UHorizontalBox* STRBox;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget)) UHorizontalBox* INTBox;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget)) UHorizontalBox* DEXBox;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget)) UHorizontalBox* ENDBox;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget)) UHorizontalBox* COVBox;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget)) UHorizontalBox* LUCKBox;
    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget)) UHorizontalBox* LVLBox;
    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget)) UHorizontalBox* AFFBox;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget)) UTextBlock* STRValue;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BintdWidget)) UTextBlock* INTValue;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget)) UTextBlock* DEXValue;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidge)) UTextBlock* ENDValue;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget)) UTextBlock* AFFValue;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget)) UTextBlock* COVValue;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget)) UTextBlock* LUCKValue;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget)) UTextBlock* LVLValue;
	
	void SetItemRequirements(const FEquippableItemData& PassedItemData, APHBaseCharacter* Character);
};
