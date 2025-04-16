// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MinMaxBox.h"
#include "RequirementsBox.h"
#include "Components/VerticalBox.h"
#include "Library/PHItemStructLibrary.h"
#include "UI/Widgets/PHUserWidget.h"
#include "EquippableStatsBox.generated.h"

class UEquippableItem;
class UTextBlock;
class UHorizontalBox;
/**
 * @class UEquippableStatsBox
 * @brief UI widget that displays stat blocks for equippable items.
 */

UCLASS(BlueprintType)
class ALS_PROJECTHUNTER_API UEquippableStatsBox : public UPHUserWidget
{
	GENERATED_BODY()

public:

	/** Called when the widget is constructed */
	virtual void NativeConstruct() override;

	/** Sets the item this widget should display stats for */
	UFUNCTION(BlueprintCallable)
	void SetEquippableItem(const FEquippableItemData Item) { ItemData.ItemData = Item; }

	
	UFUNCTION(BlueprintCallable)
	FEquippableItemData GetEquippableItem() { return  ItemData.ItemData;}

	/** Generates elemental damage min-max boxes */
	UFUNCTION(BlueprintCallable)
	void CreateMinMaxBoxByDamageTypes();

	/** Sets values for non-elemental stats */
	UFUNCTION()
	void SetMinMaxForOtherStats() const;
	void CreateResistanceBoxes();

	URequirementsBox* GetRequirementsBox() const { return RequirementsBox;}

protected:

	/** The equippable item to display */
	UPROPERTY(EditAnywhere, Category = "Item")
	FItemInformation ItemData;

	/** Widget class used for each elemental MinMaxBox */
	UPROPERTY(EditAnywhere, Category = "Class")
	TSubclassOf<UMinMaxBox> MinMaxBoxClass;

	/** Elemental Damage Box container */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UHorizontalBox* EDBox;

	/** Physical Damage Display */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UHorizontalBox* PhysicalDamageBox;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UTextBlock* PhysicalDamageValueMin;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UTextBlock* PhysicalDamageValueMax;

	/** Armor Display */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UHorizontalBox* ArmourBox;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UTextBlock* ArmourValue;

	/** Critical Chance Display */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UHorizontalBox* CritChanceBox;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UTextBlock* CritChanceValue;

	/** Attack Speed Display */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UHorizontalBox* AtkSpeedBox;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UTextBlock* AtkSpeedValue;

	/** Cast Time Display */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UHorizontalBox* CastTimeBox;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UTextBlock* CastTimeValue;

	/** Mana Cost Display */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UHorizontalBox* ManaCostBox;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UTextBlock* ManaCostValue;

	/** Stamina Cost Display */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UHorizontalBox* StaminaCostBox;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UTextBlock* StaminaCostValue;

	/** Weapon Range Display */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UHorizontalBox* WeaponRangeBox;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UTextBlock* WeaponRangeValue;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UHorizontalBox* PoiseBox;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UTextBlock* PoiseValue;

	// Resistances
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UVerticalBox* ResistanceBoxContainer;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UHorizontalBox* ResistanceBox;

	/** Requirements Box (e.g., STR, INT, etc.) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	URequirementsBox* RequirementsBox;
};
