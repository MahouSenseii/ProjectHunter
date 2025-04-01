// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Library/PHItemEnumLibrary.h"
#include "Components/TextBlock.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Item/EquippableItem.h"
#include "UI/ToolTip/PHBaseToolTip.h"
#include "UI/ToolTip/EquippableStatsBox.h"
#include "EquippableToolTip.generated.h"

class UBaseItem;
/**
 * 
 */

USTRUCT(BlueprintType)
struct FStatCheckInfo
{
	GENERATED_BODY()

	// Enum for the stat type
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EItemStats StatType;

	// Pointer to the size box widget
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USizeBox* SizeBox;

	// Pointer to the text block widget
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTextBlock* TextBlock;
};

USTRUCT(BlueprintType)
struct FStatRequirementInfo
{
	GENERATED_BODY()

	// Enum for the stat type
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EItemRequiredStatsCategory StatType;

	// Pointer to the size box widget
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USizeBox* SizeBox;

	// Pointer to the text block widget
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTextBlock* TextBlock;
};
UCLASS(BlueprintType)
class ALS_PROJECTHUNTER_API UEquippableToolTip : public UPHBaseToolTip
{
	GENERATED_BODY()
	
public:

	virtual void NativeConstruct() override;
	virtual void InitializeToolTip() override;
	virtual void SetItemInfo(const FItemInformation& Item) override;

	void CreateAffixBox(); 
	
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UTextBlock* LoreText;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UVerticalBox* AffixBoxContainer;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UEquippableStatsBox* StatsBox;
	
};
