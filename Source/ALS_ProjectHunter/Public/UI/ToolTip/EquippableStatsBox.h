// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "RequirementsBox.h"
#include "UI/Widgets/PHUserWidget.h"
#include "EquippableStatsBox.generated.h"

/**
 * 
 */
UCLASS()
class ALS_PROJECTHUNTER_API UEquippableStatsBox : public UPHUserWidget
{
	GENERATED_BODY()

public:
	
	//  Equippable Stats	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UHorizontalBox* EDBox;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UTextBlock* EDValue;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UHorizontalBox* PhysicalDamageBox;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UTextBlock* PhysicalDamageValue;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UHorizontalBox* ArmourBox;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UTextBlock* ArmourValue;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UHorizontalBox* CritChanceBox;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UTextBlock* CritChanceValue;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UHorizontalBox* AtkSpeedBox; // atk per sec

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UTextBlock* AtkSpeedValue;// atk per sec

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UHorizontalBox* CastTimeBox;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UTextBlock* CastTimeValue;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UHorizontalBox* ManaCostBox;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UTextBlock* ManaCostValue;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UHorizontalBox* StaminaCostBox;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UTextBlock* StaminaCostValue;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UHorizontalBox* WeaponRangeBox;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UTextBlock* WeaponRangeValue;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	URequirementsBox* RequirementsBox;
};
