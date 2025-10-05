// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Item/ItemWidget.h"
#include "Item/BaseItem.h"
#include "UI/DragDrop/DragWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanelSlot.h"
#include "Interactables/Pickups/ConsumablePickup.h"
#include "Materials/MaterialInstance.h"
#include "UI/ToolTip/ConsumableToolTip.h"


class UConsumableToolTip;
class UEquippableToolTip;

void UItemWidget::NativeConstruct()
{
    Super::NativeConstruct();
    this->SetIsFocusable(true);
    Refresh();

    InitializeComponents(); // Initialize button events,
    
}

void UItemWidget::NativeDestruct()
{
	Super::NativeDestruct();
}

void UItemWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
    Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);

    // 1. Validate item data
    if (!IsValid(ItemObject))
    {
        UE_LOG(LogTemp, Warning, TEXT("NativeOnDragDetected: ItemObject is null."));
        return;
    }

    // 2. Create drag operation
    if (UDragWidget* DragOp = NewObject<UDragWidget>(this))
    {
        DragOp->Payload = ItemObject;
        DragOp->DefaultDragVisual = this;
        DragOp->Pivot = EDragPivot::CenterCenter;
        check(DragOp->DefaultDragVisual != nullptr);

        OutOperation = DragOp;

        // 3. Notify grid and detach visual
        OnRemoved.Broadcast(ItemObject);     // Let grid know it's being moved
        RemoveFromParent();                  // Remove from canvas

        UE_LOG(LogTemp, Log, TEXT("NativeOnDragDetected: Drag operation started for %s"), *ItemObject->GetItemInfo()->Base.ItemName.ToString());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("NativeOnDragDetected: Failed to create drag operation."));
    }
}




FReply UItemWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
    {
        UE_LOG(LogTemp, Warning, TEXT("Non-left mouse button pressed."));
        return FReply::Unhandled();
    }

    FEventReply DragDetect = UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent, this, EKeys::LeftMouseButton);
    UE_LOG(LogTemp, Log, TEXT("Left mouse button pressed and handled."));

    return DragDetect.NativeReply;
}


void UItemWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	constexpr FLinearColor BrushColor(1.0, 1.0, 1.0, 0.0);
    EventHovered();
    BackgroundBorder->SetBrushColor(BrushColor);
    
}

void UItemWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
	constexpr FLinearColor BrushColor(0.0, 0.0, 0.0, 0.0);
    EventMouseUnHovered();
    BackgroundBorder->SetBrushColor(BrushColor);
}

void UItemWidget::InitializeComponents()
{
    if (!BtnItem->OnHovered.IsAlreadyBound(this, &UItemWidget::EventHovered))
    {
        BtnItem->OnHovered.AddDynamic(this, &UItemWidget::EventHovered);
    }
    
    if (!BtnItem->OnUnhovered.IsAlreadyBound(this, &UItemWidget::EventMouseUnHovered))
    {
        BtnItem->OnUnhovered.AddDynamic(this, &UItemWidget::EventMouseUnHovered);
    }
}
FSlateBrush UItemWidget::GetIcon() const
{
    // Initialize empty brush
    FSlateBrush Brush;

    if (ItemObject)
    {
        // Fetch the material interface for the icon

        // Check if the Material is valid
        if (UMaterialInterface* Material = ItemObject->GetIcon())
        {
            // Create a new FSlateBrush with the given material and size
            Brush = UWidgetBlueprintLibrary::MakeBrushFromMaterial(Material, Size.X, Size.Y);
        }
        else
        {
            // Log a warning
            UE_LOG(LogTemp, Warning, TEXT("Material for icon is not available."));
        }
    }
    else
    {
        // Log a warning
        UE_LOG(LogTemp, Warning, TEXT("ItemObject is null."));
    }
    
    return Brush;
}



void UItemWidget::Refresh()
{
    // Check for null pointers before dereferencing
    if (!ItemObject)
    {
        UE_LOG(LogTemp, Warning, TEXT("ItemObject is null in UItemWidget::Refresh"));
        return;
    }
    if (!BackgroundSizeBox)
    {
        UE_LOG(LogTemp, Warning, TEXT("BackgroundSizeBox is null in UItemWidget::Refresh"));
        return;
    }
    if (!ItemImage)
    {
        UE_LOG(LogTemp, Warning, TEXT("ItemImage is null in UItemWidget::Refresh"));
        return;
    }

    // Get item dimensions and calculate size
    const FIntPoint ItemDimensions = ItemObject->GetDimensions();
    Size.X = ItemDimensions.X * TileSize;
    Size.Y = ItemDimensions.Y * TileSize;

    // Determine the appropriate material instance (rotated or default)
    UMaterialInstance* MaterialInstance;
    if (ItemObject->IsRotated())
    {
        MaterialInstance = ItemObject->GetItemInfo()->Base.ItemImageRotated;
    }
    else
    {
        MaterialInstance = ItemObject->GetItemInfo()->Base.ItemImage;
    }

    if (MaterialInstance)
    {
        // Create a dynamic material instance if necessary
        if (!CachedDynamicMaterial || CachedDynamicMaterial->Parent != MaterialInstance)
        {
            CachedDynamicMaterial = UMaterialInstanceDynamic::Create(MaterialInstance, this);
        }

        // Apply the dynamic material to the UImage widget
        if (CachedDynamicMaterial)
        {
            FSlateBrush Brush;
            Brush.SetResourceObject(CachedDynamicMaterial);
            ItemImage->SetBrush(Brush);
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("MaterialInstance is null in UItemWidget::Refresh"));
    }

    // Update the SizeBox dimensions
    BackgroundSizeBox->SetWidthOverride(Size.X);
    BackgroundSizeBox->SetHeightOverride(Size.Y);

    // Update the Canvas Panel Slot dimensions for the ItemImage
    if (UCanvasPanelSlot* CanvasSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(ItemImage))
    {
        CanvasSlot->SetSize(Size);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("ItemImage is not in a CanvasPanelSlot in UItemWidget::Refresh"));
    }
}


