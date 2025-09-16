// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/Inventory/InventoryGrid.h"

#include "Blueprint/SlateBlueprintLibrary.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Character/PHBaseCharacter.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/InventoryManager.h"
#include "Slate/SlateBrushAsset.h"
#include "UI/Item/ItemWidget.h"
#include "Engine/World.h"
#include "TimerManager.h"

/* ============================= */
/* === Initialization Header === */
/* ============================= */

void UInventoryGrid::NativeConstruct()
{
	Super::NativeConstruct();

	if (!CanvasPanel || !GridBorder || !GridCanvas)
	{
		UE_LOG(LogTemp, Warning, TEXT("CanvasPanel, GridBorder, or GridCanvas is null."));
		return;
	}

	CanvasPanel->AddChild(GridBorder);
	GridBorder->AddChild(GridCanvas);

	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(GridCanvas->Slot))
	{
		CanvasSlot->SetZOrder(-1);
	}
}

void UInventoryGrid::NativeDestruct()
{
	CleanupDelegateBindings();
	
	// Clear any cached references
	if (GridCanvas)
	{
		GridCanvas->ClearChildren();
	}
	
	Lines.Empty();
	Super::NativeDestruct();
}

/* ============================= */
/* === Painting and Drawing === */
/* ============================= */

int32 UInventoryGrid::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
	int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	// 1. Always call Super and get the next available layer
	const int32 NewLayer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	// 2. Build drawing context
	const FPaintContext Context(AllottedGeometry, MyCullingRect, OutDrawElements, NewLayer, InWidgetStyle, bParentEnabled);

	// 3. Draw grid lines
	DrawGridLines(AllottedGeometry.ToPaintGeometry(), OutDrawElements, NewLayer);

	// 4. Optionally draw drop highlight box
	if (DrawDropLocation && UWidgetBlueprintLibrary::IsDragDropping())
	{
		DrawDragDropBox(Context, AllottedGeometry.ToPaintGeometry(), OutDrawElements, NewLayer);
	}

	// 5. Return layer to continue stacking above this widget
	return NewLayer + 1;
}

void UInventoryGrid::DrawGridLines(const FPaintGeometry& PaintGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const
{
	if (!GridBorder || Lines.Num() == 0)
	{
		return;
	}

	// Cache the local top left to avoid repeated calls
	static FVector2D CachedLocalTopLeft = FVector2D::ZeroVector;
	static const UBorder* LastGridBorder = nullptr;
    
	if (LastGridBorder != GridBorder)
	{
		CachedLocalTopLeft = USlateBlueprintLibrary::GetLocalTopLeft(GridBorder->GetCachedGeometry());
		LastGridBorder = GridBorder;
	}

	// Draw each line individually
	for (const FLine& Line : Lines)
	{
		TArray<FVector2D> LinePoints;
		LinePoints.Add(Line.Start + CachedLocalTopLeft);
		LinePoints.Add(Line.End + CachedLocalTopLeft);
        
		FSlateDrawElement::MakeLines(
			OutDrawElements, 
			LayerId + 1, 
			PaintGeometry, 
			LinePoints, 
			ESlateDrawEffect::None, 
			FLinearColor::White, 
			true, 
			1.0f
		);
	}
}

void UInventoryGrid::DrawDragDropBox(FPaintContext Context, const FPaintGeometry& PaintGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const
{
	// 1. Get the dragged item payload
	UBaseItem* Payload = const_cast<UInventoryGrid*>(this)->GetPayload(UWidgetBlueprintLibrary::GetDragDroppingContent());
	if (!Payload)
	{
		UE_LOG(LogTemp, Warning, TEXT("DrawDragDropBox: Payload is null."));
		return;
	}

	// 2. Validate TileSize
	if (TileSize <= 0.0f)
	{
		UE_LOG(LogTemp, Error, TEXT("DrawDragDropBox: TileSize is 0 or invalid!"));
		return;
	}

	// 3. Determine if room is available
	const bool bIsRoomAvailable = IsRoomAvailableforPayload(Payload);
	const FLinearColor BoxColor = bIsRoomAvailable
		? FLinearColor(0.0f, 1.0f, 0.0f, 0.35f)   // Green
		: FLinearColor(1.0f, 0.0f, 0.0f, 0.35f);  // Red

	// 4. Get position and size
	const FVector2D Position = FVector2D(DraggedItemTopLeft) * TileSize;
	const FVector2D Size = FVector2D(Payload->GetDimensions()) * TileSize;

	UE_LOG(LogTemp, Log, TEXT("DrawDragDropBox: TopLeft=(%d,%d) | Pos=(%.1f,%.1f) | Size=(%.1f,%.1f) | Available=%s"),
		DraggedItemTopLeft.X, DraggedItemTopLeft.Y,
		Position.X, Position.Y,
		Size.X, Size.Y,
		bIsRoomAvailable ? TEXT("Yes") : TEXT("No"));

	// 5. Check if brush is valid
	if (!Brush || !Brush->Brush.GetResourceObject())
	{
		UE_LOG(LogTemp, Error, TEXT("DrawDragDropBox: Brush or its resource is missing!"));
		return;
	}

	// 6. Draw the translucent drop area box
	UWidgetBlueprintLibrary::DrawBox(Context, Position, Size, Brush, BoxColor);
}

/* ============================= */
/* === Drag and Drop Events === */
/* ============================= */

void UInventoryGrid::NativeOnDragEnter(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDragEnter(InGeometry, InDragDropEvent, InOperation);
	DrawDropLocation = true;
}

void UInventoryGrid::NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDragLeave(InDragDropEvent, InOperation);
	DrawDropLocation = false;
}

FReply UInventoryGrid::NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::R)
	{
		if (UDragDropOperation* CurrentOperation = UWidgetBlueprintLibrary::GetDragDroppingContent())
		{
			if (UBaseItem* OutPayload = GetPayload(CurrentOperation))
			{
				OutPayload->ToggleRotation();
				if (UItemWidget* DragVisual = Cast<UItemWidget>(CurrentOperation->DefaultDragVisual))
				{
					CurrentOperation->Offset = FVector2D (0.5f,0.5f);
					DragVisual->Refresh();
					CurrentOperation->Offset = FVector2D (-0.25f,-0.25f);
				}
			}
		}
	}
	return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}

