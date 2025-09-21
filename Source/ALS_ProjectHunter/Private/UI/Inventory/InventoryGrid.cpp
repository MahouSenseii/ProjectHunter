// InventoryGrid.cpp
#include "UI/Inventory/InventoryGrid.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/DragDropOperation.h"
#include "Layout/Geometry.h"
#include "Layout/PaintGeometry.h"
#include "Layout/SlateRect.h"
#include "Rendering/DrawElements.h"
#include "Engine/Engine.h"

DEFINE_LOG_CATEGORY_STATIC(LogInventoryGrid, Log, All);

UInventoryGrid::UInventoryGrid(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer), GridCanvas(nullptr), GridBorder(nullptr)
{
    TileSize = 64.0f;
    DrawDropLocation = false;
    DraggedItemTopLeft = FIntPoint(0, 0);
    CurrentPreviewColor = FLinearColor::White;
    GridLineColor = FLinearColor(0.1f, 0.1f, 0.1f, 0.8f);
    GridLineThickness = 1.0f;
    ValidDropColor = FLinearColor::Green;
    InvalidDropColor = FLinearColor::Red;
    bShowRarityColors = true;
    bShowDragTooltips = true;
    MinLevelForTooltips = 5;
}

void UInventoryGrid::NativeConstruct()
{
    Super::NativeConstruct();
    
    // Initialize any additional setup here
    if (GridCanvas)
    {
        UE_LOG(LogInventoryGrid, Log, TEXT("InventoryGrid initialized successfully"));
    }
    else
    {
        UE_LOG(LogInventoryGrid, Warning, TEXT("GridCanvas not found - check widget bindings"));
    }
}

void UInventoryGrid::NativeDestruct()
{
    // Clean up any bound events or references
    if (DragTooltipWidget)
    {
        DragTooltipWidget->RemoveFromParent();
        DragTooltipWidget = nullptr;
    }
    
    CurrentDraggedItem = nullptr;
    CurrentDraggedInstance = nullptr;
    
    Super::NativeDestruct();
}

/* ============================= */
/* === INITIALIZATION === */
/* ============================= */

void UInventoryGrid::InitializeGrid(UInventoryManager* Owner, UInventoryManager* Other)
{
    OwnerInventory = Owner;
    OtherInventory = Other;
    
    if (OwnerInventory)
    {
        UE_LOG(LogInventoryGrid, Log, TEXT("Grid initialized with owner inventory"));
    }
    else
    {
        UE_LOG(LogInventoryGrid, Warning, TEXT("Grid initialized without owner inventory"));
    }
}



void UInventoryGrid::SetTileSize(float NewTileSize)
{
    if (NewTileSize > 0.0f)
    {
        TileSize = NewTileSize;
        // Force a redraw
        Invalidate(EInvalidateWidgetReason::Paint);
    }
}

/* ============================= */
/* === DRAG AND DROP EVENTS === */
/* ============================= */

bool UInventoryGrid::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
    DrawDropLocation = false;
    CurrentDraggedItem = nullptr;
    CurrentDraggedInstance = nullptr;
    
    if (!InOperation || !OwnerInventory)
    {
        return false;
    }

    // Try to get ItemInstance first, then fall back to BaseItem
    UItemInstanceObject* PayloadInstance = Cast<UItemInstanceObject>(InOperation->Payload);
    UBaseItem* PayloadItem = PayloadInstance ? PayloadInstance : Cast<UBaseItem>(InOperation->Payload);
    
    if (!PayloadItem)
    {
        UE_LOG(LogInventoryGrid, Warning, TEXT("Drop operation payload is not a valid item"));
        return false;
    }

    // Calculate drop position
    const FVector2D LocalPosition = InGeometry.AbsoluteToLocal(InDragDropEvent.GetScreenSpacePosition());
    const FTile DropTile = ScreenPositionToTile(LocalPosition);
    const int32 Index = OwnerInventory->TileToIndex(DropTile);

    bool bDropSuccess;
    
    // Handle ItemInstance drops
    if (PayloadInstance)
    {
        bDropSuccess = HandleItemInstanceDrop(PayloadInstance, Index);
    }
    // Handle legacy BaseItem drops
    else
    {
        bDropSuccess = HandleItemDrop(PayloadItem, Index);
    }

    if (bDropSuccess)
    {
        OnItemDropped.Broadcast();
        UE_LOG(LogInventoryGrid, Log, TEXT("Item successfully dropped at index %d"), Index);
    }
    else
    {
        UE_LOG(LogInventoryGrid, Log, TEXT("Failed to drop item at index %d"), Index);
    }

    OnDragEnded.Broadcast();
    return bDropSuccess;
}

