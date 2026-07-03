// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Item/ItemInstance.h"
#include "Item/Library/ItemEnums.h"
#include "Item/Library/ItemTooltipStructs.h"
#include "ItemTooltipWidget.generated.h"

UCLASS()
class UItemTooltipWidget : public UUserWidget
{
	GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable)
    void UpdateTooltip(UItemInstance* Item);

    UFUNCTION(BlueprintCallable, Category = "Tooltip")
    void ClearTooltip();

    UFUNCTION(BlueprintPure, Category = "Tooltip")
    FItemTooltipData GetTooltipData() const { return TooltipData; }

protected:
    /**
     * Fired after the C++ population pass so a Blueprint child can extend the
     * tooltip (extra sections, animations) without replacing the base logic.
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "Tooltip")
    void OnTooltipUpdated(UItemInstance* Item);

    /**
     * New modular data event. Blueprint children can loop Sections/Lines and
     * create only the rows that exist instead of keeping blank stat widgets.
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "Tooltip")
    void OnTooltipDataUpdated(const FItemTooltipData& InTooltipData);

    UFUNCTION(BlueprintImplementableEvent, Category = "Tooltip")
    void OnTooltipCleared();

    UPROPERTY(BlueprintReadOnly, Category = "Tooltip")
    FItemTooltipData TooltipData;

    // ═══════════════════════════════════════════════
    // HEADER SECTION (Name + Icon)
    // ═══════════════════════════════════════════════
    
    UPROPERTY(meta = (BindWidget))
    class UBorder* HeaderBorder;
    
    UPROPERTY(meta = (BindWidget))
    class UImage* ItemIconImage;
    
    UPROPERTY(meta = (BindWidget))
    class UTextBlock* ItemNameText;
    
    UPROPERTY(meta = (BindWidget))
    class UTextBlock* ItemTypeText;
    
    // ═══════════════════════════════════════════════
    // BASE STATS BOX
    // ═══════════════════════════════════════════════
    
    UPROPERTY(meta = (BindWidget))
    class UBorder* BaseStatsBox;
    
    UPROPERTY(meta = (BindWidget))
    class UVerticalBox* BaseStatsContainer;
    
    // ═══════════════════════════════════════════════
    // AFFIXES BOX (Max 6 affixes)
    // ═══════════════════════════════════════════════
    
    UPROPERTY(meta = (BindWidget))
    class UBorder* AffixesBox;
    
    UPROPERTY(meta = (BindWidget))
    class UVerticalBox* AffixesContainer;
    
    // ═══════════════════════════════════════════════
    // LORE TEXT (Optional)
    // ═══════════════════════════════════════════════
    
    UPROPERTY(meta = (BindWidget))
    class UBorder* LoreBox;
    
    UPROPERTY(meta = (BindWidget))
    class UTextBlock* LoreText;
    
    // ═══════════════════════════════════════════════
    // YOUR GRADE COLORS (From InteractableWidget)
    // ═══════════════════════════════════════════════
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GradeColors")
    FLinearColor Color_GradeF = FLinearColor(0.5f, 0.5f, 0.5f, 1.0f);

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GradeColors")
    FLinearColor Color_GradeE = FLinearColor::White;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GradeColors")
    FLinearColor Color_GradeD = FLinearColor(0.3f, 0.9f, 0.3f, 1.0f);
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GradeColors")
    FLinearColor Color_GradeC = FLinearColor(0.4f, 0.6f, 1.0f, 1.0f);
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GradeColors")
    FLinearColor Color_GradeB = FLinearColor(0.7f, 0.3f, 0.9f, 1.0f);
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GradeColors")
    FLinearColor Color_GradeA = FLinearColor(1.0f, 0.7f, 0.0f, 1.0f);
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GradeColors")
    FLinearColor Color_GradeS = FLinearColor(1.0f, 0.3f, 0.0f, 1.0f);

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GradeColors")
    FLinearColor Color_GradeSS = FLinearColor(1.0f, 0.0f, 0.0f, 1.0f);
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GradeColors")
    FLinearColor Color_GradeUnkown = FLinearColor(0.2f, 0.2f, 0.2f, 1.0f);
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GradeColors")
    FLinearColor Color_GradeCorrupted = FLinearColor(0.5f, 0.0f, 0.3f, 1.0f);
    
    // ═══════════════════════════════════════════════
    // OTHER COLORS
    // ═══════════════════════════════════════════════
    
    UPROPERTY(EditDefaultsOnly, Category = "Colors")
    FLinearColor AffixColor = FLinearColor(0.5f, 0.8f, 1.0f);  // Light blue
    
    UPROPERTY(EditDefaultsOnly, Category = "Colors")
    FLinearColor BaseStatColor = FLinearColor(0.9f, 0.9f, 0.9f);  // White
    
    UPROPERTY(EditDefaultsOnly, Category = "Colors")
    FLinearColor LoreColor = FLinearColor(0.9f, 0.6f, 0.3f);  // Orange/Gold

private:
    void SetGradeVisuals(EItemRarity Grade);
    FLinearColor GetGradeColor(EItemRarity Grade) const;
    void PopulateBaseStats(UItemInstance* Item);
    void PopulateAffixes(UItemInstance* Item);
    void PopulateLore(UItemInstance* Item);
    UTextBlock* CreateStatTextBlock(const FString& Text, FLinearColor Color);
};
