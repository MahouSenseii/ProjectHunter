// Copyright@2024 Quentin Davis

#pragma once


#include "CoreMinimal.h"
#include "Data/UItemDefinitionAsset.h"
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
	virtual void Initialize(UItemDefinitionAsset* ItemInfo);

	/** Generate a unique ID for this item instance */
	void GenerateUniqueID();

	/* ============================= */
	/* ===      Properties       === */
	/* ============================= */

	/** Get the unique identifier for this specific item instance */
	UFUNCTION(BlueprintCallable, Category = "Item")
	virtual FString GetItemInstanceID() const;

	/** Returns the dimensions of the item, considering rotation */
	UFUNCTION(BlueprintCallable, Category = "Item")
	virtual FIntPoint GetDimensions() const;
	
	/** Check if the item is rotated */
	UFUNCTION(BlueprintCallable, Category = "Item")
	virtual bool IsRotated()  { return RuntimeData.bRotated; }
	
	/** Set the rotation state of the item */
	UFUNCTION(BlueprintCallable, Category = "Item")
	virtual void SetRotated(bool bNewRotated);

	/** Toggles the rotation state of the item */
	UFUNCTION(BlueprintCallable, Category = "Item")
	virtual void ToggleRotation();

	/** Returns the icon of the item, considering rotation */
	UFUNCTION(BlueprintCallable, Category = "Item")
	UMaterialInstance* GetIcon() ;

	/* ============================= */
	/* ===      Stacking         === */
	/* ============================= */

	/** Get the current quantity/stack size */
	UFUNCTION(BlueprintCallable, Category = "Item|Stacking")
	virtual int32 GetQuantity() ;

	/** Add to the item's quantity (respects max stack size) */
	UFUNCTION(BlueprintCallable, Category = "Item|Stacking")
	virtual bool AddQuantity(int32 InQty);

	/** Remove from the item's quantity */
	UFUNCTION(BlueprintCallable, Category = "Item|Stacking")
	virtual bool RemoveQuantity(int32 InQty);

	/** Set the item's quantity directly */
	UFUNCTION(BlueprintCallable, Category = "Item|Stacking")
	virtual void SetQuantity(int32 NewQty);

	/** Check if this item can be stacked */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item|Stacking")
	virtual bool IsStackable()  { return ItemDefinition->Base.Stackable; }

	/** Get the maximum stack size (0 = unlimited) */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item|Stacking")
	virtual int32 GetMaxStackSize()  { return ItemDefinition->Base.MaxStackSize; }

	/** Check if this item can accept more quantity */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item|Stacking")
	bool CanAddQuantity(int32 Amount) ;

	/* ============================= */
	/* ===    Item Information   === */
	/* ============================= */

	/** Get read-only access to item information */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item|Info")
	virtual  UItemDefinitionAsset* GetItemInfo()  { return ItemDefinition; }

	/** Get the item's display name */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item|Info")
	const FText& GetItemName()  { return ItemDefinition->Base.ItemName; }

	/** Get the item's description */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item|Info")
	const FText& GetItemDescription()  { return ItemDefinition->Base.ItemDescription; }

	/** Get the item's type */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item|Info")
	EItemType GetItemType() const { return ItemDefinition->Base.ItemType; }

	/** Get the item's rarity */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item|Info")
	EItemRarity GetItemRarity()  { return ItemDefinition->Base.ItemRarity; }

	/** Get the item's ID (type identifier, not unique instance ID) */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item|Info")
	FName GetItemID()  { return ItemDefinition->Base.ItemID; }

	/* ============================= */
	/* ===      Modification     === */
	/* ============================= */

	/** Update item information (use with caution) */
	UFUNCTION(BlueprintCallable, Category = "Item|Modification")
	virtual void SetItemInfo(UItemDefinitionAsset* NewItemInfo);

	/** Set equipment data for this item */
	UFUNCTION(BlueprintCallable, Category = "Item|Modification")
	void SetEquipmentData( FEquippableItemData& InData);

	/** Get equipment data for this item */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item|Equipment", meta = (DisplayName = "Get Equipment Data"))
	const FEquippableItemData& GetEquipmentData() const { return ItemDefinition->Equip; }

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
	virtual bool ValidateItemData() ;

	/** Check if this item is valid for use */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item|Debug")
	bool IsValidItem() ;

	/* ============================= */
	/* ===      Comparison       === */
	/* ============================= */

	/** Check if this item can stack with another */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item|Stacking")
	bool CanStackWith( UBaseItem* Other) ;

	/** Check if this is the same item type as another */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item|Comparison")
	bool IsSameItemType(const UBaseItem* Other) const;

protected:
	/** Called when quantity changes */
	virtual void OnQuantityChanged(int32 OldQuantity, int32 NewQuantity);

	/** Called when rotation changes */
	virtual void OnRotationChanged(bool bWasRotated);

	/** Internal method to set item info without validation */
	void InternalSetItemInfo( UItemDefinitionAsset*& NewItemInfo);

	bool GetIsInitialized() const;
	bool SetIsInitialized(bool bNewIsInitialized);

public:

	// Immutable reference
	UPROPERTY()
	UItemDefinitionAsset* ItemDefinition;

	// Instance-specific state
	UPROPERTY()
	FItemInstanceData RuntimeData; 

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
	FString GetDebugString() ;
	
	/** Log item information for debugging */
	void LogItemInfo() ;
#endif
};