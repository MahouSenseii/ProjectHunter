// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Item/ItemInstance.h"
#include "Item/Library/ItemEnums.h"
#include "ItemTooltipWidget.generated.h"

UCLASS()
class UItemTooltipWidget : public UUserWidget
{
	GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable)
    void UpdateTooltip(UItemInstance* Item);

protected:
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
    FLinearColor Color_GradeF;
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GradeColors")
    FLinearColor Color_GradeD;
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GradeColors")
    FLinearColor Color_GradeC;
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GradeColors")
    FLinearColor Color_GradeB;
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GradeColors")
    FLinearColor Color_GradeA;
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GradeColors")
    FLinearColor Color_GradeS;
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GradeColors")
    FLinearColor Color_GradeUnkown;
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GradeColors")
    FLinearColor Color_GradeCorrupted;
    
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
