// PHBaseToolTip.h

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Item/Data/UItemDefinitionAsset.h"
#include "Library/PHItemStructLibrary.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "PHBaseToolTip.generated.h"

class UBaseItem;
class APHBaseCharacter;

UCLASS()
class ALS_PROJECTHUNTER_API UPHBaseToolTip : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
    
    /** Initialize the tooltip with item data */
    virtual void InitializeToolTip();
    
    /** Set item information from a UBaseItem */
    UFUNCTION(BlueprintCallable, Category = "Tooltip")
    virtual void SetItemFromBaseItem(UBaseItem* Item);
    
    /** Set item information directly */
    UFUNCTION(BlueprintCallable, Category = "Tooltip")
    virtual void SetItemInfo( UItemDefinitionAsset* Definition,  FItemInstanceData& InstanceData);
    
    /** Position tooltip at bottom right of screen */
    UFUNCTION(BlueprintCallable, Category = "Tooltip")
    void PositionAtBottomRight(FVector2D Offset = FVector2D(-20.0f, -20.0f));
    
    /** Set whether to auto-position at bottom right */
    UFUNCTION(BlueprintCallable, Category = "Tooltip")
    void SetAutoPosition(bool bEnabled) { bAutoPositionBottomRight = bEnabled; }
    
    /** Get the item definition */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Tooltip")
    const UItemDefinitionAsset* GetItemDefinition() const { return ItemDefinition; }
    
    /** Get the instance data */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Tooltip")
    const FItemInstanceData& GetInstanceData() const { return InstanceData; }
    
    /** Set the owner character for requirement checks */
    UFUNCTION(BlueprintCallable, Category = "Tooltip")
    void SetOwnerCharacter(APHBaseCharacter* Character) { OwnerCharacter = Character; }

protected:
    /** Update tooltip colors based on item rarity */
    UFUNCTION(BlueprintCallable, Category = "Tooltip")
    void UpdateRarityColors();
    
    /** Get color for a specific rarity */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Tooltip")
    FLinearColor GetRarityColor(EItemRarity Rarity) const;

protected:
    // Immutable definition
    UPROPERTY(BlueprintReadOnly, Category = "Item")
    const UItemDefinitionAsset* ItemDefinition;
    
    // Mutable instance data
    UPROPERTY(BlueprintReadOnly, Category = "Item")
    FItemInstanceData InstanceData;
    
    UPROPERTY(BlueprintReadOnly, Category = "Owner")
    APHBaseCharacter* OwnerCharacter;
    
    // Tooltip visual elements
    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
    UImage* TooltipBackground;
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
    UTextBlock* ItemNameText;
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
    UTextBlock* ItemTypeText;
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
    UImage* ItemIcon;
    
    // Rarity color configuration
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Rarity Colors")
    TMap<EItemRarity, FLinearColor> RarityColorMap;
    
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Positioning")
    bool bAutoPositionBottomRight = true;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Positioning")
    FVector2D BottomRightOffset = FVector2D(-20.0f, -20.0f);
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Positioning")
    FVector2D TooltipSize = FVector2D(400.0f, 600.0f);
};