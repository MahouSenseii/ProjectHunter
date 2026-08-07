#pragma once

#include "CoreMinimal.h"
#include "Character/HUD/HunterHUDBaseWidget.h"
#include "Menu/Library/Structs/MenuStructs.h"
#include "PHEquipmentSlotWidget.generated.h"

class APHBaseCharacter;
class UButton;
class UEquipmentManager;
class UImage;
class UItemInstance;
class UTextBlock;
class UPHEquipmentMenuPageWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEquipmentSlotWidgetDataRefreshed, FEquipmentMenuSlotViewData, SlotData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnEquipmentSlotWidgetChanged,
	EEquipmentSlot, EquipmentSlot, UItemInstance*, NewItem, UItemInstance*, OldItem);

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

	UPROPERTY(BlueprintAssignable, Category = "Equipment Slot|Events")
	FOnEquipmentSlotWidgetDataRefreshed SlotDataRefreshed;

	UPROPERTY(BlueprintAssignable, Category = "Equipment Slot|Events")
	FOnEquipmentSlotWidgetChanged EquipmentSlotChanged;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeInitializeForCharacter(APHBaseCharacter* Character) override;
	virtual void NativeReleaseCharacter() override;

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

	UPROPERTY(BlueprintReadOnly, Category = "Equipment Slot")
	FEquipmentMenuSlotViewData SlotData;

	UPROPERTY(BlueprintReadOnly, Category = "Equipment Slot")
	TObjectPtr<UPHEquipmentMenuPageWidget> OwningEquipmentPage = nullptr;

	UFUNCTION(BlueprintImplementableEvent, Category = "Equipment Slot|Events")
	void OnSlotDataRefreshed(FEquipmentMenuSlotViewData NewSlotData);

	UFUNCTION(BlueprintImplementableEvent, Category = "Equipment Slot|Events")
	void OnEquipmentSlotChanged(EEquipmentSlot EquipmentSlot, UItemInstance* NewItem, UItemInstance* OldItem);

	UFUNCTION(BlueprintImplementableEvent, Category = "Equipment Slot|Events")
	void OnConnectedEquipmentSlotChanged(EEquipmentSlot NewSlot);

private:
	void BindManagerDelegates();
	void UnbindManagerDelegates();
	void RefreshVisuals();

	UFUNCTION()
	void HandleSlotClicked();

	UFUNCTION()
	void HandleEquipmentChanged(EEquipmentSlot EquipmentSlot, UItemInstance* NewItem, UItemInstance* OldItem);

	UPROPERTY(Transient)
	TObjectPtr<UEquipmentManager> EquipmentManager = nullptr;
};
