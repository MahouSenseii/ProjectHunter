// Character/Component/CharacterSystemCoordinatorComponent.h
// PH-0.1 — Character System Coordinator
//
// OWNER:    Cross-system bootstrap and listener wiring for character managers.
// HELPERS:  None. Pure orchestration; no gameplay logic lives here.
// LISTENERS: This component IS the listener boundary. Managers bind to each
//            other exclusively through this coordinator — never directly.
//
// Completed tickets:
//   PH-0.1 skeleton
//   PH-0.2 manager discovery (CacheManagerReferences)
//   PH-0.3 cross-manager listener binding (BindCrossSystemListeners)
//   PH-0.4 PHBaseCharacter creates this component (see PHBaseCharacter.cpp)
//   PH-0.5 idempotent wiring — never double-wires on late join / possession churn

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Item/Library/ItemEnums.h"
#include "CharacterSystemCoordinatorComponent.generated.h"

class UStatsManager;
class UEquipmentManager;
class UInventoryManager;
class UInteractionManager;
class UCombatManager;
class UMovesetManager;
class UEquipmentPresentationComponent;
class UItemInstance;

DECLARE_LOG_CATEGORY_EXTERN(LogCharacterSystemCoordinator, Log, All);

/**
 * UCharacterSystemCoordinatorComponent
 *
 * Single point of cross-system discovery, listener wiring, and bootstrap
 * wiring for character managers. APHBaseCharacter creates this component and
 * lets it own the wiring; APHBaseCharacter itself is a composition root only.
 *
 * Bootstrap order (idempotent — see PH-0.5):
 *   1. CacheManagerReferences — single FindComponentByClass pass on BeginPlay.
 *   2. BindCrossSystemListeners — wire Equipment→Stats, Equipment→Presentation,
 *      Equipment→Moveset, etc. Listeners call back into their own managers.
 *   3. bWired = true — prevents double-wiring on possession change or late join.
 */
UCLASS(ClassGroup = (ProjectHunter), meta = (BlueprintSpawnableComponent))
class ALS_PROJECTHUNTER_API UCharacterSystemCoordinatorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCharacterSystemCoordinatorComponent();

	//~ Begin UActorComponent
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~ End UActorComponent

	// ─────────────────────────────────────────────────────────────
	// CACHED MANAGER ACCESSORS
	// Safe to call after BeginPlay; all return nullptr before wiring completes.
	// ─────────────────────────────────────────────────────────────

	UFUNCTION(BlueprintPure, Category = "ProjectHunter|Coordinator")
	UStatsManager* GetStatsManager() const { return StatsManager; }

	UFUNCTION(BlueprintPure, Category = "ProjectHunter|Coordinator")
	UEquipmentManager* GetEquipmentManager() const { return EquipmentManager; }

	UFUNCTION(BlueprintPure, Category = "ProjectHunter|Coordinator")
	UInventoryManager* GetInventoryManager() const { return InventoryManager; }

	UFUNCTION(BlueprintPure, Category = "ProjectHunter|Coordinator")
	UInteractionManager* GetInteractionManager() const { return InteractionManager; }

	UFUNCTION(BlueprintPure, Category = "ProjectHunter|Coordinator")
	UCombatManager* GetCombatManager() const { return CombatManager; }

	UFUNCTION(BlueprintPure, Category = "ProjectHunter|Coordinator")
	UMovesetManager* GetMovesetManager() const { return MovesetManager; }

	UFUNCTION(BlueprintPure, Category = "ProjectHunter|Coordinator")
	UEquipmentPresentationComponent* GetEquipmentPresentation() const { return EquipmentPresentation; }

	/** Returns true once cross-system wiring has completed exactly once. */
	bool IsWired() const { return bWired; }

protected:
	// ─────────────────────────────────────────────────────────────
	// BOOTSTRAP (PH-0.2 / PH-0.3)
	// ─────────────────────────────────────────────────────────────

	/** Single discovery pass — replaces hot-path FindComponentByClass calls in managers. */
	void CacheManagerReferences();

	/** Bind Equipment→Stats, Equipment→Presentation, Equipment→Moveset listeners. */
	void BindCrossSystemListeners();

	/** Tear down listeners on EndPlay so we never leak across level travel. */
	void UnbindCrossSystemListeners();

	// ─────────────────────────────────────────────────────────────
	// CROSS-SYSTEM HANDLERS
	// ─────────────────────────────────────────────────────────────

	/**
	 * Fires on UEquipmentManager::OnEquipmentChanged.
	 * Routes to StatsManager (apply/remove GEs) and EquipmentPresentation (visual rebuild).
	 */
	UFUNCTION()
	void HandleEquipmentChanged(EEquipmentSlot Slot, UItemInstance* NewItem, UItemInstance* OldItem);

	/**
	 * Fires on UEquipmentPresentationComponent::OnWeaponUpdated after the visual is rebuilt.
	 * Use this hook for downstream systems that need to react to the final visual state
	 * (e.g. moveset cosmetic updates, FX attachment).
	 */
	UFUNCTION()
	void HandleEquipmentPresentationUpdated(EEquipmentSlot Slot, UItemInstance* NewItem);

private:
	// ─────────────────────────────────────────────────────────────
	// CACHED MANAGER REFERENCES  (Transient — not replicated)
	// ─────────────────────────────────────────────────────────────

	UPROPERTY(Transient) TObjectPtr<UStatsManager>                 StatsManager          = nullptr;
	UPROPERTY(Transient) TObjectPtr<UEquipmentManager>             EquipmentManager      = nullptr;
	UPROPERTY(Transient) TObjectPtr<UInventoryManager>             InventoryManager      = nullptr;
	UPROPERTY(Transient) TObjectPtr<UInteractionManager>           InteractionManager    = nullptr;
	UPROPERTY(Transient) TObjectPtr<UCombatManager>                CombatManager         = nullptr;
	UPROPERTY(Transient) TObjectPtr<UMovesetManager>               MovesetManager        = nullptr;
	UPROPERTY(Transient) TObjectPtr<UEquipmentPresentationComponent> EquipmentPresentation = nullptr;

	/** Guard: true once wiring completed. Prevents double-wiring on possession churn. */
	bool bWired = false;
};
