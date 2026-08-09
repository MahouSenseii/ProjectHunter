#pragma once

#include "CoreMinimal.h"
#include "UI/Menu/Library/Structs/MenuStructs.h"
#include "UI/Menu/Widgets/PHMenuPageWidgetBase.h"
#include "PHEquipmentMenuPageWidget.generated.h"

class UItemInstance;
class UPHEquipmentMenuPanelWidget;
class UPHEquipmentSlotWidget;
class UPHInventoryMenuPanelWidget;

/**
 * C++ base for the equipment menu page.
 *
 * Blueprint WBP owns layout. This class supplies:
 * - left-side equipment slot data
 * - right-side inventory slot data
 * - selection state
 * - equip/unequip request helpers
 */
UCLASS(BlueprintType, Blueprintable)
class ALS_PROJECTHUNTER_API UPHEquipmentMenuPageWidget : public UPHMenuPageWidgetBase
{
	GENERATED_BODY()

public:
	UPHEquipmentMenuPageWidget();

	UFUNCTION(BlueprintCallable, Category = "Equipment Menu")
	void RefreshMenuData();

	UFUNCTION(BlueprintPure, Category = "Equipment Menu")
	TArray<FEquipmentMenuSlotViewData> GetEquipmentSlots() const { return EquipmentSlots; }

	UFUNCTION(BlueprintPure, Category = "Equipment Menu")
	TArray<FEquipmentMenuInventorySlotViewData> GetInventorySlots() const { return InventorySlots; }

	UFUNCTION(BlueprintPure, Category = "Equipment Menu")
	bool GetEquipmentSlotData(EEquipmentSlot EquipmentSlot, FEquipmentMenuSlotViewData& OutData) const;

	UFUNCTION(BlueprintPure, Category = "Equipment Menu")
	bool GetInventorySlotData(int32 SlotIndex, FEquipmentMenuInventorySlotViewData& OutData) const;

	UFUNCTION(BlueprintPure, Category = "Equipment Menu")
	UItemInstance* GetEquippedItem(EEquipmentSlot EquipmentSlot) const;

	UFUNCTION(BlueprintPure, Category = "Equipment Menu")
	UItemInstance* GetInventoryItem(int32 SlotIndex) const;

	UFUNCTION(BlueprintPure, Category = "Equipment Menu")
	bool CanEquipInventorySlotToSlot(int32 SlotIndex, EEquipmentSlot TargetSlot = EEquipmentSlot::ES_None) const;

	UFUNCTION(BlueprintPure, Category = "Equipment Menu")
	bool CanEquipItemToSlot(UItemInstance* Item, EEquipmentSlot TargetSlot = EEquipmentSlot::ES_None) const;

	UFUNCTION(BlueprintCallable, Category = "Equipment Menu|Selection")
	void SelectInventorySlot(int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category = "Equipment Menu|Selection")
	void SelectEquipmentSlot(EEquipmentSlot EquipmentSlot);

	UFUNCTION(BlueprintCallable, Category = "Equipment Menu|Selection")
	void ClearSelection();

	UFUNCTION(BlueprintPure, Category = "Equipment Menu|Selection")
	UItemInstance* GetSelectedItem() const { return SelectedItem; }

	UFUNCTION(BlueprintPure, Category = "Equipment Menu|Selection")
	int32 GetSelectedInventorySlotIndex() const { return SelectedInventorySlotIndex; }

	UFUNCTION(BlueprintPure, Category = "Equipment Menu|Selection")
	EEquipmentSlot GetSelectedEquipmentSlot() const { return SelectedEquipmentSlot; }

	UFUNCTION(BlueprintCallable, Category = "Equipment Menu|Actions")
	bool RequestEquipInventorySlot(int32 SlotIndex, EEquipmentSlot TargetSlot = EEquipmentSlot::ES_None);

	UFUNCTION(BlueprintCallable, Category = "Equipment Menu|Actions")
	bool RequestEquipItem(UItemInstance* Item, EEquipmentSlot TargetSlot = EEquipmentSlot::ES_None);

	UFUNCTION(BlueprintCallable, Category = "Equipment Menu|Actions")
	bool RequestEquipSelectedItem(EEquipmentSlot TargetSlot = EEquipmentSlot::ES_None);

	UFUNCTION(BlueprintCallable, Category = "Equipment Menu|Actions")
	bool RequestUnequipSlot(EEquipmentSlot EquipmentSlot, bool bMoveToBag = true);

