// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Item/BaseItem.h"
#include "UI/Widgets/PHUserWidget.h"
#include "PHBaseToolTip.generated.h"

enum class EItemRarity : uint8;
class UBaseItem;
/**
 * 
 */
UCLASS()
class ALS_PROJECTHUNTER_API UPHBaseToolTip : public UPHUserWidget
{
	GENERATED_BODY()

public:

	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;

	UPROPERTY()
	UItemDefinitionAsset* ItemInfo;
	
	UFUNCTION(BlueprintCallable) virtual void InitializeToolTip();
	UFUNCTION(BlueprintCallable) static FString GetRarityText(const EItemRarity Rarity);
	UFUNCTION(BlueprintCallable) virtual void ChangeColorByRarity();
	UFUNCTION() FLinearColor GetColorBaseOnRarity();
	UFUNCTION(BlueprintSetter, BlueprintCallable) virtual void SetItemInfo(UItemDefinitionAsset*& Item);


	
protected:

	UPROPERTY(EditAnywhere) EItemRarity ItemGrade;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget)) UImage* ToolTipBackgroundImage;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget)) UImage* ToolTipBackgroundImageFlicker;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget)) UImage* ItemImage;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget)) UTextBlock* ItemName;
	UPROPERTY() UMaterialInstanceDynamic* ToolTipBGMaterial;
	UPROPERTY() UMaterialInstanceDynamic* ToolTipBGMaterial2;
	
	UPROPERTY(BlueprintReadWrite,EditAnywhere, Category = "Color")
	TMap<EItemRarity, FLinearColor> RarityColors;
};