bool UInventoryGrid::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	UBaseItem* Payload = GetPayload(InOperation);
	if (!Payload) 
		return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);

	const FTile InTile{ DraggedItemTopLeft.X, DraggedItemTopLeft.Y };
	const int32 Index = OwnerInventory->TileToIndex(InTile);

	const bool bHandled = Payload->GetItemInfo().ItemInfo.OwnerID == OwnerInventory->GetOwnerCharacter()->GetInventoryManager()->GetID()
		? HandleOwnedItemDrop(Payload, Index)
		: HandleUnownedItemDrop(Payload, Index);

	return bHandled;
}

bool UInventoryGrid::HandleOwnedItemDrop(UBaseItem* Payload, const int32 Index) const
{
	if (!Payload || !OwnerInventory) 
		return false;

	UBaseItem* ExistingItem = OwnerInventory->GetItemAt(Index);

	// Step 1: Handle stacking with safer memory management
	if (Payload->IsStackable() && OwnerInventory->AreItemsStackable(ExistingItem, Payload))
	{
		const int32 MaxStack = ExistingItem->GetItemInfo().ItemInfo.MaxStackSize;
		const int32 CurrentQty = ExistingItem->GetItemInfo().ItemInfo.Quantity;
		const int32 NewQty = Payload->GetItemInfo().ItemInfo.Quantity;

		if (MaxStack == 0 || (CurrentQty + NewQty) <= MaxStack)
		{
			ExistingItem->AddQuantity(NewQty);
			
			// Remove the dragged item from inventory since we're merging it
			OwnerInventory->RemoveItemFromInventory(Payload);
			
			// Use a timer to delay destruction to avoid immediate reference issues
			FTimerHandle DestroyHandle;
			if (GetWorld())
			{
				GetWorld()->GetTimerManager().SetTimer(DestroyHandle, 
					[Payload]()
					{
						if (IsValid(Payload))
						{
							Payload->ConditionalBeginDestroy();
						}
					}, 
					0.1f, false);
			}
			
			OwnerInventory->OnInventoryChanged.Broadcast();
			return true;
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
	
	// Check if we have another inventory to trade with
	if (!OtherInventory)
	{
		UE_LOG(LogTemp, Warning, TEXT("OtherInventory is null - cannot handle unowned item drop"));
		return false;
	}
	
	bool WasAdded = false;
	
	// Try to process the buy/sell transaction
	const_cast<UInventoryGrid*>(this)->BuySellLogic(Payload, WasAdded);
	
	if (WasAdded)
	{
		// If purchase was successful, try to place at specific location
		if (OwnerInventory->IsRoomAvailable(Payload, Index))
		{
			return OwnerInventory->TryToAddItemAt(Payload, Index);
		}
		else
		{
			// Try to add anywhere in inventory
			return OwnerInventory->TryToAddItemToInventory(Payload, true);
		}
	}
	
	return false;
}

void UInventoryGrid::BuySellLogic(UBaseItem* Item, bool& WasAdded)
{
	// TODO: Implement buy/sell logic here
	// For now, just set WasAdded to false
	WasAdded = false;
}

void UInventoryGrid::ProcessTransaction(UBaseItem* Item, UInventoryManager* Seller, UInventoryManager* Buyer)
{
	// TODO: Implement transaction processing
}

void UInventoryGrid::HandleFailedTransaction(UBaseItem* Item, UInventoryManager* Seller, UInventoryManager* Buyer)
{
	// TODO: Implement failed transaction handling
}

bool UInventoryGrid::NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent,
                                      UDragDropOperation* InOperation)
{
	// Validate payload first
	UBaseItem* DraggedItem = Cast<UBaseItem>(InOperation->Payload);
	if (!DraggedItem)
	{
		return false;
	}

	// Convert mouse position to local coordinates
	const FVector2D MousePositionLocal = InGeometry.AbsoluteToLocal(InDragDropEvent.GetScreenSpacePosition());
	
	// Validate TileSize
	if (TileSize <= 0.0f)
	{
		UE_LOG(LogTemp, Error, TEXT("TileSize is invalid: %f"), TileSize);
		return false;
	}

	// Get item dimensions
	const FIntPoint Dimensions = DraggedItem->GetDimensions();
	
	// Calculate top-left position for centering the item under mouse
	DraggedItemTopLeft.X = FMath::FloorToInt(MousePositionLocal.X / TileSize) - (Dimensions.X / 2);
	DraggedItemTopLeft.Y = FMath::FloorToInt(MousePositionLocal.Y / TileSize) - (Dimensions.Y / 2);
	
	// Clamp to valid grid bounds
	const int32 MaxX = OwnerInventory->Columns - Dimensions.X;
	const int32 MaxY = OwnerInventory->Rows - Dimensions.Y;
	
	DraggedItemTopLeft.X = FMath::Clamp(DraggedItemTopLeft.X, 0, FMath::Max(0, MaxX));
	DraggedItemTopLeft.Y = FMath::Clamp(DraggedItemTopLeft.Y, 0, FMath::Max(0, MaxY));

	return true;
}

/* ============================= */
/* === Grid Management === */
/* ============================= */

void UInventoryGrid::GridInitialize(UInventoryManager* PlayerInventory, float InTileSize)
{
	// Check and assign the inventory owner
	if (!IsValid(PlayerInventory))
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayerInventory is null. Initialization aborted."));
		return;
	}
	OwnerInventory = PlayerInventory;
	TileSize = InTileSize;

	// Set up the canvas panel for the grid border
	if (UCanvasPanelSlot* CanvasSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(GridBorder))
	{
		// Calculate and set grid size based on the number of rows and columns
		const float InSizeX = OwnerInventory->Columns * TileSize;
		const float InSizeY = OwnerInventory->Rows * TileSize;

		CanvasSlot->SetSize(FVector2D(InSizeX, InSizeY));

		// Clear lines before creating new ones
		Lines.Empty();
		CreateLineSegments();
		Refresh();

		// Safely manage delegate binding - remove old bindings first
		CleanupDelegateBindings();
		
		// Bind new delegates
		OwnerInventory->OnInventoryChanged.AddDynamic(this, &UInventoryGrid::Refresh);
		if (OwnerCharacter && OwnerCharacter->GetEquipmentManager())
		{
			OwnerCharacter->GetEquipmentManager()->OnEquipmentChanged.AddDynamic(this, &UInventoryGrid::Refresh);
		}
	}
	else
	{
		// Log a warning if the cast to UCanvasPanelSlot failed
		UE_LOG(LogTemp, Warning, TEXT("GridBorder is not inside a UCanvasPanel."));
	}
}