void UItemWidget::EventHovered()
{
    // Check if the tooltip is not already created to prevent memory leaks and redundant creation.
    if (!CurrentToolTip && EquippableToolTipClass)
    {
        CreateToolTip();
    }
}


void UItemWidget::CreateToolTip()
{
    check(ItemObject); 
    check(EquippableToolTipClass);


    TSubclassOf<UUserWidget> TooltipClassToUse = nullptr;

    UItemDefinitionAsset* ItemInfo = ItemObject->GetItemInfo();
    switch (ItemInfo->Base.ItemType)
    {
        case EItemType::IT_Armor:
        case EItemType::IT_Weapon:
        case EItemType::IT_Shield:
            TooltipClassToUse = EquippableToolTipClass;
            break;

        case EItemType::IT_Consumable:
            TooltipClassToUse = ConsumableToolTipClass;
            break;

        default:
            break;
    }

    if (TooltipClassToUse)
    {
        UUserWidget* NewToolTip = CreateWidget<UUserWidget>(GetWorld(), TooltipClassToUse);
        if (!NewToolTip) return;


        if (UEquippableToolTip* AsEquippable = Cast<UEquippableToolTip>(NewToolTip))
        {
            AsEquippable->SetItemInfo(ItemObject->GetItemInfo());
            CurrentToolTip = AsEquippable;
        }
        else if (UConsumableToolTip* AsConsumable = Cast<UConsumableToolTip>(NewToolTip))
        {
            AsConsumable->SetItemInfo(ItemObject->GetItemInfo());
        }

        // Positioning
        const FVector2D DesiredPosition = CalculateTooltipPosition();
        const float Width = GetTooltipWidth();
        const float Height = GetTooltipHeight();
        Cast<UEquippableToolTip>(CurrentToolTip)->StatsBox->GetRequirementsBox()->SetItemRequirements(
            ItemObject->GetItemInfo()->Equip,OwnerCharacter);
        SetToolTip(NewToolTip);
        NewToolTip->SetDesiredSizeInViewport(FVector2D(Width, Height));
        NewToolTip->SetPositionInViewport(DesiredPosition);
        NewToolTip->AddToViewport(100);
       
    }
}

void UItemWidget::EventMouseUnHovered()
{
    RemoveToolTip();
}

void UItemWidget::RemoveToolTip()
{
   if (IsValid(CurrentToolTip))
    {
        CurrentToolTip->RemoveFromParent();
        CurrentToolTip = nullptr;
    }
}

FVector2D UItemWidget::CalculateTooltipPosition() const
{
    FVector2D ScreenSize;
    GEngine->GameViewport->GetViewportSize(ScreenSize);

    FVector2D MousePosition;
    GetOwningPlayer()->GetMousePosition(MousePosition.X, MousePosition.Y);

    constexpr float OffsetX = 50.0f;
    constexpr float OffsetY = 95.0f;
    FVector2D DesiredPosition = MousePosition + FVector2D(OffsetX, OffsetY);

    // Ensure tooltip does not exceed screen boundaries
    DesiredPosition.X = FMath::Min(DesiredPosition.X, ScreenSize.X - GetTooltipWidth());
    DesiredPosition.Y = FMath::Min(DesiredPosition.Y, ScreenSize.Y - GetTooltipHeight());

    return DesiredPosition;
}

float UItemWidget::GetTooltipWidth() const
{
    FVector2d ViewportSize;
    GEngine->GameViewport->GetViewportSize(ViewportSize);
    return  ViewportSize.X * 0.8f;  // Adjust width based on requirement
}

float UItemWidget::GetTooltipHeight() const
{
    FVector2d ViewportSize;
    GEngine->GameViewport->GetViewportSize(ViewportSize);
    return  ViewportSize.Y * 0.8f;  // Adjust width based on requirement // Adjust height based on requirement
}