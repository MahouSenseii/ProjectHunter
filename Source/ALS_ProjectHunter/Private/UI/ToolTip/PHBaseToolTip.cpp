// PHBaseToolTip.cpp

#include "UI/ToolTip/PHBaseToolTip.h"
#include "Item/BaseItem.h"
#include "Character/PHBaseCharacter.h"
#include "Components/Image.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetLayoutLibrary.h"


void UPHBaseToolTip::NativeConstruct()
{
    Super::NativeConstruct();
    

}

void UPHBaseToolTip::NativePreConstruct()
{
    Super::NativePreConstruct();
    
}

void UPHBaseToolTip::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
}

void UPHBaseToolTip::PositionAtBottomRight(FVector2D Offset)
{
    // Try to get canvas slot first
    UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slot);
    
    if (CanvasSlot)
    {
        // We're in a canvas panel - use slot positioning
        CanvasSlot->SetAnchors(FAnchors(1.0f, 1.0f, 1.0f, 1.0f));
        CanvasSlot->SetAlignment(FVector2D(1.0f, 1.0f));
        CanvasSlot->SetPosition(FVector2D(-Offset.X, -Offset.Y));
        CanvasSlot->SetSize(TooltipSize);
        
        UE_LOG(LogTemp, Log, TEXT("✅ Positioned via Canvas Panel Slot"));
    }
    else
    {
        // We're added directly to viewport - use position and alignment
        FVector2D ViewportSize = UWidgetLayoutLibrary::GetViewportSize(this);
        
        // Calculate position from bottom-right
        FVector2D DesiredPosition;
        DesiredPosition.X = ViewportSize.X - Offset.X - TooltipSize.X;
        DesiredPosition.Y = ViewportSize.Y - Offset.Y - TooltipSize.Y;
        
        // Set position in viewport space
        SetPositionInViewport(DesiredPosition, false);
        
        // Set desired size
        SetDesiredSizeInViewport(TooltipSize);
        
        UE_LOG(LogTemp, Log, TEXT("✅ Positioned via SetPositionInViewport at %s"), 
            *DesiredPosition.ToString());
    }
}

void UPHBaseToolTip::InitializeToolTip()
{
    if (!ItemDefinition)
    {
        UE_LOG(LogTemp, Warning, TEXT("InitializeToolTip: No item definition set"));
        return;
    }

    // Set basic item info
    if (ItemNameText)
    {
        FText DisplayName = InstanceData.bHasNameBeenGenerated ? 
            InstanceData.DisplayName : 
            ItemDefinition->Base.ItemName;
            
        ItemNameText->SetText(DisplayName);
        const FSlateFontInfo FontInfo = FCoreStyle::GetDefaultFontStyle("Bold", 24);
        ItemNameText->SetFont(FontInfo);
    }

    if (ItemTypeText)
    {
         ItemTypeText->SetText(UEnum::GetDisplayValueAsText(ItemDefinition->Base.ItemType));
    }
    
    
    SetColorMap();
    UpdateRarityColors();
}

void UPHBaseToolTip::SetItemFromBaseItem(UBaseItem* Item)
{
    if (!Item || !Item->ItemDefinition)
    {
        UE_LOG(LogTemp, Warning, TEXT("SetItemFromBaseItem: Invalid item"));
        return;
    }

    SetItemInfo(Item->ItemDefinition, Item->RuntimeData);
}

void UPHBaseToolTip::SetItemInfo( UItemDefinitionAsset* Definition,  FItemInstanceData& InInstanceData)
{
    if (!Definition)
    {
        UE_LOG(LogTemp, Warning, TEXT("SetItemInfo: Null definition"));
        return;
    }

    ItemDefinition = Definition;
    InstanceData = InInstanceData;

    InitializeToolTip();
}

void UPHBaseToolTip::SetColorMap()
{
    // Initialize rarity colors
    if (RarityColorMap.Num() == 0)
    {
        RarityColorMap.Add(EItemRarity::IR_None,      FLinearColor(0.5f, 0.5f, 0.5f));
        RarityColorMap.Add(EItemRarity::IR_GradeF,    FLinearColor(0.4f, 0.3f, 0.2f));
        RarityColorMap.Add(EItemRarity::IR_GradeD,    FLinearColor::White);
        RarityColorMap.Add(EItemRarity::IR_GradeC,    FLinearColor(0.4f, 1.0f, 0.3f));
        RarityColorMap.Add(EItemRarity::IR_GradeB,    FLinearColor(0.2f, 0.6f, 1.0f));
        RarityColorMap.Add(EItemRarity::IR_GradeA,    FLinearColor(0.7f, 0.2f, 1.0f));
        RarityColorMap.Add(EItemRarity::IR_GradeS,    FLinearColor(1.0f, 0.65f, 0.0f));
        RarityColorMap.Add(EItemRarity::IR_Unknown,   FLinearColor(1.0f, 0.0f, 1.0f));
        RarityColorMap.Add(EItemRarity::IR_Corrupted, FLinearColor(0.5f, 0.0f, 0.15f));
    }
}

void UPHBaseToolTip::UpdateRarityColors()
{
    // Get rarity
    EItemRarity Rarity = InstanceData.Rarity;
    if (Rarity == EItemRarity::IR_None && ItemDefinition)
    {
        Rarity = ItemDefinition->Base.ItemRarity;
    }
    
    const FLinearColor RarityColor = GetRarityColor(Rarity);
    
    // Apply subtle tint to background
    if (TooltipBackground)
    {
        FLinearColor BackgroundTint = RarityColor * 0.12f ;
        BackgroundTint.A = 0.95f;
        
        
        if (Rarity == EItemRarity::IR_Corrupted)
        {
            BackgroundTint = RarityColor * 0.25f;
            BackgroundTint.A = 0.98f;
        }
        else if (Rarity == EItemRarity::IR_Unknown)
        {
            BackgroundTint = RarityColor * 0.2f;
            BackgroundTint.A = 0.9f;
        }
        
        TooltipBackground->SetColorAndOpacity(BackgroundTint);
        if (TooltipBackgroundFlicker)
        {
            // Get the base material (M_SquarFlicker_Inst)
            UMaterialInterface* BaseMat = TooltipBackgroundFlicker->GetBrush().GetResourceObject()
                ? Cast<UMaterialInterface>(TooltipBackgroundFlicker->GetBrush().GetResourceObject())
                : nullptr;

            if (BaseMat)
            {
                // Create a dynamic instance from it
                UMaterialInstanceDynamic* MatInst = UMaterialInstanceDynamic::Create(BaseMat, this);

                // Assign it back to the image brush
                TooltipBackgroundFlicker->SetBrushFromMaterial(MatInst);
                float Intensity = 1.0f;
                FLinearColor StrongColor = RarityColor * Intensity;
                // Now you can set the color parameter
                MatInst->SetVectorParameterValue(FName("ImageTint"), StrongColor);
            }
        }
    }

}

FLinearColor UPHBaseToolTip::GetRarityColor(EItemRarity Rarity) const
{
    if (const FLinearColor* Color = RarityColorMap.Find(Rarity))
    {
        return *Color;
    }
    
    return FLinearColor::White;
}