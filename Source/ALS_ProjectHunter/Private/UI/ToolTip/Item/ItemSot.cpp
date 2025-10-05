// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Item/ItemSot.h"

#include "Blueprint/DragDropOperation.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/InventoryManager.h"
#include "Item/EquippedObject.h"
#include "Library/PHItemFunctionLibrary.h"
#include "UI/Item/ItemWidget.h"

void UItemSot::NativeConstruct()
{
	Super::NativeConstruct();
	//get owner to add equipment manager 
	APHBaseCharacter* Owner = Cast<APHBaseCharacter>(GetOwningPlayerPawn());
	if(!Owner)
	{
		UE_LOG(LogTemp, Warning, TEXT("Owner not set. : Itemslot line 24"));
	}
	else
	{
		// added equipment manager
		Equipment = Owner->GetEquipmentManager();
		Inventory = Owner->GetInventoryManager();
		Equipment->OnEquipmentChanged.AddDynamic(this, &ThisClass::Refresh);
		Refresh();
	}
	if(EquipmentSlot!= EEquipmentSlot::ES_None)
	{
		SlotData->Base.EquipmentSlot = EquipmentSlot;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Equipment slot not set please set in BP_Inventory"));
	}
}

void UItemSot::NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDragLeave(InDragDropEvent, InOperation);

	if (Item_Button)
	{
		Item_Button->SetBackgroundColor(FLinearColor::White);
	}
}

bool UItemSot::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation)
{
 if (!CanvasPanel || !InOperation)
    {
        return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
    }

    UBaseItem* CastedItem = Cast<UBaseItem>(InOperation->Payload);
    if (!CastedItem)
    {
        return false; // Drop payload is invalid
    }

    if (UPHItemFunctionLibrary::AreItemSlotsEqual(CastedItem->GetItemInfo(), SlotData))
    {
        if (CanvasPanel->HasChild(ChildContent))
        {
            // Remove current child if one already exists in the slot
            CanvasPanel->RemoveChild(ChildContent);
        }

        // Rotate the item for placement
		UItemDefinitionAsset * TempItemInformation = CastedItem->GetItemInfo();
    	TempItemInformation->Base.Rotated = false;
    	CastedItem->SetItemInfo(TempItemInformation);
    	if(ItemWidgetClass)
    	{
    		CreateChildContent();
    	}
    	
            // Try equipping the item
            Equipment->TryToEquip(CastedItem, true, EquipmentSlot);

            // Set button color to indicate successful drop
            if (Item_Button)
            {
                Item_Button->SetBackgroundColor(FLinearColor::White);
            }
        
    }
    else
    {
        // If item doesn't match the slot, add it back to the inventory
        Inventory->TryToAddItemToInventory(CastedItem, true);
        
        // Set button color to indicate unsuccessful drop (back to inventory)
        if (Item_Button)
        {
            Item_Button->SetBackgroundColor(FLinearColor::White);
        }
    }
	this->Refresh();
    // Drop handled successfully, no need to call Super::NativeOnDrop
    return true;
}

bool UItemSot::NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent,
                                UDragDropOperation* InOperation)
{
	if (!InOperation)
	{
		return Super::NativeOnDragOver(InGeometry, InDragDropEvent, InOperation);
	}

	UBaseItem* AsItemObj = Cast<UBaseItem>(InOperation->Payload);
	if (!AsItemObj || !Item_Button)
	{
		return Super::NativeOnDragOver(InGeometry, InDragDropEvent, InOperation);
	}

	FLinearColor Color;

	if (UPHItemFunctionLibrary::AreItemSlotsEqual(AsItemObj->GetItemInfo(), SlotData))
	{
		// Set color to green-ish with slight transparency to indicate item can be placed
		Color = FLinearColor(0.0f, 1.0f, 0.080762f, 0.25f);
	}
	else
	{
		// Set color to red-ish with slight transparency to indicate item cannot be placed
		Color = FLinearColor(1.0f, 0.0f, 0.0f, 0.60f);

	}

	// Set button background color
	Item_Button->SetBackgroundColor(Color);

	return true; // Indicate the drag is being handled
	
}

