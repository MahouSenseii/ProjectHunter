// EquippableStatsBox.h

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Item/Data/UItemDefinitionAsset.h"
#include "Library/PHItemStructLibrary.h"
#include "Components/TextBlock.h"
#include "Components/HorizontalBox.h"
#include "Components/VerticalBox.h"
#include "EquippableStatsBox.generated.h"

class UMinMaxBox;
class URequirementsBox;

UCLASS()
class ALS_PROJECTHUNTER_API UEquippableStatsBox : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;

    /** Set item data for display - NEW METHOD */
    UFUNCTION(BlueprintCallable, Category = "Stats")
    void SetItemData(const UItemDefinitionAsset* Definition, const FItemInstanceData& InstanceData);
    
    /** Create damage type boxes */
    UFUNCTION(BlueprintCallable, Category = "Stats")
    void CreateMinMaxBoxByDamageTypes();
    
    /** Set other stat displays */
    UFUNCTION(BlueprintCallable, Category = "Stats")
    void SetMinMaxForOtherStats();
    
    /** Create resistance boxes */
    UFUNCTION(BlueprintCallable, Category = "Stats")
    void CreateResistanceBoxes();
    
    /** Get requirements box */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Stats")
    URequirementsBox* GetRequirementsBox() { return RequirementsBox; }

protected:
    // ✅ Separate definition and instance
    UPROPERTY(BlueprintReadOnly, Category = "Item")
    const UItemDefinitionAsset* ItemDefinition;
    
    UPROPERTY(BlueprintReadOnly, Category = "Item")
    FItemInstanceData InstanceData;

    // Widget references
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    URequirementsBox* RequirementsBox;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UHorizontalBox* PhysicalDamageBox;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* PhysicalDamageValueMin;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* PhysicalDamageValueMax;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UHorizontalBox* EDBox;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UHorizontalBox* ArmourBox;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* ArmourValue;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UHorizontalBox* PoiseBox;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* PoiseValue;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UHorizontalBox* CritChanceBox;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* CritChanceValue;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UHorizontalBox* AtkSpeedBox;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* AtkSpeedValue;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UHorizontalBox* CastTimeBox;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* CastTimeValue;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UHorizontalBox* ManaCostBox;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* ManaCostValue;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UHorizontalBox* StaminaCostBox;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* StaminaCostValue;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UHorizontalBox* WeaponRangeBox;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* WeaponRangeValue;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UHorizontalBox* ResistanceBox;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UVerticalBox* ResistanceBoxContainer;

    UPROPERTY(EditDefaultsOnly, Category = "Widgets")
    TSubclassOf<UMinMaxBox> MinMaxBoxClass;
};