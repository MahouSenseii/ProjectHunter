// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/Widgets/PHUserWidget.h"
#include "Library/PHItemEnumLibrary.h"
#include "Components/TextBlock.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "ToolTip.generated.h"

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
class ALS_PROJECTHUNTER_API UToolTip : public UPHUserWidget
{
	GENERATED_BODY()
	public:

	virtual void NativeConstruct() override;

	//Getter
	UFUNCTION(BlueprintSetter, BlueprintCallable)
	void SetItemObj(UBaseItem* Item) { ItemObj = Item;}

	UFUNCTION(BlueprintCallable)
	void InitializeToolTip();

	//Setter
	UFUNCTION(BlueprintGetter, BlueprintCallable)
	UBaseItem* GetItemObject() { return  ItemObj; }

	void CheckDamageType() const;
	void UpdateUIDamageIndicators(float ElementalMax, float ElementalMin, float PhysicalMax, float PhysicalMin) const;

	UFUNCTION()
	FLinearColor GetColorBaseOnRarity() const;

	UFUNCTION(BlueprintCallable)
	void ChangeColorByRarity();

	UFUNCTION(BlueprintSetter)
	void SetItemGrade();
	void CheckStats() const;
	static void CheckAndRemoveUIElement(const TMap<EDefenseTypes, float>& StatsMap, EDefenseTypes StatType, USizeBox* UIElement);
	void CheckAndRemoveStatUIElements() const;
	void UpdateRequirementsVisibility() const;
	void SetTextDescription() const;

	static FString GetRarityText(EItemRarity Rarity);

	void UpdateRarity() const;

	void SetDPSValue() const;

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	TObjectPtr<UBaseItem> ItemObj;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	EDamageTypes DamageStats;


	//Boxes 
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UHorizontalBox* ArmorBox;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UHorizontalBox* AttackSpeedBox;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UHorizontalBox* CritChanceBox;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UHorizontalBox* DexBox;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UHorizontalBox* EdBox;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UHorizontalBox* DPSBox;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UHorizontalBox* INTBox;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UHorizontalBox* PdBox;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UHorizontalBox* StaminaCostBox;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UHorizontalBox* StrBox;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UHorizontalBox* WeaponRangeBox;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UHorizontalBox* AffBox;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UHorizontalBox* EndBox;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UHorizontalBox* CovBox;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UHorizontalBox* LuckBox;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UHorizontalBox* CastTimeBox;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UHorizontalBox* ManaCostBox;

	//Values
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UTextBlock* ArmorValue;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UTextBlock* AttackSpeedValue;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UTextBlock* CastTimeValue;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UTextBlock* ManaCostValue;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UTextBlock* CritChanceValue;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UTextBlock* DexValue;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UTextBlock* EdValue;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UTextBlock* DPSValue;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UHorizontalBox* StaminaCostValueBox;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UTextBlock* INTValue;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UImage* Item_Image;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UTextBlock* Item_Rarity;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UTextBlock* Item_Type;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UTextBlock* WeaponRangeValue;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UTextBlock* PdValue;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UTextBlock* StaminaCostValue;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UTextBlock* StrValue;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UTextBlock* EndValue;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UTextBlock* AffValue;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UTextBlock* CovValue;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UTextBlock* LuckValue;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UTextBlock* DescriptionText;

	//SizeBox
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	USizeBox* ArmorSizeBox;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	USizeBox* StrSizeBox;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	USizeBox* CastTimeSizeBox;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	USizeBox* IntSizeBox;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	USizeBox* DexSizeBox;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	USizeBox* EndSizeBox;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	USizeBox* AffSizeBox;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	USizeBox* LuckSizeBox;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	USizeBox* CovSizeBox;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	USizeBox* CritChanceSizeBox;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	USizeBox* AttackSpeedSizeBox;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	USizeBox* StaminaCostSizeBox;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	USizeBox* WeaponRangeSizeBox;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	USizeBox* RequirementsBox;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	USizeBox* ManaCostSizeBox;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	USizeBox* DescriptionSizeBox;

	//Other
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UTextBlock* Stat1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UTextBlock* Stat2;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UTextBlock* Stat3;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UTextBlock* Stat4;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UImage* ToolTipBackgroundImage;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UImage* Spacer1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UImage* Spacer2;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UImage* Spacer3;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	EItemRarity ItemGrade;
};
