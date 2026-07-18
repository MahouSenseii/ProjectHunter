#include "Character/Components/CharacterSystemCoordinatorComponent.h"

#include "Core/Logging/ProjectHunterLogMacros.h"
#include "Character/HUD/HunterHUDBaseWidget.h"
#include "Character/PHBaseCharacter.h"
#include "Components/WidgetComponent.h"
#include "Equipment/Components/EquipmentManager.h"
#include "Equipment/Components/EquipmentPresentationComponent.h"
#include "Character/Components/Interaction/InteractionManager.h"
#include "Inventory/Components/InventoryManager.h"
#include "Stats/Components/StatsManager.h"
#include "Combat/Components/CombatManager.h"
#include "Combat/Components/UCombatStatusEffectApplier.h"
#include "Item/ItemInstance.h"

DEFINE_LOG_CATEGORY(LogCharacterSystemCoordinator);

UCharacterSystemCoordinatorComponent::UCharacterSystemCoordinatorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bWantsInitializeComponent = false;
	// The coordinator is not replicated - each machine wires its own managers.
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
	InitializeAttachedHUDWidgets();
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

UCombatStatusEffectApplier* UCharacterSystemCoordinatorComponent::GetCombatStatusManager() const
{
	return CombatManager ? CombatManager->GetCombatStatusManager() : nullptr;
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
	EquipmentPresentation = Owner->FindComponentByClass<UEquipmentPresentationComponent>();
}

void UCharacterSystemCoordinatorComponent::InitializeAttachedHUDWidgets()
{
	APHBaseCharacter* Character = Cast<APHBaseCharacter>(GetOwner());
	if (!Character)
	{
		return;
	}

	TArray<UWidgetComponent*> WidgetComponents;
	Character->GetComponents<UWidgetComponent>(WidgetComponents);

	for (UWidgetComponent* WidgetComponent : WidgetComponents)
	{
		if (!WidgetComponent)
		{
			continue;
		}

		WidgetComponent->InitWidget();

		UHunterHUDBaseWidget* HUDWidget = Cast<UHunterHUDBaseWidget>(WidgetComponent->GetUserWidgetObject());
		if (!HUDWidget)
		{
			continue;
		}

		HUDWidget->InitializeForCharacter(Character);
		WidgetComponent->RequestRedraw();

		UE_LOG(LogCharacterSystemCoordinator, Verbose,
			TEXT("Initialized attached HUD widget '%s' on '%s' from WidgetComponent '%s'."),
			*GetNameSafe(HUDWidget),
			*GetNameSafe(Character),
			*GetNameSafe(WidgetComponent));
	}
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
	if (EquipmentPresentation && EquipmentManager)
	{
		EquipmentPresentation->RefreshOverlayStateFromEquipment(EquipmentManager);
	}

	(void)Slot;
	(void)NewItem;
}
