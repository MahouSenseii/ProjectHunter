// Copyright@2024 Quentin Davis

#pragma once

#include <functional>
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Library/PHItemStructLibrary.h"
#include "UI/Widgets/PHUserWidget.h"
#include "InventoryGrid.generated.h"

// Forward Declarations
class UBaseItem;
class AALSBaseCharacter;
class UInventoryManager;
class UItemWidget;

// Struct for grid lines used in drawing the inventory layout
USTRUCT(BlueprintType)
struct FLine
{
	GENERATED_BODY()

	FVector2D Start;
	FVector2D End;
};

/**
 * Represents an Inventory Grid UI where items are arranged and managed.
 */
UCLASS()
class ALS_PROJECTHUNTER_API UInventoryGrid : public UPHUserWidget
{
	GENERATED_BODY()

/* ============================= */
/* === UI ELEMENTS === */
/* ============================= */
public:
	/** The main UI panel for the inventory grid */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UCanvasPanel* CanvasPanel = nullptr;

	/** The border surrounding the inventory grid */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UBorder* GridBorder = nullptr;

	/** The canvas containing the grid layout */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UCanvasPanel* GridCanvas = nullptr;

	/** The UI widget class representing an inventory item */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TSubclassOf<UItemWidget> ItemClass;

	/** The brush asset used for styling grid elements */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	USlateBrushAsset* Brush = nullptr;

/* ============================= */
/* === GRID MANAGEMENT === */
/* ============================= */
public:
	/** Initializes the grid with the given inventory and tile size */
	UFUNCTION(BlueprintCallable)
	void GridInitialize(UInventoryManager* PlayerInventory, float InTileSize);

	/** Refreshes the UI grid by reloading all items */
	UFUNCTION(BlueprintCallable)
	void Refresh();

	/** Adds an item to the grid at a specified tile */
	void AddItemToGrid(UBaseItem* Item, const FTile TopLeftTile);

	/** Removes an item from the grid */
	UFUNCTION(BlueprintCallable)
	void OnItemRemoved(UBaseItem* InItemInfo);

	/** Checks if the grid has space for an item */
	UFUNCTION(BlueprintCallable)
	bool IsRoomAvailableforPayload(UBaseItem* Payload) const;

	/** Logs invalid pointers to help with debugging */
	void LogInvalidPointers() const;

	/** Gets the item being dragged */
	UFUNCTION(BlueprintCallable)
	UBaseItem* GetPayload(UDragDropOperation* DragDropOperation);

/* ============================= */
/* === GRID LINES (DRAWING) === */
/* ============================= */
public:
	/** Creates the vertical and horizontal grid lines */
	UFUNCTION(BlueprintCallable)
	void CreateLineSegments();

	/** Draws vertical grid lines */
	UFUNCTION(BlueprintCallable)
	void CreateVerticalLines();

	/** Draws horizontal grid lines */
	UFUNCTION(BlueprintCallable)
	void CreateHorizontalLines();

protected:
	/** The collection of grid lines for UI rendering */
	UPROPERTY(BlueprintReadOnly)
	TArray<FLine> Lines;

/* ============================= */
/* === DRAG & DROP HANDLING === */
/* ============================= */
public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** Handles drag enter event */
	virtual void NativeOnDragEnter(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	/** Handles drag leave event */
	virtual void NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	/** Handles key press events */
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	/** Handles item dropping */
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	/** Handles drag over event */
	virtual bool NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

/* ============================= */
/* === DRAG & DROP LOGIC === */
/* ============================= */
private:
	/** Handles item dropping when the item belongs to the player */
	bool HandleOwnedItemDrop(UBaseItem* Payload, int32 Index) const;

	/** Handles item dropping when the item belongs to another inventory */
	bool HandleUnownedItemDrop(UBaseItem* Payload, int32 Index);

/* ============================= */
/* === TRANSACTIONS & TRADING === */
/* ============================= */
public:
	/** Handles buying or selling an item */
	UFUNCTION(BlueprintCallable)
	void BuySellLogic(UBaseItem* Item, bool& WasAdded);

	/** Processes the transaction between two inventories */
	static void ProcessTransaction(UBaseItem* Item, UInventoryManager* Seller, UInventoryManager* Buyer);

	/** Handles failed transactions */
	static void HandleFailedTransaction(UBaseItem* Item, UInventoryManager* Seller, UInventoryManager* Buyer);

	/** Finds the owner of an item and its associated inventory */
	UFUNCTION(BlueprintCallable, BlueprintPure)
	UInventoryManager* FindOwners(UBaseItem* Item, UInventoryManager*& Other) const;


	void BuyFromAnotherID();

	/* ============================= */
/* === UI PAINTING FUNCTIONS === */
/* ============================= */
protected:
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
	void DrawDragDropBox(FPaintContext Context, const FPaintGeometry& PaintGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const;

/* ============================= */
/* === GRID CALCULATIONS === */
/* ============================= */
public:
	/** Determines the tile position based on the mouse */
	void MousePositioninTile(FVector2D MousePosition, bool& Right, bool& Down) const;

	/** Iterates over each item in the grid and executes a callback */
	static void ForEachItem(TMap<UBaseItem*, FTile> ItemTileMap, const std::function<void(UBaseItem*, FTile)>& Callback);

/* ============================= */
/* === INVENTORY REFERENCES === */
/* ============================= */
public:
	/** The size of each tile */
	float TileSize = 0.0;

	/** Stores the position of the dragged item */
	FIntPoint DraggedItemTopLeft = FIntPoint(0, 0);

	/** Reference to the player's inventory */
	TObjectPtr<UInventoryManager> OwnerInventory;

	/** Reference to another inventory (for trading/selling) */
	TObjectPtr<UInventoryManager> OtherInventory;

	/** Flag for drawing drop location */
	UPROPERTY(BlueprintReadOnly)
	bool DrawDropLocation;
};