void UInventoryGrid::CleanupDelegateBindings()
{
	if (IsValid(OwnerInventory) && OwnerInventory->OnInventoryChanged.IsBound())
	{
		OwnerInventory->OnInventoryChanged.RemoveDynamic(this, &UInventoryGrid::Refresh);
	}
	
	if (IsValid(OwnerCharacter) && OwnerCharacter->GetEquipmentManager() 
		&& OwnerCharacter->GetEquipmentManager()->OnEquipmentChanged.IsBound())
	{
		OwnerCharacter->GetEquipmentManager()->OnEquipmentChanged.RemoveDynamic(this, &UInventoryGrid::Refresh);
	}
}

void UInventoryGrid::CreateLineSegments()
{
	CreateVerticalLines();
	CreateHorizontalLines();
}

void UInventoryGrid::CreateVerticalLines()
{
	for (int32 x = 0; x <= OwnerInventory->Columns; x++)
	{
		const float XLocal = x * TileSize;

		// Initialize the FVector2D objects for Start and End
		const FVector2D Start(XLocal, 0.0f);
		const float EndY = OwnerInventory->Rows * TileSize;
		const FVector2D End(XLocal, EndY);

		// Initialize the FLine struct
		FLine NewLine;
		NewLine.Start = Start;
		NewLine.End = End;

		// Add the new line to the Lines array
		Lines.Add(NewLine);
	}
}