void UInventoryGrid::NativeOnDragEnter(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
    DrawDropLocation = true;
    OnDragStarted.Broadcast();
    
    if (InOperation)
    {
        // Set the current dragged item for visual feedback
        CurrentDraggedInstance = Cast<UItemInstanceObject>(InOperation->Payload);
        CurrentDraggedItem = CurrentDraggedInstance ? CurrentDraggedInstance : Cast<UBaseItem>(InOperation->Payload);
        
        if (CurrentDraggedItem)
        {
            const FVector2D LocalPosition = InGeometry.AbsoluteToLocal(InDragDropEvent.GetScreenSpacePosition());
            
            if (CurrentDraggedInstance)
            {
                UpdateDragPreviewForInstance(CurrentDraggedInstance, LocalPosition);
            }
            else
            {
                UpdateDragPreview(CurrentDraggedItem, LocalPosition);
            }
        }
    }
}

void UInventoryGrid::NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
    DrawDropLocation = false;
    CurrentDraggedItem = nullptr;
    CurrentDraggedInstance = nullptr;
    HideDragTooltip();
}

bool UInventoryGrid::NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
    if (!InOperation || !OwnerInventory)
    {
        return false;
    }

    const FVector2D LocalPosition = InGeometry.AbsoluteToLocal(InDragDropEvent.GetScreenSpacePosition());
    
    // Update visual feedback based on item type
    if (CurrentDraggedInstance)
    {
        UpdateDragPreviewForInstance(CurrentDraggedInstance, LocalPosition);
    }
    else if (CurrentDraggedItem)
    {
        UpdateDragPreview(CurrentDraggedItem, LocalPosition);
    }

    return true;
}

/* ============================= */
/* === ITEM DROP HANDLING === */
/* ============================= */

bool UInventoryGrid::HandleItemDrop(UBaseItem* Payload, const int32 Index)
{
    if (!Payload || !OwnerInventory)
    {
        return false;
    }

    // Check if this item belongs to our inventory or another
    const bool bIsOwnedItem = OwnerInventory->ContainsItem(Payload);
    
    return bIsOwnedItem 
        ? HandleOwnedItemDrop(Payload, Index)
        : HandleUnownedItemDrop(Payload, Index);
}

bool UInventoryGrid::HandleOwnedItemDrop(UBaseItem* Payload, const int32 Index) const
{
    if (!Payload || !OwnerInventory) 
        return false;

    UBaseItem* ExistingItem = OwnerInventory->GetItemAt(Index);

    // Step 1: Handle stacking with safer memory management
    if (ExistingItem && Payload->IsStackable() && OwnerInventory->AreItemsStackable(ExistingItem, Payload))
    {
        const int32 MaxStack = ExistingItem->GetItemInfo().ItemInfo.MaxStackSize;
        const int32 CurrentQty = ExistingItem->GetItemInfo().ItemInfo.Quantity;
        const int32 NewQty = Payload->GetItemInfo().ItemInfo.Quantity;

        if (MaxStack == 0 || (CurrentQty + NewQty) <= MaxStack)
        {
            // Full stack - item is completely consumed
            ExistingItem->AddQuantity(NewQty);
            OwnerInventory->RemoveItemFromInventory(Payload);
            OwnerInventory->OnInventoryChanged.Broadcast();
            return true; // ✅ SUCCESS - item fully stacked
        }
        else if (CurrentQty < MaxStack)
        {
            // Partial stack - item has remaining quantity
            const int32 CanAdd = MaxStack - CurrentQty;
            ExistingItem->AddQuantity(CanAdd);
            Payload->SetQuantity(NewQty - CanAdd);
            OwnerInventory->OnInventoryChanged.Broadcast();
            
            // Continue with remaining quantity - don't return here
        }
    }

    // Step 2: Try to place item directly at specified location
    if (OwnerInventory->IsRoomAvailable(Payload, Index))
    {
        return OwnerInventory->TryToAddItemAt(Payload, Index);
    }

    // Step 3: Try to auto-place somewhere else in inventory
    if (OwnerInventory->TryToAddItemToInventory(Payload, true))
    {
        return true;
    }

    // Step 4: No valid space - drop to world
    return OwnerInventory->DropItemFromInventory(Payload);
}

