// InventoryGrid.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Library/PHItemStructLibrary.h"
#include "Library/PHItemEnumLibrary.h"
#include "Item/BaseItem.h"
#include "Item/ItemInstance.h"
#include "Components/InventoryManager.h"
#include "InventoryGrid.generated.h"

class UInventoryManager;
class UItemInstanceObject;

/**
 * Widget responsible for displaying and handling interactions with an inventory grid
 * Supports both legacy BaseItem objects and new ItemInstance objects
 */
UCLASS()
class ALS_PROJECTHUNTER_API UInventoryGrid : public UUserWidget
{
    GENERATED_BODY()

public:
    UInventoryGrid(const FObjectInitializer& ObjectInitializer);

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
    virtual void NativeOnDragEnter(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
    virtual void NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
    virtual bool NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

/* ============================= */
/* === WIDGET REFERENCES === */
/* ============================= */
protected:
    UPROPERTY(meta = (BindWidget))
    UCanvasPanel* GridCanvas;

    UPROPERTY(meta = (BindWidget))
    UBorder* GridBorder;

/* ============================= */
/* === DRAG AND DROP HANDLING === */
/* ============================= */
protected:
    virtual bool HandleItemDrop(UBaseItem* Payload, const int32 Index);
    virtual bool HandleOwnedItemDrop(UBaseItem* Payload, const int32 Index) const;
    virtual bool HandleUnownedItemDrop(UBaseItem* Payload, const int32 Index) const;

    // ItemInstance-specific handling
    virtual bool HandleItemInstanceDrop(UItemInstanceObject* Payload, const int32 Index);
    virtual bool HandleOwnedItemInstanceDrop(UItemInstanceObject* Payload, const int32 Index) const;
    virtual bool HandleUnownedItemInstanceDrop(UItemInstanceObject* Payload, const int32 Index) const;

    // Validation methods
    bool CanDropItemAt(UBaseItem* Item, const FVector2D& Position) const;
    bool CanDropItemInstanceAt(UItemInstanceObject* Item, const FVector2D& Position) const;

/* ============================= */
/* === VISUAL FEEDBACK === */
/* ============================= */
protected:
    // Drag preview and visual feedback
    void UpdateDragPreview(UBaseItem* DraggedItem, const FVector2D& MousePosition);
    void UpdateDragPreviewForInstance(UItemInstanceObject* DraggedInstance, const FVector2D& MousePosition);
    
    // Rarity and visual helpers
    FLinearColor GetRarityColor(EItemRarity Rarity) const;
    void ShowDragTooltip(UItemInstanceObject* Item);
    void HideDragTooltip();
    
    // Drawing and rendering
    virtual int32 NativePaint(
        const FPaintArgs& Args,
        const FGeometry& AllottedGeometry,
        const FSlateRect& MyCullingRect,
        FSlateWindowElementList& OutDrawElements,
        int32 LayerId,
        const FWidgetStyle& InWidgetStyle,
        bool bParentEnabled) const override;

    /** Draws the grid lines */
    void DrawGridLines(const FPaintGeometry& PaintGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const;

    /** Draws the drag-and-drop box */
    void DrawDragDropBox(FPaintContext Context, const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const;

    /** Update preview colors */
    void UpdateDragPreviewColor(const FLinearColor& Color);

/* ============================= */
/* === GRID CALCULATIONS === */
/* ============================= */
public:
    /** Convert screen position to tile coordinates */
    UFUNCTION(BlueprintCallable, Category = "Inventory Grid")
    FTile ScreenPositionToTile(const FVector2D& ScreenPosition) const;

    /** Convert tile coordinates to screen position */
    UFUNCTION(BlueprintCallable, Category = "Inventory Grid") 
    FVector2D TileToScreenPosition(const FTile& Tile) const;

    /** Determines the tile position based on the mouse */
    void MousePositioninTile(FVector2D MousePosition, bool& Right, bool& Down) const;

    /** Iterates over each item in the grid and executes a callback */
    static void ForEachItem(TMap<UBaseItem*, FTile> ItemTileMap, const std::function<void(UBaseItem*, FTile)>& Callback);

    /** Iterates over each item instance in the grid and executes a callback */
    static void ForEachItemInstance(TMap<UItemInstanceObject*, FTile> ItemTileMap, const std::function<void(UItemInstanceObject*, FTile)>& Callback);

/* ============================= */
/* === STACKING HELPERS === */
/* ============================= */
protected:
    // Legacy BaseItem stacking
    bool TryStackBaseItems(UBaseItem* ExistingItem, UBaseItem* NewItem) const;
    
    // ItemInstance stacking
    bool TryStackItemInstances(UItemInstanceObject* ExistingInstance, UItemInstanceObject* NewInstance) const;
    
    // Mixed stacking validation
    bool CanStackMixedItems(UBaseItem* ExistingItem, UBaseItem* NewItem) const;

/* ============================= */
/* === INVENTORY REFERENCES === */
/* ============================= */
public:
    /** Initialize the grid with inventory references */
    UFUNCTION(BlueprintCallable, Category = "Inventory Grid")
    void InitializeGrid(UInventoryManager* Owner, UInventoryManager* Other = nullptr);

    /** Set the tile size for the grid */
    UFUNCTION(BlueprintCallable, Category = "Inventory Grid")
    void SetTileSize(float NewTileSize);

    /** Get the current tile size */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Inventory Grid")
    float GetTileSize() const { return TileSize; }

public:
    /** The size of each tile */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory Grid")
    float TileSize = 64.0f;

    /** Stores the position of the dragged item */
    UPROPERTY(BlueprintReadOnly, Category = "Inventory Grid")
    FIntPoint DraggedItemTopLeft = FIntPoint(0, 0);

    /** Reference to the player's inventory */
    UPROPERTY(BlueprintReadOnly, Category = "Inventory Grid")
    TObjectPtr<UInventoryManager> OwnerInventory;

    /** Reference to another inventory (for trading/selling) */
    UPROPERTY(BlueprintReadOnly, Category = "Inventory Grid")
    TObjectPtr<UInventoryManager> OtherInventory;

    /** Flag for drawing drop location */
    UPROPERTY(BlueprintReadOnly, Category = "Inventory Grid")
    bool DrawDropLocation = false;

protected:
    /** Currently dragged item for visual feedback */
    UPROPERTY()
    TObjectPtr<UBaseItem> CurrentDraggedItem;

    /** Currently dragged item instance for visual feedback */
    UPROPERTY()
    TObjectPtr<UItemInstanceObject> CurrentDraggedInstance;

    /** Current drag preview color */
    UPROPERTY()
    FLinearColor CurrentPreviewColor = FLinearColor::White;

    /** Tooltip widget for drag operations */
    UPROPERTY()
    TObjectPtr<UUserWidget> DragTooltipWidget;

/* ============================= */
/* === CONFIGURATION === */
/* ============================= */
protected:
    /** Grid line color */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory Grid|Visuals")
    FLinearColor GridLineColor = FLinearColor(0.1f, 0.1f, 0.1f, 0.8f);

    /** Grid line thickness */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory Grid|Visuals")
    float GridLineThickness = 1.0f;

    /** Drop preview color for valid placement */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory Grid|Visuals")
    FLinearColor ValidDropColor = FLinearColor::Green;

    /** Drop preview color for invalid placement */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory Grid|Visuals")
    FLinearColor InvalidDropColor = FLinearColor::Red;

    /** Whether to show rarity colors during drag */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory Grid|Visuals")
    bool bShowRarityColors = true;

    /** Whether to show tooltips during drag for high-level items */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory Grid|Visuals")
    bool bShowDragTooltips = true;

    /** Minimum item level to show drag tooltips */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory Grid|Visuals")
    int32 MinLevelForTooltips = 5;

/* ============================= */
/* === EVENTS === */
/* ============================= */
public:
    /** Called when an item is successfully dropped */
    UPROPERTY(BlueprintAssignable, Category = "Inventory Grid|Events")
    FOnInventoryChanged OnItemDropped;

    /** Called when drag operation starts */
    UPROPERTY(BlueprintAssignable, Category = "Inventory Grid|Events") 
    FOnInventoryChanged OnDragStarted;

    /** Called when drag operation ends */
    UPROPERTY(BlueprintAssignable, Category = "Inventory Grid|Events")
    FOnInventoryChanged OnDragEnded;
};