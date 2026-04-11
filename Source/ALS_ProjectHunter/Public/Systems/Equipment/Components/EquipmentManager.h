// Character/Component/EquipmentManager.h
// PH-1.4 / PH-1.5 — Equipment Manager (state owner only)
//
// OWNER:    Equipped slot state — what is equipped, how it changes, when it broadcasts.
//           Replication of EquippedItemsArray. Server-authority mutations.
// HELPERS:  UEquipmentFunctionLibrary — stateless slot/rule/compatibility helpers.
// LISTENERS: UCharacterSystemCoordinatorComponent binds to OnEquipmentChanged and
//            routes stats (→UStatsManager) and visuals (→UEquipmentPresentationComponent).
//
// What this manager must NOT do after PH-1.4/1.5:
//   × Touch USkeletalMeshComponent or spawn/destroy weapon actors.
//   × Call UStatsManager directly.
//   × Bind to another manager's delegate directly.
//
// Completed tickets:
//   PH-1.2 inline slot rules replaced with UEquipmentFunctionLibrary calls
//   PH-1.4 visual code moved to UEquipmentPresentationComponent
//   PH-1.5 stat application moved to coordinator (Equipment→Stats listener)

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "Item/Library/ItemEnums.h"
#include "Systems/Equipment/Library/EquipmentLog.h"
#include "Systems/Equipment/Library/EquipmentStructs.h"
#include "EquipmentManager.generated.h"

class UItemInstance;
class UInventoryManager;
class UCharacterSystemCoordinatorComponent;
class UEquipmentPresentationComponent;
class AEquippedItemRuntimeActor;
class FEquipmentMutationHelper;
class FEquipmentReplicationHelper;
class FEquipmentSlotResolver;
struct FItemBase;