bool UInventoryGrid::HandleUnownedItemDrop(UBaseItem* Payload, const int32 Index) const
{
    if (!Payload || !OwnerInventory)
    {
        return false;
    }

    // Try to add the item to our inventory
    if (OwnerInventory->IsRoomAvailable(Payload, Index))
    {
        return OwnerInventory->TryToAddItemAt(Payload, Index);
    }

    // Try to auto-place if direct placement failed
    return OwnerInventory->TryToAddItemToInventory(Payload, true);
}

/* ============================= */
/* === ITEMINSTANCE DROP HANDLING === */
/* ============================= */

bool UInventoryGrid::HandleItemInstanceDrop(UItemInstanceObject* Payload, const int32 Index)
{
    if (!Payload || !OwnerInventory)
    {
        return false;
    }

    // Check if this item belongs to our inventory or another
    const bool bIsOwnedItem = OwnerInventory->ContainsItem(Payload);
    
    return bIsOwnedItem 
        ? HandleOwnedItemInstanceDrop(Payload, Index)
        : HandleUnownedItemInstanceDrop(Payload, Index);
}

bool UInventoryGrid::HandleOwnedItemInstanceDrop(UItemInstanceObject* Payload, const int32 Index) const
{
    if (!Payload || !OwnerInventory) 
        return false;

    UBaseItem* ExistingItem = OwnerInventory->GetItemAt(Index);
    UItemInstanceObject* ExistingInstance = Cast<UItemInstanceObject>(ExistingItem);

    // Step 1: Handle ItemInstance stacking
    if (ExistingInstance && 
        Payload->GetItemInfo().ItemInfo.Stackable &&
        TryStackItemInstances(ExistingInstance, Payload))
    {
        return true;
    }

    // Step 2: Try to place item directly at specified location
    // Cast UItemInstanceObject to UBaseItem for compatibility
    UBaseItem* PayloadAsBaseItem = Cast<UBaseItem>(Payload);
    if (PayloadAsBaseItem && OwnerInventory->IsRoomAvailable(PayloadAsBaseItem, Index))
    {
        return OwnerInventory->TryToAddItemAt(PayloadAsBaseItem, Index);
    }

    // Step 3: Try to auto-place somewhere else in inventory
    if (PayloadAsBaseItem && OwnerInventory->TryToAddItemToInventory(PayloadAsBaseItem, true))
    {
        return true;
    }

    // Step 4: No valid space - drop to world
    if (PayloadAsBaseItem)
    {
        return OwnerInventory->DropItemFromInventory(PayloadAsBaseItem);
    }

    return false;
}

bool UInventoryGrid::HandleUnownedItemInstanceDrop(UItemInstanceObject* Payload, const int32 Index) const
{
    if (!Payload || !OwnerInventory)
    {
        return false;
    }

    // Cast to UBaseItem for compatibility with InventoryManager methods
    UBaseItem* PayloadAsBaseItem = Cast<UBaseItem>(Payload);
    if (!PayloadAsBaseItem)
    {
        return false;
    }

    // Try to add the item instance to our inventory
    if (OwnerInventory->IsRoomAvailable(PayloadAsBaseItem, Index))
    {
        return OwnerInventory->TryToAddItemAt(PayloadAsBaseItem, Index);
    }

    // Try to auto-place if direct placement failed
    return OwnerInventory->TryToAddItemToInventory(PayloadAsBaseItem, true);
}

/* ============================= */
/* === STACKING HELPERS === */
/* ============================= */

