// Character/Component/EquipmentManager.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Item/Library/ItemEnums.h"
#include "EquipmentManager.generated.h"

class UItemInstance;
class UInventoryManager;
class UAbilitySystemComponent;
class UStatsManager;
class USkeletalMeshComponent;
struct FItemBase;
struct FItemAttachmentRules;

DECLARE_LOG_CATEGORY_EXTERN(LogEquipmentManager, Log, All);
/**
 * Single equipped item entry (for replication)
 * TMaps cannot be replicated, so we use TArray of this struct
 */
USTRUCT(BlueprintType)
struct FEquipmentSlotEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	EEquipmentSlot Slot = EEquipmentSlot::ES_None;

	UPROPERTY(BlueprintReadOnly)
	UItemInstance* Item = nullptr;

	FEquipmentSlotEntry() = default;
	
	FEquipmentSlotEntry(EEquipmentSlot InSlot, UItemInstance* InItem)
		: Slot(InSlot), Item(InItem)
	{}
};

// Delegate for equipment changes
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnEquipmentChanged, EEquipmentSlot, Slot, UItemInstance*, NewItem, UItemInstance*, OldItem);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeaponUpdated, EEquipmentSlot, Slot, UItemInstance*, Item);

/**
 * Manages equipped items and their stats
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ALS_PROJECTHUNTER_API UEquipmentManager : public UActorComponent
{
	GENERATED_BODY()

public:
	UEquipmentManager();

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ═══════════════════════════════════════════════
	// PUBLIC API
	// ═══════════════════════════════════════════════

	/**
	 * Equip item to specified slot
	 * @param Item Item to equip
	 * @param Slot Slot to equip to (if ES_None, auto-determine)
	 * @param bSwapToBag If true, unequipped item goes to inventory. If false, returns it.
	 * @return Previously equipped item (can be nullptr)
	 */
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	UItemInstance* EquipItem(UItemInstance* Item, EEquipmentSlot Slot = EEquipmentSlot::ES_None, bool bSwapToBag = true);

	/**
	 * Unequip item from slot
	 * @param Slot Slot to unequip from
	 * @param bMoveToBag If true, item goes to inventory. If false, returns it.
	 * @return Unequipped item (can be nullptr)
	 */
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	UItemInstance* UnequipItem(EEquipmentSlot Slot, bool bMoveToBag = true);

	/**
	 * Swap equipped item with another
	 * @param Item New item to equip
	 * @param Slot Slot to swap in (if ES_None, auto-determine)
	 * @return Previously equipped item
	 */
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	UItemInstance* SwapEquipment(UItemInstance* Item, EEquipmentSlot Slot = EEquipmentSlot::ES_None);

	/**
	 * Get item in specific slot
	 */
	UFUNCTION(BlueprintPure, Category = "Equipment")
	UItemInstance* GetEquippedItem(EEquipmentSlot Slot) const;

	/**
	 * Check if slot is occupied
	 */
	UFUNCTION(BlueprintPure, Category = "Equipment")
	bool IsSlotOccupied(EEquipmentSlot Slot) const;

	/**
	 * Get all equipped items
	 */
	UFUNCTION(BlueprintPure, Category = "Equipment")
	TArray<UItemInstance*> GetAllEquippedItems() const;

	/**
	 * Determine which slot an item should go to
	 */
	UFUNCTION(BlueprintPure, Category = "Equipment")
	EEquipmentSlot DetermineEquipmentSlot(UItemInstance* Item) const;

	/**
	 * Check if item can be equipped to slot
	 */
	UFUNCTION(BlueprintPure, Category = "Equipment")
	bool CanEquipToSlot(UItemInstance* Item, EEquipmentSlot Slot) const;

	/**
	 * Unequip all items
	 */
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void UnequipAll(bool bMoveToBag = true);

	// ═══════════════════════════════════════════════
	// EQUIPMENT SLOTS
	// ═══════════════════════════════════════════════

	/**
	 * Replicated equipment array (TMaps can't be replicated)
	 * Do not access directly - use GetEquippedItem() instead
	 */
	UPROPERTY(ReplicatedUsing = OnRep_EquippedItems, BlueprintReadOnly, Category = "Equipment")
	TArray<FEquipmentSlotEntry> EquippedItemsArray;

	// ═══════════════════════════════════════════════
	// EVENTS
	// ═══════════════════════════════════════════════

	/** Called when any equipment changes */
	UPROPERTY(BlueprintAssignable, Category = "Equipment|Events")
	FOnEquipmentChanged OnEquipmentChanged;

	/** Called when weapon representation needs updating (visual + combat) */
	UPROPERTY(BlueprintAssignable, Category = "Equipment|Events")
	FOnWeaponUpdated OnWeaponUpdated;

	// ═══════════════════════════════════════════════
	// CONFIGURATION
	// ═══════════════════════════════════════════════

	/** Number of ring slots available (Solo Leveling style) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Config")
	int32 MaxRingSlots = 10;

	/** Auto-determine slot when equipping? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Config")
	bool bAutoSlotSelection = true;

	/** Apply stat effects immediately? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Config")
	bool bApplyStatsOnEquip = true;

	/** Update weapon representation automatically? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Config")
	bool bAutoUpdateWeapons = true;

protected:
	// ═══════════════════════════════════════════════
	// REPLICATION
	// ═══════════════════════════════════════════════

	UFUNCTION()
	void OnRep_EquippedItems();

	// ═══════════════════════════════════════════════
	// INTERNAL LOGIC
	// ═══════════════════════════════════════════════

	/** Actually equip the item (internal) */
	UItemInstance* EquipItemInternal(UItemInstance* Item, EEquipmentSlot Slot, bool bSwapToBag);

	/** Apply item stats to character */
	void ApplyItemStats(UItemInstance* Item);

	/** Remove item stats from character */
	void RemoveItemStats(UItemInstance* Item);

	/** Handle two-handed weapon logic */
	bool HandleTwoHandedWeapon(UItemInstance* Item, bool bSwapToBag, UItemInstance*& OutOldMainHand, UItemInstance*& OutOldOffHand);

	/** Get next available ring slot */
	EEquipmentSlot GetNextAvailableRingSlot() const;

	/** Check if slot is a ring slot */
	bool IsRingSlot(EEquipmentSlot Slot) const;

	/**
	 * Update equipped weapon (visual + combat representation)
	 * Spawns weapon actor or mesh component based on item configuration
	 * 
	 * Weapon Actor (bUseWeaponActor = true):
	 *   - Spawns BP with combat logic (mesh, collision, damage traces, VFX)
	 *   - Use for: Combat-enabled weapons
	 * 
	 * Mesh Component (bUseWeaponActor = false):
	 *   - Spawns USkeletalMeshComponent or UStaticMeshComponent
	 *   - Use for: Cosmetic items, shields, visual-only equipment
	 * 
	 * @param Slot - Equipment slot (MainHand/OffHand/TwoHand)
	 * @param Item - Item to represent (nullptr to clear/unequip)
	 */
	void UpdateEquippedWeapon(EEquipmentSlot Slot, UItemInstance* Item);

	/** Get socket context for equipment slot */
	FName GetSocketContextForSlot(EEquipmentSlot Slot) const;

	/** Spawn weapon actor blueprint (visual + combat) */
	void SpawnWeaponActor(EEquipmentSlot Slot, UItemInstance* Item, FItemBase* BaseData, 
	                     FName SocketName, FName ComponentTag);

	/** Spawn weapon mesh component (visual only) */
	void SpawnWeaponMesh(EEquipmentSlot Slot, UItemInstance* Item, FItemBase* BaseData,
	                    FName SocketName, FName ComponentTag);

	/** Clean up weapon (actor or component) */
	void CleanupWeapon(FName ComponentTag, EEquipmentSlot Slot);

	/** Convert item attachment rules to Unreal's attachment rules */
	FAttachmentTransformRules ConvertAttachmentRules(const FItemAttachmentRules& ItemRules) const;

	/** Cache components */
	void CacheComponents();

	/** Rebuild TMap from replicated TArray */
	void RebuildEquipmentMap();

	/** Add item to both array and map */
	void AddEquipment(EEquipmentSlot Slot, UItemInstance* Item);

	/** Remove item from both array and map */
	void RemoveEquipment(EEquipmentSlot Slot);

	// ═══════════════════════════════════════════════
	// CACHED COMPONENTS
	// ═══════════════════════════════════════════════

	UPROPERTY()
	UInventoryManager* InventoryManager;

	UPROPERTY()
	UAbilitySystemComponent* AbilitySystemComponent;

	UPROPERTY()
	UStatsManager* StatsManager;

	UPROPERTY()
	USkeletalMeshComponent* CharacterMesh;

	/**
	 * Internal TMap for fast lookups (rebuilt from array on replication)
	 * This is NOT replicated - it's reconstructed from EquippedItemsArray
	 */
	UPROPERTY(Transient)
	TMap<EEquipmentSlot, UItemInstance*> EquippedItemsMap;

	/**
	 * Active weapon actors (for combat-enabled weapons)
	 * Stores spawned weapon actor blueprints with combat logic
	 */
	UPROPERTY(Transient)
	TMap<EEquipmentSlot, AActor*> ActiveWeaponActors;

	// ═══════════════════════════════════════════════
	// NETWORK
	// ═══════════════════════════════════════════════

	/** Server RPC: Equip item */
	UFUNCTION(Server, Reliable)
	void ServerEquipItem(UItemInstance* Item, EEquipmentSlot Slot, bool bSwapToBag);

	/** Server RPC: Unequip item */
	UFUNCTION(Server, Reliable)
	void ServerUnequipItem(EEquipmentSlot Slot, bool bMoveToBag);

	/** Multicast RPC: Broadcast equipment change */
	UFUNCTION(NetMulticast, Reliable)
	void MulticastEquipmentChanged(EEquipmentSlot Slot, UItemInstance* NewItem, UItemInstance* OldItem);
};