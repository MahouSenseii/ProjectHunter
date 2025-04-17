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
	if (!GridBorder)
	{
		UE_LOG(LogTemp, Warning, TEXT("GridBorder is null."));
		return;
	}

	const FVector2D LocalTopLeft = USlateBlueprintLibrary::GetLocalTopLeft(GridBorder->GetCachedGeometry());

	for (const FLine& Line : Lines)
	{
		TArray<FVector2D> Points{ Line.Start + LocalTopLeft, Line.End + LocalTopLeft };
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, PaintGeometry, Points, ESlateDrawEffect::None, FLinearColor::White, true, 3.0f);
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
				OutPayload->Rotate();
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
	if (!Payload) return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);

	const FTile InTile{ DraggedItemTopLeft.X, DraggedItemTopLeft.Y };
	const int32 Index = OwnerInventory->TileToIndex(InTile);

	const bool bHandled = Payload->GetItemInfo().ItemInfo.OwnerID == OwnerInventory->GetOwnerCharacter()->GetInventoryManager()->GetID()
		? HandleOwnedItemDrop(Payload, Index)
		: HandleUnownedItemDrop(Payload, Index);

	return bHandled;
}



bool UInventoryGrid::HandleOwnedItemDrop(UBaseItem* Payload, const int32 Index) const
{
	if (!Payload || !OwnerInventory) return false;

	UBaseItem* ExistingItem = OwnerInventory->GetItemAt(Index);

	// Step 1: Explicit merge if dropped on matching stackable
	if (Payload->IsStackable() && OwnerInventory->AreItemsStackable(ExistingItem, Payload))
	{
		const int32 MaxStack = ExistingItem->GetItemInfo().ItemInfo.MaxStackSize;
		const int32 CurrentQty = ExistingItem->GetItemInfo().ItemInfo.Quantity;
		const int32 NewQty = Payload->GetItemInfo().ItemInfo.Quantity;

		if (MaxStack == 0 || (CurrentQty + NewQty) <= MaxStack)
		{
			ExistingItem->AddQuantity(NewQty);
			Payload->ConditionalBeginDestroy(); // Clean up merged item
			OwnerInventory->OnInventoryChanged.Broadcast();
			return true;
		}
	}

	// Step 2: Try to place item directly at specified location
	if (OwnerInventory->CanAcceptItemAt(Payload, Index))
	{
		OwnerInventory->AddItemAt(Payload, Index);
		return true;
	}

	// Step 3: Try to auto-place somewhere else in inventory
	if (OwnerInventory->TryToAddItemToInventory(Payload, true))
	{
		return true;
	}

	// Step 4: No valid space — drop to world
	return OwnerInventory->DropItemInInventory(Payload);
}


bool UInventoryGrid::HandleUnownedItemDrop(UBaseItem* Payload, int32 Index)
{
    bool WasAdded = false;
    if (!OtherInventory)
    {
		return false;
    }
    
    if (OwnerInventory->IsRoomAvailable(Payload, Index))
    {
        BuySellLogic(Payload, WasAdded);
    }
    else
    {
        BuySellLogic(Payload, WasAdded);
        if (WasAdded)
        {
            return OwnerInventory->TryToAddItemToInventory(Payload, true);
        }
    }
    
    if (WasAdded)
    {
        OwnerInventory->AddItemAt(Payload, Index);
    }
    return WasAdded; // Return true if the item was added, false otherwise
}


