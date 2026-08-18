// Inventory/Components/InventoryManager.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Item/Library/Enums/ItemEnums.h"
#include "Inventory/Library/Enums/InventoryEnums.h"
#include "Inventory/Library/InventoryLog.h"
#include "InventoryManager.generated.h"

class UItemInstance;
class UActorChannel;
class FOutBunch;
struct FReplicationFlags;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemAdded, UItemInstance*, Item);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemRemoved, UItemInstance*, Item);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeightChanged, float, CurrentWeight, float, MaxWeight);

/**
 * Owns item slots and routes inventory mutations through focused helpers.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ALS_PROJECTHUNTER_API UInventoryManager : public UActorComponent
{
	GENERATED_BODY()

	friend class FInventoryAdder;
	friend class FInventoryRemover;
	friend class FInventoryStackHelper;
	friend class FInventorySwapper;
	friend class FInventoryValidator;
	friend class FInventoryWeightCalculator;

public:
	UInventoryManager();

	virtual void BeginPlay() override;

	// Owner-only inventory replication keeps equipment and pickup validation authoritative.
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags) override;

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

	/** Distance in front of the owner used by DropItemAtSlotToGround. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Config", meta = (ClampMin = "0.0"))
	float GroundDropForwardDistance = 150.0f;

	/** All items in inventory (slot-based array).
	 *  OnRep_Items keeps the client UI consistent after server-authoritative changes.
	 *  COND_OwnerOnly: only the owning client receives the full inventory.
	 */
	UPROPERTY(SaveGame, BlueprintReadOnly, ReplicatedUsing = OnRep_Items, Category = "Inventory")
	TArray<UItemInstance*> Items;

	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnItemAdded OnItemAdded;

	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnItemRemoved OnItemRemoved;

	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnInventoryChanged OnInventoryChanged;

	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnWeightChanged OnWeightChanged;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory")
	bool AddItem(UItemInstance* Item);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory")
	bool AddItemToSlot(UItemInstance* Item, int32 SlotIndex);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory")
	bool RemoveItem(UItemInstance* Item);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory")
	UItemInstance* RemoveItemAtSlot(int32 SlotIndex);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory")
	bool RemoveQuantity(UItemInstance* Item, int32 Quantity);

	/**
	 * Move/swap two slots.
	 *
	 * Safe to call from client UI: when the caller has no authority this
	 * forwards to the server and returns false (the change arrives via
	 * OnRep_Items). Mirrors how UEquipmentManager::EquipItem behaves.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool SwapItems(int32 SlotA, int32 SlotB);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory")
	void DropItem(UItemInstance* Item, FVector DropLocation);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory")
	void DropItemAtSlot(int32 SlotIndex, FVector DropLocation);

	/**
	 * Drop a slot to the ground in front of the owner.
	 *
	 * Client-safe entry point for the menu: the drop location is always
	 * computed on the server, so a client can't choose where the item lands.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void DropItemAtSlotToGround(int32 SlotIndex);

	/**
	 * Drop a specific item to the ground in front of the owner.
	 *
	 * Client-safe. Use when the slot index is not known locally - e.g. an item
	 * just unequipped into the bag by a server RPC.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void DropItemToGround(UItemInstance* Item);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory|Stacking")
	bool TryStackItem(UItemInstance* Item);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory|Stacking")
	bool StackItems(UItemInstance* SourceItem, UItemInstance* TargetItem);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory|Stacking")
	UItemInstance* SplitStack(UItemInstance* Item, int32 Amount);

	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool IsFull() const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool IsOverweight() const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetItemCount() const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetMaxSlots() const { return MaxSlots; }

	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetSlotCount() const { return FMath::Max(MaxSlots, Items.Num()); }

	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetAvailableSlots() const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	float GetTotalWeight() const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	float GetMaxWeight() const { return MaxWeight; }

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

	/**
	 * Check whether a specific item instance is present in this inventory.
	 * Used by EquipmentManager to validate server-side equip requests.
	 * @param Item - Item instance to search for
	 * @return True if the item is in the inventory
	 */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool ContainsItem(UItemInstance* Item) const;

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

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory|Organization")
	void SortInventory(ESortMode SortMode = ESortMode::SM_Type);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory|Organization")
	void CompactInventory();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory|Organization")
	void ClearAll();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory|Weight")
	void UpdateMaxWeightFromStrength(int32 Strength);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory|Weight")
	void SetMaxWeight(float NewMaxWeight);

	UFUNCTION(BlueprintPure, Category = "Inventory|Weight")
	bool WouldExceedWeight(UItemInstance* Item) const;

private:
	void UpdateWeight();

	void BroadcastInventoryChanged();

	UItemInstance* FindStackableItem(UItemInstance* Item) const;

	bool HasInventoryWriteAuthority(const TCHAR* FunctionName) const;

	/** True when this instance may mutate the inventory directly. */
	bool HasInventoryAuthority() const;

	/** Server-side drop location derived from the owner's transform. */
	FVector GetGroundDropLocation() const;

	UFUNCTION(Server, Reliable)
	void ServerSwapItems(int32 SlotA, int32 SlotB);

	UFUNCTION(Server, Reliable)
	void ServerDropItemAtSlotToGround(int32 SlotIndex);

	UFUNCTION(Server, Reliable)
	void ServerDropItemToGround(UItemInstance* Item);

	/** Called on owning client when Items array replicates from server.
	 *  Rebroadcasts OnInventoryChanged and OnWeightChanged so UI stays in sync. */
	UFUNCTION()
	void OnRep_Items();
};

