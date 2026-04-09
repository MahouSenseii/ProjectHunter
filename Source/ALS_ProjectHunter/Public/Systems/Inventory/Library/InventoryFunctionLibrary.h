#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Item/Library/ItemEnums.h"
#include "Systems/Inventory/Library/InventoryEnums.h"
#include "InventoryFunctionLibrary.generated.h"

class UItemInstance;

UCLASS()
class ALS_PROJECTHUNTER_API UInventoryFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static UItemInstance* FindStackableItem(const TArray<UItemInstance*>& Items, const UItemInstance* Item);
	static int32 FindFirstEmptySlot(const TArray<UItemInstance*>& Items, int32 MaxSlots);
	static int32 FindSlotForItem(const TArray<UItemInstance*>& Items, const UItemInstance* Item);
	static TArray<UItemInstance*> FindItemsByBaseID(const TArray<UItemInstance*>& Items, FName BaseItemID);
	static TArray<UItemInstance*> FindItemsByType(const TArray<UItemInstance*>& Items, EItemType ItemType);
	static TArray<UItemInstance*> FindItemsByRarity(const TArray<UItemInstance*>& Items, EItemRarity Rarity);
	static bool HasItemWithID(const TArray<UItemInstance*>& Items, FGuid UniqueID);
	static int32 GetTotalQuantityOfItem(const TArray<UItemInstance*>& Items, FName BaseItemID);
	static void SortItems(TArray<UItemInstance*>& InOutItems, ESortMode SortMode, int32 MaxSlots);
	static void CompactItems(TArray<UItemInstance*>& InOutItems, int32 MaxSlots);
};