	UFUNCTION(BlueprintCallable, Category = "Equipment Menu|Actions")
	bool RequestUnequipSelectedSlot(bool bMoveToBag = true);

	UFUNCTION(BlueprintPure, Category = "Equipment Menu")
	static FText GetEquipmentSlotDisplayName(EEquipmentSlot EquipmentSlot);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeInitializeForCharacter(APHBaseCharacter* Character) override;
	virtual void NativeReleaseCharacter() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment Menu|Config")
	TArray<EEquipmentSlot> EquipmentSlotOrder;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment Menu|Config")
	bool bIncludeEmptyInventorySlots = true;

	UPROPERTY(BlueprintReadOnly, Category = "Equipment Menu")
	TArray<FEquipmentMenuSlotViewData> EquipmentSlots;

	UPROPERTY(BlueprintReadOnly, Category = "Equipment Menu")
	TArray<FEquipmentMenuInventorySlotViewData> InventorySlots;

	UPROPERTY(BlueprintReadOnly, Category = "Equipment Menu|Equipment")
	TArray<TObjectPtr<UPHEquipmentSlotWidget>> EquipmentSlotWidgets;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Equipment Menu|Equipment")
	TObjectPtr<UPHEquipmentMenuPanelWidget> EquipmentPanel;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Equipment Menu|Inventory")
	TObjectPtr<UPHInventoryMenuPanelWidget> InventoryPanel;

	UPROPERTY(BlueprintReadOnly, Category = "Equipment Menu|Selection")
	TObjectPtr<UItemInstance> SelectedItem = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Equipment Menu|Selection")
	int32 SelectedInventorySlotIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Equipment Menu|Selection")
	EEquipmentSlot SelectedEquipmentSlot = EEquipmentSlot::ES_None;

	UPROPERTY(BlueprintReadOnly, Category = "Equipment Menu|Inventory")
	float CurrentCarryWeight = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Equipment Menu|Inventory")
	float MaxCarryWeight = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Equipment Menu|Inventory")
	int32 OccupiedInventorySlots = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Equipment Menu|Inventory")
	int32 MaxInventorySlots = 0;

	UFUNCTION(BlueprintImplementableEvent, Category = "Equipment Menu|Events")
	void OnMenuDataRefreshed();

	UFUNCTION(BlueprintImplementableEvent, Category = "Equipment Menu|Events")
	void OnSelectionChanged(UItemInstance* NewSelectedItem, int32 NewInventorySlotIndex, EEquipmentSlot NewEquipmentSlot);

	UFUNCTION(BlueprintImplementableEvent, Category = "Equipment Menu|Events")
	void OnEquipmentSlotChanged(EEquipmentSlot EquipmentSlot, UItemInstance* NewItem, UItemInstance* OldItem);

	UFUNCTION(BlueprintImplementableEvent, Category = "Equipment Menu|Events")
	void OnInventoryChanged();

	UFUNCTION(BlueprintImplementableEvent, Category = "Equipment Menu|Events")
	void OnCarryWeightChanged(float NewCurrentWeight, float NewMaxWeight);

private:
	void CacheChildWidgets();
	void BindInventoryPanelDelegates();
	void UnbindInventoryPanelDelegates();
	void RefreshEquipmentSlotWidgets();
	void SyncInventoryStateFromPanel();
	void BindManagerDelegates();
	void UnbindManagerDelegates();
	void RebuildEquipmentSlots();
	void RebuildInventorySlots();
	void UpdateInventorySummary();
	EEquipmentSlot ResolveSuggestedSlot(UItemInstance* Item) const;
	void SetSelection(UItemInstance* Item, int32 InventorySlotIndex, EEquipmentSlot EquipmentSlot);

	UFUNCTION()
	void HandleEquipmentChanged(EEquipmentSlot EquipmentSlot, UItemInstance* NewItem, UItemInstance* OldItem);

	UFUNCTION()
	void HandleInventoryChanged();

	UFUNCTION()
	void HandleCarryWeightChanged(float NewCurrentWeight, float NewMaxWeight);

	UFUNCTION()
	void HandleInventoryPanelDataRefreshed();

	UFUNCTION()
	void HandleInventoryPanelSelectionChanged(UItemInstance* NewSelectedItem, int32 NewInventorySlotIndex, EEquipmentSlot SuggestedEquipmentSlot);

	UFUNCTION()
	void HandleInventoryPanelCarryWeightChanged(float NewCurrentWeight, float NewMaxWeight);
};
