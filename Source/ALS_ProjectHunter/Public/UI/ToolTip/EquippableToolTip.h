// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Library/PHItemEnumLibrary.h"
#include "Components/TextBlock.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
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
UCLASS()
class ALS_PROJECTHUNTER_API UEquippableToolTip : public UPHBaseToolTip
{
	GENERATED_BODY()
	public:

	virtual void NativeConstruct() override;
	virtual void InitializeToolTip() override;

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	EDamageTypes DamageStats;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UTextBlock* LoreText;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UTextBlock* PrefixStat1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UTextBlock* PrefixStat2;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UTextBlock* PrefixStat3;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UTextBlock* SuffixStat1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UTextBlock* SuffixStat2;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UTextBlock* SuffixStat3;

	

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UImage* Spacer1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UImage* Spacer2;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UImage* Spacer3;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UEquippableStatsBox* StatsBox;
	
};
