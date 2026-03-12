// Character/Component/InventoryManager.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Library/InventoryEnum.h"
#include "InventoryManager.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogInventoryManager, Log, All);
class UItemInstance;

/**
 * Event delegates for inventory changes
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemAdded, UItemInstance*, Item);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemRemoved, UItemInstance*, Item);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeightChanged, float, CurrentWeight, float, MaxWeight);



/**
 * Slot-based + Weight-based Inventory System 
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ALS_PROJECTHUNTER_API UInventoryManager : public UActorComponent
{
	GENERATED_BODY()

public:
	UInventoryManager();

	virtual void BeginPlay() override;

	// ═══════════════════════════════════════════════
	// CONFIGURATION (Hunter Manga Style)
	// ═══════════════════════════════════════════════

	/** Maximum number of item slots (default 60) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Config")
	int32 MaxSlots = 60;

	/** Maximum weight capacity (Hunter Strength-based) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Config")
	float MaxWeight = 100.0f;

	/** Weight per point of Strength (default: 10.0 = 1 STR gives 10 carry weight) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Config")
	float WeightPerStrength = 10.0f;

	/** Auto-stack identical items? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Config")
	bool bAutoStack = true;

	/** Auto-sort on pickup? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Config")
	bool bAutoSort = false;

	// ═══════════════════════════════════════════════
	// STORAGE
	// ═══════════════════════════════════════════════

	/** All items in inventory (slot-based array) */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Inventory")
	TArray<UItemInstance*> Items;

	// ═══════════════════════════════════════════════
	// EVENTS
	// ═══════════════════════════════════════════════

	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnItemAdded OnItemAdded;

	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnItemRemoved OnItemRemoved;

	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnInventoryChanged OnInventoryChanged;

	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnWeightChanged OnWeightChanged;

	// ═══════════════════════════════════════════════
	// BASIC OPERATIONS
	// ═══════════════════════════════════════════════

	/**
	 * Add item to inventory
	 * Handles auto-stacking if enabled
	 * @param Item - Item to add
	 * @return True if successfully added (or partially stacked)
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool AddItem(UItemInstance* Item);

	/**
	 * Add item to specific slot
	 * @param Item - Item to add
	 * @param SlotIndex - Target slot index
	 * @return True if successfully added
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool AddItemToSlot(UItemInstance* Item, int32 SlotIndex);

	/**
	 * Remove item from inventory
	 * @param Item - Item to remove
	 * @return True if successfully removed
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveItem(UItemInstance* Item);

	/**
	 * Remove item at specific slot
	 * @param SlotIndex - Slot index
	 * @return Removed item (or nullptr)
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	UItemInstance* RemoveItemAtSlot(int32 SlotIndex);

	/**
	 * Remove quantity from item stack
	 * @param Item - Item to remove from
	 * @param Quantity - Amount to remove
	 * @return True if successfully removed
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveQuantity(UItemInstance* Item, int32 Quantity);

	/**
	 * Swap items between two slots
	 * @param SlotA - First slot
	 * @param SlotB - Second slot
	 * @return True if successfully swapped
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool SwapItems(int32 SlotA, int32 SlotB);

	/**
	 * Drop item on ground
	 * @param Item - Item to drop
	 * @param DropLocation - Where to drop it
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void DropItem(UItemInstance* Item, FVector DropLocation);

	/**
	 * Drop item at slot on ground
	 * @param SlotIndex - Slot to drop from
	 * @param DropLocation - Where to drop it
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void DropItemAtSlot(int32 SlotIndex, FVector DropLocation);

	// ═══════════════════════════════════════════════
	// STACKING
	// ═══════════════════════════════════════════════

	/**
	 * Try to stack item with existing items
	 * @param Item - Item to stack
	 * @return True if fully stacked (item consumed)
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Stacking")
	bool TryStackItem(UItemInstance* Item);

	/**
	 * Stack two items together
	 * @param SourceItem - Item to take from
	 * @param TargetItem - Item to add to
	 * @return True if stacked
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Stacking")
	bool StackItems(UItemInstance* SourceItem, UItemInstance* TargetItem);

	/**
	 * Split stack into new item
	 * @param Item - Item to split
	 * @param Amount - Amount to split off
	 * @return New item instance with split amount
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Stacking")
	UItemInstance* SplitStack(UItemInstance* Item, int32 Amount);

	// ═══════════════════════════════════════════════
	// QUERIES
	// ═══════════════════════════════════════════════

	/**
	 * Check if inventory is full (no empty slots)
	 */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool IsFull() const;

	/**
	 * Check if overweight (exceeds max weight)
	 */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool IsOverweight() const;

	/**
	 * Get item count (occupied slots)
	 */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetItemCount() const;

	/**
	 * Get max slot count
	 */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetMaxSlots() const { return MaxSlots; }

	/**
	 * Get available slot count
	 */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetAvailableSlots() const;

	/**
	 * Get current total weight
	 */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	float GetTotalWeight() const;

	/**
	 * Get remaining weight capacity
	 */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	float GetRemainingWeight() const;

	/**
	 * Get weight percentage (0.0 to 1.0)
	 */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	float GetWeightPercent() const;

	/**
	 * Check if can add item (weight/slot check)
	 */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool CanAddItem(UItemInstance* Item) const;

	/**
	 * Check if specific slot is empty
	 */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool IsSlotEmpty(int32 SlotIndex) const;

	/**
	 * Get item at slot
	 */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	UItemInstance* GetItemAtSlot(int32 SlotIndex) const;

	/**
	 * Find first empty slot
	 */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 FindFirstEmptySlot() const;

	/**
	 * Find slot containing item
	 */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 FindSlotForItem(UItemInstance* Item) const;

	// ═══════════════════════════════════════════════
	// SEARCH & FILTER
	// ═══════════════════════════════════════════════

	/**
	 * Find items by base item ID
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Search")
	TArray<UItemInstance*> FindItemsByBaseID(FName BaseItemID) const;

	/**
	 * Find items by type (Weapon, Armor, Consumable, etc.)
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Search")
	TArray<UItemInstance*> FindItemsByType(EItemType ItemType) const;

	/**
	 * Find items by rarity (Grade F-SS)
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Search")
	TArray<UItemInstance*> FindItemsByRarity(EItemRarity Rarity) const;

	/**
	 * Has item with unique ID
	 */
	UFUNCTION(BlueprintPure, Category = "Inventory|Search")
	bool HasItemWithID(FGuid UniqueID) const;

	/**
	 * Get total quantity of base item
	 */
	UFUNCTION(BlueprintPure, Category = "Inventory|Search")
	int32 GetTotalQuantityOfItem(FName BaseItemID) const;

	// ═══════════════════════════════════════════════
	// ORGANIZATION
	// ═══════════════════════════════════════════════

	/**
	 * Sort inventory (by type, rarity, name, etc.)
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Organization")
	void SortInventory(ESortMode SortMode = ESortMode::SM_Type);

	/**
	 * Compact inventory (remove empty slots, stack items)
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Organization")
	void CompactInventory();

	/**
	 * Clear all items
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Organization")
	void ClearAll();

	// ═══════════════════════════════════════════════
	// WEIGHT MANAGEMENT (Hunter Manga)
	// ═══════════════════════════════════════════════

	/**
	 * Update max weight based on hunter strength
	 * @param Strength - Hunter's strength stat
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Weight")
	void UpdateMaxWeightFromStrength(int32 Strength);

	/**
	 * Set max weight directly
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Weight")
	void SetMaxWeight(float NewMaxWeight);

	/**
	 * Check if adding item would exceed weight limit
	 */
	UFUNCTION(BlueprintPure, Category = "Inventory|Weight")
	bool WouldExceedWeight(UItemInstance* Item) const;

private:
	/** Recalculate and broadcast weight change */
	void UpdateWeight();

	/** Broadcast inventory changed event */
	void BroadcastInventoryChanged();

	/** Find stackable item matching this item */
	UItemInstance* FindStackableItem(UItemInstance* Item) const;

	/** Remove all null/invalid items */
	void CleanupInvalidItems();
};

