// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/EquipmentManager.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "UI/Widgets/PHUserWidget.h"
#include "ItemSot.generated.h"

class UItemWidget;
/**
 * Class representing an item slot within a user interface.
 *
 * This class extends UPHUserWidget and provides functionality for managing an item slot including the display elements and interaction logic.
 */
UCLASS()
class ALS_PROJECTHUNTER_API UItemSot : public UPHUserWidget
{
	GENERATED_BODY()
	
private:
	
	
	// Button representing the clickable area of the item slot.
	UPROPERTY()
	UButton* Item_Button;

	// Border image for item slot.
	UPROPERTY()
	UImage* Img_Border;

	// Image of the item in the slot.
	UPROPERTY()
	UImage* Img_Item;

	// Text showing the quantity of the item in the slot.
	UPROPERTY()
	UTextBlock* Quantity;

	UPROPERTY()
	TObjectPtr<UEquipmentManager> Equipment;

	UPROPERTY()
	TObjectPtr<UInventoryManager> Inventory;

	UPROPERTY()
	TObjectPtr<UBaseItem>ItemObj;

	UPROPERTY()
	UItemDefinitionAsset* SlotData;
public:


	// Called after the underlying slate widget is constructed.
	virtual void NativeConstruct() override;

	virtual void NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual bool NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	/**
	 * Refresh the display elements of the item slot.
	 * Useful after an item has been added or removed.
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility")
	void Refresh();

	void CreateChildContent();

	UFUNCTION(BlueprintCallable, Category = "Getter")
	FSlateBrush GetItemImageBrush();

	UFUNCTION(BlueprintCallable, Category = "Remove")
	void RemoveItem(UBaseItem* Item);

	UFUNCTION(BlueprintCallable, BlueprintGetter, Category = "Getter")
	UEquipmentManager* GetEquipmentManager() {return  Equipment; }

	UFUNCTION(BlueprintCallable, BlueprintSetter, Category = "Setter")
	void SetEquipmentManager(UEquipmentManager* InEquipmentManager);

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UCanvasPanel* CanvasPanel;

	//UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	//UItemWidget* ItemWidget;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UItemWidget* ChildContent;

	UPROPERTY(BlueprintReadWrite,EditAnywhere, Category ="Brush")
	FSlateBrush Head;

	UPROPERTY(BlueprintReadWrite,EditAnywhere, Category ="Brush")
	FSlateBrush Gloves;

	UPROPERTY(BlueprintReadWrite,EditAnywhere, Category ="Brush")
	FSlateBrush Neck;

	UPROPERTY(BlueprintReadWrite,EditAnywhere, Category ="Brush")
	FSlateBrush Cloak;

	UPROPERTY(BlueprintReadWrite,EditAnywhere, Category ="Brush")
	FSlateBrush Chestplate;

	UPROPERTY(BlueprintReadWrite,EditAnywhere, Category ="Brush")
	FSlateBrush Legs;

	UPROPERTY(BlueprintReadWrite,EditAnywhere, Category ="Brush")
	FSlateBrush Boots;

	UPROPERTY(BlueprintReadWrite,EditAnywhere, Category ="Brush")
	FSlateBrush SingleHanded;	

	UPROPERTY(BlueprintReadWrite,EditAnywhere, Category ="Brush")
	FSlateBrush Belt;

	UPROPERTY(BlueprintReadWrite,EditAnywhere, Category ="Brush")
	FSlateBrush Ring;

	UPROPERTY(BlueprintReadWrite,EditAnywhere, Category ="Brush")
	FSlateBrush Flask;

	UPROPERTY(BlueprintReadWrite,EditAnywhere, Category ="Slot Data")
	EEquipmentSlot EquipmentSlot;

	UPROPERTY(BlueprintReadWrite,EditAnywhere, Category ="Slot Data")
	TSubclassOf<UItemWidget> ItemWidgetClass;
};
