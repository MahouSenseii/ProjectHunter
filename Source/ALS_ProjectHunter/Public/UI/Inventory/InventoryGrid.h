// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include <functional>
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Library/PHItemStructLibrary.h"
#include "UI/Widgets/PHUserWidget.h"
#include "InventoryGrid.generated.h"

class UBaseItem;
class AALSBaseCharacter;  // Forward declaration instead of include
class UInventoryManager;  // Assuming UInventoryManager is a class, forward declare it
class UItemWidget;
USTRUCT(BlueprintType)
struct FLine
{
	GENERATED_BODY()

	FVector2D Start;
	FVector2D End;
};

/**
 * 
 */
UCLASS()
class ALS_PROJECTHUNTER_API UInventoryGrid : public UPHUserWidget
{
	GENERATED_BODY()
	
public:
		// UI Elements
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UCanvasPanel* CanvasPanel = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UBorder* GridBorder = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UCanvasPanel* GridCanvas = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TSubclassOf<UItemWidget> ItemClass;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	USlateBrushAsset* Brush = nullptr;


	// Game Logic Members
	float TileSize = 0.0;
	FIntPoint DraggedItemTopLeft = FIntPoint(0, 0);
	TObjectPtr<UInventoryManager> OwnerInventory;
	TObjectPtr<UInventoryManager> OtherInventory;


protected:

	UPROPERTY(BlueprintReadOnly)
	TArray<FLine> Lines;

	UPROPERTY(BlueprintReadOnly)
	bool DrawDropLocation;

public:
	// Overridden Native functions
	virtual void NativeConstruct() override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	void DrawGridLines( const FPaintGeometry& PaintGeometry, FSlateWindowElementList& OutDrawElements,
	                   int32 LayerId) const;
	void DrawDragDropBox(FPaintContext Context, const FPaintGeometry& PaintGeometry, FSlateWindowElementList& OutDrawElements,
	                     int32 LayerId) const;
	virtual void NativeOnDragEnter(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual void NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual bool NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	bool HandleOwnedItemDrop(UBaseItem* Payload, int32 Index) const;
	bool HandleUnownedItemDrop(UBaseItem* Payload, int32 Index);
	// Custom functions
	UFUNCTION(BlueprintCallable)
	void GridInitialize(UInventoryManager* PlayerInventory, float InTileSize);

	UFUNCTION(BlueprintCallable)
	void CreateLineSegments();

	UFUNCTION(BlueprintCallable)
	void CreateVerticalLines();

	UFUNCTION(BlueprintCallable)
	void CreateHorizontalLines();

	UFUNCTION(BlueprintCallable)
	UBaseItem* GetPayload(UDragDropOperation* DragDropOperation);

	UFUNCTION(BlueprintCallable)
	void Refresh();
	void LogInvalidPointers() const;
	virtual void NativeDestruct() override;
	void AddItemToGrid(UBaseItem* Item, const FTile TopLeftTile);

	UFUNCTION(BlueprintCallable)
	void OnItemRemoved(UBaseItem* InItemInfo);

	UFUNCTION(BlueprintCallable)
	bool IsRoomAvailableforPayload(UBaseItem* Payload) const;

	void MousePositioninTile(FVector2D MousePosition, bool& Right, bool& Down) const;

	static void ForEachItem(TMap<UBaseItem*, FTile> ItemTileMap, const std::function<void(UBaseItem*, FTile)>& Callback);

	UFUNCTION(BlueprintCallable)
	void BuySellLogic(UBaseItem* Item, bool& WasAdded);
	static void ProcessTransaction(UBaseItem* Item, UInventoryManager* Seller, UInventoryManager* Buyer);

	static void HandleFailedTransaction(UBaseItem* Item, UInventoryManager* Seller, UInventoryManager* Buyer);
	UFUNCTION(BlueprintCallable, BlueprintPure)
	UInventoryManager* FindOwners(UBaseItem* Item, UInventoryManager*& Other) const;


	UFUNCTION(BlueprintCallable)
	void BuyFromAnotherID();
};
