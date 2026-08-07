#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Menu/Library/Structs/MenuStructs.h"
#include "PHInventorySlotWidget.generated.h"

class UPHInventoryMenuPanelWidget;
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
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class ALS_PROJECTHUNTER_API UPHInventorySlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Inventory Slot")
	void InitializeInventorySlot(
		UPHInventoryMenuPanelWidget* InOwningPanel,
		const FEquipmentMenuInventorySlotViewData& InSlotData);

	UFUNCTION(BlueprintCallable, Category = "Inventory Slot")
	void SetSlotData(const FEquipmentMenuInventorySlotViewData& InSlotData);

	UFUNCTION(BlueprintPure, Category = "Inventory Slot")
	FEquipmentMenuInventorySlotViewData GetSlotData() const { return SlotData; }

	UFUNCTION(BlueprintPure, Category = "Inventory Slot")
	int32 GetSlotIndex() const { return SlotData.SlotIndex; }

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

	/** Optional named children make the generated grid cells data-driven. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Inventory Slot|Widgets")
	TObjectPtr<UButton> SlotButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Inventory Slot|Widgets")
	TObjectPtr<UImage> ItemIcon;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Inventory Slot|Widgets")
	TObjectPtr<UTextBlock> ItemNameText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Inventory Slot|Widgets")
	TObjectPtr<UTextBlock> SlotIndexText;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory Slot")
	FEquipmentMenuInventorySlotViewData SlotData;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory Slot")
	TObjectPtr<UPHInventoryMenuPanelWidget> OwningInventoryPanel = nullptr;

	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory Slot|Events")
	void OnSlotDataRefreshed(FEquipmentMenuInventorySlotViewData NewSlotData);

private:
	void RefreshVisuals();

	UFUNCTION()
	void HandleSlotClicked();
};