FReply UItemSot::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{

	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent, this, EKeys::LeftMouseButton);
		return FReply::Handled();
	}
	return FReply::Unhandled();

}

void UItemSot::Refresh()
{
	if (!Equipment || !CanvasPanel || !Item_Button || !ItemWidgetClass)
	{
		return; // Exit early if any essential component is missing
	}
	
	CreateChildContent();
}

void UItemSot::CreateChildContent()
{
	check(ItemWidgetClass);
	if (UBaseItem** pRetrievedItem = Equipment->EquipmentData.Find(EquipmentSlot))
	{
		if (*pRetrievedItem)
		{
			UBaseItem* RetrievedItem = *pRetrievedItem;
			RetrievedItem->SetRotated(false);

			if (UItemWidget* CreatedWidget = CreateWidget<UItemWidget>(GetOwningPlayer(), ItemWidgetClass))
			{
				CreatedWidget->TileSize = 25.0f;
				CreatedWidget->SetAnchorsInViewport(FAnchors(1,1));
				CreatedWidget->ItemObject = RetrievedItem;
				CreatedWidget->OwnerInventory = Inventory;
				CreatedWidget->OnRemoved.AddDynamic(this, &UItemSot::RemoveItem);
				

				CanvasPanel->AddChild(CreatedWidget);
		
				if (UCanvasPanelSlot* CanvasSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(CreatedWidget))
				{
					const FVector2D InSize(100.0f, 100.0f); // Set to desired size
					const FVector2D InPosition(1.0f, 1.0f); // Set to desired position

					CanvasSlot->SetSize(InSize);
					CanvasSlot->SetPosition(InPosition);
				}
			}

			//Equipment->TryToEquip(RetrievedItem, true, EquipmentSlot);

			if (Item_Button)
			{
				Item_Button->SetBackgroundColor(FLinearColor::White);
			}
		}
	}
}

FSlateBrush UItemSot::GetItemImageBrush()
{
	FSlateBrush ItemBrush;

	switch (EquipmentSlot)
	{
	case EEquipmentSlot::ES_Head:
		// Assuming GetBrushFromPath is a function that creates a brush from a texture path or asset reference
			ItemBrush = Head;
		break;
        
	case EEquipmentSlot::ES_Gloves:
		ItemBrush = Gloves;
		break;

	case EEquipmentSlot::ES_Neck:
		ItemBrush = Neck;
		break;

	case EEquipmentSlot::ES_Chestplate:
		ItemBrush = Chestplate;
		break;

	case EEquipmentSlot::ES_Legs:
		ItemBrush = Legs;
		break;

	case EEquipmentSlot::ES_Boots:
		ItemBrush = Boots;
		break;

	case EEquipmentSlot::ES_MainHand:
		ItemBrush = SingleHanded ;
		break;

	case EEquipmentSlot::ES_OffHand:
		ItemBrush = SingleHanded;
		break;

	case EEquipmentSlot::ES_Ring:
		ItemBrush = Ring;
		break;

	case EEquipmentSlot::ES_Flask:
		ItemBrush = Flask;
		break;

	case EEquipmentSlot::ES_Belt:
		ItemBrush = Belt;
		break;

	case EEquipmentSlot::ES_None:
	default:
		// Set the ItemBrush to an empty or placeholder brush
		ItemBrush = static_cast<FSlateBrush>(FSlateNoResource());
		break;
	}

	return ItemBrush;
}

void UItemSot::RemoveItem(UBaseItem* Item)
{
	if(Item)
	{
		Equipment->RemoveEquippedItem(Item, EquipmentSlot);
	}
}

void UItemSot::SetEquipmentManager(UEquipmentManager* InEquipmentManager)
{
	Equipment = InEquipmentManager;
}
