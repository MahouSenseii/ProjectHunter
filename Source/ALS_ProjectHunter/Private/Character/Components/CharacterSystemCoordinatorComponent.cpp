// Character/Component/CharacterSystemCoordinatorComponent.cpp
// PH-0.3 / PH-0.4 — Character System Coordinator (complete)

#include "Character/Components/CharacterSystemCoordinatorComponent.h"

#include "Core/Logging/ProjectHunterLogMacros.h"
#include "Equipment/Components/EquipmentManager.h"
#include "Equipment/Components/EquipmentPresentationComponent.h"
#include "Character/Component/Interaction/InteractionManager.h"
#include "Inventory/Components/InventoryManager.h"
#include "Stats/Components/StatsManager.h"
#include "Combat/Components/CombatManager.h"
#include "Combat/Components/CombatStatusManager.h"
#include "Combat/Components/CombatSystemManagerComponent.h"
#include "Item/ItemInstance.h"

DEFINE_LOG_CATEGORY(LogCharacterSystemCoordinator);

// ═══════════════════════════════════════════════════════════════════════
// LIFECYCLE
// ═══════════════════════════════════════════════════════════════════════

UCharacterSystemCoordinatorComponent::UCharacterSystemCoordinatorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bWantsInitializeComponent = false;
	// The coordinator is not replicated — each machine wires its own managers.
	SetIsReplicatedByDefault(false);
}

void UCharacterSystemCoordinatorComponent::BeginPlay()
{
	Super::BeginPlay();

	if (bWired)
	{
		// PH-0.5: Idempotent — never wire twice.
		// Handles late join, possession churn, and component re-registration.
		return;
	}

	CacheManagerReferences();
	BindCrossSystemListeners();
	bWired = true;

	UE_LOG(LogCharacterSystemCoordinator, Verbose,
		TEXT("Coordinator wired on '%s'."), *GetNameSafe(GetOwner()));
}

void UCharacterSystemCoordinatorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (bWired)
	{
		UnbindCrossSystemListeners();
		bWired = false;
	}

	Super::EndPlay(EndPlayReason);
}

// ═══════════════════════════════════════════════════════════════════════
// PH-0.2: SINGLE MANAGER DISCOVERY PASS
// ═══════════════════════════════════════════════════════════════════════

void UCharacterSystemCoordinatorComponent::CacheManagerReferences()
{
	// One pass at BeginPlay replaces per-frame FindComponentByClass calls in managers.
	// After PH-0.4 lands, grep Source/ for FindComponentByClass<UStatsManager> etc.
	// in domain hot paths and route them through this coordinator instead.
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		PH_LOG_ERROR(LogCharacterSystemCoordinator,
			"CacheManagerReferences failed: Owner was null and manager references will remain unset.");
		return;
	}

	StatsManager          = Owner->FindComponentByClass<UStatsManager>();
	EquipmentManager      = Owner->FindComponentByClass<UEquipmentManager>();
	InventoryManager      = Owner->FindComponentByClass<UInventoryManager>();
	InteractionManager    = Owner->FindComponentByClass<UInteractionManager>();
	CombatManager         = Owner->FindComponentByClass<UCombatManager>();
	CombatSystemManager   = Owner->FindComponentByClass<UCombatSystemManagerComponent>();
	CombatStatusManager   = Owner->FindComponentByClass<UCombatStatusManager>();
	EquipmentPresentation = Owner->FindComponentByClass<UEquipmentPresentationComponent>();
}

// ═══════════════════════════════════════════════════════════════════════
// PH-0.3: CROSS-SYSTEM LISTENER WIRING
// ═══════════════════════════════════════════════════════════════════════

void UCharacterSystemCoordinatorComponent::BindCrossSystemListeners()
{
	// Equipment -> Coordinator (stats + presentation).
	if (EquipmentManager)
	{
		EquipmentManager->OnEquipmentChanged.AddDynamic(
			this, &UCharacterSystemCoordinatorComponent::HandleEquipmentChanged);
	}

	// ── Presentation → Coordinator (downstream cosmetic listeners) ────────
	if (EquipmentPresentation)
	{
		EquipmentPresentation->OnWeaponUpdated.AddDynamic(
			this, &UCharacterSystemCoordinatorComponent::HandleEquipmentPresentationUpdated);
	}

	// ── Warm up: apply equipment stats + visuals already in the slot array ─
	// Needed for save-game loads and pool recycling where items arrive before
	// BeginPlay on the coordinator fires (e.g. server-side actor recycling).
	if (EquipmentManager)
	{
		const bool bApplyStats = EquipmentManager->bApplyStatsOnEquip
			&& StatsManager
			&& GetOwner()
			&& GetOwner()->HasAuthority();

		const bool bApplyVisuals = EquipmentManager->bAutoUpdateWeapons
			&& EquipmentPresentation;

		for (const FEquipmentSlotEntry& Entry : EquipmentManager->EquippedItemsArray)
		{
			if (!Entry.Item || Entry.Slot == EEquipmentSlot::ES_None)
			{
				continue;
			}

			if (bApplyStats)
			{
				StatsManager->ApplyEquipmentStats(Entry.Item);
			}

			if (bApplyVisuals)
			{
				EquipmentPresentation->HandleEquipmentChanged(Entry.Slot, Entry.Item);
			}

		}
	}
}

void UCharacterSystemCoordinatorComponent::UnbindCrossSystemListeners()
{
	if (EquipmentManager)
	{
		EquipmentManager->OnEquipmentChanged.RemoveDynamic(
			this, &UCharacterSystemCoordinatorComponent::HandleEquipmentChanged);
	}

	if (EquipmentPresentation)
	{
		EquipmentPresentation->OnWeaponUpdated.RemoveDynamic(
			this, &UCharacterSystemCoordinatorComponent::HandleEquipmentPresentationUpdated);
	}
}

// ═══════════════════════════════════════════════════════════════════════
// PH-0.3: CROSS-SYSTEM HANDLERS
// ═══════════════════════════════════════════════════════════════════════

void UCharacterSystemCoordinatorComponent::HandleEquipmentChanged(
	EEquipmentSlot Slot, UItemInstance* NewItem, UItemInstance* OldItem)
{
	// ── 1. Stats (server-authoritative) ────────────────────────────────────
	// PH-1.5: Equipment no longer calls StatsManager directly.
	// The coordinator is the single place stats are applied/removed on equip events.
	if (StatsManager && EquipmentManager && EquipmentManager->bApplyStatsOnEquip
		&& GetOwner() && GetOwner()->HasAuthority())
	{
		if (OldItem)
		{
			StatsManager->RemoveEquipmentStats(OldItem);
		}
		if (NewItem)
		{
			StatsManager->ApplyEquipmentStats(NewItem);
		}
	}

	// ── 2. Visual / Presentation (cosmetic — runs on all machines) ─────────
	// PH-1.4: Equipment no longer calls UpdateEquippedWeapon directly.
	// The presentation component owns all mesh / runtime-actor lifecycle.
	if (EquipmentPresentation && EquipmentManager && EquipmentManager->bAutoUpdateWeapons)
	{
		EquipmentPresentation->HandleEquipmentChanged(Slot, NewItem);
	}

	(void)OldItem;
}

void UCharacterSystemCoordinatorComponent::HandleEquipmentPresentationUpdated(
	EEquipmentSlot Slot, UItemInstance* NewItem)
{
	// Hook for systems that need to react after the visual is fully built
	// (e.g. future cosmetic animations, FX attachment points).
	// For now this is a deliberate no-op forward stub.
	(void)Slot;
	(void)NewItem;
}