bool UInventoryGrid::TryStackBaseItems(UBaseItem* ExistingItem, UBaseItem* NewItem) const
{
    if (!ExistingItem || !NewItem || !OwnerInventory)
    {
        return false;
    }

    if (!OwnerInventory->AreItemsStackable(ExistingItem, NewItem))
    {
        return false;
    }

    const int32 MaxStack = ExistingItem->GetItemInfo().ItemInfo.MaxStackSize;
    const int32 CurrentQty = ExistingItem->GetItemInfo().ItemInfo.Quantity;
    const int32 NewQty = NewItem->GetItemInfo().ItemInfo.Quantity;

    if (MaxStack == 0 || (CurrentQty + NewQty) <= MaxStack)
    {
        // Full stack
        ExistingItem->AddQuantity(NewQty);
        OwnerInventory->RemoveItemFromInventory(NewItem);
        OwnerInventory->OnInventoryChanged.Broadcast();
        return true;
    }
    else if (CurrentQty < MaxStack)
    {
        // Partial stack
        const int32 CanAdd = MaxStack - CurrentQty;
        ExistingItem->AddQuantity(CanAdd);
        NewItem->SetQuantity(NewQty - CanAdd);
        OwnerInventory->OnInventoryChanged.Broadcast();
        return false; // Item still has quantity left
    }

    return false;
}

bool UInventoryGrid::TryStackItemInstances(UItemInstanceObject* ExistingInstance, UItemInstanceObject* NewInstance) const
{
    if (!ExistingInstance || !NewInstance || !OwnerInventory)
    {
        return false;
    }

    // Check if instances can be stacked (now using the actual method)
    if (!OwnerInventory->CanStackItemInstances(ExistingInstance, NewInstance))
    {
        return false;
    }

    const int32 MaxStack = ExistingInstance->GetItemInfo().ItemInfo.MaxStackSize;
    const int32 CurrentQty = ExistingInstance->GetItemInfo().ItemInfo.Quantity;
    const int32 NewQty = NewInstance->GetItemInfo().ItemInfo.Quantity;

    if (MaxStack == 0 || (CurrentQty + NewQty) <= MaxStack)
    {
        // Full stack - this is tricky because we can't access ItemInfoView directly
        // We need to add proper quantity management methods to ItemInstanceObject
        // For now, let the InventoryManager handle the stacking
        return OwnerInventory->TryStackItemInstance(NewInstance);
    }
    else if (CurrentQty < MaxStack)
    {
        // Partial stack - similarly needs proper ItemInstance quantity management
        UE_LOG(LogInventoryGrid, Warning, TEXT("Partial stacking of ItemInstances not fully implemented"));
        return false;
    }

    return false;
}

bool UInventoryGrid::CanStackMixedItems(UBaseItem* ExistingItem, UBaseItem* NewItem) const
{
    if (!ExistingItem || !NewItem)
    {
        return false;
    }

    // Try ItemInstance stacking first
    UItemInstanceObject* ExistingInstance = Cast<UItemInstanceObject>(ExistingItem);
    UItemInstanceObject* NewInstance = Cast<UItemInstanceObject>(NewItem);
    
    if (ExistingInstance && NewInstance)
    {
        return OwnerInventory ? OwnerInventory->CanStackItemInstances(ExistingInstance, NewInstance) : false;
    }
    
    // Fall back to BaseItem stacking
    return OwnerInventory ? OwnerInventory->AreItemsStackable(ExistingItem, NewItem) : false;
}

/* ============================= */
/* === VALIDATION METHODS === */
/* ============================= */

bool UInventoryGrid::CanDropItemAt(UBaseItem* Item, const FVector2D& Position) const
{
    if (!Item || !OwnerInventory)
    {
        return false;
    }

    const FTile DropTile = ScreenPositionToTile(Position);
    const int32 Index = OwnerInventory->TileToIndex(DropTile);

    // Check if we can stack with existing item at this position
    if (UBaseItem* ExistingItem = OwnerInventory->GetItemAt(Index))
    {
        return CanStackMixedItems(ExistingItem, Item);
    }

    // Check if there's room for the item
    return OwnerInventory->IsRoomAvailable(Item, Index);
}

