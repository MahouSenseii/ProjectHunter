#pragma once

#include "CoreMinimal.h"
#include "UI/HUD/HunterHUDBaseWidget.h"
#include "UI/Menu/Library/Structs/MenuStructs.h"
#include "PHEquipmentSlotWidget.generated.h"

class APHBaseCharacter;
class UButton;
class UDragDropOperation;
class UEquipmentManager;
class UImage;
class UItemInstance;
class UTextBlock;
class UPHEquipmentMenuPageWidget;
class UPHItemDragDropOperation;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEquipmentSlotWidgetDataRefreshed, FEquipmentMenuSlotViewData, SlotData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnEquipmentSlotWidgetChanged,
	EEquipmentSlot, EquipmentSlot, UItemInstance*, NewItem, UItemInstance*, OldItem);

/**
 * One equipment slot in the menu.
 *
 * Mouse behaviour handled here (no Blueprint graph required):
 * - hover shows the shared item tooltip and follows the cursor
 * - press-and-drag starts an item drag carrying this equipment slot
 * - dropping an inventory item here equips it, another equipment slot re-slots
 * - releasing outside the menu unequips into the bag
 *
 * There is no two-hand slot. A two-handed weapon is stored once, in ES_TwoHand,
 * and both hand slots show it and act on it while it is equipped.
 */
UCLASS(BlueprintType, Blueprintable)
class ALS_PROJECTHUNTER_API UPHEquipmentSlotWidget : public UHunterHUDBaseWidget
{
	GENERATED_BODY()

public:
	UPHEquipmentSlotWidget();

	UFUNCTION(BlueprintCallable, Category = "Equipment Slot")
	void SetConnectedEquipmentSlot(EEquipmentSlot NewSlot);

	UFUNCTION(BlueprintPure, Category = "Equipment Slot")
	EEquipmentSlot GetConnectedEquipmentSlot() const { return ConnectedEquipmentSlot; }

	UFUNCTION(BlueprintCallable, Category = "Equipment Slot")
	void SetOwningEquipmentPage(UPHEquipmentMenuPageWidget* EquipmentPage);

	UFUNCTION(BlueprintCallable, Category = "Equipment Slot")
	void RefreshSlot();

	UFUNCTION(BlueprintPure, Category = "Equipment Slot")
	FEquipmentMenuSlotViewData GetSlotData() const { return SlotData; }

	UFUNCTION(BlueprintPure, Category = "Equipment Slot")
	UItemInstance* GetItem() const { return SlotData.Item; }

	UFUNCTION(BlueprintPure, Category = "Equipment Slot")
	bool IsOccupied() const { return SlotData.bOccupied; }

	/**
	 * True when this hand slot is showing the two-handed weapon that fills both
	 * hands, rather than an item equipped into this slot alone.
	 */
	UFUNCTION(BlueprintPure, Category = "Equipment Slot")
	bool IsFilledByTwoHandedWeapon() const;

	UFUNCTION(BlueprintPure, Category = "Equipment Slot")
	bool CanAcceptItem(UItemInstance* Item) const;

	UFUNCTION(BlueprintCallable, Category = "Equipment Slot|Actions")
	bool RequestEquipItem(UItemInstance* Item);

	UFUNCTION(BlueprintCallable, Category = "Equipment Slot|Actions")
	bool RequestEquipSelectedItem();

	UFUNCTION(BlueprintCallable, Category = "Equipment Slot|Actions")
	bool RequestUnequip(bool bMoveToBag = true);

	UFUNCTION(BlueprintCallable, Category = "Equipment Slot|Selection")
	void SelectSlot();

	UFUNCTION(BlueprintPure, Category = "Equipment Slot")
	static FText GetEquipmentSlotDisplayName(EEquipmentSlot EquipmentSlot);

	// DRAG AND DROP

	/** True when Operation may be dropped on this slot. */
	UFUNCTION(BlueprintPure, Category = "Equipment Slot|Drag Drop")
	bool CanAcceptDroppedItem(UPHItemDragDropOperation* Operation) const;

	/** Applies a drop: equip from the bag, or re-slot from another equipment slot. */
	UFUNCTION(BlueprintCallable, Category = "Equipment Slot|Drag Drop")
	bool HandleItemDropped(UPHItemDragDropOperation* Operation);

	UPROPERTY(BlueprintAssignable, Category = "Equipment Slot|Events")
	FOnEquipmentSlotWidgetDataRefreshed SlotDataRefreshed;

