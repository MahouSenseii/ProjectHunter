#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Menu/Library/Structs/MenuStructs.h"
#include "MenuFunctionLibrary.generated.h"

UCLASS()
class ALS_PROJECTHUNTER_API UMenuFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Menu|Equipment")
	static TArray<EEquipmentSlot> GetDefaultEquipmentSlotOrder();

	UFUNCTION(BlueprintPure, Category = "Menu|Equipment")
	static FText GetEquipmentSlotDisplayName(EEquipmentSlot EquipmentSlot);

	UFUNCTION(BlueprintPure, Category = "Menu|Equipment")
	static FEquipmentMenuSlotViewData MakeEquipmentSlotViewData(EEquipmentSlot EquipmentSlot, UItemInstance* Item);

	UFUNCTION(BlueprintPure, Category = "Menu|Inventory")
	static FEquipmentMenuInventorySlotViewData MakeInventorySlotViewData(
		int32 SlotIndex,
		UItemInstance* Item,
		EEquipmentSlot SuggestedEquipmentSlot);
};
