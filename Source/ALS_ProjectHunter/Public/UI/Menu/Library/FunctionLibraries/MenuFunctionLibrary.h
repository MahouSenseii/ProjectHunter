#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Item/Library/Enums/ItemEnums.h"
#include "UI/Menu/Library/Structs/MenuStructs.h"
#include "MenuFunctionLibrary.generated.h"

UCLASS()
class ALS_PROJECTHUNTER_API UMenuFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * The item-grade colour ramp, so a slot cell, a rarity glyph and the item
	 * tooltip cannot drift apart.
	 *
	 * UItemTooltipWidget keeps its own EditAnywhere copies for per-tooltip
	 * overrides; their defaults come from the same PHUIStyle constants, so
	 * leaving those untouched means both paths agree.
	 */
	UFUNCTION(BlueprintPure, Category = "Menu|Style")
	static FLinearColor GetItemGradeColor(EItemRarity Grade);

	/** Grade shown as the bracketed glyph the reference art uses: [S], [SS]. */
	UFUNCTION(BlueprintPure, Category = "Menu|Style")
	static FText GetItemGradeGlyph(EItemRarity Grade);

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