	UPROPERTY(BlueprintAssignable, Category = "Equipment Slot|Events")
	FOnEquipmentSlotWidgetChanged EquipmentSlotChanged;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeInitializeForCharacter(APHBaseCharacter* Character) override;
	virtual void NativeReleaseCharacter() override;

	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	virtual void NativeOnDragEnter(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual void NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	/** Optional named children let one WBP render every EEquipmentSlot. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Equipment Slot|Widgets")
	TObjectPtr<UButton> SlotButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Equipment Slot|Widgets")
	TObjectPtr<UImage> ItemIcon;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Equipment Slot|Widgets")
	TObjectPtr<UTextBlock> SlotNameText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Equipment Slot|Widgets")
	TObjectPtr<UTextBlock> ItemNameText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment Slot|Config")
	EEquipmentSlot ConnectedEquipmentSlot = EEquipmentSlot::ES_None;

	// CONFIG

	/** The equipped item is identified by icon; its name lives in the tooltip. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment Slot|Config")
	bool bShowItemNameInSlot = false;

	/** Keep the slot label ("Helmet", "Main Hand") visible when the slot is empty. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment Slot|Config")
	bool bHideSlotNameWhenOccupied = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment Slot|Config")
	bool bShowTooltipOnHover = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment Slot|Config")
	bool bTooltipFollowsMouse = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment Slot|Config")
	bool bEnableDragAndDrop = true;

	/**
	 * Releasing a drag outside any slot unequips the item and drops it into the
	 * world, matching what the inventory cells do. Only reached when the menu
	 * root did not handle the drop first.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment Slot|Config")
	bool bDropToWorldWhenDraggedOutOfMenu = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment Slot|Config")
	TSubclassOf<UUserWidget> DragVisualWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment Slot|Config")
	FVector2D DragVisualSize = FVector2D(64.0f, 64.0f);

	// STATE

	UPROPERTY(BlueprintReadOnly, Category = "Equipment Slot")
	FEquipmentMenuSlotViewData SlotData;

	UPROPERTY(BlueprintReadOnly, Category = "Equipment Slot")
	TObjectPtr<UPHEquipmentMenuPageWidget> OwningEquipmentPage = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Equipment Slot|State")
	bool bSlotHovered = false;

	UPROPERTY(BlueprintReadOnly, Category = "Equipment Slot|State")
	bool bDragOver = false;

	UPROPERTY(BlueprintReadOnly, Category = "Equipment Slot|State")
	bool bValidDropTarget = false;

	// BLUEPRINT EVENTS

	UFUNCTION(BlueprintImplementableEvent, Category = "Equipment Slot|Events")
	void OnSlotDataRefreshed(FEquipmentMenuSlotViewData NewSlotData);

	UFUNCTION(BlueprintImplementableEvent, Category = "Equipment Slot|Events")
	void OnEquipmentSlotChanged(EEquipmentSlot EquipmentSlot, UItemInstance* NewItem, UItemInstance* OldItem);

	UFUNCTION(BlueprintImplementableEvent, Category = "Equipment Slot|Events")
	void OnConnectedEquipmentSlotChanged(EEquipmentSlot NewSlot);

	/** Drive hover visuals here - the slot button is not used for hit-testing. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Equipment Slot|Events")
	void OnSlotHoverChanged(bool bHovered);

	UFUNCTION(BlueprintImplementableEvent, Category = "Equipment Slot|Events")
	void OnSlotDragOverChanged(bool bIsDragOver, bool bIsValidTarget);

	UFUNCTION(BlueprintImplementableEvent, Category = "Equipment Slot|Events")
	void OnSlotDragStarted(UPHItemDragDropOperation* Operation);

private:
	/** Slot this widget reads and mutates - a hand slot follows the two-hander. */
	EEquipmentSlot GetOccupyingEquipmentSlot() const;

	/** True when a change to ChangedSlot alters what this slot shows. */
	bool IsAffectedByEquipmentChange(EEquipmentSlot ChangedSlot) const;

	void BindManagerDelegates();
	void UnbindManagerDelegates();
	void RefreshVisuals();
	void SetHovered(bool bNewHovered);
	void SetDragOverState(bool bNewDragOver, bool bNewValidTarget);

	UFUNCTION()
	void HandleSlotClicked();

	UFUNCTION()
	void HandleEquipmentChanged(EEquipmentSlot EquipmentSlot, UItemInstance* NewItem, UItemInstance* OldItem);

	UFUNCTION()
	void HandleDragCancelled(UDragDropOperation* Operation);

	UPROPERTY(Transient)
	TObjectPtr<UEquipmentManager> EquipmentManager = nullptr;
};
