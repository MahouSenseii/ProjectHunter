// Public/Systems/Inventory/Components/InventoryManager.h
//
// F5 split version: ownership-only manager. State, replication, public
// Blueprint API, and event broadcasts live here. Add/remove/drop/sort/swap/
// weight/validation logic lives in the FInventory* helper classes under
// Systems/Inventory/Helpers/.
//
// Public API is preserved 1:1 so existing Blueprint references continue to
// work without rebinding.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Item/Library/ItemEnums.h"
#include "Systems/Inventory/Library/InventoryEnums.h"
#include "Systems/Inventory/Library/InventoryLog.h"
#include "InventoryManager.generated.h"

class UItemInstance;

// Helper classes — forward declared so this header stays dependency-light.
class FInventoryAdder;
class FInventoryRemover;
class FInventoryDropper;
class FInventoryWeightCalculator;
class FInventoryValidator;
class FInventoryOrganizer;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemAdded, UItemInstance*, Item);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemRemoved, UItemInstance*, Item);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeightChanged, float, CurrentWeight, float, MaxWeight);

/**
 * Slot-based + Weight-based Inventory System.
 *
 * Owner of all inventory state. Routes mutation work to FInventoryAdder /
 * Remover / Dropper / Organizer / WeightCalculator and validation work to
 * FInventoryValidator. Helpers are friend classes so they can touch the
 * Items array and broadcast through the manager without needing public
 * mutator API noise.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ALS_PROJECTHUNTER_API UInventoryManager : public UActorComponent
{
	GENERATED_BODY()

	// Helpers need direct access to Items + the broadcast methods. They
	// must remain stateless.
	friend class FInventoryAdder;
	friend class FInventoryRemover;
	friend class FInventoryDropper;
	friend class FInventoryWeightCalculator;
	friend class FInventoryValidator;
	friend class FInventoryOrganizer;

public:
	UInventoryManager();

	virtual void BeginPlay() override;

	// N-04 FIX preserved: replicated inventory.
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ═══════════════════════════════════════════════
	// CONFIGURATION
	// ═══════════════════════════════════════════════

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Config")
	int32 MaxSlots = 60;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Config")
	float MaxWeight = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Config")
	float WeightPerStrength = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Config")
	bool bAutoStack = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Config")
	bool bAutoSort = false;

	// ═══════════════════════════════════════════════
	// STORAGE
	// ═══════════════════════════════════════════════

	UPROPERTY(SaveGame, BlueprintReadOnly, ReplicatedUsing = OnRep_Items, Category = "Inventory")
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
	// PUBLIC API — preserved 1:1 from pre-split version.
	// All implementations forward to helper classes.
	// ═══════════════════════════════════════════════

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool AddItem(UItemInstance* Item);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool AddItemToSlot(UItemInstance* Item, int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveItem(UItemInstance* Item);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	UItemInstance* RemoveItemAtSlot(int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveQuantity(UItemInstance* Item, int32 Quantity);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool SwapItems(int32 SlotA, int32 SlotB);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void DropItem(UItemInstance* Item, FVector DropLocation);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void DropItemAtSlot(int32 SlotIndex, FVector DropLocation);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Stacking")
	bool TryStackItem(UItemInstance* Item);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Stacking")
	bool StackItems(UItemInstance* SourceItem, UItemInstance* TargetItem);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Stacking")
	UItemInstance* SplitStack(UItemInstance* Item, int32 Amount);

	// ── Queries ──

	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool IsFull() const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool IsOverweight() const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetItemCount() const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetMaxSlots() const { return MaxSlots; }

	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetAvailableSlots() const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	float GetTotalWeight() const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	float GetRemainingWeight() const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	float GetWeightPercent() const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool CanAddItem(UItemInstance* Item) const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool IsSlotEmpty(int32 SlotIndex) const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	UItemInstance* GetItemAtSlot(int32 SlotIndex) const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 FindFirstEmptySlot() const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 FindSlotForItem(UItemInstance* Item) const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool ContainsItem(UItemInstance* Item) const;

	// ── Search ──

	UFUNCTION(BlueprintCallable, Category = "Inventory|Search")
	TArray<UItemInstance*> FindItemsByBaseID(FName BaseItemID) const;

	UFUNCTION(BlueprintCallable, Category = "Inventory|Search")
	TArray<UItemInstance*> FindItemsByType(EItemType ItemType) const;

	UFUNCTION(BlueprintCallable, Category = "Inventory|Search")
	TArray<UItemInstance*> FindItemsByRarity(EItemRarity Rarity) const;

	UFUNCTION(BlueprintPure, Category = "Inventory|Search")
	bool HasItemWithID(FGuid UniqueID) const;

	UFUNCTION(BlueprintPure, Category = "Inventory|Search")
	int32 GetTotalQuantityOfItem(FName BaseItemID) const;

	// ── Organization ──

	UFUNCTION(BlueprintCallable, Category = "Inventory|Organization")
	void SortInventory(ESortMode SortMode = ESortMode::SM_Type);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Organization")
	void CompactInventory();

	UFUNCTION(BlueprintCallable, Category = "Inventory|Organization")
	void ClearAll();

	// ── Weight ──

	UFUNCTION(BlueprintCallable, Category = "Inventory|Weight")
	void UpdateMaxWeightFromStrength(int32 Strength);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Weight")
	void SetMaxWeight(float NewMaxWeight);

	UFUNCTION(BlueprintPure, Category = "Inventory|Weight")
	bool WouldExceedWeight(UItemInstance* Item) const;

	// ═══════════════════════════════════════════════
	// BROADCAST HOOKS — called by helpers and OnRep.
	// Public so the helper friends can call them; do not add real logic
	// here, just delegate broadcasts.
	// ═══════════════════════════════════════════════

	void BroadcastInventoryChanged();
	void BroadcastWeightChanged();

private:
	UFUNCTION()
	void OnRep_Items();
};
