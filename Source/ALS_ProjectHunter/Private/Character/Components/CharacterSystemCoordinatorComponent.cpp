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

void UCharacterSystemCoordinatorComponent::CacheManagerReferences()
{
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

void UCharacterSystemCoordinatorComponent::BindCrossSystemListeners()
{
	if (EquipmentManager)
	{
		EquipmentManager->OnEquipmentChanged.AddDynamic(
			this, &UCharacterSystemCoordinatorComponent::HandleEquipmentChanged);
	}

	if (EquipmentPresentation)
	{
		EquipmentPresentation->OnWeaponUpdated.AddDynamic(
			this, &UCharacterSystemCoordinatorComponent::HandleEquipmentPresentationUpdated);
	}

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

void UCharacterSystemCoordinatorComponent::HandleEquipmentChanged(
	EEquipmentSlot Slot, UItemInstance* NewItem, UItemInstance* OldItem)
{
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

	if (EquipmentPresentation && EquipmentManager && EquipmentManager->bAutoUpdateWeapons)
	{
		EquipmentPresentation->HandleEquipmentChanged(Slot, NewItem);
	}

	(void)OldItem;
}

void UCharacterSystemCoordinatorComponent::HandleEquipmentPresentationUpdated(
	EEquipmentSlot Slot, UItemInstance* NewItem)
{
	(void)Slot;
	(void)NewItem;
}
