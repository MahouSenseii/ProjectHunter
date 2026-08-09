#pragma once

#include "CoreMinimal.h"
#include "UI/HUD/HunterHUDBaseWidget.h"
#include "Item/Library/Enums/ItemEnums.h"
#include "PHEquipmentMenuPanelWidget.generated.h"

class UPanelWidget;
class UPHEquipmentMenuPageWidget;
class UPHEquipmentSlotWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEquipmentMenuPanelSlotsRebuilt);

/**
 * Reusable equipment-slot layout module.
 *
 * A Blueprint can either supply a slot class for automatic construction or
 * place handcrafted equipment-slot children in this panel's widget tree.
 */
UCLASS(BlueprintType, Blueprintable)
class ALS_PROJECTHUNTER_API UPHEquipmentMenuPanelWidget : public UHunterHUDBaseWidget
{
	GENERATED_BODY()

public:
	UPHEquipmentMenuPanelWidget();

	UFUNCTION(BlueprintCallable, Category = "Equipment Panel")
	void RebuildEquipmentSlotWidgets();

	UFUNCTION(BlueprintCallable, Category = "Equipment Panel")
	void RefreshEquipmentSlotWidgets();

	UFUNCTION(BlueprintCallable, Category = "Equipment Panel")
	void SetEquipmentSlotOrder(const TArray<EEquipmentSlot>& NewEquipmentSlotOrder);

	UFUNCTION(BlueprintCallable, Category = "Equipment Panel|Config")
	void SetEquipmentSlotWidgetClass(TSubclassOf<UPHEquipmentSlotWidget> InSlotWidgetClass)
	{
		EquipmentSlotWidgetClass = InSlotWidgetClass;
	}

	void SetOwningEquipmentPage(UPHEquipmentMenuPageWidget* EquipmentPage);

	UPROPERTY(BlueprintAssignable, Category = "Equipment Panel|Events")
	FOnEquipmentMenuPanelSlotsRebuilt EquipmentSlotWidgetsRebuilt;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeInitializeForCharacter(APHBaseCharacter* Character) override;
	virtual void NativeReleaseCharacter() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment Panel|Config")
	TArray<EEquipmentSlot> EquipmentSlotOrder;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment Panel|Config")
	TSubclassOf<UPHEquipmentSlotWidget> EquipmentSlotWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment Panel|Config")
	bool bAutoBuildEquipmentSlotWidgets = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment Panel|Config", meta = (ClampMin = "1"))
	int32 GridColumns = 4;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Equipment Panel")
	TObjectPtr<UPanelWidget> EquipmentSlotContainer;

	UPROPERTY(BlueprintReadOnly, Category = "Equipment Panel")
	TArray<TObjectPtr<UPHEquipmentSlotWidget>> EquipmentSlotWidgets;

	UFUNCTION(BlueprintImplementableEvent, Category = "Equipment Panel|Events")
	void OnEquipmentSlotWidgetsRebuilt();

private:
	void CacheDesignedSlotWidgets();
	void ConfigureSlotWidget(UPHEquipmentSlotWidget* SlotWidget, EEquipmentSlot EquipmentSlot);

	UPROPERTY(Transient)
	TObjectPtr<UPHEquipmentMenuPageWidget> OwningEquipmentPage = nullptr;
};