bool UInventoryGrid::CanDropItemInstanceAt(UItemInstanceObject* Item, const FVector2D& Position) const
{
    if (!Item || !OwnerInventory)
    {
        return false;
    }

    const FTile DropTile = ScreenPositionToTile(Position);
    const int32 Index = OwnerInventory->TileToIndex(DropTile);

    // Check if we can stack with existing item at this position
    if (UBaseItem* ExistingItem = OwnerInventory->GetItemAt(Index))
    {
        if (UItemInstanceObject* ExistingInstance = Cast<UItemInstanceObject>(ExistingItem))
        {
            return OwnerInventory->CanStackItemInstances(ExistingInstance, Item);
        }
    }

    // Check if there's room for the item - cast to UBaseItem for compatibility
    UBaseItem* ItemAsBaseItem = Cast<UBaseItem>(Item);
    if (ItemAsBaseItem)
    {
        return OwnerInventory->IsRoomAvailable(ItemAsBaseItem, Index);
    }

    return false;
}

/* ============================= */
/* === VISUAL FEEDBACK === */
/* ============================= */

void UInventoryGrid::UpdateDragPreview(UBaseItem* DraggedItem, const FVector2D& MousePosition)
{
    if (!DraggedItem)
    {
        return;
    }

    // Determine preview color based on drop validity
    FLinearColor PreviewColor = CanDropItemAt(DraggedItem, MousePosition) ? ValidDropColor : InvalidDropColor;
    
    // Apply rarity color if enabled
    if (bShowRarityColors)
    {
        const EItemRarity Rarity = DraggedItem->GetItemInfo().ItemInfo.ItemRarity;
        FLinearColor RarityColor = GetRarityColor(Rarity);
        PreviewColor = FLinearColor::LerpUsingHSV(PreviewColor, RarityColor, 0.3f);
    }
    
    UpdateDragPreviewColor(PreviewColor);
}

void UInventoryGrid::UpdateDragPreviewForInstance(UItemInstanceObject* DraggedInstance, const FVector2D& MousePosition)
{
    if (!DraggedInstance)
    {
        return;
    }

    // Determine preview color based on drop validity
    FLinearColor PreviewColor = CanDropItemInstanceAt(DraggedInstance, MousePosition) ? ValidDropColor : InvalidDropColor;
    
    // Apply rarity color if enabled (use instance rarity)
    if (bShowRarityColors)
    {
        const EItemRarity Rarity = DraggedInstance->Instance.Rarity;
        FLinearColor RarityColor = GetRarityColor(Rarity);
        PreviewColor = FLinearColor::LerpUsingHSV(PreviewColor, RarityColor, 0.4f);
    }
    
    // Make preview more transparent for invalid placement
    if (!CanDropItemInstanceAt(DraggedInstance, MousePosition))
    {
        PreviewColor.A = 0.5f;
    }
    
    UpdateDragPreviewColor(PreviewColor);
    
    // Show tooltip for high-level items
    if (bShowDragTooltips && DraggedInstance->Instance.ItemLevel >= MinLevelForTooltips)
    {
        ShowDragTooltip(DraggedInstance);
    }
    else
    {
        HideDragTooltip();
    }
}

FLinearColor UInventoryGrid::GetRarityColor(EItemRarity Rarity) const
{
    switch (Rarity)
    {
        case EItemRarity::IR_GradeS: return FLinearColor::Yellow;           // Legendary
        case EItemRarity::IR_GradeA: return FLinearColor(1.0f, 0.5f, 0.0f); // Orange - Epic
        case EItemRarity::IR_GradeB: return FLinearColor::Blue;             // Rare
        case EItemRarity::IR_GradeC: return FLinearColor::Green;            // Uncommon
        case EItemRarity::IR_GradeD: return FLinearColor::White;            // Common
        case EItemRarity::IR_GradeF: return FLinearColor::Gray;             // Poor
        default: return FLinearColor::White;
    }
}

