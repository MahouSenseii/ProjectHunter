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

	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintCallable) virtual void InitializeToolTip();
	UFUNCTION(BlueprintGetter, BlueprintCallable) UBaseItem* GetItemObject() const { return  ItemObj; }
	UFUNCTION(BlueprintCallable) static FString GetRarityText(const EItemRarity Rarity);
	UFUNCTION(BlueprintCallable) virtual void ChangeColorByRarity();
	UFUNCTION() FLinearColor GetColorBaseOnRarity() const;
	UFUNCTION(BlueprintSetter, BlueprintCallable) void SetItemObj(UBaseItem* Item) { ItemObj = Item;}


	
protected:

	UPROPERTY(EditAnywhere) TObjectPtr<UBaseItem> ItemObj;
	UPROPERTY(EditAnywhere) EItemRarity ItemGrade;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget)) UImage* ToolTipBackgroundImage;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget)) UImage* ItemImage;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget)) UTextBlock* ItemName;
	
	// Str Requirements
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget)) UHorizontalBox* StrBox;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget)) UTextBlock* StrText;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget)) UTextBlock* StrValue;
	
	// Level Requirements
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget)) UHorizontalBox* LvlBox;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget)) UTextBlock* LvlText;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget)) UTextBlock* LvlValue;
	
	// Int Requirements
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget)) UHorizontalBox* IntBox;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget)) UTextBlock*IntText;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget)) UTextBlock* IntValue;
	
	// Int Requirements
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget)) UHorizontalBox* DexBox;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget)) UTextBlock* DexText;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget)) UTextBlock* DexValue;

	// End Requirements
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget)) UHorizontalBox* EndBox;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget)) UTextBlock* EndText;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget)) UTextBlock* EndValue;
	
	// Aff Requirements
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget)) UHorizontalBox* AffBox;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget)) UTextBlock* AffText;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget)) UTextBlock* AffValue;
	
	// Luck Requirements
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget)) UHorizontalBox* LuckBox;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget)) UTextBlock* LuckText;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget)) UTextBlock* LuckValue;
	
	//  Cov Requirements
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget)) UHorizontalBox* CovBox;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget)) UTextBlock*  CovText;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget)) UTextBlock*  CovValue;

};
