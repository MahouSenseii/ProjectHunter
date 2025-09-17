// Copyright@2024 Quentin Davis

#pragma once

#include "CoreMinimal.h"
#include "Library/PHItemStructLibrary.h"
#include "BaseItem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogBaseItem, Log, All);

/**
 * Base class for all inventory items
 */
UCLASS(Abstract, Blueprintable)
class ALS_PROJECTHUNTER_API UBaseItem : public UObject
{
	GENERATED_BODY()

public:
	UBaseItem();

	/* ============================= */
	/* ===    Initialization     === */
	/* ============================= */

	/** Initialize the item with given information */
	virtual void Initialize( FItemInformation& ItemInfo);

	/** Generate a unique ID for this item instance */
	void GenerateUniqueID();

	/* ============================= */
	/* ===      Properties       === */
	/* ============================= */

	/** Get the unique identifier for this specific item instance */
	UFUNCTION(BlueprintCallable, Category = "Item")
	FString GetUniqueID() const;

	/** Returns the dimensions of the item, considering rotation */
	UFUNCTION(BlueprintCallable, Category = "Item")
	FIntPoint GetDimensions() const;
	
	/** Check if the item is rotated */
	UFUNCTION(BlueprintCallable, Category = "Item")
	bool IsRotated() const { return ItemInfos.ItemInfo.Rotated; }
	
	/** Set the rotation state of the item */
	UFUNCTION(BlueprintCallable, Category = "Item")
	void SetRotated(bool bNewRotated);

	/** Toggles the rotation state of the item */
	UFUNCTION(BlueprintCallable, Category = "Item")
	void ToggleRotation();

	/** Returns the icon of the item, considering rotation */
	UFUNCTION(BlueprintCallable, Category = "Item")
	UMaterialInstance* GetIcon() const;

	/* ============================= */
	/* ===      Stacking         === */
	/* ============================= */

	/** Get the current quantity/stack size */
	UFUNCTION(BlueprintCallable, Category = "Item|Stacking")
	virtual int32 GetQuantity() const;

	/** Add to the item's quantity (respects max stack size) */
	UFUNCTION(BlueprintCallable, Category = "Item|Stacking")
	bool AddQuantity(int32 InQty);

	/** Remove from the item's quantity */
	UFUNCTION(BlueprintCallable, Category = "Item|Stacking")
	bool RemoveQuantity(int32 InQty);

	/** Set the item's quantity directly */
	UFUNCTION(BlueprintCallable, Category = "Item|Stacking")
	void SetQuantity(int32 NewQty);

	/** Check if this item can be stacked */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item|Stacking")
	bool IsStackable() const { return ItemInfos.ItemInfo.Stackable; }

	/** Get the maximum stack size (0 = unlimited) */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item|Stacking")
	int32 GetMaxStackSize() const { return ItemInfos.ItemInfo.MaxStackSize; }

	/** Check if this item can accept more quantity */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item|Stacking")
	bool CanAddQuantity(int32 Amount) const;

	/* ============================= */
	/* ===    Item Information   === */
	/* ============================= */

	/** Get read-only access to item information */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item|Info")
	const FItemInformation& GetItemInfo() const { return ItemInfos; }

	/** Get the item's display name */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item|Info")
	const FText& GetItemName() const { return ItemInfos.ItemInfo.ItemName; }

	/** Get the item's description */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item|Info")
	const FText& GetItemDescription() const { return ItemInfos.ItemInfo.ItemDescription; }

	/** Get the item's type */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item|Info")
	EItemType GetItemType() const { return ItemInfos.ItemInfo.ItemType; }

	/** Get the item's rarity */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item|Info")
	EItemRarity GetItemRarity() const { return ItemInfos.ItemInfo.ItemRarity; }

	/** Get the item's ID (type identifier, not unique instance ID) */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item|Info")
	FName GetItemID() const { return ItemInfos.ItemInfo.ItemID; }

	/* ============================= */
	/* ===      Modification     === */
	/* ============================= */

	/** Update item information (use with caution) */
	UFUNCTION(BlueprintCallable, Category = "Item|Modification")
	virtual void SetItemInfo(const FItemInformation& NewItemInfo);

	/** Set equipment data for this item */
	UFUNCTION(BlueprintCallable, Category = "Item|Modification")
	void SetEquipmentData(const FEquippableItemData& InData);

	/** Get equipment data for this item */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item|Equipment", meta = (DisplayName = "Get Equipment Data"))
	const FEquippableItemData& GetEquipmentData() const { return ItemInfos.ItemData; }

	/* ============================= */
	/* ===    Gameplay Effects   === */
	/* ============================= */

	/** Apply a gameplay effect to the target actor */
	UFUNCTION(BlueprintCallable, Category = "Item|Effects")
	static void ApplyEffectToTarget(AActor* Target, TSubclassOf<UGameplayEffect> GameplayEffectClass);

	/* ============================= */
	/* ===      Validation       === */
	/* ============================= */

	/** Validate that this item's data is correctly set up */
	UFUNCTION(BlueprintCallable, Category = "Item|Debug")
	virtual bool ValidateItemData() const;

	/** Check if this item is valid for use */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item|Debug")
	bool IsValidItem() const;

	/* ============================= */
	/* ===      Comparison       === */
	/* ============================= */

	/** Check if this item can stack with another */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item|Stacking")
	bool CanStackWith(const UBaseItem* Other) const;

	/** Check if this is the same item type as another */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item|Comparison")
	bool IsSameItemType(const UBaseItem* Other) const;

protected:
	/** Called when quantity changes */
	virtual void OnQuantityChanged(int32 OldQuantity, int32 NewQuantity);

	/** Called when rotation changes */
	virtual void OnRotationChanged(bool bWasRotated);

	/** Internal method to set item info without validation */
	void InternalSetItemInfo(const FItemInformation& NewItemInfo);

protected:
	/** Information related to the item */
	UPROPERTY(BlueprintReadOnly, Category = "Item")
	FItemInformation ItemInfos;

private:
	/** Unique identifier for this specific item instance */
	UPROPERTY()
	FString UniqueID;

	/** Track if this item has been initialized */
	UPROPERTY()
	bool bIsInitialized;

public:
	/* ============================= */
	/* ===        Debug          === */
	/* ============================= */

#if WITH_EDITOR
	/** Debug string representation of this item */
	FString GetDebugString() const;
	
	/** Log item information for debugging */
	void LogItemInfo() const;
#endif
};