void UInventoryGrid::CreateHorizontalLines()
{
	for (int32 y = 0; y <= OwnerInventory->Rows; y++)
	{
		const float YLocal = y * TileSize;

		// Initialize the FVector2D objects for Start and End
		const FVector2D Start(0.0f, YLocal);
		const float EndX = OwnerInventory->Columns * TileSize;
		const FVector2D End(EndX, YLocal);

		// Initialize the FLine struct
		FLine NewLine;
		NewLine.Start = Start;
		NewLine.End = End;

		// Add the new line to the Lines array
		Lines.Add(NewLine);
	}
}

UBaseItem* UInventoryGrid::GetPayload(UDragDropOperation* DragDropOperation) 
{
	// Early return if DragDropOperation is null
	if (!DragDropOperation)
	{
		UE_LOG(LogTemp, Warning, TEXT("GetPayload: DragDropOperation is null."));
		return nullptr;
	}

	// Attempt to cast the Payload to a UBaseItem pointer
	if (UBaseItem* ItemPayload = Cast<UBaseItem>(DragDropOperation->Payload))
	{
		return ItemPayload;
	}

	// Log if the cast fails
	UE_LOG(LogTemp, Warning, TEXT("GetPayload: Payload cast to UBaseItem failed."));
	return nullptr;
}

void UInventoryGrid::Refresh()
{
	if (!IsValid(GridCanvas) || !IsValid(OwnerInventory))
	{
		LogInvalidPointers();
		return;
	}

	// Get current items from inventory
	const TMap<UBaseItem*, FTile>& CurrentItems = OwnerInventory->GetAllItems();
    
	// Track which widgets to keep/remove
	TArray<UWidget*> WidgetsToRemove;
	TSet<UBaseItem*> ItemsToAdd;
    
	// Collect current items that need widgets
	for (const auto& ItemPair : CurrentItems)
	{
		ItemsToAdd.Add(ItemPair.Key);
	}
    
	// Check existing widgets
	for (UWidget* Child : GridCanvas->GetAllChildren())
	{
		if (UItemWidget* ItemWidget = Cast<UItemWidget>(Child))
		{
			if (ItemWidget->ItemObject && ItemsToAdd.Contains(ItemWidget->ItemObject))
			{ 
				// Update position if needed
				if (const FTile* TilePtr = CurrentItems.Find(ItemWidget->ItemObject))
				{
					UpdateItemWidgetPosition(ItemWidget, *TilePtr);
					ItemsToAdd.Remove(ItemWidget->ItemObject);
				}
			}
			else
			{
				// Item no longer exists, mark for removal
				WidgetsToRemove.Add(Child);
			}
		}
	}
    
	// Remove outdated widgets
	for (UWidget* Widget : WidgetsToRemove)
	{
		GridCanvas->RemoveChild(Widget);
	}
    
	// Add new items
	for (UBaseItem* Item : ItemsToAdd)
	{
		if (const FTile* TilePtr = CurrentItems.Find(Item))
		{
			AddItemToGrid(Item, *TilePtr);
		}
	}
}

