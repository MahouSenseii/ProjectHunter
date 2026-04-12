// Character/Component/EquipmentPresentationComponent.h
// PH-1.4 — Equipment Presentation Component (complete)
//
// OWNER:    Visual / runtime-actor side of equipment.
// HELPERS:  Calls helpers in ItemStructs for socket resolution and attachment.
//           Does NOT own gameplay state; purely cosmetic + runtime-actor lifecycle.
// LISTENERS: Bound by UCharacterSystemCoordinatorComponent to
//            UEquipmentManager::OnEquipmentChanged.
//            Re-broadcasts as OnWeaponUpdated for downstream visual systems
//            (FX, anim, moveset cosmetic hooks).
//
// Completed tickets:
//   PH-1.3 skeleton
//   PH-1.4 migrate attached-mesh / runtime-actor / socket / cleanup from UEquipmentManager

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Item/Library/ItemStructs.h"
#include "Systems/Equipment/Library/EquipmentEnums.h"
#include "Systems/Equipment/Library/EquipmentLog.h"
#include "EquipmentPresentationComponent.generated.h"

class UItemInstance;
class USceneComponent;
class USkeletalMeshComponent;
class AEquippedItemRuntimeActor;
struct FItemBase;
struct FItemAttachmentRules;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnEquipmentVisualUpdated,
	EEquipmentSlot, Slot,
	UItemInstance*, NewItem);

/**
 * UEquipmentPresentationComponent
 *
 * Owns the visual side of equipment: attached meshes, runtime weapon actors,
 * socket resolution, and cleanup. UEquipmentManager must NOT touch
 * USkeletalMeshComponent or spawn/destroy weapon actors after PH-1.4.
 *
 * Entry-point: HandleEquipmentChanged() — bound by the coordinator.
 */
UCLASS(ClassGroup = (ProjectHunter), meta = (BlueprintSpawnableComponent))
class ALS_PROJECTHUNTER_API UEquipmentPresentationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEquipmentPresentationComponent();

	//~ Begin UActorComponent
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~ End UActorComponent

	// ─────────────────────────────────────────────────────────────
	// LISTENER ENTRYPOINT (bound by UCharacterSystemCoordinatorComponent)
	// ─────────────────────────────────────────────────────────────

	/** Called when equipped state changes. Tears down old visual, builds new one. */
	UFUNCTION()
	void HandleEquipmentChanged(EEquipmentSlot Slot, UItemInstance* NewItem);

	// ─────────────────────────────────────────────────────────────
	// ACCESSORS
	// ─────────────────────────────────────────────────────────────

	/** Returns the active runtime actor for the slot, or nullptr. */
	UFUNCTION(BlueprintPure, Category = "ProjectHunter|Equipment|Presentation")
	AEquippedItemRuntimeActor* GetActiveRuntimeItemActor(EEquipmentSlot Slot) const;

	// ─────────────────────────────────────────────────────────────
	// BROADCASTS
	// ─────────────────────────────────────────────────────────────

	/** Fired after the visual is built. Downstream systems (FX, anim, moveset) react here. */
	UPROPERTY(BlueprintAssignable, Category = "ProjectHunter|Equipment|Presentation")
	FOnEquipmentVisualUpdated OnWeaponUpdated;

protected:
	// ─────────────────────────────────────────────────────────────
	// VISUAL LIFECYCLE
	// ─────────────────────────────────────────────────────────────

	/** Build and attach the visual for a newly equipped item. */
	void AttachItemVisual(EEquipmentSlot Slot, UItemInstance* Item);

	/** Destroy all visuals for a slot (actor + mesh). */
	void DetachItemVisual(EEquipmentSlot Slot);

	// ─────────────────────────────────────────────────────────────
	// SPAWN HELPERS
	// ─────────────────────────────────────────────────────────────

	/** Spawn a runtime actor (weapon with gameplay logic) and attach it. */
	void SpawnWeaponActor(EEquipmentSlot Slot, UItemInstance* Item,
	                      const FItemBase* BaseData, FName SocketName);

	/** Spawn a mesh-only component (static or skeletal) and attach it. */
	void SpawnWeaponMesh(EEquipmentSlot Slot, UItemInstance* Item,
	                     const FItemBase* BaseData, FName SocketName);

	// ─────────────────────────────────────────────────────────────
	// PURE HELPERS (no state)
	// ─────────────────────────────────────────────────────────────

	/** Returns the named socket context ("MainHand", "OffHand", "TwoHand") for a slot. */
	static FName GetSocketContextForSlot(EEquipmentSlot Slot);

	/** Resolve the attachment socket from item data for the given slot. */
	static FName ResolveSocketForSlot(EEquipmentSlot Slot, const FItemBase* BaseData);

	/** Convert item-level attachment rules to the engine attachment rule struct. */
	static FAttachmentTransformRules ConvertAttachmentRules(const FItemAttachmentRules& ItemRules);

private:
	// ─────────────────────────────────────────────────────────────
	// CACHED REFERENCES
	// ─────────────────────────────────────────────────────────────

	/** Skeletal mesh of the owning character. Resolved once in BeginPlay. */
	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> CharacterMesh = nullptr;

	// ─────────────────────────────────────────────────────────────
	// LIVE VISUAL STATE
	// ─────────────────────────────────────────────────────────────

	/** Runtime actors spawned for slots that use AEquippedItemRuntimeActor. */
	UPROPERTY(Transient)
	TMap<EEquipmentSlot, TObjectPtr<AEquippedItemRuntimeActor>> SpawnedActors;

	/**
	 * Mesh components (static or skeletal) spawned for slots that use a
	 * mesh-only representation. O(1) access and reliable cleanup.
	 */
	UPROPERTY(Transient)
	TMap<EEquipmentSlot, TObjectPtr<USceneComponent>> SpawnedMeshComponents;
};