/** Fired whenever an equipment slot changes (equip, unequip, swap). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnEquipmentChanged,
	EEquipmentSlot, Slot, UItemInstance*, NewItem, UItemInstance*, OldItem);

/**
 * UEquipmentManager
 *
 * Owns equipped slot state and its mutations. Broadcasts OnEquipmentChanged.
 * All cross-system effects (stats, visuals, moveset) happen through the
 * coordinator listener, NOT inside this manager.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ALS_PROJECTHUNTER_API UEquipmentManager : public UActorComponent
{
	GENERATED_BODY()

	friend class FEquipmentMutationHelper;
	friend class FEquipmentReplicationHelper;
	friend class FEquipmentSlotResolver;

public:
	UEquipmentManager();

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ═══════════════════════════════════════════════
	// PUBLIC API
	// ═══════════════════════════════════════════════

	/**
	 * Equip an item to the given slot (auto-determines slot if ES_None).
	 * Runs on server only; clients call the Server RPC path automatically.
	 * @return The item that was displaced (may be nullptr).
	 */
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	UItemInstance* EquipItem(UItemInstance* Item, EEquipmentSlot Slot = EEquipmentSlot::ES_None, bool bSwapToBag = true);

	/**
	 * Unequip the item in the given slot.
	 * @param bMoveToBag If true, the removed item is added to InventoryManager.
	 * @return The unequipped item (may be nullptr if slot was empty).
	 */
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	UItemInstance* UnequipItem(EEquipmentSlot Slot, bool bMoveToBag = true);

	/**
	 * Swap the given item into the specified slot (shorthand for EquipItem with swap).
	 * @return The displaced item.
	 */
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	UItemInstance* SwapEquipment(UItemInstance* Item, EEquipmentSlot Slot = EEquipmentSlot::ES_None);

	/** Return the item currently in the given slot, or nullptr. */
	UFUNCTION(BlueprintPure, Category = "Equipment")
	UItemInstance* GetEquippedItem(EEquipmentSlot Slot) const;

	/** Return true if the slot is occupied. */
	UFUNCTION(BlueprintPure, Category = "Equipment")
	bool IsSlotOccupied(EEquipmentSlot Slot) const;

	/** Return all currently equipped items. */
	UFUNCTION(BlueprintPure, Category = "Equipment")
	TArray<UItemInstance*> GetAllEquippedItems() const;

	/** Determine the canonical slot for an item (delegates to UEquipmentFunctionLibrary). */
	UFUNCTION(BlueprintPure, Category = "Equipment")
	EEquipmentSlot DetermineEquipmentSlot(UItemInstance* Item) const;

	/** Return true if the item is allowed in the given slot. */
	UFUNCTION(BlueprintPure, Category = "Equipment")
	bool CanEquipToSlot(UItemInstance* Item, EEquipmentSlot Slot) const;

	/**
	 * Ground-pickup equip helper.
	 * Uses pickup-specific hand fallback rules, reports the slot that ended up
	 * owning the item, and returns false when the item should fall back to the bag.
	 */
	bool TryEquipGroundPickupItem(UItemInstance* Item, EEquipmentSlot& OutEquippedSlot, bool bSwapToBag = true);

	/** Unequip all slots. */
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void UnequipAll(bool bMoveToBag = true);

	// ═══════════════════════════════════════════════
	// RAPID-TEST HELPERS
	// ═══════════════════════════════════════════════

	/**
	 * One-shot helper for rapid weapon testing from Blueprint.
	 *
	 * Constructs a fresh UItemInstance from the supplied DataTable row, initializes
	 * it with the given level/rarity/affix settings, and equips it.  Slot selection
	 * is automatic via DetermineEquipmentSlot — a one-handed weapon goes to MainHand,
	 * a two-hander goes to TwoHand, etc.  Any item already in the resolved slot is
	 * displaced to the bag (matching EquipItem's bSwapToBag = true behaviour).
	 *
	 * Intended for testing — wire it to a debug key in the player BP and you can
	 * spawn-and-equip any item from the DataTable in one node.
	 *
	 * @param BaseItemHandle  Row handle pointing at an FItemBase row in your item DataTable.
	 * @param ItemLevel       Item level (1–100) used for affix tier rolls.
	 * @param Rarity          Item grade (F–SS) — determines affix count.
	 * @param bGenerateAffixes True to roll affixes (only meaningful for Grade E+ equipment).
	 * @return The freshly created item instance, or nullptr if the row handle was
	 *         invalid or the resulting item could not be equipped.
	 */
	UFUNCTION(BlueprintCallable, Category = "Equipment|Debug",
		meta = (AutoCreateRefTerm = "BaseItemHandle"))
	UItemInstance* GiveWeapon(
		const FDataTableRowHandle& BaseItemHandle,
		int32 ItemLevel = 1,
		EItemRarity Rarity = EItemRarity::IR_GradeF,
		bool bGenerateAffixes = true);

	/**
	 * Return the active runtime actor for a slot, forwarded from
	 * UEquipmentPresentationComponent. May be nullptr if the item uses a mesh-only
	 * representation or if the presentation component is absent.
	 */
	UFUNCTION(BlueprintPure, Category = "Equipment")
	AEquippedItemRuntimeActor* GetActiveRuntimeItemActor(EEquipmentSlot Slot) const;

	// ═══════════════════════════════════════════════
	// REPLICATED STATE
	// ═══════════════════════════════════════════════

	/**
	 * Replicated flat array of equipped entries.
	 * TMaps cannot replicate; EquippedItemsMap is rebuilt from this on each rep notify.
	 * Do not write directly — use AddEquipment/RemoveEquipment.
	 */
	UPROPERTY(ReplicatedUsing = OnRep_EquippedItems, BlueprintReadOnly, Category = "Equipment")
	TArray<FEquipmentSlotEntry> EquippedItemsArray;

	// ═══════════════════════════════════════════════
	// DELEGATES
	// ═══════════════════════════════════════════════

	/** Broadcast whenever any slot changes (equip, unequip, swap). */
	UPROPERTY(BlueprintAssignable, Category = "Equipment|Events")
	FOnEquipmentChanged OnEquipmentChanged;

	// ═══════════════════════════════════════════════
	// CONFIGURATION
	// ═══════════════════════════════════════════════

	/** Maximum ring slots (Ring1..Ring10). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Config")
	int32 MaxRingSlots = 10;

	/** Automatically determine the target slot when ES_None is passed to EquipItem. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Config")
	bool bAutoSlotSelection = true;

	/**
	 * When true the coordinator applies/removes GE stat effects on equip/unequip.
	 * Readable by UCharacterSystemCoordinatorComponent.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Config")
	bool bApplyStatsOnEquip = true;

	/**
	 * When true the coordinator asks UEquipmentPresentationComponent to rebuild the
	 * visual on equip/unequip. Readable by UCharacterSystemCoordinatorComponent.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Config")
	bool bAutoUpdateWeapons = true;

protected:
	// ═══════════════════════════════════════════════
	// REPLICATION
	// ═══════════════════════════════════════════════

	UFUNCTION()
	void OnRep_EquippedItems();

	// ═══════════════════════════════════════════════
	// INTERNAL MUTATIONS
	// ═══════════════════════════════════════════════

	UItemInstance* EquipItemInternal(UItemInstance* Item, EEquipmentSlot Slot, bool bSwapToBag,
	                                 bool bUseGroundPickupRules = false);
	bool HandleTwoHandedWeapon(UItemInstance* Item, bool bSwapToBag,
	                           UItemInstance*& OutOldMainHand, UItemInstance*& OutOldOffHand,
	                           UItemInstance*& OutOldTwoHand);

	EEquipmentSlot GetNextAvailableRingSlot() const;
	bool IsRingSlot(EEquipmentSlot Slot) const;

	void CacheComponents();
	void RebuildEquipmentMap();
	void AddEquipment(EEquipmentSlot Slot, UItemInstance* Item);
	void RemoveEquipment(EEquipmentSlot Slot);

	// ═══════════════════════════════════════════════
	// CACHED REFERENCES
	// ═══════════════════════════════════════════════

	UPROPERTY(Transient)
	TObjectPtr<UInventoryManager> InventoryManager = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UCharacterSystemCoordinatorComponent> CharacterSystemCoordinator = nullptr;

	/** Cached reference for GetActiveRuntimeItemActor pass-through. */
	UPROPERTY(Transient)
	TObjectPtr<UEquipmentPresentationComponent> EquipmentPresentation = nullptr;

	// ═══════════════════════════════════════════════
	// FAST-LOOKUP MAP (non-replicated; rebuilt from array on rep)
	// ═══════════════════════════════════════════════

	UPROPERTY(Transient)
	TMap<EEquipmentSlot, UItemInstance*> EquippedItemsMap;

	// ═══════════════════════════════════════════════
	// NETWORK RPCs
	// ═══════════════════════════════════════════════

	UFUNCTION(Server, Reliable)
	void ServerEquipItem(UItemInstance* Item, EEquipmentSlot Slot, bool bSwapToBag);

	UFUNCTION(Server, Reliable)
	void ServerUnequipItem(EEquipmentSlot Slot, bool bMoveToBag);

	/**
	 * Kept around to mirror equipment changes to all clients.
	 * Server invokes this after a successful equip/unequip mutation so that
	 * remote-client UIs and listeners can react via OnEquipmentChanged.
	 */
	UFUNCTION(NetMulticast, Reliable)
	void MulticastEquipmentChanged(EEquipmentSlot Slot, UItemInstance* NewItem, UItemInstance* OldItem);
};