bool UInventoryGrid::NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation)
{
	// Convert mouse position to local coordinates
	const FVector2D MousePositionLocal = InGeometry.AbsoluteToLocal(InDragDropEvent.GetScreenSpacePosition());

	// Calculate mouse position in tile
	bool RightLocal, DownLocal;
	MousePositioninTile(MousePositionLocal, RightLocal, DownLocal);

	// Retrieve the dimensions of the dragged item
	const FIntPoint Dimensions = Cast<UBaseItem>(InOperation->Payload)->GetDimensions();

	// Calculate X and Y values based on mouse position
	const int32 XValue = RightLocal ? Dimensions.X - 1 : Dimensions.X;
	const int32 YValue = RightLocal ? Dimensions.Y - 1 : Dimensions.Y;

	// Clamp X and Y values (Note: This clamp might not be very useful in this context)
	const int32 ClampedXValue = FMath::Clamp(XValue, 0, Dimensions.X - 1);
	const int32 ClampedYValue = FMath::Clamp(YValue, 0, Dimensions.Y - 1);

	// Create point for clamped values
	const FIntPoint CreatedPoint(ClampedXValue, ClampedYValue);

	// Calculate the top-left point of the dragged item
	DraggedItemTopLeft.X = (MousePositionLocal.X / TileSize) - (CreatedPoint.X / 2);
	DraggedItemTopLeft.Y = (MousePositionLocal.Y / TileSize) - (CreatedPoint.Y / 2);

	return true;
}

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
		const float InSizeX = OwnerInventory->Colums * TileSize;
		const float InSizeY = OwnerInventory->Rows * TileSize;

		CanvasSlot->SetSize(FVector2D(InSizeX, InSizeY));

		// Create line segments for visual representation and refresh the inventory grid
		CreateLineSegments();
		Refresh();

		// Bind dynamic event to refresh the grid when the inventory changes
		if (OwnerInventory->OnInventoryChanged.IsBound())
		{
			OwnerInventory->OnInventoryChanged.RemoveDynamic(this, &UInventoryGrid::Refresh);
			OwnerCharacter->GetEquipmentManager()->OnEquipmentChanged.RemoveDynamic(this, &UInventoryGrid::Refresh);
		}
		OwnerInventory->OnInventoryChanged.AddDynamic(this, &UInventoryGrid::Refresh);
		OwnerCharacter->GetEquipmentManager()->OnEquipmentChanged.AddDynamic(this, &UInventoryGrid::Refresh);
	}
	else
	{
		// Log a warning if the cast to UCanvasPanelSlot failed
		UE_LOG(LogTemp, Warning, TEXT("GridBorder is not inside a UCanvasPanel."));
	}
}

void UInventoryGrid::CreateLineSegments()
{
	CreateVerticalLines();
	CreateHorizontalLines();

}