void UInventoryGrid::ShowDragTooltip(UItemInstanceObject* Item)
{
    if (!Item || !bShowDragTooltips)
    {
        return;
    }

    // Create tooltip text
    FString TooltipText = Item->GetItemInfo().ItemInfo.ItemName.ToString();
    TooltipText += FString::Printf(TEXT("\nLevel: %d"), Item->Instance.ItemLevel);
    
    // Add rarity
    FString RarityString = UEnum::GetValueAsString(Item->Instance.Rarity);
    RarityString = RarityString.Replace(TEXT("EItemRarity::IR_"), TEXT(""));
    TooltipText += FString::Printf(TEXT("\nRarity: %s"), *RarityString);
    
    // Add affix counts
    if (Item->Instance.Prefixes.Num() > 0)
    {
        TooltipText += FString::Printf(TEXT("\n%d Prefix(es)"), Item->Instance.Prefixes.Num());
    }
    
    if (Item->Instance.Suffixes.Num() > 0)
    {
        TooltipText += FString::Printf(TEXT("\n%d Suffix(es)"), Item->Instance.Suffixes.Num());
    }

    // Add durability if applicable
    const float DurabilityPercent = Item->Instance.Durability.GetDurabilityPercent();
    if (DurabilityPercent < 1.0f)
    {
        TooltipText += FString::Printf(TEXT("\nDurability: %.0f%%"), DurabilityPercent * 100.0f);
    }
    
    // Create or update tooltip widget (implementation depends on your UI system)
    // This is a placeholder - you'll need to implement based on your tooltip system
    UE_LOG(LogInventoryGrid, VeryVerbose, TEXT("Drag Tooltip: %s"), *TooltipText);
}

void UInventoryGrid::HideDragTooltip()
{
    if (DragTooltipWidget)
    {
        DragTooltipWidget->SetVisibility(ESlateVisibility::Hidden);
    }
}

void UInventoryGrid::UpdateDragPreviewColor(const FLinearColor& Color)
{
    CurrentPreviewColor = Color;
    
    // Force a repaint to show the updated color
    Invalidate(EInvalidateWidgetReason::Paint);
}

/* ============================= */
/* === GRID CALCULATIONS === */
/* ============================= */

FTile UInventoryGrid::ScreenPositionToTile(const FVector2D& ScreenPosition) const
{
    const int32 TileX = FMath::FloorToInt(ScreenPosition.X / TileSize);
    const int32 TileY = FMath::FloorToInt(ScreenPosition.Y / TileSize);
    return FTile(FMath::Max(0, TileX), FMath::Max(0, TileY));
}

FVector2D UInventoryGrid::TileToScreenPosition(const FTile& Tile) const
{
    return FVector2D(Tile.X * TileSize, Tile.Y * TileSize);
}

void UInventoryGrid::MousePositioninTile(const FVector2D MousePosition, bool& Right, bool& Down) const
{
    // Calculate the position of the mouse relative to a tile and
    // determine whether it's on the right half or bottom half of a tile.
    const float HalfTileSize = TileSize / 2.0f;

    Right = fmod(MousePosition.X, TileSize) > HalfTileSize;
    Down = fmod(MousePosition.Y, TileSize) > HalfTileSize;
}

void UInventoryGrid::ForEachItem(TMap<UBaseItem*, FTile> ItemTileMap, const std::function<void(UBaseItem*, FTile)>& Callback)
{
    for (const auto& Elem : ItemTileMap)
    {
        UBaseItem* Item = Elem.Key;
        const FTile& TopLeftTile = Elem.Value;

        // Call the callback function with the item and tile
        Callback(Item, TopLeftTile);
    }
}

void UInventoryGrid::ForEachItemInstance(TMap<UItemInstanceObject*, FTile> ItemTileMap, const std::function<void(UItemInstanceObject*, FTile)>& Callback)
{
    for (const auto& Elem : ItemTileMap)
    {
        UItemInstanceObject* Item = Elem.Key;
        const FTile& TopLeftTile = Elem.Value;

        // Call the callback function with the item instance and tile
        Callback(Item, TopLeftTile);
    }
}

/* ============================= */
/* === RENDERING === */
/* ============================= */

