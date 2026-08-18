#pragma once

#include "CoreMinimal.h"
#include "UI/HUD/HunterHUDBaseWidget.h"
#include "UI/Menu/Interfaces/PHInventorySlotHost.h"
#include "UI/Menu/Library/Structs/MenuStructs.h"
#include "PHInventoryMenuPanelWidget.generated.h"

class APHBaseCharacter;
class UEquipmentManager;
class UInventoryManager;
class UItemInstance;
class UPanelWidget;
class UPHInventorySlotWidget;
class UPHItemDragDropOperation;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryMenuDataRefreshed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnInventoryMenuSelectionChanged,
	UItemInstance*, SelectedItem, int32, SlotIndex, EEquipmentSlot, SuggestedEquipmentSlot);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInventoryMenuCarryWeightChanged,
	float, CurrentWeight, float, MaxWeight);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryMenuSlotWidgetsRebuilt);

UCLASS(BlueprintType, Blueprintable)
class ALS_PROJECTHUNTER_API UPHInventoryMenuPanelWidget
	: public UHunterHUDBaseWidget
	, public IPHInventorySlotHost
{
	GENERATED_BODY()

public:
	UPHInventoryMenuPanelWidget();

	UFUNCTION(BlueprintCallable, Category = "Inventory Menu")
	void RefreshInventoryData();

	UFUNCTION(BlueprintCallable, Category = "Inventory Menu")
	void SetEquipmentSlotOrder(const TArray<EEquipmentSlot>& NewEquipmentSlotOrder);

	UFUNCTION(BlueprintCallable, Category = "Inventory Menu")
	void RebuildInventorySlotWidgets();

	UFUNCTION(BlueprintCallable, Category = "Inventory Menu|Config")
	void SetInventorySlotWidgetClass(TSubclassOf<UPHInventorySlotWidget> InSlotWidgetClass)
	{
		InventorySlotWidgetClass = InSlotWidgetClass;
	}

	UFUNCTION(BlueprintPure, Category = "Inventory Menu")
	TArray<FEquipmentMenuInventorySlotViewData> GetInventorySlots() const { return InventorySlots; }

	UFUNCTION(BlueprintPure, Category = "Inventory Menu")
	bool GetInventorySlotData(int32 SlotIndex, FEquipmentMenuInventorySlotViewData& OutData) const;

	UFUNCTION(BlueprintPure, Category = "Inventory Menu")
	UItemInstance* GetInventoryItem(int32 SlotIndex) const;

	UFUNCTION(BlueprintPure, Category = "Inventory Menu")
	virtual bool CanEquipInventorySlotToSlot(int32 SlotIndex, EEquipmentSlot TargetSlot = EEquipmentSlot::ES_None) const override;

	UFUNCTION(BlueprintPure, Category = "Inventory Menu")
	bool CanEquipItemToSlot(UItemInstance* Item, EEquipmentSlot TargetSlot = EEquipmentSlot::ES_None) const;

	UFUNCTION(BlueprintCallable, Category = "Inventory Menu|Selection")
	virtual void SelectInventorySlot(int32 SlotIndex) override;

	UFUNCTION(BlueprintCallable, Category = "Inventory Menu|Selection")
	void ClearSelection();

	UFUNCTION(BlueprintPure, Category = "Inventory Menu|Selection")
	UItemInstance* GetSelectedItem() const { return SelectedItem; }

	UFUNCTION(BlueprintPure, Category = "Inventory Menu|Selection")
	int32 GetSelectedInventorySlotIndex() const { return SelectedInventorySlotIndex; }

	UFUNCTION(BlueprintPure, Category = "Inventory Menu|Selection")
	EEquipmentSlot GetSelectedSuggestedEquipmentSlot() const { return SelectedSuggestedEquipmentSlot; }

	UFUNCTION(BlueprintCallable, Category = "Inventory Menu|Actions")
	virtual bool RequestEquipInventorySlot(int32 SlotIndex, EEquipmentSlot TargetSlot = EEquipmentSlot::ES_None) override;

	UFUNCTION(BlueprintCallable, Category = "Inventory Menu|Actions")
	bool RequestEquipSelectedItem(EEquipmentSlot TargetSlot = EEquipmentSlot::ES_None);

	// DRAG AND DROP

	/** True when Operation may be dropped onto TargetSlotIndex. */
	UFUNCTION(BlueprintPure, Category = "Inventory Menu|Drag Drop")
	virtual bool CanAcceptDroppedItem(UPHItemDragDropOperation* Operation, int32 TargetSlotIndex) const override;

	/**
	 * Applies a drop onto an inventory slot.
	 * Inventory source -> move/swap. Equipment source -> unequip into the bag.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory Menu|Drag Drop")
	virtual bool HandleItemDroppedOnSlot(UPHItemDragDropOperation* Operation, int32 TargetSlotIndex) override;

	/** Move or swap two inventory slots. Empty targets are handled by the swap. */
	UFUNCTION(BlueprintCallable, Category = "Inventory Menu|Actions")
	bool RequestMoveInventoryItem(int32 FromSlotIndex, int32 ToSlotIndex);

	/** Unequip into the bag. The item lands in the first free slot. */
	UFUNCTION(BlueprintCallable, Category = "Inventory Menu|Actions")
	bool RequestUnequipToInventory(EEquipmentSlot EquipmentSlot);

	/** Drop an inventory slot on the ground in front of the character. */
	UFUNCTION(BlueprintCallable, Category = "Inventory Menu|Actions")
	virtual bool RequestDropInventorySlotToGround(int32 SlotIndex) override;

	UFUNCTION(BlueprintPure, Category = "Inventory Menu|Summary")
	float GetCurrentCarryWeight() const { return CurrentCarryWeight; }

	UFUNCTION(BlueprintPure, Category = "Inventory Menu|Summary")
	float GetMaxCarryWeight() const { return MaxCarryWeight; }

	UFUNCTION(BlueprintPure, Category = "Inventory Menu|Summary")
	int32 GetOccupiedInventorySlots() const { return OccupiedInventorySlots; }

	UFUNCTION(BlueprintPure, Category = "Inventory Menu|Summary")
	int32 GetMaxInventorySlots() const { return MaxInventorySlots; }

	UPROPERTY(BlueprintAssignable, Category = "Inventory Menu|Events")
	FOnInventoryMenuDataRefreshed InventoryDataRefreshed;

	UPROPERTY(BlueprintAssignable, Category = "Inventory Menu|Events")
	FOnInventoryMenuSelectionChanged InventorySelectionChanged;

	UPROPERTY(BlueprintAssignable, Category = "Inventory Menu|Events")
	FOnInventoryMenuCarryWeightChanged InventoryCarryWeightChanged;

	UPROPERTY(BlueprintAssignable, Category = "Inventory Menu|Events")
	FOnInventoryMenuSlotWidgetsRebuilt InventorySlotWidgetsRebuilt;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeInitializeForCharacter(APHBaseCharacter* Character) override;
	virtual void NativeReleaseCharacter() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory Menu|Config")
	TArray<EEquipmentSlot> EquipmentSlotOrder;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory Menu|Config")
	bool bIncludeEmptyInventorySlots = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory Menu|Config")
	TSubclassOf<UPHInventorySlotWidget> InventorySlotWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory Menu|Config")
	bool bAutoBuildInventorySlotWidgets = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory Menu|Config", meta = (ClampMin = "1"))
	int32 GridColumns = 4;

	/**
	 * Size forced on every grid cell.
	 *
	 * Set to (0,0) to let the cell widget size itself - only safe when the cell's
	 * root is something with a real desired size (SizeBox, Overlay, VerticalBox).
	 * A CanvasPanel root reports zero, which collapses every cell in a
	 * UniformGridPanel and renders the grid as nothing.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory Menu|Config")
	FVector2D InventoryCellSize = FVector2D(128.0f, 144.0f);


	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Inventory Menu")
	TObjectPtr<UPanelWidget> InventorySlotContainer;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory Menu")
	TArray<FEquipmentMenuInventorySlotViewData> InventorySlots;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory Menu")
	TArray<TObjectPtr<UPHInventorySlotWidget>> InventorySlotWidgets;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory Menu|Selection")
	TObjectPtr<UItemInstance> SelectedItem = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory Menu|Selection")
	int32 SelectedInventorySlotIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory Menu|Selection")
	EEquipmentSlot SelectedSuggestedEquipmentSlot = EEquipmentSlot::ES_None;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory Menu|Summary")
	float CurrentCarryWeight = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory Menu|Summary")
	float MaxCarryWeight = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory Menu|Summary")
	int32 OccupiedInventorySlots = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory Menu|Summary")
	int32 MaxInventorySlots = 0;

	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory Menu|Events")
	void OnInventoryDataRefreshed();

	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory Menu|Events")
	void OnInventorySlotWidgetsRebuilt();

	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory Menu|Events")
	void OnInventorySelectionChanged(UItemInstance* NewSelectedItem, int32 NewInventorySlotIndex, EEquipmentSlot SuggestedEquipmentSlot);

	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory Menu|Events")
	void OnInventoryChanged();

	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory Menu|Events")
	void OnCarryWeightChanged(float NewCurrentWeight, float NewMaxWeight);

private:
	void BindManagerDelegates();
	void UnbindManagerDelegates();
	void RebuildInventorySlots();
	void UpdateInventorySummary();
	EEquipmentSlot ResolveSuggestedSlot(UItemInstance* Item) const;
	void SetSelection(UItemInstance* Item, int32 InventorySlotIndex, EEquipmentSlot SuggestedEquipmentSlot);

	UFUNCTION()
	void HandleEquipmentChanged(EEquipmentSlot EquipmentSlot, UItemInstance* NewItem, UItemInstance* OldItem);

	UFUNCTION()
	void HandleInventoryChanged();

	UFUNCTION()
	void HandleCarryWeightChanged(float NewCurrentWeight, float NewMaxWeight);

	UPROPERTY(Transient)
	TObjectPtr<UEquipmentManager> EquipmentManager = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UInventoryManager> InventoryManager = nullptr;
};