void UInventoryGrid::CreateVerticalLines()
{
	for (int32 x = 0; x <= OwnerInventory->Colums; x++)  // Changed the loop condition
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
		const float EndX = OwnerInventory->Colums * TileSize;
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
	if (IsValid(GridCanvas) && IsValid(OwnerInventory))
	{
		// Clear all children from GridCanvas before re-populating
		GridCanvas->ClearChildren();

		// Retrieve all items from OwnerInventory
		const TMap<UBaseItem*, FTile>& RetrievedItems = OwnerInventory->GetAllItems();

		// Iterate over each item in the inventory and add it to the grid
		for (const auto& ItemPair : RetrievedItems)
		{
			UBaseItem* Item = ItemPair.Key;
			const FTile& TopLeftTile = ItemPair.Value;

			AddItemToGrid(Item, TopLeftTile);
		}
	}
	else
	{
		LogInvalidPointers();
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

void UInventoryGrid::NativeDestruct()
{
	// Unbind dynamic delegate to prevent memory leaks or invalid calls
	if (IsValid(OwnerInventory) && OwnerInventory->OnInventoryChanged.IsBound())
	{
		OwnerInventory->OnInventoryChanged.RemoveDynamic(this, &UInventoryGrid::Refresh);
	}
	Super::NativeDestruct();
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
	TempItemInfo.ItemInfo.LastSavedSlot  = ECurrentItemSlot::CIS_Inventory;
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
	OwnerInventory->RemoveItemInInventory(InItemInfo);
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

void UInventoryGrid::BuySellLogic(UBaseItem* Item, bool& WasAdded)
{
	 if (!IsValid(Item) || !IsValid(OwnerInventory))
    {
        UE_LOG(LogTemp, Warning, TEXT("BuySellLogic: Invalid Item or OwnerInventory."));
        WasAdded = false;
        return;
    }

	 const FTile SetTiles = { DraggedItemTopLeft.X, DraggedItemTopLeft.Y };

    // If the item already belongs to the owner, just reposition it without buying or selling.
    if (Item->GetItemInfo().ItemInfo.OwnerID == OwnerInventory->GetOwnerCharacter()->GetInventoryManager()->GetID())
    {
        if (OwnerInventory->IsRoomAvailable(Item, OwnerInventory->TileToIndex(SetTiles)))
        {
            OwnerInventory->AddItemAt(Item, OwnerInventory->TileToIndex(SetTiles));
            WasAdded = true;
        }
        else
        {
            WasAdded = false;
        }
        return;
    }

    // Ensure the other inventory is valid.
    if (!IsValid(OtherInventory))
    {
        OwnerInventory->RemoveItemInInventory(Item);
        WasAdded = false;
        return;
    }

    // Determine the buyer and seller based on the current item owner.

	 // Determine the seller and buyer explicitly
    UInventoryManager* Seller = Item->GetItemInfo().ItemInfo.OwnerID == OwnerInventory->GetOwnerCharacter()->GetInventoryManager()->
                                                                               GetID()
	                                ? OwnerInventory
	                                : OtherInventory;
    UInventoryManager* Buyer = Seller == OwnerInventory ? OtherInventory : OwnerInventory;

    if (!Seller || !Buyer)
    {
        UE_LOG(LogTemp, Warning, TEXT("BuySellLogic: Seller or Buyer is not valid."));
        WasAdded = false;
        return;
    }

    // Check if the buyer has enough gems
    if (Buyer->HasEnoughGems(Item))
    {
        if (OwnerInventory->IsRoomAvailable(Item, OwnerInventory->TileToIndex(SetTiles)))
        {
            ProcessTransaction(Item, Seller, Buyer);
            Buyer->AddItemAt(Item, Buyer->TileToIndex(SetTiles));
            WasAdded = true;
        }
        else if (OwnerInventory->TryToAddItemToInventory(Item, true))
        {
            WasAdded = true;
        }
        else
        {
            OwnerInventory->DropItemInInventory(Item);
            WasAdded = false;
        }
    }
    else
    {
        HandleFailedTransaction(Item, Seller, Buyer);
        WasAdded = false;
    }

    Refresh(); // Refresh UI or state to reflect changes.
}


auto UInventoryGrid::ProcessTransaction(UBaseItem* Item, UInventoryManager* Seller, UInventoryManager* Buyer) -> void
{
	const int32 OriginalValue = Seller->CalculateValue(Item-> GetItemInfo());
	int32 TransactionValue = OriginalValue;

	if (Seller->GetOwnerCharacter()->IsPlayerControlled())
	{
		// Halve the item's value when selling by the player.
		TransactionValue /= 2;
	}

	Buyer->SubtractGems(TransactionValue);
	Seller->AddGems(TransactionValue);
}

void UInventoryGrid::HandleFailedTransaction(UBaseItem* Item, UInventoryManager* Seller, UInventoryManager* Buyer)
{
	if (Buyer && Buyer->ContainsItem(Item))
	{
		Buyer->RemoveItemInInventory(Item);
	}

	if (Seller && !Seller->TryToAddItemToInventory(Item, true))
	{
		UE_LOG(LogTemp, Warning, TEXT("HandleFailedTransaction: Failed to return item to seller's inventory."));
	}
}


UInventoryManager* UInventoryGrid::FindOwners(UBaseItem* Item, UInventoryManager*& Other) const
{
	if (!Item || !OwnerInventory || !OwnerInventory->GetOwnerCharacter() ||
		!OwnerInventory->GetOwnerCharacter()->GetInventoryManager())
	{
		UE_LOG(LogTemp, Warning, TEXT("FindOwners: Invalid parameters or states."));
		return nullptr;  // Early return for null checks and invalid states.
	}

	// If the Item's OwnerID matches the character ID of the owner of OwnerInventory,
	// this inventory is the owner of the item.
	if (Item->GetItemInfo().ItemInfo.OwnerID == OwnerInventory->GetOwnerCharacter()->GetInventoryManager()->GetID())
	{
		// The item belongs to the OwnerInventory.
		// Set 'Other' to represent the non-owner inventory in this scenario.
		Other = OtherInventory;
		return OwnerInventory;
	}
	else
	{
		// The item does not belong to the OwnerInventory, meaning it belongs to some 'Other' inventory.
		// In this case, we confirm that 'OtherInventory' is this 'Other' inventory,
		// and since 'Other' is meant to represent the alternative, it should reflect the 'OwnerInventory' in this context.
		Other = OwnerInventory;
		return OtherInventory;
	}
}


void UInventoryGrid::BuyFromAnotherID()
{
}