void UInventoryGrid::UpdateItemWidgetPosition(const UItemWidget* ItemWidget, const FTile& TopLeftTile) const
{
	if (UCanvasPanelSlot* PHSlot = Cast<UCanvasPanelSlot>(ItemWidget->Slot))
	{
		FVector2D NewPosition;
		NewPosition.X = TopLeftTile.X * TileSize;
		NewPosition.Y = TopLeftTile.Y * TileSize;
		PHSlot->SetPosition(NewPosition);
	}
}

void UInventoryGrid::LogInvalidPointers() const
{
	if (!IsValid(GridCanvas))
	{
		UE_LOG(LogTemp, Warning, TEXT("GridCanvas is null in UInventoryGrid::Refresh."));
	}

	if (!IsValid(OwnerInventory))
	{
		UE_LOG(LogTemp, Warning, TEXT("OwnerInventory is null in UInventoryGrid::Refresh."));
	}
}

void UInventoryGrid::AddItemToGrid(UBaseItem* Item, const FTile TopLeftTile)
{
	if (!Item)
	{
		UE_LOG(LogTemp, Warning, TEXT("AddItemToGrid: Item is null."));
		return;
	}

	if (!GridCanvas)
	{
		UE_LOG(LogTemp, Warning, TEXT("AddItemToGrid: GridCanvas is null."));
		return;
	}
	
	FItemInformation TempItemInfo = Item->GetItemInfo();
	TempItemInfo.ItemInfo.LastSavedSlot = ECurrentItemSlot::CIS_Inventory;
	Item->SetItemInfo(TempItemInfo);
	
	APlayerController* Owner = GetOwningPlayer();
	if (!Owner)
	{
		UE_LOG(LogTemp, Warning, TEXT("AddItemToGrid: OwnerPlayerController is null."));
		return;
	}

	// Create the item widget
	UItemWidget* CreatedWidget = CreateWidget<UItemWidget>(Owner, ItemClass);
	if (!CreatedWidget)
	{
		UE_LOG(LogTemp, Warning, TEXT("AddItemToGrid: Could not create UItemWidget."));
		return;
	}

	// Configure the widget properties
	CreatedWidget->TileSize = TileSize;
	CreatedWidget->OwnerInventory = OwnerInventory;
	CreatedWidget->ItemObject = Item;
	CreatedWidget->OnRemoved.AddDynamic(this, &UInventoryGrid::OnItemRemoved);

	// Add the widget to the grid canvas
	UPanelSlot* ReturnValue = GridCanvas->AddChild(CreatedWidget);
	if (UCanvasPanelSlot* RefSlot = Cast<UCanvasPanelSlot>(ReturnValue))
	{
		RefSlot->SetZOrder(100);
		RefSlot->SetAutoSize(true);

		FVector2D Position;
		Position.X = TopLeftTile.X * TileSize;
		Position.Y = TopLeftTile.Y * TileSize;
		RefSlot->SetPosition(Position);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("AddItemToGrid: Failed to cast slot to UCanvasPanelSlot."));
	}
}

void UInventoryGrid::OnItemRemoved(UBaseItem* InItemInfo)
{
	if (OwnerInventory)
	{
		OwnerInventory->RemoveItemFromInventory(InItemInfo);
	}
}

bool UInventoryGrid::IsRoomAvailableforPayload(UBaseItem* Payload) const 
{
	// Check if Payload and OwnerInventory are valid
	if (!IsValid(Payload) || !OwnerInventory)
	{
		UE_LOG(LogTemp, Warning, TEXT("IsRoomAvailableForItem: Invalid Payload or OwnerInventory is null."));
		return false;
	}

	// Initialize InTile with the dragged item's top-left position
	const FTile InTile{ DraggedItemTopLeft.X, DraggedItemTopLeft.Y };

	// Convert tile to index and check room availability in the inventory
	const int32 Index = OwnerInventory->TileToIndex(InTile);
	return OwnerInventory->IsRoomAvailable(Payload, Index);
}

void UInventoryGrid::MousePositioninTile(const FVector2D MousePosition, bool& Right, bool& Down) const
{
	// Calculate the position of the mouse relative to a tile and
	// determine whether it's on the right half or bottom half of a tile.
	// Use constant to avoid duplicate calculations
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