int32 UInventoryGrid::NativePaint(
    const FPaintArgs& Args,
    const FGeometry& AllottedGeometry,
    const FSlateRect& MyCullingRect,
    FSlateWindowElementList& OutDrawElements,
    int32 LayerId,
    const FWidgetStyle& InWidgetStyle,
    bool bParentEnabled) const
{
    // Let parent paint first; keep the top layer it used.
    const int32 MaxLayerId = Super::NativePaint(
        Args, AllottedGeometry, MyCullingRect,
        OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

    // Draw the grid lines at the next layer.
    DrawGridLines(AllottedGeometry.ToPaintGeometry(), OutDrawElements, MaxLayerId + 1);

    // Drag preview (use the geometry itself; Do NOT convert it to FPaintGeometry here)
    if (DrawDropLocation)
    {
        FPaintContext Context(AllottedGeometry, MyCullingRect, OutDrawElements, MaxLayerId + 2, InWidgetStyle, bParentEnabled);

        // IMPORTANT: pass FGeometry, not FPaintGeometry
        DrawDragDropBox(Context, AllottedGeometry, OutDrawElements, MaxLayerId + 2);
    }

    // Reserve one more layer beyond everything we drew
    return MaxLayerId + 3;
}


void UInventoryGrid::DrawGridLines(const FPaintGeometry& PaintGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const
{
    if (!OwnerInventory || TileSize <= 0.0f)
    {
        return;
    }

    const FVector2D LocalSize = PaintGeometry.GetLocalSize();
    const int32 Columns = OwnerInventory->Columns;  // Use the actual property
    const int32 Rows = OwnerInventory->Rows;        // Use the actual property

    // Draw vertical lines
    for (int32 Col = 0; Col <= Columns; ++Col)
    {
        const float X = Col * TileSize;
        if (X <= LocalSize.X)
        {
            FSlateDrawElement::MakeLines(
                OutDrawElements,
                LayerId,
                PaintGeometry,
                TArray<FVector2D>{ FVector2D(X, 0.0f), FVector2D(X, Rows * TileSize) },
                ESlateDrawEffect::None,
                GridLineColor,
                false,
                GridLineThickness
            );
        }
    }

    // Draw horizontal lines
    for (int32 Row = 0; Row <= Rows; ++Row)
    {
        const float Y = Row * TileSize;
        if (Y <= LocalSize.Y)
        {
            FSlateDrawElement::MakeLines(
                OutDrawElements,
                LayerId,
                PaintGeometry,
                TArray<FVector2D>{ FVector2D(0.0f, Y), FVector2D(Columns * TileSize, Y) },
                ESlateDrawEffect::None,
                GridLineColor,
                false,
                GridLineThickness
            );
        }
    }
}

void UInventoryGrid::DrawDragDropBox(
    FPaintContext Context,
    const FGeometry& AllottedGeometry,
    FSlateWindowElementList& OutDrawElements,
    int32 LayerId) const
{
    // Allow either a BaseItem or an ItemInstance to drive the preview
    const bool bHasDraggedThing = (CurrentDraggedItem != nullptr) || (CurrentDraggedInstance != nullptr);
    if (!bHasDraggedThing || !OwnerInventory)
    {
        return;
    }

    // Get item dimensions (prefer instance if present)
    FIntPoint ItemDimensions;
    if (CurrentDraggedInstance)
    {
        ItemDimensions = CurrentDraggedInstance->GetItemInfo().ItemInfo.Dimensions;
    }
    else
    {
        ItemDimensions = CurrentDraggedItem->GetItemInfo().ItemInfo.Dimensions;
    }

    // Compute size and position in local space (tile units → pixels)
    const FVector2D BoxSize(ItemDimensions.X * TileSize, ItemDimensions.Y * TileSize);
    const FVector2D BoxPosition(DraggedItemTopLeft.X * TileSize, DraggedItemTopLeft.Y * TileSize);

    // Build a paint geometry using the widget's allotted geometry
    const FPaintGeometry BoxGeometry = AllottedGeometry.ToPaintGeometry(
        BoxSize,
        FSlateLayoutTransform(BoxPosition)
    );

    // Draw the drag preview rectangle
    FSlateDrawElement::MakeBox(
        OutDrawElements,
        LayerId,
        BoxGeometry,
        FCoreStyle::Get().GetBrush("WhiteBrush"),
        ESlateDrawEffect::None,
        CurrentPreviewColor
    );
}

