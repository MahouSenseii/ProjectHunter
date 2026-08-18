// Author: Quentin Davis

#include "UI/Menu/Helpers/MenuItemDragDropHelper.h"

#include "Blueprint/UserWidget.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "Item/ItemInstance.h"
#include "UI/Menu/DragDrop/PHItemDragDropOperation.h"

UPHItemDragDropOperation* FMenuItemDragDropHelper::MakeInventoryDrag(
	UUserWidget& SourceWidget,
	UItemInstance* Item,
	const int32 SourceSlotIndex,
	TSubclassOf<UUserWidget> DragVisualClass,
	const FVector2D DragVisualSize)
{
	UPHItemDragDropOperation* Operation =
		MakeOperation(SourceWidget, Item, DragVisualClass, DragVisualSize);
	if (!Operation)
	{
		return nullptr;
	}

	Operation->SourceType = EPHItemDragSource::IDS_Inventory;
	Operation->SourceInventorySlotIndex = SourceSlotIndex;
	return Operation;
}

UPHItemDragDropOperation* FMenuItemDragDropHelper::MakeEquipmentDrag(
	UUserWidget& SourceWidget,
	UItemInstance* Item,
	const EEquipmentSlot SourceSlot,
	TSubclassOf<UUserWidget> DragVisualClass,
	const FVector2D DragVisualSize)
{
	UPHItemDragDropOperation* Operation =
		MakeOperation(SourceWidget, Item, DragVisualClass, DragVisualSize);
	if (!Operation)
	{
		return nullptr;
	}

	Operation->SourceType = EPHItemDragSource::IDS_Equipment;
	Operation->SourceEquipmentSlot = SourceSlot;
	return Operation;
}

UWidget* FMenuItemDragDropHelper::MakeDragVisual(
	UUserWidget& SourceWidget,
	UItemInstance* Item,
	TSubclassOf<UUserWidget> DragVisualClass,
	const FVector2D DragVisualSize)
{
	if (!Item)
	{
		return nullptr;
	}

	if (DragVisualClass)
	{
		return CreateWidget<UUserWidget>(&SourceWidget, DragVisualClass);
	}

	UTexture2D* IconTexture = Item->GetInventoryIcon();
	if (!IconTexture)
	{
		return nullptr;
	}

	// No configured visual: build a plain image so the cursor still carries the
	// icon. DefaultDragVisual accepts any UWidget, not just a UUserWidget.
	UImage* IconImage = NewObject<UImage>(&SourceWidget);
	if (!IconImage)
	{
		return nullptr;
	}

	IconImage->SetBrushFromTexture(IconTexture, /*bMatchSize=*/false);
	IconImage->SetDesiredSizeOverride(DragVisualSize);
	IconImage->SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, 0.75f));
	return IconImage;
}

UPHItemDragDropOperation* FMenuItemDragDropHelper::MakeOperation(
	UUserWidget& SourceWidget,
	UItemInstance* Item,
	TSubclassOf<UUserWidget> DragVisualClass,
	const FVector2D DragVisualSize)
{
	if (!Item)
	{
		return nullptr;
	}

	UPHItemDragDropOperation* Operation =
		NewObject<UPHItemDragDropOperation>(&SourceWidget, UPHItemDragDropOperation::StaticClass());
	if (!Operation)
	{
		return nullptr;
	}

	Operation->Item = Item;
	Operation->SourceWidget = &SourceWidget;
	Operation->Pivot = EDragPivot::CenterCenter;
	Operation->DefaultDragVisual =
		MakeDragVisual(SourceWidget, Item, DragVisualClass, DragVisualSize);

	return Operation;
}
