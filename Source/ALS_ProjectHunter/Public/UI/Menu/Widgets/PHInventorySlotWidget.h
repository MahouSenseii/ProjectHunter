#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Menu/Interfaces/PHInventorySlotHost.h"
#include "UI/Menu/Library/Structs/MenuStructs.h"
#include "PHInventorySlotWidget.generated.h"

class IPHInventorySlotHost;
class UDragDropOperation;
class UPHItemDragDropOperation;
class UButton;
class UImage;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnInventorySlotWidgetDataRefreshed,
	FEquipmentMenuInventorySlotViewData, SlotData);

/**
 * Reusable visual cell for one inventory slot.
 *
 * The inventory panel supplies view data and owns requests. Blueprint children
 * only render the cell and forward selection/equip input through this API.
 *
 * Mouse behaviour handled here (no Blueprint graph required):
 * - hover shows the shared item tooltip and follows the cursor
 * - press-and-drag starts an item drag carrying this slot index
 * - dropping another slot here moves/swaps, or unequips into the bag
 * - releasing outside the menu drops the item on the ground
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class ALS_PROJECTHUNTER_API UPHInventorySlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * Binds this cell to whatever owns the grid - the inventory panel, or the
	 * equipment page when it hosts the grid directly.
	 */
	void InitializeInventorySlot(
		const TScriptInterface<IPHInventorySlotHost>& InSlotHost,
		const FEquipmentMenuInventorySlotViewData& InSlotData);

	/** The owning panel or page, for Blueprint access. */
	UFUNCTION(BlueprintPure, Category = "Inventory Slot")
	UObject* GetSlotHostObject() const { return SlotHost.GetObject(); }

	UFUNCTION(BlueprintCallable, Category = "Inventory Slot")
	void SetSlotData(const FEquipmentMenuInventorySlotViewData& InSlotData);

	UFUNCTION(BlueprintPure, Category = "Inventory Slot")
	FEquipmentMenuInventorySlotViewData GetSlotData() const { return SlotData; }

	UFUNCTION(BlueprintPure, Category = "Inventory Slot")
	int32 GetSlotIndex() const { return SlotData.SlotIndex; }

	UFUNCTION(BlueprintPure, Category = "Inventory Slot")
	UItemInstance* GetItem() const { return SlotData.Item; }

	UFUNCTION(BlueprintPure, Category = "Inventory Slot")
	bool IsOccupied() const { return SlotData.Item != nullptr; }

	UFUNCTION(BlueprintCallable, Category = "Inventory Slot|Selection")
	void SelectSlot();

	UFUNCTION(BlueprintPure, Category = "Inventory Slot|Actions")
	bool CanEquipToSlot(EEquipmentSlot TargetSlot = EEquipmentSlot::ES_None) const;

	UFUNCTION(BlueprintCallable, Category = "Inventory Slot|Actions")
	bool RequestEquip(EEquipmentSlot TargetSlot = EEquipmentSlot::ES_None);

	UPROPERTY(BlueprintAssignable, Category = "Inventory Slot|Events")
	FOnInventorySlotWidgetDataRefreshed SlotDataRefreshed;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// HOVER / TOOLTIP

	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	// DRAG AND DROP

	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	virtual void NativeOnDragEnter(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual void NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	/** Optional named children make the generated grid cells data-driven. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Inventory Slot|Widgets")
	TObjectPtr<UButton> SlotButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Inventory Slot|Widgets")
	TObjectPtr<UImage> ItemIcon;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Inventory Slot|Widgets")
	TObjectPtr<UTextBlock> ItemNameText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Inventory Slot|Widgets")
	TObjectPtr<UTextBlock> SlotIndexText;

	/** Stack size. Worth binding now that the item name is hidden. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Inventory Slot|Widgets")
	TObjectPtr<UTextBlock> StackText;

	/**
	 * Equipment-slot leftover. An inventory cell has no slot name, so if a WBP
	 * copied from the equipment cell still has this it is bound here purely so it
	 * can be collapsed instead of showing its design-time text forever.
	 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Inventory Slot|Widgets")
	TObjectPtr<UTextBlock> SlotNameText;

	// CONFIG

	/**
	 * Slots identify items by icon. The name lives in the hover tooltip, so
	 * ItemNameText is collapsed unless this is turned back on.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory Slot|Config")
	bool bShowItemNameInSlot = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory Slot|Config")
	bool bShowTooltipOnHover = true;

	/** Tooltip tracks the cursor while hovering instead of staying put. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory Slot|Config")
	bool bTooltipFollowsMouse = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory Slot|Config")
	bool bEnableDragAndDrop = true;

	/** Releasing a drag outside any slot drops the item on the ground. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory Slot|Config")
	bool bDropToGroundWhenDraggedOutOfMenu = true;

	/** Optional WBP for the dragged icon. Falls back to a plain item image. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory Slot|Config")
	TSubclassOf<UUserWidget> DragVisualWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory Slot|Config")
	FVector2D DragVisualSize = FVector2D(64.0f, 64.0f);

	// STATE

	UPROPERTY(BlueprintReadOnly, Category = "Inventory Slot")
	FEquipmentMenuInventorySlotViewData SlotData;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory Slot")
	TScriptInterface<IPHInventorySlotHost> SlotHost;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory Slot|State")
	bool bSlotHovered = false;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory Slot|State")
	bool bDragOver = false;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory Slot|State")
	bool bValidDropTarget = false;

	// BLUEPRINT EVENTS

	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory Slot|Events")
	void OnSlotDataRefreshed(FEquipmentMenuInventorySlotViewData NewSlotData);

	/** Drive hover visuals here - the slot button is not used for hit-testing. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory Slot|Events")
	void OnSlotHoverChanged(bool bHovered);

	/** Drive drop-target highlighting (valid = green, invalid = red, etc). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory Slot|Events")
	void OnSlotDragOverChanged(bool bIsDragOver, bool bIsValidTarget);

	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory Slot|Events")
	void OnSlotDragStarted(UPHItemDragDropOperation* Operation);

private:
	void RefreshVisuals();
	void SetHovered(bool bNewHovered);
	void SetDragOverState(bool bNewDragOver, bool bNewValidTarget);

	UFUNCTION()
	void HandleSlotClicked();

	UFUNCTION()
	void HandleDragCancelled(UDragDropOperation* Operation);
};
